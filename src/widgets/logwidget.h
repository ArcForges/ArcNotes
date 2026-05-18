#pragma once

#include <QEvent>
#include <QFrame>

class QPlainTextEdit;

class LogWidget : public QFrame {
    Q_OBJECT

public:
    enum LogType { DebugLogType, InfoLogType, WarningLogType, CriticalLogType, FatalLogType, StatusLogType };
    Q_ENUM(LogType)

    explicit LogWidget(QWidget* parent = nullptr);
    ~LogWidget() override;

    static void logMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    bool eventFilter(QObject* obj, QEvent* event) override;
    [[nodiscard]] QString getLogText() const;

public slots:
    void log(LogWidget::LogType logType, const QString& text);

private:
    QPlainTextEdit* _logTextEdit = nullptr;

    static QString logTypeText(LogType logType);
};
