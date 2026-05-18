#include "notelistviewmodel.h"

#include <core/commands/commandbus.h>
#include <core/data/commands.h>
#include <core/state/appstate.h>

#include <algorithm>

NoteListViewModel::NoteListViewModel(CommandBus* commandBus, AppState* appState, QObject* parent)
    : QObject(parent), _commandBus(commandBus), _appState(appState), _model(this) {
    connect(&_model, &QAbstractItemModel::rowsInserted, this, [this]() { emit noteCountChanged(noteCount()); });
    connect(&_model, &QAbstractItemModel::rowsRemoved, this, [this]() { emit noteCountChanged(noteCount()); });
    connect(&_model, &QAbstractItemModel::modelReset, this, [this]() { emit noteCountChanged(noteCount()); });
    connect(&_model, &NoteListModel::noteRenameRequested, this, &NoteListViewModel::renameNote);
}

NoteListModel* NoteListViewModel::model() {
    return &_model;
}

const NoteListModel* NoteListViewModel::model() const {
    return &_model;
}

QString NoteListViewModel::sortOrder() const {
    return _sortOrder;
}

QString NoteListViewModel::sortField() const {
    return _sortField;
}

int NoteListViewModel::noteCount() const {
    return _model.rowCount();
}

QVariantList NoteListViewModel::selectedNoteIds() const {
    return toVariantList(_selectedNoteIds);
}

void NoteListViewModel::setCommandBus(CommandBus* commandBus) {
    _commandBus = commandBus;
}

void NoteListViewModel::setAppState(AppState* appState) {
    _appState = appState;
}

void NoteListViewModel::setNotes(const QVector<NoteData>& notes) {
    QVector<NoteData> sortedNotes = notes;
    const bool ascending = _sortOrder == QStringLiteral("ascending");
    std::sort(sortedNotes.begin(), sortedNotes.end(), [this, ascending](const NoteData& left, const NoteData& right) {
        int result = 0;
        if (_sortField == QStringLiteral("name")) {
            result = QString::localeAwareCompare(left.name, right.name);
        } else if (_sortField == QStringLiteral("created")) {
            result = left.created < right.created ? -1 : (right.created < left.created ? 1 : 0);
        } else {
            result = left.modified < right.modified ? -1 : (right.modified < left.modified ? 1 : 0);
        }
        return ascending ? result < 0 : result > 0;
    });
    _model.setNotes(sortedNotes);
}

void NoteListViewModel::setSortOrder(const QString& sortOrder) {
    if (_sortOrder == sortOrder) {
        return;
    }
    _sortOrder = sortOrder;
    emit sortOrderChanged(_sortOrder);
}

void NoteListViewModel::setSortField(const QString& sortField) {
    if (_sortField == sortField) {
        return;
    }
    _sortField = sortField;
    emit sortFieldChanged(_sortField);
}

void NoteListViewModel::setSelectedNoteIds(const QVariantList& noteIds) {
    QVector<int> selectedIds;
    selectedIds.reserve(noteIds.count());
    for (const QVariant& noteId : noteIds) {
        selectedIds.append(noteId.toInt());
    }

    if (_selectedNoteIds == selectedIds) {
        return;
    }
    _selectedNoteIds = selectedIds;
    emit selectedNoteIdsChanged(selectedNoteIds());
}

void NoteListViewModel::selectNote(int noteId) {
    setSelectedNoteIds({noteId});
    if (_appState != nullptr) {
        const int row = _model.rowForNoteId(noteId);
        if (row >= 0) {
            _appState->setCurrentNote(_model.noteAt(row));
        }
    }
    emit noteSelected(noteId);
}

void NoteListViewModel::openNote(int noteId) {
    if (_commandBus == nullptr || noteId <= 0) {
        return;
    }

    OpenNoteCommand command;
    command.noteId = noteId;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void NoteListViewModel::createNote(const QString& name, int folderId, int subFolderId) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    CreateNoteCommand command;
    command.name = name;
    command.folderId = folderId;
    command.subFolderId = subFolderId;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void NoteListViewModel::renameNote(int noteId, const QString& name) {
    if (_commandBus == nullptr || noteId <= 0 || name.trimmed().isEmpty()) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    RenameNoteCommand command;
    command.noteId = noteId;
    command.newName = name.trimmed();
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void NoteListViewModel::deleteSelectedNotes(bool moveToTrash) {
    if (_commandBus == nullptr) {
        emit commandFailed(tr("Command bus is not available."));
        return;
    }

    DeleteNoteCommand command;
    command.noteIds = selectedNoteIdVector();
    command.moveToTrash = moveToTrash;
    const CommandResult result = _commandBus->dispatch(command);
    if (!result.success) {
        emit commandFailed(result.errorMessage);
    }
}

void NoteListViewModel::sortBy(const QString& field, const QString& order) {
    setSortField(field);
    setSortOrder(order);
    setNotes(_model.notes());
}

void NoteListViewModel::refresh() {
    emit refreshRequested();
}

QVector<int> NoteListViewModel::selectedNoteIdVector() const {
    return _selectedNoteIds;
}

QVariantList NoteListViewModel::toVariantList(const QVector<int>& noteIds) const {
    QVariantList values;
    values.reserve(noteIds.count());
    for (int noteId : noteIds) {
        values.append(noteId);
    }
    return values;
}
