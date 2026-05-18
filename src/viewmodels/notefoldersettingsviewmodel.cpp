#include "notefoldersettingsviewmodel.h"

#include <core/commands/commandbus.h>
#include <core/data/commands.h>
#include <core/repositories/notefolderrepository.h>
#include <core/repositories/notesubfolderrepository.h>

NoteFolderSettingsViewModel::NoteFolderSettingsViewModel(CommandBus* commandBus,
                                                         NoteFolderRepository* noteFolderRepository,
                                                         NoteSubFolderRepository* noteSubFolderRepository,
                                                         QObject* parent)
    : QObject(parent),
      _commandBus(commandBus),
      _noteFolderRepository(noteFolderRepository),
      _noteSubFolderRepository(noteSubFolderRepository) {}

QList<NoteFolderData> NoteFolderSettingsViewModel::noteFolders() const {
    return _noteFolderRepository == nullptr ? QList<NoteFolderData>() : _noteFolderRepository->findAll();
}

NoteFolderData NoteFolderSettingsViewModel::noteFolder(int folderId) const {
    return _noteFolderRepository == nullptr ? NoteFolderData() : _noteFolderRepository->findById(folderId);
}

int NoteFolderSettingsViewModel::noteFolderCount() const {
    return _noteFolderRepository == nullptr ? 0 : _noteFolderRepository->countAll();
}

int NoteFolderSettingsViewModel::currentFolderId() const {
    return _noteFolderRepository == nullptr ? 0 : _noteFolderRepository->currentFolderId();
}

QVariant NoteFolderSettingsViewModel::noteFolderSetting(int folderId, const QString& key,
                                                        const QVariant& defaultValue) const {
    return _noteFolderRepository == nullptr ? defaultValue
                                            : _noteFolderRepository->settingValue(folderId, key, defaultValue);
}

QString NoteFolderSettingsViewModel::subFolderTreeExpandStateSettingsKey(int noteFolderId) const {
    return _noteSubFolderRepository == nullptr
               ? QString()
               : _noteSubFolderRepository->treeWidgetExpandStateSettingsKey(noteFolderId);
}

bool NoteFolderSettingsViewModel::willSubFolderBeIgnored(const QString& folderName) const {
    return _noteSubFolderRepository != nullptr && _noteSubFolderRepository->willFolderBeIgnored(folderName);
}

QString NoteFolderSettingsViewModel::defaultIgnoredSubfoldersPattern() const {
    return _noteSubFolderRepository == nullptr ? QString()
                                               : _noteSubFolderRepository->defaultIgnoredSubfoldersPattern();
}

NoteFolderData NoteFolderSettingsViewModel::saveNoteFolder(const NoteFolderData& folder) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return NoteFolderData();
    }

    SaveNoteFolderCommand command;
    command.folder = folder;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
        return NoteFolderData();
    }
    return result.resultData.value<NoteFolderData>();
}

bool NoteFolderSettingsViewModel::removeNoteFolder(int folderId) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return false;
    }

    RemoveNoteFolderCommand command;
    command.folderId = folderId;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
    return result.success;
}

bool NoteFolderSettingsViewModel::setCurrentFolderId(int folderId) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return false;
    }

    SetCurrentNoteFolderCommand command;
    command.folderId = folderId;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
    return result.success;
}

bool NoteFolderSettingsViewModel::setNoteFolderSetting(int folderId, const QString& key, const QVariant& value) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return false;
    }

    SetNoteFolderSettingCommand command;
    command.folderId = folderId;
    command.key = key;
    command.value = value;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
    return result.success;
}

bool NoteFolderSettingsViewModel::removePersistentSetting(const QString& key) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return false;
    }

    RemoveSettingCommand command;
    command.key = key;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
    return result.success;
}

bool NoteFolderSettingsViewModel::updateFolderPriorities(const QVector<int>& folderIds) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return false;
    }

    UpdateNoteFolderPrioritiesCommand command;
    command.folderIds = folderIds;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
    return result.success;
}
