#pragma once

#include <core/data/notedata.h>

#include <QString>

class NoteFileService {
public:
    bool storeNoteTextFileToDisk(NoteData& note, bool& currentNoteTextChanged,
                                 bool* wasCancelledDueToExternalModification = nullptr) const;
    bool storeNoteTextFileToDisk(NoteData& note) const;
    bool updateNoteTextFromDisk(NoteData& note) const;
    bool renameNoteFile(NoteData& note, const QString& newName) const;
    bool removeNoteFile(const NoteData& note) const;
    bool canWriteToNoteFile(const NoteData& note) const;
    QString calculateChecksum(const QString& text) const;
};
