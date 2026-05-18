#include "mediaservice.h"

#include <core/repositories/notefolderrepository.h>
#include <core/repositories/settingsrepository.h>
#include <utils/misc.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMimeDatabase>
#include <QMimeType>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <utility>

namespace {
QString currentNoteFolderPath() {
    QString path = NoteFolderRepository().current().localPath;
    if (path.isEmpty()) {
        path = Utils::Misc::prependPortableDataPathIfNeeded(
            SettingsRepository().value(QStringLiteral("notesPath")).toString());
    }

    return Utils::Misc::removeIfEndsWith(std::move(path), QDir::separator());
}

QString currentMediaPath() {
    return currentNoteFolderPath() + QDir::separator() + QStringLiteral("media");
}

QString currentAttachmentsPath() {
    return currentNoteFolderPath() + QDir::separator() + QStringLiteral("attachments");
}

int noteDepth(const NoteData& note) {
    if (note.relativeNoteSubFolderPath.isEmpty()) {
        return 0;
    }

    return note.relativeNoteSubFolderPath.split(QChar('/'), Qt::SkipEmptyParts).count();
}

QString urlStringForFileName(const NoteData& note, const QString& folder, const QString& fileName) {
    QString urlString;
    if (SettingsRepository().value(QStringLiteral("legacyLinking")).toBool()) {
        urlString = QStringLiteral("file://") + folder + QChar('/') + fileName;
    } else {
        for (int i = 0; i < noteDepth(note); ++i) {
            urlString += QStringLiteral("../");
        }
        urlString += folder + QChar('/') + fileName;
    }

    return Utils::Misc::encodeFilePath(urlString);
}

bool scaleDownImageFileIfNeeded(QFile& file) {
    const SettingsRepository settings;
    if (!settings.value(QStringLiteral("imageScaleDown"), false).toBool()) {
        return true;
    }

    QMimeDatabase db;
    const QMimeType type = db.mimeTypeForFile(file.fileName());
    if (type.name().contains(QStringLiteral("image/svg"))) {
        return true;
    }

    QImage image;
    if (!image.load(file.fileName())) {
        return false;
    }

    const int width = settings.value(QStringLiteral("imageScaleDownMaximumWidth"), 1024).toInt();
    const int height = settings.value(QStringLiteral("imageScaleDownMaximumHeight"), 1024).toInt();
    if (image.width() <= width && image.height() <= height) {
        return true;
    }

    const QImage scaled = image.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << file.fileName();
        return false;
    }

    const bool saved = scaled.save(&file);
    file.close();
    return saved;
}
}  // namespace

QString MediaService::getInsertMediaMarkdown(const NoteData& note, QFile* file, bool addNewLine, bool returnUrlOnly,
                                             QString title) const {
    if (file == nullptr || file->size() == 0) {
        return QLatin1String("");
    }

    QDir mediaDir(currentMediaPath());
    if (!mediaDir.exists()) {
        mediaDir.mkpath(mediaDir.path());
    }

    const QFileInfo fileInfo(file->fileName());
    QString suffix = fileInfo.suffix();
    QMimeDatabase db;
    const QMimeType type = db.mimeTypeForFile(file->fileName());
    if (type.isValid() && !type.suffixes().isEmpty()) {
        suffix = type.suffixes().at(0);
    }

    const QString newFileName = Utils::Misc::findAvailableFileName(file->fileName(), mediaDir.path(), suffix);
    const QString newFilePath = mediaDir.path() + QDir::separator() + newFileName;
    file->copy(newFilePath);

    QFile newFile(newFilePath);
    scaleDownImageFileIfNeeded(newFile);

    QString mediaUrlString = mediaUrlStringForFileName(note, newFileName);
    if (returnUrlOnly) {
        return mediaUrlString;
    }

    if (title.isEmpty()) {
        title = fileInfo.baseName();
    }

    return QStringLiteral("![") + title + QStringLiteral("](") + mediaUrlString + QStringLiteral(")") +
           (addNewLine ? QStringLiteral("\n") : QLatin1String(""));
}

