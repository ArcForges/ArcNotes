#pragma once

#include <QMetaType>
#include <QString>

struct EditorSessionSnapshot {
    int noteId = 0;
    QString baseChecksum;
    QString localText;
    QString lastSavedText;
    bool dirty = false;
    bool saving = false;
    bool conflict = false;
    int cursorPosition = 0;
    int scrollPosition = 0;

    bool operator==(const EditorSessionSnapshot& other) const {
        return noteId == other.noteId && baseChecksum == other.baseChecksum && localText == other.localText &&
               lastSavedText == other.lastSavedText && dirty == other.dirty && saving == other.saving &&
               conflict == other.conflict && cursorPosition == other.cursorPosition &&
               scrollPosition == other.scrollPosition;
    }

    bool operator!=(const EditorSessionSnapshot& other) const { return !(*this == other); }
};

Q_DECLARE_METATYPE(EditorSessionSnapshot)
