#include "mediacommandhandlers.h"

#include <core/data/commands.h>
#include <core/services/mediaservice.h>
#include <core/services/noteservice.h>

#include <QFile>
#include <QRegularExpression>

#include "commandbus.h"

namespace {
CommandResult renameReferences(const QVector<int>& noteIds, const QString& oldFileName, const QString& newFileName,
                               const QString& directoryName, const QString& failureText, NoteService* noteService) {
    if (noteService == nullptr) {
        return CommandResult::fail(failureText);
    }

    const QRegularExpression filePattern(QStringLiteral(R"((\[.*?\]|!\[.*?\])\((.*?)") + directoryName +
                                         QStringLiteral(R"(\/.*?)()") + QRegularExpression::escape(oldFileName) +
                                         QStringLiteral(R"(\))"));
    bool changed = false;
    for (int noteId : noteIds) {
        NoteData note = noteService->getNote(noteId);
        if (note.id <= 0) {
            continue;
        }

        QString text = note.noteText;
        text.replace(filePattern, QStringLiteral("\\1(\\2") + newFileName + QStringLiteral(")"));

        if (text != note.noteText) {
            changed = noteService->saveNoteText(note.id, text) || changed;
        }
    }

    return changed ? CommandResult::ok() : CommandResult::fail(failureText);
}
}  // namespace

void MediaCommandHandlers::registerHandlers(CommandBus* bus, MediaService* mediaService, NoteService* noteService) {
    bus->registerHandler<InsertMediaCommand>([mediaService, noteService](const InsertMediaCommand& command) {
        QFile file(command.sourcePath);
        QString markdown = mediaService->getInsertMediaMarkdown(noteService->getNote(command.noteId), &file, true,
                                                                command.returnUrlOnly, command.title);
        return markdown.isEmpty() ? CommandResult::fail(QObject::tr("Media insert failed."))
                                  : CommandResult::ok(QString(), markdown);
    });

    bus->registerHandler<InsertAttachmentCommand>([mediaService, noteService](const InsertAttachmentCommand& command) {
        QFile file(command.sourcePath);
        QString markdown = mediaService->getInsertAttachmentMarkdown(
            noteService->getNote(command.noteId), &file, command.title, command.returnUrlOnly, command.fileName);
        return markdown.isEmpty() ? CommandResult::fail(QObject::tr("Attachment insert failed."))
                                  : CommandResult::ok(QString(), markdown);
    });

    bus->registerHandler<RenameMediaReferencesCommand>([noteService](const RenameMediaReferencesCommand& command) {
        return renameReferences(command.noteIds, command.oldFileName, command.newFileName, QStringLiteral("media"),
                                QObject::tr("Media references were not renamed."), noteService);
    });

    bus->registerHandler<RenameAttachmentReferencesCommand>(
        [noteService](const RenameAttachmentReferencesCommand& command) {
            return renameReferences(command.noteIds, command.oldFileName, command.newFileName,
                                    QStringLiteral("attachments"),
                                    QObject::tr("Attachment references were not renamed."), noteService);
        });
}
