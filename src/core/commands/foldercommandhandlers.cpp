#include "foldercommandhandlers.h"

#include <core/data/commands.h>
#include <core/services/notefolderservice.h>
#include <core/services/notesubfolderservice.h>
#include <core/state/appstate.h>

#include "commandbus.h"

void FolderCommandHandlers::registerHandlers(CommandBus* bus, NoteFolderService* folderService,
                                             NoteSubFolderService* subFolderService, AppState* appState) {
    bus->registerHandler<SwitchFolderCommand>([folderService, appState](const SwitchFolderCommand& command) {
        for (const NoteFolderData& folder : folderService->folders()) {
            if (folder.id == command.folderId) {
                if (appState != nullptr) {
                    appState->setCurrentNoteFolder(folder);
                }
                return CommandResult::ok(QString(), command.folderId);
            }
        }
        return CommandResult::fail(QObject::tr("Folder could not be switched."));
    });

    bus->registerHandler<SaveNoteFolderCommand>([folderService, appState](const SaveNoteFolderCommand& command) {
        const NoteFolderData folder = folderService->saveFolderAndReturn(command.folder);
        if (folder.id <= 0) {
            return CommandResult::fail(QObject::tr("Folder could not be saved."));
        }

        if (appState != nullptr && appState->currentNoteFolder().id == folder.id) {
            appState->setCurrentNoteFolder(folder);
        }
        return CommandResult::ok(QString(), QVariant::fromValue(folder));
    });

    bus->registerHandler<RemoveNoteFolderCommand>([folderService, appState](const RemoveNoteFolderCommand& command) {
        const bool wasCurrent = folderService->currentFolderId() == command.folderId;
        if (!folderService->deleteFolder(command.folderId)) {
            return CommandResult::fail(QObject::tr("Folder could not be removed."));
        }

        if (wasCurrent) {
            const QList<NoteFolderData> folders = folderService->folders();
            const NoteFolderData nextFolder = folders.isEmpty() ? NoteFolderData() : folders.at(0);
            if (nextFolder.id > 0) {
                folderService->setCurrentFolderId(nextFolder.id);
            }
            if (appState != nullptr) {
                appState->setCurrentNoteFolder(nextFolder);
            }
        }

        return CommandResult::ok(QString(), command.folderId);
    });

    bus->registerHandler<SetCurrentNoteFolderCommand>(
        [folderService, appState](const SetCurrentNoteFolderCommand& command) {
            const NoteFolderData folder = folderService->folder(command.folderId);
            if (folder.id <= 0) {
                return CommandResult::fail(QObject::tr("Folder could not be selected."));
            }

            folderService->setCurrentFolderId(folder.id);
            if (appState != nullptr) {
                appState->setCurrentNoteFolder(folder);
            }
            return CommandResult::ok(QString(), folder.id);
        });

    bus->registerHandler<SetNoteFolderSettingCommand>([folderService](const SetNoteFolderSettingCommand& command) {
        if (command.folderId <= 0 || command.key.isEmpty()) {
            return CommandResult::fail(QObject::tr("Folder setting could not be saved."));
        }

        folderService->setSettingValue(command.folderId, command.key, command.value);
        return CommandResult::ok();
    });

    bus->registerHandler<UpdateNoteFolderPrioritiesCommand>(
        [folderService](const UpdateNoteFolderPrioritiesCommand& command) {
            return folderService->updatePriorities(command.folderIds)
                       ? CommandResult::ok()
                       : CommandResult::fail(QObject::tr("Folder order could not be saved."));
        });

    bus->registerHandler<CreateSubFolderCommand>([subFolderService, appState](const CreateSubFolderCommand& command) {
        const NoteSubFolderData subFolder = subFolderService->createSubFolder(command.name, command.parentId);
        if (subFolder.id > 0 && appState != nullptr) {
            appState->setCurrentNoteSubFolder(subFolder);
        }
        return subFolder.id > 0 ? CommandResult::ok(QString(), subFolder.id)
                                : CommandResult::fail(QObject::tr("Subfolder could not be created."));
    });

    bus->registerHandler<SetActiveSubFolderCommand>(
        [subFolderService, appState](const SetActiveSubFolderCommand& command) {
            if (!subFolderService->setActiveSubFolder(command.subFolderId)) {
                return CommandResult::fail(QObject::tr("Subfolder could not be selected."));
            }

            if (appState != nullptr) {
                appState->setCurrentNoteSubFolder(subFolderService->subFolder(command.subFolderId));
            }
            return CommandResult::ok(QString(), command.subFolderId);
        });

    bus->registerHandler<DeleteSubFolderCommand>([subFolderService](const DeleteSubFolderCommand& command) {
        return subFolderService->deleteSubFolder(command.subFolderId)
                   ? CommandResult::ok(QString(), command.subFolderId)
                   : CommandResult::fail(QObject::tr("Subfolder could not be deleted."));
    });

    bus->registerHandler<RenameSubFolderCommand>([subFolderService](const RenameSubFolderCommand& command) {
        return subFolderService->renameSubFolder(command.subFolderId, command.name)
                   ? CommandResult::ok(QString(), command.subFolderId)
                   : CommandResult::fail(QObject::tr("Subfolder could not be renamed."));
    });

    bus->registerHandler<MoveSubFolderCommand>([subFolderService](const MoveSubFolderCommand& command) {
        return subFolderService->moveSubFolder(command.subFolderId, command.destinationParentId)
                   ? CommandResult::ok(QString(), command.subFolderId)
                   : CommandResult::fail(QObject::tr("Subfolder could not be moved."));
    });
}
