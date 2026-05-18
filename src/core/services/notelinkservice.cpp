#include "notelinkservice.h"

#include <core/repositories/notefolderrepository.h>
#include <core/repositories/noterepository.h>
#include <core/repositories/notesubfolderrepository.h>
#include <core/repositories/settingsrepository.h>
#include <core/services/mediaservice.h>
#include <utils/misc.h>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>

namespace {
QString stripNoteExtension(const QString& fileName) {
    return QFileInfo(fileName).completeBaseName();
}

QString generateTextForLink(QString text) {
    static const QRegularExpression re(QStringLiteral("[^\\d\\w]"), QRegularExpression::CaseInsensitiveOption |
                                                                        QRegularExpression::UseUnicodePropertiesOption);
    text.replace(re, QStringLiteral("_"));

    static const QRegularExpression numberOnlyRe(QStringLiteral(R"(^(\d+)$)"));
    bool addAtSign = numberOnlyRe.match(text).hasMatch();

    if (!addAtSign) {
        static const QRegularExpression digitRe(QStringLiteral("\\d"));
        addAtSign = text.contains(digitRe) && text.toLocal8Bit().size() != text.length();
    }

    if (addAtSign || text.length() > 46) {
        text += QChar('@');
    }

    return text;
}

QString noteUrl(const QString& baseName) {
    return QStringLiteral("note://") + generateTextForLink(baseName);
}

QString urlEncodeNoteUrl(const QString& url, bool escapeSlashes = false) {
    QString encoded = QString(QUrl::toPercentEncoding(url));
    if (!escapeSlashes) {
        encoded.replace(QStringLiteral("%2F"), QStringLiteral("/"));
    }
    return encoded;
}

QString urlDecodeNoteUrl(QString url) {
    return QUrl::fromPercentEncoding(url.replace(QStringLiteral("&#32;"), QStringLiteral(" ")).toUtf8());
}

bool wikiLinkSupportEnabled() {
    return SettingsRepository().value(QStringLiteral("Editor/wikiLinkSupport"), false).toBool();
}

QString relativePathBetweenNotes(const NoteData& source, const NoteData& target) {
    const QDir dir(source.fullNoteFilePath);
    static const QRegularExpression re(QStringLiteral(R"(^\.\.\/)"));
    return dir.relativeFilePath(target.fullNoteFilePath).remove(re);
}

bool noteMatchesWikiName(const NoteData& note, const QString& name) {
    return note.name.compare(name, Qt::CaseInsensitive) == 0 ||
           stripNoteExtension(note.fileName).compare(name, Qt::CaseInsensitive) == 0;
}

NoteData selectBestWikiLinkMatch(const QVector<NoteData>& notes, const QString& name) {
    NoteData bestNote;
    for (const NoteData& note : notes) {
        if (!noteMatchesWikiName(note, name)) {
            continue;
        }

        if (bestNote.id == 0 || note.fileLastModified > bestNote.fileLastModified) {
            bestNote = note;
        }
    }
    return bestNote;
}

int activeNoteSubFolderId() {
    const NoteFolderData folder = NoteFolderRepository().current();
    if (folder.activeNoteSubFolderData.isEmpty()) {
        return 0;
    }

    return NoteSubFolderRepository().findByPathData(folder.activeNoteSubFolderData, QStringLiteral("\n")).id;
}

struct WikiTarget {
    QString noteName;
    QString subfolderPath;
};

WikiTarget parseWikiTarget(QString target, int currentNoteSubFolderId) {
    WikiTarget parsed;
    target = target.trimmed();

    const int pipePos = target.indexOf(QChar('|'));
    if (pipePos >= 0) {
        target = target.left(pipePos).trimmed();
    }

    const int hashPos = target.indexOf(QChar('#'));
    if (hashPos >= 0) {
        target = target.left(hashPos).trimmed();
    }

    QString resolvedPath = Utils::Misc::removeIfStartsWith(target, QStringLiteral("/"));
    const int slashPos = resolvedPath.lastIndexOf(QChar('/'));
    if (slashPos >= 0) {
        parsed.subfolderPath = resolvedPath.left(slashPos).trimmed();
        parsed.noteName = resolvedPath.mid(slashPos + 1).trimmed();

        if (!parsed.subfolderPath.isEmpty() && currentNoteSubFolderId > 0) {
            const NoteSubFolderData currentSubFolder = NoteSubFolderRepository().findById(currentNoteSubFolderId);
            if (currentSubFolder.id > 0) {
                QString currentPath;
                NoteSubFolderData folder = currentSubFolder;
                while (folder.id > 0) {
                    currentPath = currentPath.isEmpty() ? folder.name : folder.name + QStringLiteral("/") + currentPath;
                    folder = NoteSubFolderRepository().findById(folder.parentId);
                }
                if (!currentPath.isEmpty()) {
                    parsed.subfolderPath = currentPath + QStringLiteral("/") + parsed.subfolderPath;
                }
            }
        }
    } else {
        parsed.noteName = resolvedPath.trimmed();
    }

    return parsed;
}

bool textLinksToNote(const QString& text, const NoteData& source, const NoteData& target) {
    const QString legacyUrl = noteUrl(target.name);
    if (text.contains(QStringLiteral("<") + legacyUrl + QStringLiteral(">")) ||
        text.contains(QStringLiteral("](") + legacyUrl + QStringLiteral(")"))) {
        return true;
    }

    const QString legacyUrlWithHyphens = QString(legacyUrl).replace(QChar('_'), QChar('-'));
    if (legacyUrlWithHyphens != legacyUrl &&
        (text.contains(QStringLiteral("<") + legacyUrlWithHyphens + QStringLiteral(">")) ||
         text.contains(QStringLiteral("](") + legacyUrlWithHyphens + QStringLiteral(")")))) {
        return true;
    }

    const QString relativePath = relativePathBetweenNotes(source, target);
    for (bool escapeSlashes : {true, false}) {
        const QString encodedPath = urlEncodeNoteUrl(relativePath, escapeSlashes);
        if (text.contains(QStringLiteral("<") + encodedPath + QStringLiteral(">")) ||
            text.contains(QStringLiteral("](") + encodedPath + QStringLiteral(")")) ||
            text.contains(QStringLiteral("](") + encodedPath + QStringLiteral("#"))) {
            return true;
        }
    }

    return relativePath != urlEncodeNoteUrl(relativePath, false) &&
           (text.contains(QStringLiteral("<") + relativePath + QStringLiteral(">")) ||
            text.contains(QStringLiteral("](") + relativePath + QStringLiteral(")")) ||
            text.contains(QStringLiteral("](") + relativePath + QStringLiteral("#")));
}
}  // namespace

