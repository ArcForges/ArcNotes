#pragma once

class CommandBus;
class NoteService;
class TagService;

class TagCommandHandlers {
public:
    static void registerHandlers(CommandBus* bus, TagService* tagService, NoteService* noteService);
};
