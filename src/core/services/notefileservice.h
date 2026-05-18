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
    [[nodiscard]] bool removeNoteFile(const NoteData& note) const;
    [[nodiscard]] bool canWriteToNoteFile(const NoteData& note) const;
    [[nodiscard]] QString calculateChecksum(const QString& text) const;
};
