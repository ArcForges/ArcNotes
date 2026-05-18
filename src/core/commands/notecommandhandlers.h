#pragma once

class CommandBus;
class AppState;
class EditorState;
class NoteFileService;
class NoteLinkService;
class NoteService;
class TrashService;

class NoteCommandHandlers {
public:
    static void registerHandlers(CommandBus* bus, NoteService* noteService, NoteFileService* noteFileService,
                                 TrashService* trashService, NoteLinkService* noteLinkService,
                                 AppState* appState = nullptr, EditorState* editorState = nullptr);
};
