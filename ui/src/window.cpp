#include "window.hpp"
#include <QApplication>
#include <QEvent>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QAccessible>
#include <utility>

namespace calculator {
Window::Window(Evaluate evaluate, QWidget* parent)
    : QWidget(parent), controller_(std::move(evaluate)) {
    setWindowTitle(tr("Calculator"));
    setFocusPolicy(Qt::ClickFocus);
    setFocus();
    setAccessibleName(tr("Calculator"));
    auto* layout = new QVBoxLayout(this);
    expression_ = new QLineEdit(this);
    expression_->setObjectName("expression");
    expression_->setReadOnly(true);
    expression_->setAccessibleName(tr("Expression"));
    expression_->setAccessibleDescription(tr("Use calculator keys to edit the expression"));
    expression_->setFocusPolicy(Qt::NoFocus);
    result_ = new QLineEdit(this);
    result_->setObjectName("result");
    result_->setReadOnly(true);
    result_->setAccessibleName(tr("Result"));
    result_->setFocusPolicy(Qt::NoFocus);
    error_ = new QLabel(this);
    error_->setObjectName("error");
    error_->setWordWrap(true);
    error_->setAccessibleName(tr("Calculation error"));
    layout->addWidget(expression_); layout->addWidget(result_); layout->addWidget(error_);
    auto* grid = new QGridLayout;
    layout->addLayout(grid);
    struct Button { const char* label; const char* name; Action action; char input; };
    const Button buttons[] = {
        {"C", "Clear", Action::Clear, 0}, {"Backspace", "Backspace", Action::Backspace, 0},
        {"+/-", "Toggle sign", Action::Sign, 0}, {"/", "Divide", Action::Input, '/'},
        {"7", "Seven", Action::Input, '7'}, {"8", "Eight", Action::Input, '8'},
        {"9", "Nine", Action::Input, '9'}, {"*", "Multiply", Action::Input, '*'},
        {"4", "Four", Action::Input, '4'}, {"5", "Five", Action::Input, '5'},
        {"6", "Six", Action::Input, '6'}, {"-", "Subtract", Action::Input, '-'},
        {"1", "One", Action::Input, '1'}, {"2", "Two", Action::Input, '2'},
        {"3", "Three", Action::Input, '3'}, {"+", "Add", Action::Input, '+'},
        {"0", "Zero", Action::Input, '0'}, {".", "Decimal point", Action::Input, '.'},
        {"e", "Exponent", Action::Input, 'e'}, {"=", "Equals", Action::Equals, 0},
        {"(", "Open parenthesis", Action::Input, '('}, {")", "Close parenthesis", Action::Input, ')'}
    };
    QPushButton* previous = nullptr;
    for (size_t i = 0; i < sizeof(buttons)/sizeof(buttons[0]); ++i) {
        auto b = buttons[i];
        auto* button = new QPushButton(tr(b.label), this);
        button->setAccessibleName(tr(b.name)); button->setObjectName(QString::fromLatin1(b.name));
        button->setMinimumSize(64, 44);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setAutoDefault(false);
        if (previous) QWidget::setTabOrder(previous, button);
        previous = button;
        connect(button, &QPushButton::clicked, this, [this, b] { act(b.action, b.input); });
        grid->addWidget(button, static_cast<int>(i/4), static_cast<int>(i%4));
    }
    // Preserve native palette and add a focus outline that is independent of color alone.
    setStyleSheet("QPushButton:focus { border: 3px solid palette(highlight); }");
    qApp->installEventFilter(this); // only descendants of this active window are routed
    refresh();
}
bool Window::eventFilter(QObject* object, QEvent* event) {
    if (event->type() != QEvent::KeyPress || !isActiveWindow()) return false;
    auto* widget = qobject_cast<QWidget*>(object);
    if (!widget || widget->window() != this) return false;
    auto* key = static_cast<QKeyEvent*>(event);
    if (key->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) return false;
    if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
        if (auto* button = qobject_cast<QPushButton*>(QApplication::focusWidget())) button->click();
        else act(Action::Equals);
        return true;
    }
    if (key->key() == Qt::Key_Backspace) { act(Action::Backspace); return true; }
    if (key->key() == Qt::Key_Escape) { act(Action::Clear); return true; }
    if (key->text().size() == 1) {
        auto mapped = keyAction(key->text().at(0).toLatin1());
        if (mapped) { setFocus(); act(mapped->action, mapped->input); return true; }
    }
    return false; // native Tab/Shift+Tab and Space activation
}
void Window::act(Action action, char input) { controller_.dispatch(action, input); refresh(); }
void Window::refresh() {
    const auto& state = controller_.state();
    const auto source = QString::fromUtf8(state.expression.data(), static_cast<qsizetype>(state.expression.size()));
    expression_->setText(source);
    result_->setText(QString::fromStdString(state.display));
    QString message;
    if (state.phase == Phase::Error) {
        auto key = errorKey(state.error);
        message = tr(std::string(key).c_str());
        if (state.errorOffset != CALC_NO_ERROR_OFFSET) {
            auto position = utf16Position(state.expression, state.errorOffset);
            message += tr(" (position %1)").arg(static_cast<qulonglong>(position+1));
            expression_->setSelection(static_cast<int>(position), 1);
        }
    }
    const bool changed = error_->text() != message;
    error_->setText(message);
    if (changed && !message.isEmpty()) {
        QAccessibleEvent announcement(error_, QAccessible::Alert);
        QAccessible::updateAccessibility(&announcement);
    }
    if (state.phase == Phase::Result) {
        QAccessibleValueChangeEvent resultEvent(result_, result_->text());
        QAccessible::updateAccessibility(&resultEvent);
    }
}
}
