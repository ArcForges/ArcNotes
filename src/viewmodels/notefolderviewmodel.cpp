#include "notefolderviewmodel.h"

#include <core/commands/commandbus.h>
#include <core/data/commands.h>
#include <core/state/appstate.h>

NoteFolderViewModel::NoteFolderViewModel(CommandBus* commandBus, AppState* appState, QObject* parent)
    : QObject(parent), _commandBus(commandBus), _appState(appState), _model(this) {
    if (_appState != nullptr) {
        connect(_appState, &AppState::currentNoteFolderChanged, this, &NoteFolderViewModel::syncFromState);
        syncFromState();
    }
}

NoteFolderListModel* NoteFolderViewModel::model() {
    return &_model;
}

const NoteFolderListModel* NoteFolderViewModel::model() const {
    return &_model;
}

int NoteFolderViewModel::currentFolderId() const {
    return _currentFolder.id;
}

QString NoteFolderViewModel::currentFolderName() const {
    return _currentFolder.name;
}

void NoteFolderViewModel::setCommandBus(CommandBus* commandBus) {
    _commandBus = commandBus;
}

void NoteFolderViewModel::setAppState(AppState* appState) {
    _appState = appState;
    syncFromState();
}

void NoteFolderViewModel::setFolders(const QVector<NoteFolderData>& folders) {
    _model.setFolders(folders);
}

void NoteFolderViewModel::switchFolder(int folderId) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    SwitchFolderCommand command;
    command.folderId = folderId;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void NoteFolderViewModel::syncFromState() {
    if (_appState == nullptr) {
        return;
    }

    const NoteFolderData folder = _appState->currentNoteFolder();
    if (_currentFolder == folder) {
        return;
    }
    _currentFolder = folder;
    emit currentFolderChanged();
}
