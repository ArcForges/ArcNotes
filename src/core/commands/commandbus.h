#pragma once

#include <core/data/commandresult.h>

#include <QHash>
#include <QObject>
#include <QString>
#include <functional>

#include "pendingcommandtracker.h"

class CommandBus : public QObject {
    Q_OBJECT

public:
    explicit CommandBus(QObject* parent = nullptr);
    ~CommandBus() override;

    template <typename TCommand>
    void registerHandler(std::function<CommandResult(const TCommand&)> handler) {
        _handlers.insert(QString::fromLatin1(TCommand::Type), [handler](const void* command) -> CommandResult {
            return handler(*static_cast<const TCommand*>(command));
        });
    }

    template <typename TCommand>
    CommandResult dispatch(const TCommand& command) {
        const QString commandType = QString::fromLatin1(TCommand::Type);
        const QString commandId = nextCommandId();

        emit commandDispatched(commandType, commandId);
        _pendingTracker->markBusy(commandType);

        CommandResult result;
        const auto handler = _handlers.constFind(commandType);
        if (handler == _handlers.constEnd()) {
            result =
                CommandResult::fail(tr("No handler registered for command '%1'.").arg(commandType), 404, commandId);
        } else {
            result = handler.value()(&command);
            if (result.commandId.isEmpty()) {
                result.commandId = commandId;
            }
        }

        _pendingTracker->clearBusy(commandType);

        if (result.success) {
            emit commandCompleted(commandId, result);
        } else {
            emit commandFailed(commandId, result);
        }

        return result;
    }

    PendingCommandTracker* pendingTracker() const;

signals:
    void commandDispatched(const QString& commandType, const QString& commandId);
    void commandCompleted(const QString& commandId, const CommandResult& result);
    void commandFailed(const QString& commandId, const CommandResult& result);

private:
    QString nextCommandId() const;

    QHash<QString, std::function<CommandResult(const void*)>> _handlers;
    PendingCommandTracker* _pendingTracker;
};
