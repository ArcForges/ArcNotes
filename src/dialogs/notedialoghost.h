#pragma once

#include <core/data/notedata.h>

#include <QString>

class NoteDialogHost {
public:
    virtual ~NoteDialogHost() = default;

    virtual NoteData noteDialogNoteById(int noteId) const = 0;
    virtual QString noteDialogRenderNoteToHtml(const NoteData& note, const QString& noteFolderPath) const = 0;
};