QVector<int> NoteLinkService::findBacklinks(const NoteData& note) const {
    QVector<int> noteIds;
    const QVector<NoteData> notes = NoteRepository().findAll();
    for (const NoteData& candidate : notes) {
        if (candidate.id != note.id && textLinksToNote(candidate.noteText, candidate, note)) {
            noteIds.append(candidate.id);
        }
    }
    return noteIds;
}

QVector<int> NoteLinkService::findLinkedNotes(const NoteData& note, const QVector<NoteData>& candidates) const {
    const QVector<NoteData> noteCandidates = candidates.isEmpty() ? NoteRepository().findAll() : candidates;
    QVector<int> noteIds;
    for (const NoteData& candidate : noteCandidates) {
        if (candidate.id != note.id && textLinksToNote(note.noteText, note, candidate)) {
            noteIds.append(candidate.id);
        }
    }
    return noteIds;
}

NoteData NoteLinkService::resolveWikiLink(const QString& target, int currentNoteSubFolderId) const {
    if (!wikiLinkSupportEnabled()) {
        return NoteData();
    }

    if (currentNoteSubFolderId == -1) {
        currentNoteSubFolderId = activeNoteSubFolderId();
    }

    const WikiTarget parsed = parseWikiTarget(target, currentNoteSubFolderId);
    if (parsed.noteName.isEmpty()) {
        return NoteData();
    }

    if (!parsed.subfolderPath.isEmpty()) {
        const NoteSubFolderData subFolder =
            NoteSubFolderRepository().findByPathData(parsed.subfolderPath, QStringLiteral("/"));
        return subFolder.id > 0
                   ? selectBestWikiLinkMatch(NoteRepository().findBySubFolderId(subFolder.id), parsed.noteName)
                   : NoteData();
    }

    if (currentNoteSubFolderId >= 0) {
        const NoteData sameFolder =
            selectBestWikiLinkMatch(NoteRepository().findBySubFolderId(currentNoteSubFolderId), parsed.noteName);
        if (sameFolder.id > 0) {
            return sameFolder;
        }
    }

    return selectBestWikiLinkMatch(NoteRepository().findAll(), parsed.noteName);
}

