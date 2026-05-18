#pragma once

#include <QWidget>

class QTextEdit;

class LogView : public QWidget {
    Q_OBJECT

public:
    explicit LogView(QWidget* parent = nullptr);

public slots:
    void appendMessage(const QString& message);
    void clear();

private:
    QTextEdit* _textEdit = nullptr;
};
