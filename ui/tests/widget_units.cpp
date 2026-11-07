// Qt interaction units with an injected recording evaluator. Not external GUI or Rust evidence.
#include "window.hpp"
#include <QApplication>
#include <QAccessible>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTest>
#include <iostream>
#include <stdexcept>
using namespace calculator;
#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
int main(int argc, char** argv) {
    QApplication app(argc,argv);
    try {
        size_t calls=0;
        Window window([&](const std::string& input) -> Evaluation {
            ++calls;
            if (input=="2+3") return {CALC_OK,5,"5",CALC_NO_ERROR_OFFSET};
            if (input=="8/0") return {CALC_DIVIDE_BY_ZERO,0,"",1};
            if (input=="3") return {CALC_OK,3,"3",CALC_NO_ERROR_OFFSET};
            return {CALC_SYNTAX,0,"",input.size()};
        });
        window.show(); window.activateWindow(); window.setFocus();
        REQUIRE(QTest::qWaitForWindowActive(&window));
        const auto buttons=window.findChildren<QPushButton*>();
        REQUIRE(buttons.size()==22);
        for (auto* button : buttons) {
            REQUIRE(!button->accessibleName().isEmpty()); REQUIRE(button->focusPolicy()==Qt::StrongFocus);
            REQUIRE(!button->text().contains('%'));
        }
        auto* expression=window.findChild<QLineEdit*>("expression");
        auto* result=window.findChild<QLineEdit*>("result");
        auto* error=window.findChild<QLabel*>("error");
        REQUIRE(expression && result && error);
        QTest::keyClicks(&window,"2+3"); QTest::keyClick(&window,Qt::Key_Return);
        REQUIRE(result->text()=="5"); REQUIRE(calls==2);
        QTest::keyClick(&window,Qt::Key_Escape); REQUIRE(expression->text().isEmpty());
        QTest::keyClicks(&window,"2+"); QTest::keyClick(&window,Qt::Key_Equal);
        REQUIRE(!error->text().isEmpty()); REQUIRE(result->text().isEmpty()); REQUIRE(expression->text()=="2+");
        QTest::keyClick(&window,Qt::Key_Backspace); QTest::keyClicks(&window,"+3");
        QTest::keyClick(&window,Qt::Key_Equal); REQUIRE(result->text()=="5");
        auto* accessible=QAccessible::queryAccessibleInterface(error);
        REQUIRE(accessible);
        QTest::keyClick(&window,Qt::Key_Escape);
        REQUIRE(accessible->text(QAccessible::Name).isEmpty());
        QTest::keyClicks(&window,"8/0=");
        REQUIRE(error->text()=="Cannot divide by zero (position 2)");
        REQUIRE(accessible->text(QAccessible::Name)==error->text());
        const auto division=accessible->text(QAccessible::Name);
        QTest::keyClick(&window,Qt::Key_Escape);
        REQUIRE(accessible->text(QAccessible::Name).isEmpty());
        QTest::keyClicks(&window,"2+=");
        REQUIRE(error->text()=="Check the expression (position 3)");
        REQUIRE(accessible->text(QAccessible::Name)==error->text());
        REQUIRE(accessible->text(QAccessible::Name)!=division);
        QTest::keyClick(&window,Qt::Key_Backspace); QTest::keyClicks(&window,"+3=");
        REQUIRE(result->text()=="5"); REQUIRE(accessible->text(QAccessible::Name).isEmpty());
        // Stable row-major traversal; native reverse traversal and Space/Enter activation.
        buttons.front()->setFocus();
        for (int i=1;i<buttons.size();++i) {
            QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab); REQUIRE(QApplication::focusWidget()==buttons[i]);
        }
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab,Qt::ShiftModifier);
        REQUIRE(QApplication::focusWidget()==buttons[buttons.size()-2]);
        buttons.front()->setFocus(); QTest::keyClick(buttons.front(),Qt::Key_Space); REQUIRE(expression->text().isEmpty());
        QTest::keyClicks(&window,"7"); buttons.front()->setFocus(); QTest::keyClick(buttons.front(),Qt::Key_Return);
        REQUIRE(expression->text().isEmpty());
        QWidget other; other.show(); other.activateWindow(); REQUIRE(QTest::qWaitForWindowActive(&other));
        QTest::keyClicks(&other,"2+3="); REQUIRE(expression->text().isEmpty());
        std::cout << "PASS Qt injected interaction/focus/error/accessibility-name units; not external GUI conformance\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
