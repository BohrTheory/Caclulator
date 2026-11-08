#include "calculator.hpp"
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace calculator;
namespace {
size_t assertions = 0;
std::vector<std::string> fixtureViolations;
void fixtureCheck(bool ok, const char* message) {
    ++assertions;
    if (!ok) {
        fixtureViolations.emplace_back(message); // survives production catch(...) boundaries
        throw std::runtime_error(message);
    }
}
#define FIXTURE(x) fixtureCheck(bool(x), #x)
#define CHECK(x) fixtureCheck(bool(x), __FILE__ ":" #x)
uint64_t bits(double value) { uint64_t output; std::memcpy(&output, &value, sizeof output); return output; }
Evaluation success(double value, std::string display) { return {CALC_OK, value, std::move(display), CALC_NO_ERROR_OFFSET}; }
Evaluation failure(uint32_t status) { return {status, 999, "sentinel", 1}; }
std::string composed(double lhs, char op, double rhs) {
    return "(" + *transport(lhs) + ")" + op + "(" + *transport(rhs) + ")";
}
struct Expected { std::string input; Evaluation output; };
struct Script {
    std::deque<Expected> expected;
    std::vector<std::string> calls;
    Evaluation operator()(const std::string& text) {
        calls.push_back(text); FIXTURE(!expected.empty());
        auto next = expected.front(); expected.pop_front();
        FIXTURE(text == next.input); return next.output;
    }
    ~Script() { if (!expected.empty()) fixtureViolations.emplace_back("unconsumed script"); }
    Evaluate evaluator() { return [this](const std::string& text) { return (*this)(text); }; }
    void add(std::string input, double value, std::string display) { expected.push_back({std::move(input), success(value, std::move(display))}); }
};
void enter(Controller& controller, std::string_view text) {
    for (char c : text) controller.dispatch(Action::Input, c);
}
void assertResult(const Controller& c, double value, std::string_view display) {
    CHECK(c.state().phase == Phase::Result); CHECK(c.state().value.has_value());
    CHECK(bits(*c.state().value) == bits(value)); CHECK(c.state().display == display);
}
void assertError(const Controller& c) {
    CHECK(c.state().phase == Phase::Error); CHECK(!c.state().value); CHECK(!c.state().repeat);
    CHECK(c.state().display.empty());
}
void repeats() {
    struct Row { const char* input; char op; const char* rhsText; double rhs; std::array<double,3> values; std::array<const char*,3> displays; };
    const Row rows[] = {
        {"2+3*4", '*', "4", 4, {14,56,224}, {"14","56","224"}},
        {"(2+3)*4", '*', "4", 4, {20,80,320}, {"20","80","320"}},
        {"2*(3+4)", '*', "(3+4)", 7, {14,98,686}, {"14","98","686"}},
        {"2+3", '+', "3", 3, {5,8,11}, {"5","8","11"}},
        {"((2+3))", '+', "3", 3, {5,8,11}, {"5","8","11"}},
        {"8/4/2", '/', "2", 2, {1,.5,.25}, {"1","0.5","0.25"}},
        {"2*-3", '*', "-3", -3, {-6,18,-54}, {"-6","18","-54"}},
        {"2+1e-3", '+', "1e-3", .001, {2.001,2.002,2.003}, {"2.001","2.002","2.003"}},
        {"-(2+3)", 0, "", 0, {-5,-5,-5}, {"-5","-5","-5"}},
        {"7", 0, "", 0, {7,7,7}, {"7","7","7"}}
    };
    for (const auto& row : rows) {
        Script script; script.add(row.input, row.values[0], row.displays[0]);
        if (row.op) script.add(row.rhsText, row.rhs, "rhs output overwrites storage");
        Controller c(script.evaluator()); enter(c, row.input);
        for (size_t i = 0; i < 3; ++i) {
            if (i && row.op) script.add(composed(row.values[i-1], row.op, row.rhs), row.values[i], row.displays[i]);
            c.dispatch(Action::Equals); assertResult(c,row.values[i],row.displays[i]);
            CHECK(c.state().repeat.has_value() == (row.op != 0));
            if (row.op) { CHECK(c.state().repeat->operation == row.op); CHECK(bits(c.state().repeat->rhs) == bits(row.rhs)); }
            CHECK(script.calls.size() == (row.op ? i+2 : 1));
        }
        CHECK(script.expected.empty());
        std::cout << "PASS U-12 injected full/RHS/repeat numeric+display+bits+calls: " << row.input << '\n';
    }
    Script script; script.add("2+3*4",14,"14"); script.add("4",4,"4");
    script.add("(14)*(4)",56,"56");
    Controller edited(script.evaluator()); enter(edited,"2+3*5"); edited.dispatch(Action::Backspace); enter(edited,"4");
    edited.dispatch(Action::Equals); assertResult(edited,14,"14"); edited.dispatch(Action::Equals); assertResult(edited,56,"56");
    CHECK(script.calls.size() == 3);
    auto lexical = selectRepeat(" \t( ( 2 + 3 ) )\r\n"); CHECK(lexical && lexical->operation == '+' && lexical->rhs == "3");
    CHECK(!selectRepeat("-(2+3)"));
    CHECK(selectRepeat("2++3")->rhs == "+3");
    CHECK(selectRepeat("2+1E+3")->rhs == "1E+3");
}
void editing() {
    Script script;
    Controller c(script.evaluator());
    enter(c,"1.25"); enter(c,"."); CHECK(c.state().expression == "1.25");
    enter(c,"e"); enter(c,"E"); enter(c,"-3"); CHECK(c.state().expression == "1.25e-3");
    c.dispatch(Action::Clear); enter(c,"e"); CHECK(c.state().expression.empty());
    enter(c,".5+5."); CHECK(c.state().expression == ".5+5.");
    c.dispatch(Action::Clear); enter(c,"123"); c.dispatch(Action::Backspace); CHECK(c.state().expression == "12");
    c.dispatch(Action::Clear); enter(c,"2*3"); c.dispatch(Action::Sign); CHECK(c.state().expression == "2*-3");
    c.dispatch(Action::Sign); CHECK(c.state().expression == "2*3");
    c.dispatch(Action::Clear); c.dispatch(Action::Sign); enter(c,"3"); CHECK(c.state().expression == "-3");
    c.dispatch(Action::Sign); CHECK(c.state().expression == "3");
    c.dispatch(Action::Clear); enter(c,"2+1e-3"); c.dispatch(Action::Sign); CHECK(c.state().expression == "2+-1e-3");
    c.dispatch(Action::Clear); enter(c,"2+3"); script.add("2+3",5,"5"); script.add("3",3,"3"); c.dispatch(Action::Equals);
    c.dispatch(Action::Backspace); CHECK(c.state().expression.empty()); CHECK(!c.state().repeat); CHECK(!c.state().value);
    enter(c,"7"); script.add("7",7,"7"); c.dispatch(Action::Equals); c.dispatch(Action::Equals); assertResult(c,7,"7");
    c.dispatch(Action::Input,'9'); CHECK(c.state().expression == "9"); CHECK(!c.state().repeat);
    c.dispatch(Action::Clear); enter(c,"2+3"); script.add("2+3",5,"5"); script.add("3",3,"3"); c.dispatch(Action::Equals);
    c.dispatch(Action::Input,'*'); CHECK(c.state().expression == "(5)*"); CHECK(!c.state().repeat);
    enter(c,"2"); script.add("(5)*2",10,"10"); script.add("2",2,"2"); c.dispatch(Action::Equals);
    script.add("(10)*(2)",20,"20"); c.dispatch(Action::Equals); assertResult(c,20,"20");
    c.dispatch(Action::Sign); CHECK(!c.state().repeat); CHECK(c.state().expression == "-20");
    c.dispatch(Action::Clear); CHECK(c.state().phase == Phase::Editing); CHECK(c.state().expression.empty());
    for (char ch : std::string("0123456789.eE+-*/()")) { auto key = keyAction(ch); CHECK(key && key->action == Action::Input && key->input == ch); }
    for (char ch : std::string("=\r\n")) CHECK(keyAction(ch)->action == Action::Equals);
    CHECK(keyAction('\b')->action == Action::Backspace); CHECK(keyAction('\x1b')->action == Action::Clear);
    CHECK(!keyAction('%')); CHECK(!keyAction('s')); enter(c,"%"); CHECK(c.state().expression.empty());
    c.dispatch(Action::Clear); enter(c,std::string(CALC_MAX_INPUT_BYTES,'1')); CHECK(c.state().expression.size() == CALC_MAX_INPUT_BYTES);
    enter(c,"1"); assertError(c);
    std::cout << "PASS PM editor/key units U-05/09/10/11/13/15/20/21 (injected results)\n";
}
void presentation() {
    // Independent fixed fixtures exercise pass-through; Rust owns these numeric/display oracles.
    const Expected fixtures[] = {
        {"2+3",success(5,"5")}, {"7-10",success(-3,"-3")},
        {"6*7",success(42,"42")}, {"8/2",success(4,"4")},
        {"1.25+2.5",success(3.75,"3.75")}, {".5+5.",success(5.5,"5.5")},
        {"1.25e-3*1000",success(1.25,"1.25")}, {"-3+1",success(-2,"-2")},
        {"(-3)*2",success(-6,"-6")}, {"0.1+0.2",success(0.30000000000000004,"0.3")},
        {"-0",success(-0.,"0")}, {"1e-6",success(1e-6,"0.000001")},
        {"1e14",success(1e14,"100000000000000")}, {"1e-7",success(1e-7,"1e-7")},
        {"1e15",success(1e15,"1e15")}, {"1e-400",success(0.,"0")}
    };
    for (const auto& fixture : fixtures) {
        Script script; script.expected.push_back(fixture);
        if (auto selected=selectRepeat(fixture.input)) script.add(selected->rhs,1,"1");
        Controller c(script.evaluator()); enter(c,fixture.input); c.dispatch(Action::Equals);
        assertResult(c,fixture.output.value,fixture.output.display); CHECK(script.expected.empty());
        // Rejected marker after result leaves all completed state intact.
        const auto expression=c.state().expression; c.dispatch(Action::Input,'e');
        CHECK(c.state().expression==expression); assertResult(c,fixture.output.value,fixture.output.display);
    }
    for (const std::string input : {"2+","(",")","()","2(3)","2*/3","1e309"}) {
        Script script; script.expected.push_back({input,failure(input=="1e309" ? CALC_RANGE : CALC_SYNTAX)});
        Controller c(script.evaluator()); enter(c,input); c.dispatch(Action::Equals); assertError(c);
        CHECK(script.calls.size()==1);
    }
    std::cout << "PASS injected PM U-01..08/17..19 presentation and error fixtures (no formatting/arithmetic implementation)\n";
}
void precision() {
    for (double value : {-0.0,0.0,std::numeric_limits<double>::denorm_min(), std::numeric_limits<double>::min(),
        std::numeric_limits<double>::max(),-123.456,1e-100,-1e100}) {
        auto text = transport(value); CHECK(text);
        char* end = nullptr; const double parsed = std::strtod(text->c_str(), &end);
        CHECK(*end == '\0'); CHECK(bits(parsed) == bits(value));
    }
    CHECK(*transport(-0.) == "-0"); CHECK(*transport(1./3.) == "0.33333333333333331");
    CHECK(*transport(0.30000000000000004) == "0.30000000000000004");
    CHECK(!transport(std::numeric_limits<double>::infinity()));
    CHECK(!transport(std::numeric_limits<double>::quiet_NaN()));
    Script script;
    script.add("1/3",1./3.,"0.333333333333333"); script.add("3",3,"3");
    script.add("(0.33333333333333331)*3",1,"1"); script.add("3",3,"3");
    Controller c(script.evaluator()); enter(c,"1/3"); c.dispatch(Action::Equals); enter(c,"*3"); c.dispatch(Action::Equals); assertResult(c,1,"1");
    c.dispatch(Action::Clear);
    script.add("0.1+0.2",0.30000000000000004,"0.3"); script.add("0.2",.2,"0.2");
    script.add("(0.30000000000000004)-0.3",0x1p-54,"5.55111512312578e-17"); script.add("0.3",.3,"0.3");
    enter(c,"0.1+0.2"); c.dispatch(Action::Equals); enter(c,"-0.3"); c.dispatch(Action::Equals); assertResult(c,0x1p-54,"5.55111512312578e-17");
    std::cout << "PASS exact-bit transport (host strtod oracle), injected precision continuations; no Rust arithmetic claim\n";
}
void recover(Controller& c, Script& s) {
    // Supplemental sequential recovery smoke test. directRecovery() below recreates
    // each original failure separately for every required recovery action.
    const auto before=s.calls.size();
    while (!c.state().expression.empty()) c.dispatch(Action::Backspace);
    CHECK(s.calls.size()==before);
    enter(c,"7"); s.add("7",7,"7"); c.dispatch(Action::Equals); assertResult(c,7,"7");
    auto induce = [&] {
        c.dispatch(Action::Clear); enter(c,"2+");
        s.expected.push_back({"2+",failure(CALC_SYNTAX)}); c.dispatch(Action::Equals); assertError(c);
    };
    induce(); enter(c,".5"); s.add(".5",.5,"0.5"); c.dispatch(Action::Equals); assertResult(c,.5,"0.5");
    induce();
    enter(c,"7"); s.add("7",7,"7"); c.dispatch(Action::Equals); c.dispatch(Action::Equals); assertResult(c,7,"7");
    c.dispatch(Action::Clear); CHECK(!c.state().repeat && !c.state().value && c.state().expression.empty());
    enter(c,"2+3"); s.add("2+3",5,"5"); s.add("3",3,"3"); c.dispatch(Action::Equals); assertResult(c,5,"5");
}
void failures() {
    for (auto status : {CALC_INVALID_ARGUMENT, CALC_INVALID_UTF8, CALC_SYNTAX, CALC_DIVIDE_BY_ZERO,
        CALC_DOMAIN,CALC_RANGE,CALC_BUFFER_TOO_SMALL,CALC_LIMIT,CALC_INTERNAL, uint32_t(999)}) {
        CHECK(!errorKey(status).empty());
        for (bool rhs : {false,true}) {
            Script s; if (rhs) s.add("2+3",5,"5");
            s.expected.push_back({rhs ? "3" : "2+3", failure(status)});
            Controller c(s.evaluator()); enter(c,"2+3"); c.dispatch(Action::Equals); assertError(c);
            CHECK(s.calls.size() == (rhs ? 2 : 1)); CHECK(c.state().expression == "2+3"); recover(c,s);
        }
    }
    Script s; s.expected.push_back({"2+", failure(CALC_SYNTAX)});
    Controller c(s.evaluator()); enter(c,"2+"); c.dispatch(Action::Equals); assertError(c);
    c.dispatch(Action::Backspace); CHECK(c.state().expression == "2"); enter(c,"+3");
    s.add("2+3",5,"5"); s.add("3",3,"3"); c.dispatch(Action::Equals); assertResult(c,5,"5");
    c.dispatch(Action::Clear); s.add("1e154*1e154",1e308,"1e308"); s.add("1e154",1e154,"1e154");
    enter(c,"1e154*1e154"); c.dispatch(Action::Equals);
    s.expected.push_back({composed(1e308,'*',1e154),failure(CALC_RANGE)});
    c.dispatch(Action::Equals); assertError(c); recover(c,s);
    for (bool continuation : {false,true}) {
        Script calls; calls.add("2+3",5,"5"); calls.add("3",3,"3");
        Controller conversion(calls.evaluator(), [](double) -> std::optional<std::string> { return std::nullopt; });
        enter(conversion,"2+3"); conversion.dispatch(Action::Equals);
        conversion.dispatch(continuation ? Action::Input : Action::Equals, '*');
        assertError(conversion); CHECK(calls.calls.size() == 2); recover(conversion,calls);
    }
    // Allocation/transport exceptions are contained; literal composition has real byte bound.
    Script huge; huge.add("7",7,"7");
    Controller limit(huge.evaluator(), [](double) { return std::optional<std::string>(std::string(4096,'1')); });
    enter(limit,"7"); limit.dispatch(Action::Equals); enter(limit,"*"); assertError(limit); CHECK(huge.calls.size() == 1);
    Controller throwing([](const std::string&) -> Evaluation { throw std::bad_alloc(); });
    enter(throwing,"7"); throwing.dispatch(Action::Equals); assertError(throwing);
    std::cout << "PASS injected full/RHS/repeat/transport errors, invalidation, no-call, correction and recovery\n";
}
// FFI unit double: scripted output, no parser/calculator or real producer linkage.
struct Ffi {
    static inline uint32_t version = CALC_ABI_VERSION;
    static inline uint32_t status = CALC_OK;
    static inline double value = 5;
    static inline std::string output = "5";
    static inline std::string input = "2+3";
    static inline size_t calls = 0;
    static inline bool retryFails = false;
    static uint32_t getVersion() { return version; }
    static uint32_t call(const uint8_t* expression,size_t length,uint32_t angle,double* out,char* display,
        size_t capacity,size_t* required,size_t* offset) {
        ++calls; FIXTURE(angle == CALC_ANGLE_RAD); FIXTURE(std::string(reinterpret_cast<const char*>(expression),length) == input);
        const auto disjoint = [](const void* a,size_t n,const void* b,size_t m) {
            auto x = reinterpret_cast<uintptr_t>(a), y = reinterpret_cast<uintptr_t>(b);
            return !n || !m || x+n <= y || y+m <= x;
        };
        const std::array<std::pair<const void*,size_t>,5> regions = {{{expression,length},{out,sizeof(*out)},
            {display,capacity},{required,sizeof(*required)},{offset,sizeof(*offset)}}};
        for (size_t i=0;i<regions.size();++i) for (size_t j=i+1;j<regions.size();++j)
            FIXTURE(disjoint(regions[i].first,regions[i].second,regions[j].first,regions[j].second));
        if (status == CALC_INVALID_ARGUMENT) return status; // leave outputs untouched
        *out=0; *required=0; *offset=CALC_NO_ERROR_OFFSET; if(capacity) display[0]='\0';
        if (status != CALC_OK) { *offset=1; return status; }
        if (retryFails && calls%2 == 0) return CALC_INTERNAL;
        *out=value; *required=output.size()+1;
        if (capacity < *required) return CALC_BUFFER_TOO_SMALL;
        std::memcpy(display,output.c_str(),*required); return CALC_OK;
    }
    static void reset() { version=CALC_ABI_VERSION; status=CALC_OK; value=5; output="5"; input="2+3"; calls=0; retryFails=false; }
};
void adapter() {
    for (size_t capacity : {size_t(0),size_t(1),size_t(64)}) {
        Ffi::reset(); Adapter a(Ffi::getVersion,Ffi::call,capacity); auto r=a("2+3");
        CHECK(r.ok() && r.value==5 && r.display=="5"); CHECK(Ffi::calls==(capacity<2 ? 2 : 1));
    }
    Ffi::reset(); Ffi::version=0x20000; CHECK(!Adapter(Ffi::getVersion,Ffi::call)("2+3").ok()); CHECK(Ffi::calls==0);
    Ffi::version=0x10001; CHECK(Adapter(Ffi::getVersion,Ffi::call)("2+3").ok());
    for (uint32_t status=1;status<=10;++status) {
        Ffi::reset(); Ffi::status=status; auto r=Adapter(Ffi::getVersion,Ffi::call)("2+3");
        CHECK(!r.ok()); CHECK(r.status==status); CHECK(r.value==0 && r.display.empty());
    }
    Ffi::reset(); Ffi::retryFails=true;
    CHECK(!Adapter(Ffi::getVersion,Ffi::call,0)("2+3").ok()); CHECK(Ffi::calls==2);
    Ffi::reset(); Ffi::input="\xc3\xa9+1"; Ffi::status=CALC_SYNTAX;
    auto r=Adapter(Ffi::getVersion,Ffi::call)(Ffi::input); CHECK(r.offset==1);
    CHECK(utf16Position(Ffi::input,2)==1); CHECK(utf16Position(Ffi::input,1)==0);
    CHECK(utf16Position("\xf0\x9f\x98\x80+1",4)==2);
    CHECK(utf16Position("abc",99)==3); CHECK(utf16Position("abc",CALC_NO_ERROR_OFFSET)==CALC_NO_ERROR_OFFSET);
    Ffi::reset(); Ffi::output="bad"; CHECK(!Adapter(Ffi::getVersion,Ffi::call)("2+3").ok());
    Ffi::reset(); Ffi::value=std::numeric_limits<double>::infinity(); CHECK(!Adapter(Ffi::getVersion,Ffi::call)("2+3").ok());
    Ffi::reset(); Ffi::output=std::string(64,'1'); CHECK(!Adapter(Ffi::getVersion,Ffi::call)("2+3").ok()); CHECK(Ffi::calls==1);
    // Actual controller -> adapter unit-double path at full, RHS, repeat; retries separate from logical calls.
    for (size_t failedStage=0;failedStage<4;++failedStage) {
        size_t logical=0;
        Controller c([&](const std::string& text) {
            ++logical; Ffi::reset(); Ffi::input=text; Ffi::retryFails=logical==failedStage;
            if(logical==1) { Ffi::value=14; Ffi::output="14"; }
            else if(logical==2) { Ffi::value=7; Ffi::output="7"; }
            else { Ffi::value=98; Ffi::output="98"; }
            auto result=Adapter(Ffi::getVersion,Ffi::call,0)(text); FIXTURE(Ffi::calls==2); return result;
        });
        enter(c,"2*(3+4)"); c.dispatch(Action::Equals);
        if(failedStage==1 || failedStage==2) { assertError(c); CHECK(logical==failedStage); }
        else { assertResult(c,14,"14"); c.dispatch(Action::Equals);
            if(failedStage==3) assertError(c); else assertResult(c,98,"98"); }
    }
    std::cout << "PASS adapter FFI-double lifetime/disjoint outputs/version/status/UTF-8/retry at each controller stage\n";
}
void signBoundary() {
    Script s; Controller c(s.evaluator());
    enter(c,std::string(CALC_MAX_INPUT_BYTES-1,'1'));
    c.dispatch(Action::Sign); CHECK(c.state().expression.size()==CALC_MAX_INPUT_BYTES);
    CHECK(c.state().expression.front()=='-');
    c.dispatch(Action::Sign); CHECK(c.state().expression.size()==CALC_MAX_INPUT_BYTES-1);
    enter(c,"1"); const auto bounded=c.state().expression;
    c.dispatch(Action::Sign); assertError(c); CHECK(c.state().error==CALC_LIMIT);
    CHECK(c.state().expression==bounded); CHECK(s.calls.empty());
    c.dispatch(Action::Backspace); CHECK(c.state().expression.size()==CALC_MAX_INPUT_BYTES-1);
    c.dispatch(Action::Sign); CHECK(c.state().expression.size()==CALC_MAX_INPUT_BYTES);
    c.dispatch(Action::Clear); CHECK(c.state().expression.empty()); CHECK(c.state().error==CALC_OK);
    enter(c,"7"); s.add("7",7,"7"); c.dispatch(Action::Equals); assertResult(c,7,"7");
    std::cout << "PASS Sign boundary 4095/4096, rejection preserves text, removal and recovery\n";
}
void directRecovery() {
    // Every action gets a newly recreated ORIGINAL failure, including adapter retries.
    const std::vector<std::string> stages={"full","rhs","repeat","transport-repeat",
        "transport-continue","transport-throw","composition-limit","retry-full","retry-rhs","retry-repeat","evaluate-throw"};
    for (const auto& stage : stages) {
        const bool normal=stage=="full" || stage=="rhs" || stage=="repeat";
        const std::vector<uint32_t> statuses=normal ? std::vector<uint32_t>{CALC_INVALID_ARGUMENT,
            CALC_INVALID_UTF8,CALC_SYNTAX,CALC_DIVIDE_BY_ZERO,CALC_DOMAIN,CALC_RANGE,
            CALC_BUFFER_TOO_SMALL,CALC_LIMIT,CALC_INTERNAL,999} : std::vector<uint32_t>{CALC_INTERNAL};
        for (auto status : statuses) for (int action=0;action<4;++action) {
            Script s; bool injecting=true; size_t physical=0, logical=0;
            const bool retry=stage.find("retry-")==0;
            const bool full=stage=="full" || stage=="retry-full" || stage=="evaluate-throw";
            const bool rhs=stage=="rhs" || stage=="retry-rhs";
            const bool repeat=stage=="repeat" || stage=="retry-repeat";
            if (full) s.expected.push_back({"2+3",failure(status)});
            else {
                s.add("2+3",5,"5");
                if (rhs) s.expected.push_back({"3",failure(status)});
                else s.add("3",3,"3");
                if (repeat) s.expected.push_back({"(5)+(3)",failure(status)});
            }
            Controller c([&](const std::string& text) {
                ++logical;
                const auto result=s(text);
                if (injecting && stage=="evaluate-throw") throw std::bad_alloc(); // deliberate injection
                if (!retry) return result;
                Ffi::reset(); Ffi::input=text; Ffi::value=result.ok()?result.value:5;
                Ffi::output=result.ok()?result.display:"5"; Ffi::retryFails=!result.ok();
                auto actual=Adapter(Ffi::getVersion,Ffi::call,0)(text);
                physical+=Ffi::calls; FIXTURE(Ffi::calls==2);
                FIXTURE(actual.status==result.status); return actual;
            }, [&](double value) -> std::optional<std::string> {
                if (injecting && stage=="transport-throw") throw std::bad_alloc(); // deliberate injection
                if (injecting && (stage=="transport-repeat" || stage=="transport-continue")) return std::nullopt;
                if (injecting && stage=="composition-limit") return std::string(CALC_MAX_INPUT_BYTES,'1');
                return transport(value);
            });
            enter(c,"2+3"); c.dispatch(Action::Equals);
            if (!full && !rhs) c.dispatch(stage=="transport-continue" || stage=="composition-limit" ? Action::Input : Action::Equals,'*');
            assertError(c);
            const auto expectedStatus=stage=="composition-limit"?CALC_LIMIT:status;
            CHECK(c.state().error==expectedStatus); CHECK(s.expected.empty());
            CHECK(logical==(full?1:rhs?2:repeat?3:2));
            if(retry) CHECK(physical==2*logical);
            CHECK(fixtureViolations.empty());
            injecting=false; const auto before=logical, physicalBefore=physical;
            std::string payload; double value=7; std::string display="7";
            if (action==0) {
                // Correct only the retained suffix, preserving the failed prefix.
                if (c.state().expression=="(5)+(3)") {
                    c.dispatch(Action::Backspace); c.dispatch(Action::Backspace); enter(c,"4)");
                    payload="(5)+(4)"; value=9; display="9";
                } else {
                    CHECK(c.state().expression=="2+3");
                    c.dispatch(Action::Backspace); enter(c,"4"); payload="2+4"; value=6; display="6";
                }
            } else if(action==1) { enter(c,"7"); payload="7"; }
            else if(action==2) { enter(c,".5"); payload=".5"; value=.5; display="0.5"; }
            else {
                c.dispatch(Action::Clear); CHECK(c.state().expression.empty());
                CHECK(c.state().phase==Phase::Editing); CHECK(c.state().error==CALC_OK);
                CHECK(!c.state().repeat && !c.state().value && c.state().display.empty());
                enter(c,"7"); payload="7";
            }
            CHECK(c.state().expression==payload); CHECK(c.state().phase==Phase::Editing);
            CHECK(c.state().error==CALC_OK); CHECK(!c.state().value && !c.state().repeat && c.state().display.empty());
            CHECK(logical==before); CHECK(physical==physicalBefore);
            s.add(payload,value,display);
            const auto selected=selectRepeat(payload);
            if(selected) s.add(selected->rhs,4,"4");
            c.dispatch(Action::Equals); assertResult(c,value,display);
            CHECK(logical==before+(selected?2:1)); CHECK(s.expected.empty());
            if(retry) CHECK(physical==2*logical);
            CHECK(fixtureViolations.empty());
            const auto completed=logical;
            if(selected) { CHECK(c.state().repeat->operation==selected->operation); CHECK(bits(c.state().repeat->rhs)==bits(4)); }
            else { c.dispatch(Action::Equals); assertResult(c,value,display); CHECK(logical==completed); }
        }
        std::cout << "PASS direct original-stage recovery (suffix/digit/decimal/Clear), exact statuses/payloads/calls: " << stage << '\n';
    }
}

}
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--fixture-violation") {
            Ffi::reset(); Ffi::input="intentionally wrong payload"; Ffi::status=CALC_INVALID_ARGUMENT;
            auto r=Adapter(Ffi::getVersion,Ffi::call)("2+3");
            CHECK(r.status==CALC_INTERNAL); // production contains the exception, fixture still fails
        } else {
            repeats(); editing(); presentation(); precision(); failures(); adapter();
            directRecovery(); signBoundary();
        }
        if (!fixtureViolations.empty()) {
            for (const auto& message : fixtureViolations) std::cerr << "FIXTURE FAILURE: " << message << '\n';
            return 1;
        }
        std::cout << "PASS " << assertions << " assertions; all evaluation results are scripted unit doubles. Qt/Rust/GUI not exercised.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
