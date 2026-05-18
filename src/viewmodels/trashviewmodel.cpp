#include "trashviewmodel.h"

#include <core/commands/commandbus.h>
#include <core/data/commands.h>

TrashViewModel::TrashViewModel(CommandBus* commandBus, QObject* parent)
    : QObject(parent), _commandBus(commandBus), _model(this) {}

TrashItemModel* TrashViewModel::model() {
    return &_model;
}

const TrashItemModel* TrashViewModel::model() const {
    return &_model;
}

void TrashViewModel::setCommandBus(CommandBus* commandBus) {
    _commandBus = commandBus;
}

void TrashViewModel::setTrashItems(const QVector<TrashItemData>& trashItems) {
    _model.setTrashItems(trashItems);
}

void TrashViewModel::restoreItem(int trashItemId) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    RestoreTrashCommand command;
    command.trashItemIds = {trashItemId};
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void TrashViewModel::clearTrash(bool expiredOnly) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    ClearTrashCommand command;
    command.expiredOnly = expiredOnly;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}
