#include "exportcommandhandlers.h"

#include <core/data/commands.h>
#include <core/services/exportservice.h>
#include <core/services/noteservice.h>

#include "commandbus.h"

void ExportCommandHandlers::registerHandlers(CommandBus* bus, ExportService* exportService, NoteService* noteService) {
    bus->registerHandler<ExportNoteCommand>([exportService, noteService](const ExportNoteCommand& command) {
        bool ok = true;
        for (int noteId : command.noteIds) {
            ok = exportService->exportNoteToPath(noteService->getNote(noteId), command.destinationPath,
                                                 command.includeAttachments) &&
                 ok;
        }
        return ok ? CommandResult::ok() : CommandResult::fail(QObject::tr("Note export failed."));
    });
}
