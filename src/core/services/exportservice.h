#pragma once

#include <core/data/notedata.h>

#include <QString>

class ExportService {
public:
    [[nodiscard]] bool exportNoteToPath(const NoteData& note, const QString& destinationPath,
                                        bool withAttachedFiles = false) const;
    [[nodiscard]] bool copyNoteToPath(const NoteData& note, const QString& destinationPath,
                                      QString noteFolderPath = QString()) const;
};
