#include "exportservice.h"

#include <core/repositories/notefolderrepository.h>
#include <utils/misc.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <algorithm>

namespace {
QStringList linkedFiles(const QString& noteText, const QString& folderName) {
    QStringList fileList;
    const QRegularExpression re(
        QStringLiteral(R"((?:!\[.*?\]|\[.*?\])\(.*%1/(.+?)\))").arg(QRegularExpression::escape(folderName)));
    QRegularExpressionMatchIterator matches = re.globalMatch(noteText);
    while (matches.hasNext()) {
        fileList.append(matches.next().captured(1));
    }
    return fileList;
}

QString currentFolderPath(const QString& childFolderName) {
    const NoteFolderData folder = NoteFolderRepository().current();
    return folder.localPath + QDir::separator() + childFolderName;
}

bool pathIsNoteFolder(const QString& path) {
    const QList<NoteFolderData> folders = NoteFolderRepository().findAll();
    return std::ranges::any_of(folders, [&path](const NoteFolderData& folder) { return path == folder.localPath; });
}

QString noteExtension(const NoteData& note) {
    const QString suffix = QFileInfo(note.fileName).suffix();
    return suffix.isEmpty() ? QStringLiteral("md") : suffix;
}

void copyLinkedFiles(const QStringList& fileList, const QString& sourcePath, const QString& destinationPath) {
    if (fileList.empty()) {
        return;
    }

    const QDir destinationDir(destinationPath);
    if (!destinationDir.exists()) {
        QDir().mkpath(destinationDir.path());
    }

    for (const QString& fileName : fileList) {
        QFile sourceFile(sourcePath + QDir::separator() + fileName);
        if (sourceFile.exists()) {
            sourceFile.copy(destinationDir.path() + QDir::separator() + fileName);
        }
    }
}

void normalizeRootAttachmentLinks(QString& noteText) {
    static const QRegularExpression mediaRe(QStringLiteral(R"((!\[.*?\])\((?:\.\.\/)*media/(.+?)\))"));
    noteText.replace(mediaRe, QStringLiteral(R"(\1(media/\2))"));

    static const QRegularExpression attachRe(QStringLiteral(R"((\[.*?\])\((?:\.\.\/)*attachments/(.+?)\))"));
    noteText.replace(attachRe, QStringLiteral(R"(\1(attachments/\2))"));
}

void rewriteExportedLinks(QString& noteText, const QString& folderName) {
    const QRegularExpression re(
        QStringLiteral(R"((%1)\(.*%2/(.+?)\))")
            .arg(folderName == QStringLiteral("media") ? QStringLiteral("!\\[.*?\\]") : QStringLiteral("\\[.*?\\]"),
                 QRegularExpression::escape(folderName)));
    QRegularExpressionMatchIterator matches = re.globalMatch(noteText);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        noteText.replace(match.captured(0), match.captured(1) + QStringLiteral("(./") + match.captured(2) + QChar(')'));
    }
}
}  // namespace

bool ExportService::exportNoteToPath(const NoteData& note, const QString& destinationPath,
                                     bool withAttachedFiles) const {
    QString noteText = note.noteText;
    const QFileInfo fileInfo(destinationPath);
    const QString absolutePath = fileInfo.absolutePath();

    if (withAttachedFiles) {
        const QStringList mediaFileList = linkedFiles(noteText, QStringLiteral("media"));
        if (!mediaFileList.empty()) {
            qDebug() << __func__ << " - 'mediaFileList': " << mediaFileList;
            copyLinkedFiles(mediaFileList, currentFolderPath(QStringLiteral("media")), absolutePath);
            rewriteExportedLinks(noteText, QStringLiteral("media"));
        }

        const QStringList attachmentFileList = linkedFiles(noteText, QStringLiteral("attachments"));
        if (!attachmentFileList.empty()) {
            qDebug() << __func__ << " - 'attachmentFileList': " << attachmentFileList;
            copyLinkedFiles(attachmentFileList, currentFolderPath(QStringLiteral("attachments")), absolutePath);
            rewriteExportedLinks(noteText, QStringLiteral("attachments"));
        }
    }

    qDebug() << "exporting note file: " << destinationPath;

    QFile file(destinationPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << file.errorString();
        return false;
    }

    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif
    out << noteText;
    file.flush();
    file.close();
    Utils::Misc::openFolderSelect(destinationPath, QStringLiteral("show-exported-note-in-file-manager"));
    return true;
}

bool ExportService::copyNoteToPath(const NoteData& note, const QString& destinationPath, QString noteFolderPath) const {
    QDir destinationDir;
    if (!QFileInfo::exists(note.fullNoteFilePath) || !destinationDir.exists(destinationPath)) {
        return false;
    }

    QFile file(note.fullNoteFilePath);
    QString destinationFileName = destinationPath + QDir::separator() + note.fileName;

    if (destinationDir.exists(destinationFileName)) {
        qDebug() << destinationFileName << "already exists!";
        const QDateTime currentDateTime = QDateTime::currentDateTime();
        destinationFileName = destinationPath + QDir::separator() + note.name + QChar(' ') +
                              currentDateTime.toString(Qt::ISODate).replace(QChar(':'), QChar('_')) + QChar('.') +
                              noteExtension(note);
        qDebug() << "New file name:" << destinationFileName;
    }

    if (!file.copy(destinationFileName)) {
        return false;
    }

    if (noteFolderPath.isEmpty()) {
        noteFolderPath = destinationPath;
    }

    if (!pathIsNoteFolder(noteFolderPath)) {
        return true;
    }

    const QStringList mediaFileList = linkedFiles(note.noteText, QStringLiteral("media"));
    copyLinkedFiles(mediaFileList, currentFolderPath(QStringLiteral("media")),
                    noteFolderPath + QDir::separator() + QStringLiteral("media"));

    const QStringList attachmentFileList = linkedFiles(note.noteText, QStringLiteral("attachments"));
    copyLinkedFiles(attachmentFileList, currentFolderPath(QStringLiteral("attachments")),
                    noteFolderPath + QDir::separator() + QStringLiteral("attachments"));

    if (!mediaFileList.empty() || !attachmentFileList.empty()) {
        QFile destFile(destinationFileName);
        if (destFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString noteContent = destFile.readAll();
            destFile.close();
            const QString originalContent = noteContent;
            normalizeRootAttachmentLinks(noteContent);

            if (noteContent != originalContent &&
                destFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                QTextStream out(&destFile);
                out << noteContent;
                destFile.close();
            }
        }
    }

    return true;
}
