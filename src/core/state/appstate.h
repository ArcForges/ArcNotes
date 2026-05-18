#pragma once

#include <core/data/notedata.h>
#include <core/data/notefolderdata.h>
#include <core/data/notehistorydata.h>
#include <core/data/notesubfolderdata.h>

#include <QObject>

class AppState : public QObject {
    Q_OBJECT
    Q_PROPERTY(NoteData currentNote READ currentNote WRITE setCurrentNote NOTIFY currentNoteChanged)
    Q_PROPERTY(NoteFolderData currentNoteFolder READ currentNoteFolder WRITE setCurrentNoteFolder NOTIFY
                   currentNoteFolderChanged)
    Q_PROPERTY(NoteSubFolderData currentNoteSubFolder READ currentNoteSubFolder WRITE setCurrentNoteSubFolder NOTIFY
                   currentNoteSubFolderChanged)
    Q_PROPERTY(int activeTagId READ activeTagId WRITE setActiveTagId NOTIFY activeTagIdChanged)
    Q_PROPERTY(NoteHistoryData noteHistory READ noteHistory WRITE setNoteHistory NOTIFY noteHistoryChanged)
    Q_PROPERTY(bool showNotesFromAllSubFolders READ showNotesFromAllSubFolders WRITE setShowNotesFromAllSubFolders
                   NOTIFY showNotesFromAllSubFoldersChanged)

public:
    explicit AppState(QObject* parent = nullptr);

    [[nodiscard]] NoteData currentNote() const;
    [[nodiscard]] NoteFolderData currentNoteFolder() const;
    [[nodiscard]] NoteSubFolderData currentNoteSubFolder() const;
    [[nodiscard]] int activeTagId() const;
    [[nodiscard]] NoteHistoryData noteHistory() const;
    [[nodiscard]] bool showNotesFromAllSubFolders() const;

public slots:
    void setCurrentNote(const NoteData& note);
    void clearCurrentNote();
    void setCurrentNoteFolder(const NoteFolderData& folder);
    void setCurrentNoteSubFolder(const NoteSubFolderData& subFolder);
    void setActiveTagId(int tagId);
    void setNoteHistory(const NoteHistoryData& history);
    void setShowNotesFromAllSubFolders(bool enabled);

signals:
    void currentNoteChanged(const NoteData& note);
    void currentNoteFolderChanged(const NoteFolderData& folder);
    void currentNoteSubFolderChanged(const NoteSubFolderData& subFolder);
    void activeTagIdChanged(int tagId);
    void noteHistoryChanged(const NoteHistoryData& history);
    void showNotesFromAllSubFoldersChanged(bool enabled);

private:
    NoteData _currentNote;
    NoteFolderData _currentNoteFolder;
    NoteSubFolderData _currentNoteSubFolder;
    int _activeTagId = 0;
    NoteHistoryData _noteHistory;
    bool _showNotesFromAllSubFolders = true;
};
