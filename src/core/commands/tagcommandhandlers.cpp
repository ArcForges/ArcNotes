#include "tagcommandhandlers.h"

#include <core/data/commands.h>
#include <core/services/noteservice.h>
#include <core/services/tagservice.h>

#include <QColor>

#include "commandbus.h"

void TagCommandHandlers::registerHandlers(CommandBus* bus, TagService* tagService, NoteService* noteService) {
    bus->registerHandler<CreateTagCommand>([tagService](const CreateTagCommand& command) {
        const TagData tag = tagService->createTag(command.name, command.parentId);
        return tag.id > 0 ? CommandResult::ok(QString(), tag.id)
                          : CommandResult::fail(QObject::tr("Tag could not be created."));
    });

    bus->registerHandler<DeleteTagCommand>([tagService](const DeleteTagCommand& command) {
        return tagService->deleteTag(command.tagId) ? CommandResult::ok(QString(), command.tagId)
                                                    : CommandResult::fail(QObject::tr("Tag could not be deleted."));
    });

    bus->registerHandler<RenameTagCommand>([tagService](const RenameTagCommand& command) {
        return tagService->renameTag(command.tagId, command.newName)
                   ? CommandResult::ok(QString(), command.tagId)
                   : CommandResult::fail(QObject::tr("Tag could not be renamed."));
    });

    bus->registerHandler<MoveTagCommand>([tagService](const MoveTagCommand& command) {
        return tagService->moveTag(command.tagId, command.parentId)
                   ? CommandResult::ok(QString(), command.tagId)
                   : CommandResult::fail(QObject::tr("Tag could not be moved."));
    });

    bus->registerHandler<SetTagColorCommand>([tagService](const SetTagColorCommand& command) {
        const auto color = command.color.value<QColor>();
        return tagService->setTagColor(command.tagId, color)
                   ? CommandResult::ok(QString(), command.tagId)
                   : CommandResult::fail(QObject::tr("Tag color could not be changed."));
    });

    bus->registerHandler<TagNoteCommand>([tagService, noteService](const TagNoteCommand& command) {
        bool ok = true;
        for (int noteId : command.noteIds) {
            ok = tagService->tagNote(noteService->getNote(noteId), command.tagId) && ok;
        }
        return ok ? CommandResult::ok() : CommandResult::fail(QObject::tr("Tagging failed."));
    });

    bus->registerHandler<UntagNoteCommand>([tagService, noteService](const UntagNoteCommand& command) {
        bool ok = true;
        for (int noteId : command.noteIds) {
            ok = tagService->untagNote(noteService->getNote(noteId), command.tagId) && ok;
        }
        return ok ? CommandResult::ok() : CommandResult::fail(QObject::tr("Untagging failed."));
    });
}
