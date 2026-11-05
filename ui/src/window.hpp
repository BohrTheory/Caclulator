#pragma once
#include "calculator.hpp"
#include <QWidget>
class QLineEdit;
class QLabel;
namespace calculator {
class Window : public QWidget {
public:
    explicit Window(Evaluate evaluate, QWidget* parent = nullptr);
protected:
    bool eventFilter(QObject* object, QEvent* event) override;
private:
    Controller controller_;
    QLineEdit* expression_;
    QLineEdit* result_;
    QLabel* error_;
    void act(Action action, char input = 0);
    void refresh();
};
}