QString MediaService::getInsertAttachmentMarkdown(const NoteData& note, QFile* file, QString title, bool returnUrlOnly,
                                                  QString fileName) const {
    if (file == nullptr || file->size() <= 0) {
        return QLatin1String("");
    }

    const QDir dir(currentAttachmentsPath());
    if (!dir.exists()) {
        QDir().mkpath(dir.path());
    }

    const QFileInfo fileInfo(file->fileName());
    const QString suffix = fileInfo.suffix();
    if (fileName.isEmpty()) {
        fileName = file->fileName();
    }

    const QString newFileName = Utils::Misc::findAvailableFileName(fileName, dir.path(), suffix);
    const QString newFilePath = dir.path() + QDir::separator() + newFileName;
    file->copy(newFilePath);

    QString attachmentUrlString = attachmentUrlStringForFileName(note, newFileName);
    if (returnUrlOnly) {
        return attachmentUrlString;
    }

    if (title.isEmpty()) {
        title = fileInfo.fileName();
    }

    return QStringLiteral("[") + title + QStringLiteral("](") + attachmentUrlString + QStringLiteral(")");
}

QStringList MediaService::mediaFileList(const NoteData& note) const {
    QStringList fileList;
    static const QRegularExpression re(QStringLiteral(R"(!\[.*?\]\(.*media/(.+?)\))"));
    QRegularExpressionMatchIterator i = re.globalMatch(note.noteText);

    while (i.hasNext()) {
        const QRegularExpressionMatch match = i.next();
        fileList << QUrl::fromPercentEncoding(match.captured(1).toUtf8());
    }

    return fileList;
}

QStringList MediaService::attachmentFileList(const NoteData& note) const {
    QStringList fileList;
    static const QRegularExpression re(QStringLiteral(R"(\[.*?\]\(.*attachments/(.+?)\))"));
    QRegularExpressionMatchIterator i = re.globalMatch(note.noteText);

    while (i.hasNext()) {
        const QRegularExpressionMatch match = i.next();
        fileList << QUrl::fromPercentEncoding(match.captured(1).toUtf8());
    }

    return fileList;
}

QString MediaService::mediaUrlStringForFileName(const NoteData& note, const QString& fileName) const {
    return urlStringForFileName(note, QStringLiteral("media"), fileName);
}

QString MediaService::attachmentUrlStringForFileName(const NoteData& note, const QString& fileName) const {
    return urlStringForFileName(note, QStringLiteral("attachments"), fileName);
}

QString MediaService::downloadUrlToMedia(const NoteData& note, const QUrl& url, bool returnUrlOnly) const {
    QString suffix = url.toString()
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
                         .split(QStringLiteral("."), QString::SkipEmptyParts)
#else
                         .split(QStringLiteral("."), Qt::SkipEmptyParts)
#endif
                         .last();

    if (suffix.isEmpty()) {
        suffix = QStringLiteral("image");
    }

    static const QRegularExpression queryRe(QStringLiteral("\\?.+$"));
    static const QRegularExpression nonCharacterRe(QStringLiteral("[^a-zA-Z0-9]"));
    suffix.remove(queryRe).remove(nonCharacterRe);

    QString text;
    auto* tempFile =
        new QTemporaryFile(QDir::tempPath() + QDir::separator() + QStringLiteral("media-XXXXXX.") + suffix);
    if (tempFile->open() && Utils::Misc::downloadUrlToFile(url, tempFile)) {
        text = getInsertMediaMarkdown(note, tempFile, true, returnUrlOnly);
    }

    delete tempFile;
    return text;
}

QString MediaService::importMediaFromBase64(const NoteData& note, QString& data, QString imageSuffix) const {
    if (data.startsWith(QLatin1String("base64,"), Qt::CaseInsensitive)) {
        data = data.mid(6);
    }

    if (imageSuffix.isEmpty()) {
        imageSuffix = QLatin1String("dat");
    }

    auto* tempFile =
        new QTemporaryFile(QDir::tempPath() + QDir::separator() + QStringLiteral("media-XXXXXX.") + imageSuffix);
    if (!tempFile->open()) {
        delete tempFile;
        return QLatin1String("");
    }

    tempFile->write(QByteArray::fromBase64(data.toLatin1()));
    const QString markdownCode = getInsertMediaMarkdown(note, tempFile);
    delete tempFile;
    return markdownCode;
}
