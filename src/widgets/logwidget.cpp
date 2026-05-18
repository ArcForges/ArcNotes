#include "logwidget.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

static QPointer<LogWidget> s_logWidget = nullptr;

LogWidget::LogWidget(QWidget* parent) : QFrame(parent), _logTextEdit(new QPlainTextEdit(this)) {
    s_logWidget = this;
    setObjectName(QStringLiteral("LogWidget"));

    auto* layout = new QVBoxLayout(this);
    _logTextEdit->setReadOnly(true);
    layout->addWidget(_logTextEdit);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    auto* clearButton = new QPushButton(tr("Clear"), this);
    connect(clearButton, &QPushButton::clicked, _logTextEdit, &QPlainTextEdit::clear);
    buttonLayout->addWidget(clearButton);
    layout->addLayout(buttonLayout);
}

LogWidget::~LogWidget() {
    if (s_logWidget == this) {
        s_logWidget = nullptr;
    }
}

bool LogWidget::eventFilter(QObject* obj, QEvent* event) {
    return QFrame::eventFilter(obj, event);
}

QString LogWidget::getLogText() const {
    return _logTextEdit == nullptr ? QString() : _logTextEdit->toPlainText();
}

void LogWidget::log(LogWidget::LogType logType, const QString& text) {
    if (_logTextEdit == nullptr) {
        return;
    }

    const QString line = QStringLiteral("[%1] %2: %3")
                             .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs), logTypeText(logType), text);
    _logTextEdit->appendPlainText(line);
}

QString LogWidget::logTypeText(LogType logType) {
    switch (logType) {
        case DebugLogType:
            return QStringLiteral("Debug");
        case InfoLogType:
            return QStringLiteral("Info");
        case WarningLogType:
            return QStringLiteral("Warning");
        case CriticalLogType:
            return QStringLiteral("Critical");
        case FatalLogType:
            return QStringLiteral("Fatal");
        case StatusLogType:
            return QStringLiteral("Status");
    }

    return QStringLiteral("Log");
}

void LogWidget::logMessageOutput(QtMsgType type, const QMessageLogContext&, const QString& msg) {
    if (s_logWidget == nullptr) {
        return;
    }

    LogType logType = InfoLogType;
    switch (type) {
        case QtDebugMsg:
            logType = DebugLogType;
            break;
        case QtInfoMsg:
            logType = InfoLogType;
            break;
        case QtWarningMsg:
            logType = WarningLogType;
            break;
        case QtCriticalMsg:
            logType = CriticalLogType;
            break;
        case QtFatalMsg:
            logType = FatalLogType;
            break;
    }

    s_logWidget->log(logType, msg);
}
