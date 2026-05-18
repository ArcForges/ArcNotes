#include "logview.h"

#include <QTextEdit>
#include <QVBoxLayout>

LogView::LogView(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    _textEdit = new QTextEdit(this);
    _textEdit->setObjectName(QStringLiteral("logTextEdit"));
    _textEdit->setReadOnly(true);
    layout->addWidget(_textEdit);
}

void LogView::appendMessage(const QString& message) {
    _textEdit->append(message);
}

void LogView::clear() {
    _textEdit->clear();
}
