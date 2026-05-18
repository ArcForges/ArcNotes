#pragma once

class CommandBus;
class ExportService;
class NoteService;

class ExportCommandHandlers {
public:
    static void registerHandlers(CommandBus* bus, ExportService* exportService, NoteService* noteService);
};
