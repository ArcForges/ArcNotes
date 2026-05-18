#pragma once

class CommandBus;
class MediaService;
class NoteService;

class MediaCommandHandlers {
public:
    static void registerHandlers(CommandBus* bus, MediaService* mediaService, NoteService* noteService);
};
