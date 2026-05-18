#pragma once

#include <core/data/notedata.h>

#include <QString>
#include <QStringList>
#include <QUrl>

class QFile;

class MediaService {
public:
    QString getInsertMediaMarkdown(const NoteData& note, QFile* file, bool addNewLine = true,
                                   bool returnUrlOnly = false, QString title = QString()) const;
    QString getInsertAttachmentMarkdown(const NoteData& note, QFile* file, QString title = QString(),
                                        bool returnUrlOnly = false, QString fileName = QString()) const;
    [[nodiscard]] QStringList mediaFileList(const NoteData& note) const;
    [[nodiscard]] QStringList attachmentFileList(const NoteData& note) const;
    [[nodiscard]] QString mediaUrlStringForFileName(const NoteData& note, const QString& fileName) const;
    [[nodiscard]] QString attachmentUrlStringForFileName(const NoteData& note, const QString& fileName) const;
    [[nodiscard]] QString downloadUrlToMedia(const NoteData& note, const QUrl& url, bool returnUrlOnly = false) const;
    QString importMediaFromBase64(const NoteData& note, QString& data, QString imageSuffix = QString()) const;
};
