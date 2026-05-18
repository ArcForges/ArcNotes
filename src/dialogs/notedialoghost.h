#pragma once

#include <core/data/notedata.h>

#include <QString>

class NoteDialogHost {
public:
    virtual ~NoteDialogHost() = default;

    [[nodiscard]] virtual NoteData noteDialogNoteById(int noteId) const = 0;
    [[nodiscard]] virtual QString noteDialogRenderNoteToHtml(const NoteData& note,
                                                             const QString& noteFolderPath) const = 0;
};
