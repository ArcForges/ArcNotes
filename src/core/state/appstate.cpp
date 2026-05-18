#include "appstate.h"

AppState::AppState(QObject* parent) : QObject(parent) {}

NoteData AppState::currentNote() const {
    return _currentNote;
}

NoteFolderData AppState::currentNoteFolder() const {
    return _currentNoteFolder;
}

NoteSubFolderData AppState::currentNoteSubFolder() const {
    return _currentNoteSubFolder;
}

int AppState::activeTagId() const {
    return _activeTagId;
}

NoteHistoryData AppState::noteHistory() const {
    return _noteHistory;
}

bool AppState::showNotesFromAllSubFolders() const {
    return _showNotesFromAllSubFolders;
}

void AppState::setCurrentNote(const NoteData& note) {
    if (_currentNote == note) {
        return;
    }

    _currentNote = note;
    emit currentNoteChanged(_currentNote);
}

void AppState::clearCurrentNote() {
    setCurrentNote(NoteData());
}

void AppState::setCurrentNoteFolder(const NoteFolderData& folder) {
    if (_currentNoteFolder == folder) {
        return;
    }

    _currentNoteFolder = folder;
    emit currentNoteFolderChanged(_currentNoteFolder);
}

void AppState::setCurrentNoteSubFolder(const NoteSubFolderData& subFolder) {
    if (_currentNoteSubFolder == subFolder) {
        return;
    }

    _currentNoteSubFolder = subFolder;
    emit currentNoteSubFolderChanged(_currentNoteSubFolder);
}

void AppState::setActiveTagId(int tagId) {
    if (_activeTagId == tagId) {
        return;
    }

    _activeTagId = tagId;
    emit activeTagIdChanged(_activeTagId);
}

void AppState::setNoteHistory(const NoteHistoryData& history) {
    if (_noteHistory == history) {
        return;
    }

    _noteHistory = history;
    emit noteHistoryChanged(_noteHistory);
}

void AppState::setShowNotesFromAllSubFolders(bool enabled) {
    if (_showNotesFromAllSubFolders == enabled) {
        return;
    }

    _showNotesFromAllSubFolders = enabled;
    emit showNotesFromAllSubFoldersChanged(_showNotesFromAllSubFolders);
}
