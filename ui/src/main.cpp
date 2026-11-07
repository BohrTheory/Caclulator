#include "window.hpp"
#include <QApplication>
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    const calculator::Adapter adapter(calc_abi_version, calc_evaluate_v1);
    calculator::Window window([&adapter](const std::string& text) { return adapter(text); });
    window.show();
    return app.exec();
}
