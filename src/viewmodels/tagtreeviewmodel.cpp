#include "tagtreeviewmodel.h"

#include <core/commands/commandbus.h>
#include <core/data/commands.h>
#include <core/state/appstate.h>

TagTreeViewModel::TagTreeViewModel(CommandBus* commandBus, AppState* appState, QObject* parent)
    : QObject(parent), _commandBus(commandBus), _appState(appState), _model(this) {
    connect(&_model, &QAbstractItemModel::modelReset, this, [this]() { emit tagCountChanged(tagCount()); });
    connect(&_model, &TagTreeModel::tagRenameRequested, this, &TagTreeViewModel::renameTag);
    if (_appState != nullptr) {
        connect(_appState, &AppState::activeTagIdChanged, this, &TagTreeViewModel::selectTag);
    }
}

TagTreeModel* TagTreeViewModel::model() {
    return &_model;
}

const TagTreeModel* TagTreeViewModel::model() const {
    return &_model;
}

int TagTreeViewModel::activeTagId() const {
    return _activeTagId;
}

int TagTreeViewModel::tagCount() const {
    return _model.rowCount();
}

void TagTreeViewModel::setCommandBus(CommandBus* commandBus) {
    _commandBus = commandBus;
}

void TagTreeViewModel::setAppState(AppState* appState) {
    _appState = appState;
}

void TagTreeViewModel::setTags(const QVector<TagData>& tags, const QHash<int, int>& noteCounts) {
    _model.setTags(tags, noteCounts);
}

void TagTreeViewModel::setSelectedNoteIds(const QVariantList& noteIds) {
    _selectedNoteIds.clear();
    _selectedNoteIds.reserve(noteIds.count());
    for (const QVariant& noteId : noteIds) {
        _selectedNoteIds.append(noteId.toInt());
    }
}

void TagTreeViewModel::selectTag(int tagId) {
    if (_activeTagId == tagId) {
        return;
    }
    _activeTagId = tagId;
    if (_appState != nullptr) {
        _appState->setActiveTagId(tagId);
    }
    emit activeTagIdChanged(_activeTagId);
}

void TagTreeViewModel::createTag(const QString& name, int parentId) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    CreateTagCommand command;
    command.name = name;
    command.parentId = parentId;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void TagTreeViewModel::deleteTag(int tagId) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    DeleteTagCommand command;
    command.tagId = tagId;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void TagTreeViewModel::renameTag(int tagId, const QString& name) {
    if (_commandBus == nullptr || tagId <= 0 || name.trimmed().isEmpty()) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    RenameTagCommand command;
    command.tagId = tagId;
    command.newName = name.trimmed();
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void TagTreeViewModel::moveTag(int tagId, int parentId) {
    if (_commandBus == nullptr || tagId <= 0) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    MoveTagCommand command;
    command.tagId = tagId;
    command.parentId = parentId;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void TagTreeViewModel::tagSelectedNotes(int tagId) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    TagNoteCommand command;
    command.tagId = tagId;
    command.noteIds = selectedNoteIdVector();
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void TagTreeViewModel::untagSelectedNotes(int tagId) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    UntagNoteCommand command;
    command.tagId = tagId;
    command.noteIds = selectedNoteIdVector();
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

QVector<int> TagTreeViewModel::selectedNoteIdVector() const {
    return _selectedNoteIds;
}
