#pragma once

#include <core/data/notedata.h>

#include <QString>

class MarkdownRenderService {
public:
    QString renderNoteToHtml(const NoteData& note, const QString& notesPath, int maxImageWidth = 980,
                             bool forExport = false, bool base64Images = false) const;
    QString renderTextToHtml(const NoteData& note, QString text, const QString& notesPath, int maxImageWidth = 980,
                             bool forExport = false, bool base64Images = false) const;
};
