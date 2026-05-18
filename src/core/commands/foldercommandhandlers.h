#pragma once

class CommandBus;
class AppState;
class NoteFolderService;
class NoteSubFolderService;

class FolderCommandHandlers {
public:
    static void registerHandlers(CommandBus* bus, NoteFolderService* folderService,
                                 NoteSubFolderService* subFolderService, AppState* appState = nullptr);
};
