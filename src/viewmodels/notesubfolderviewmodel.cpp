#include "notesubfolderviewmodel.h"

#include <core/commands/commandbus.h>
#include <core/data/commands.h>
#include <core/state/appstate.h>

NoteSubFolderViewModel::NoteSubFolderViewModel(CommandBus* commandBus, AppState* appState, QObject* parent)
    : QObject(parent), _commandBus(commandBus), _appState(appState), _model(this) {
    if (_appState != nullptr) {
        connect(_appState, &AppState::currentNoteSubFolderChanged, this,
                [this](const NoteSubFolderData& subFolder) { selectSubFolder(subFolder.id); });
        connect(_appState, &AppState::showNotesFromAllSubFoldersChanged, this,
                &NoteSubFolderViewModel::setShowNotesFromAll);
    }
    connect(&_model, &NoteSubFolderTreeModel::subFolderRenameRequested, this, &NoteSubFolderViewModel::renameSubFolder);
}

NoteSubFolderTreeModel* NoteSubFolderViewModel::model() {
    return &_model;
}

const NoteSubFolderTreeModel* NoteSubFolderViewModel::model() const {
    return &_model;
}

int NoteSubFolderViewModel::activeSubFolderId() const {
    return _activeSubFolderId;
}

bool NoteSubFolderViewModel::showNotesFromAll() const {
    return _showNotesFromAll;
}

void NoteSubFolderViewModel::setCommandBus(CommandBus* commandBus) {
    _commandBus = commandBus;
}

void NoteSubFolderViewModel::setAppState(AppState* appState) {
    _appState = appState;
}

void NoteSubFolderViewModel::setSubFolders(const QVector<NoteSubFolderData>& subFolders) {
    _model.setSubFolders(subFolders);
}

void NoteSubFolderViewModel::setShowNotesFromAll(bool enabled) {
    if (_showNotesFromAll == enabled) {
        return;
    }
    _showNotesFromAll = enabled;
    if (_appState != nullptr) {
        _appState->setShowNotesFromAllSubFolders(enabled);
    }
    emit showNotesFromAllChanged(_showNotesFromAll);
}

void NoteSubFolderViewModel::selectSubFolder(int subFolderId) {
    if (_activeSubFolderId == subFolderId) {
        return;
    }
    _activeSubFolderId = subFolderId;
    if (_commandBus != nullptr) {
        SetActiveSubFolderCommand command;
        command.subFolderId = subFolderId;
        const CommandResult result = _commandBus->dispatch(command);
        if (!result.success) {
            emit commandFailed(result.errorMessage);
        }
    }
    emit activeSubFolderChanged(_activeSubFolderId);
}

void NoteSubFolderViewModel::createSubFolder(const QString& name, int parentId) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    CreateSubFolderCommand command;
    command.name = name;
    command.parentId = parentId;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void NoteSubFolderViewModel::deleteSubFolder(int subFolderId) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    DeleteSubFolderCommand command;
    command.subFolderId = subFolderId;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void NoteSubFolderViewModel::renameSubFolder(int subFolderId, const QString& name) {
    if (_commandBus == nullptr || subFolderId <= 0 || name.trimmed().isEmpty()) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    RenameSubFolderCommand command;
    command.subFolderId = subFolderId;
    command.name = name.trimmed();
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void NoteSubFolderViewModel::moveSubFolder(int subFolderId, int destinationParentId) {
    if (_commandBus == nullptr || subFolderId <= 0) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    MoveSubFolderCommand command;
    command.subFolderId = subFolderId;
    command.destinationParentId = destinationParentId;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}
