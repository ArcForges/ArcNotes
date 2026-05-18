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
    QStringList mediaFileList(const NoteData& note) const;
    QStringList attachmentFileList(const NoteData& note) const;
    QString mediaUrlStringForFileName(const NoteData& note, const QString& fileName) const;
    QString attachmentUrlStringForFileName(const NoteData& note, const QString& fileName) const;
    QString downloadUrlToMedia(const NoteData& note, const QUrl& url, bool returnUrlOnly = false) const;
    QString importMediaFromBase64(const NoteData& note, QString& data, QString imageSuffix = QString()) const;
};
