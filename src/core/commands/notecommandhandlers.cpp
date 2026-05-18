#include "notecommandhandlers.h"

#include <core/data/commands.h>
#include <core/services/notefileservice.h>
#include <core/services/notelinkservice.h>
#include <core/services/noteservice.h>
#include <core/services/trashservice.h>
#include <core/state/appstate.h>
#include <core/state/editorstate.h>

#include <QVariant>

#include "commandbus.h"

void NoteCommandHandlers::registerHandlers(CommandBus* bus, NoteService* noteService, NoteFileService* noteFileService,
                                           TrashService* trashService, NoteLinkService* noteLinkService,
                                           AppState* appState, EditorState* editorState) {
    bus->registerHandler<CreateNoteCommand>(
        [noteService, noteFileService, appState, editorState](const CreateNoteCommand& command) {
            NoteData note = noteService->createNote(command.name, command.folderId, command.subFolderId);
            if (note.id <= 0) {
                return CommandResult::fail(QObject::tr("Note could not be created."));
            }
            if (!noteFileService->storeNoteTextFileToDisk(note)) {
                (void)noteService->deleteNote(note.id, false);
                return CommandResult::fail(QObject::tr("Note file could not be stored."));
            }
            if (appState != nullptr) {
                appState->setCurrentNote(note);
            }
            if (editorState != nullptr) {
                editorState->setNoteId(note.id);
                editorState->setBaseChecksum(note.fileChecksum);
                editorState->setDirty(false);
                editorState->setHasConflict(false);
            }
            return CommandResult::ok(QString(), note.id);
        });

    bus->registerHandler<OpenNoteCommand>([noteService, appState, editorState](const OpenNoteCommand& command) {
        const NoteData note = noteService->getNote(command.noteId);
        if (note.id <= 0) {
            return CommandResult::fail(QObject::tr("Note was not found."));
        }
        if (appState != nullptr) {
            appState->setCurrentNote(note);
        }
        if (editorState != nullptr) {
            editorState->setNoteId(note.id);
            editorState->setBaseChecksum(note.fileChecksum);
            editorState->setDirty(false);
            editorState->setHasConflict(false);
        }
        return CommandResult::ok(QString(), note.id);
    });

    bus->registerHandler<SaveNoteCommand>(
        [noteService, noteFileService, appState, editorState](const SaveNoteCommand& command) {
            NoteData note = noteService->getNote(command.noteId);
            if (note.id <= 0) {
                return CommandResult::fail(QObject::tr("Note was not found."));
            }
            if (!command.baseChecksum.isEmpty() && note.fileChecksum != command.baseChecksum) {
                if (editorState != nullptr) {
                    editorState->setHasConflict(true);
                }
                return CommandResult::fail(QObject::tr("Note has changed on disk."), 409);
            }
            if (editorState != nullptr) {
                editorState->setSaving(true);
            }
            if (!noteService->saveNoteText(command.noteId, command.text)) {
                if (editorState != nullptr) {
                    editorState->setSaving(false);
                }
                return CommandResult::fail(QObject::tr("Note text could not be stored."));
            }
            note = noteService->getNote(command.noteId);
            noteFileService->storeNoteTextFileToDisk(note);
            if (appState != nullptr) {
                appState->setCurrentNote(note);
            }
            if (editorState != nullptr) {
                editorState->setBaseChecksum(note.fileChecksum);
                editorState->setDirty(false);
                editorState->setSaving(false);
                editorState->setHasConflict(false);
            }
            return CommandResult::ok(QString(), command.noteId);
        });

    bus->registerHandler<DeleteNoteCommand>([noteService, trashService](const DeleteNoteCommand& command) {
        bool ok = true;
        for (int noteId : command.noteIds) {
            const NoteData note = noteService->getNote(noteId);
            if (note.id <= 0) {
                ok = false;
                continue;
            }
            if (command.moveToTrash) {
                ok = trashService->moveNoteToTrash(note) && ok;
            }
            ok = noteService->deleteNote(noteId, true) && ok;
        }
        return ok ? CommandResult::ok() : CommandResult::fail(QObject::tr("Note deletion failed."));
    });

    bus->registerHandler<RenameNoteCommand>([noteService, noteFileService, appState](const RenameNoteCommand& command) {
        NoteData note = noteService->getNote(command.noteId);
        if (note.id <= 0) {
            return CommandResult::fail(QObject::tr("Note was not found."));
        }
        if (!noteFileService->renameNoteFile(note, command.newName)) {
            return CommandResult::fail(QObject::tr("Note file could not be renamed."));
        }
        note.name = command.newName;
        if (appState != nullptr && appState->currentNote().id == note.id) {
            appState->setCurrentNote(note);
        }
        return CommandResult::ok(QString(), command.noteId);
    });

    bus->registerHandler<MoveNoteCommand>([noteService, noteLinkService](const MoveNoteCommand& command) {
        bool ok = true;
        for (int noteId : command.noteIds) {
            NoteData note = noteService->getNote(noteId);
            ok = noteService->moveNote(note, command.targetSubFolderId) && ok;
            ok = noteLinkService->updateRelativeMediaFileLinks(note) && ok;
            ok = noteLinkService->updateRelativeAttachmentFileLinks(note) && ok;
        }
        return ok ? CommandResult::ok() : CommandResult::fail(QObject::tr("Note move failed."));
    });

    bus->registerHandler<DuplicateNoteCommand>([noteService](const DuplicateNoteCommand& command) {
        const NoteData note = noteService->duplicateNote(command.noteId, QString(), command.targetSubFolderId);
        if (note.id <= 0) {
            return CommandResult::fail(QObject::tr("Note could not be duplicated."));
        }
        return CommandResult::ok(QString(), note.id);
    });

    bus->registerHandler<ToggleFavoriteNoteCommand>([noteService](const ToggleFavoriteNoteCommand& command) {
        return noteService->toggleFavorite(command.noteId)
                   ? CommandResult::ok(QString(), command.noteId)
                   : CommandResult::fail(QObject::tr("Favorite state could not be changed."));
    });
}