QString NoteLinkService::getNoteUrlForLinkingTo(const NoteData& source, const NoteData& target,
                                                bool forceLegacy) const {
    if (forceLegacy || SettingsRepository().value(QStringLiteral("legacyLinking")).toBool()) {
        return noteUrl(target.name);
    }

    QString url = urlEncodeNoteUrl(relativePathBetweenNotes(source, target));
    static const QRegularExpression invalidMarkdownLinkRe(QRegularExpression(R"([<>()])"));
    if (url.contains(invalidMarkdownLinkRe)) {
        url = noteUrl(target.name);
    }
    return url;
}

QString NoteLinkService::relativeFilePath(const NoteData& note, const QString& path) const {
    const QDir dir(note.fullNoteFilePath);
    static const QRegularExpression re(QStringLiteral(R"(^\.\.\/)"));
    return dir.relativeFilePath(path).remove(re);
}

QString NoteLinkService::fileUrlFromFileName(const NoteData& note, const QString& fileName, bool withFragment) const {
    const auto splitList = fileName.split(QChar('#'));
    QString cleanFileName = splitList.at(0);
    const QString fragment = splitList.count() > 1 ? splitList.at(1) : QString();
    cleanFileName = urlDecodeNoteUrl(cleanFileName);

    if (!note.relativeNoteSubFolderPath.isEmpty()) {
        cleanFileName.prepend(note.relativeNoteSubFolderPath + QStringLiteral("/"));
    }

    QString url =
        QString::fromLatin1(QUrl::fromLocalFile(Utils::Misc::appendIfDoesNotEndWith(
                                                    NoteFolderRepository().current().localPath, QStringLiteral("/")) +
                                                cleanFileName)
                                .toEncoded());
    if (withFragment && !fragment.isEmpty()) {
        url += QStringLiteral("#") + fragment;
    }
    return url;
}

bool NoteLinkService::updateRelativeMediaFileLinks(NoteData& note) const {
    static const QRegularExpression re(QStringLiteral(R"((!\[.*?\])\((.*media/(.+?))\))"));
    QRegularExpressionMatchIterator i = re.globalMatch(note.noteText);
    bool textWasUpdated = false;
    QString newText = note.noteText;

    while (i.hasNext()) {
        const QRegularExpressionMatch match = i.next();
        QString filePath = match.captured(2);
        if (filePath.startsWith(QLatin1String("file://"))) {
            continue;
        }

        const QString wholeLinkText = match.captured(0);
        const QString titlePart = match.captured(1);
        const QString fileName = match.captured(3);
        filePath = MediaService().mediaUrlStringForFileName(note, fileName);
        newText.replace(wholeLinkText, titlePart + QChar('(') + filePath + QChar(')'));
        textWasUpdated = true;
    }

    if (textWasUpdated) {
        note.noteText = newText;
        note.hasDirtyData = true;
        NoteRepository().save(note);
    }
    return textWasUpdated;
}

bool NoteLinkService::updateRelativeAttachmentFileLinks(NoteData& note) const {
    static const QRegularExpression re(QStringLiteral(R"((\[.*?\])\((.*attachments/(.+?))\))"));
    QRegularExpressionMatchIterator i = re.globalMatch(note.noteText);
    bool textWasUpdated = false;
    QString newText = note.noteText;

    while (i.hasNext()) {
        const QRegularExpressionMatch match = i.next();
        QString filePath = match.captured(2);
        if (filePath.startsWith(QLatin1String("file://"))) {
            continue;
        }

        const QString wholeLinkText = match.captured(0);
        const QString titlePart = match.captured(1);
        const QString fileName = match.captured(3);
        filePath = MediaService().attachmentUrlStringForFileName(note, fileName);
        newText.replace(wholeLinkText, titlePart + QChar('(') + filePath + QChar(')'));
        textWasUpdated = true;
    }

    if (textWasUpdated) {
        note.noteText = newText;
        note.hasDirtyData = true;
        NoteRepository().save(note);
    }
    return textWasUpdated;
}
