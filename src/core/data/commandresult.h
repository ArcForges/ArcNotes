#pragma once

#include <QString>
#include <QVariant>

struct CommandResult {
    QString commandId;
    bool success = false;
    int errorCode = 0;
    QString errorMessage;
    QVariant resultData;

    static CommandResult ok(const QString& commandId = QString(), const QVariant& resultData = QVariant()) {
        CommandResult result;
        result.commandId = commandId;
        result.success = true;
        result.resultData = resultData;
        return result;
    }

    static CommandResult fail(const QString& message, int code = 0, const QString& commandId = QString()) {
        CommandResult result;
        result.commandId = commandId;
        result.success = false;
        result.errorCode = code;
        result.errorMessage = message;
        return result;
    }
};

Q_DECLARE_METATYPE(CommandResult)
