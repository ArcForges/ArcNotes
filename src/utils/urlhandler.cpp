#include "urlhandler.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextCursor>
#include <QTextDocument>

#include "core/services/noteservice.h"
#include "utils/gui.h"
#include "utils/misc.h"
#include "widgets/arcnotesmarkdowntextedit.h"
#include "widgets/navigationwidget.h"

UrlHandler::UrlHandler(UrlHandlerContext* context) : _context(context) {}

bool UrlHandler::isUrlSchemeLocal(const QUrl& url) {
    const QString scheme = url.scheme();
    const NoteService noteService;
    return scheme == QLatin1String("note") || scheme == QLatin1String("noteid") ||
           scheme == QLatin1String("checkbox") || scheme == QStringLiteral("wikilink") ||
           (scheme == QLatin1String("file") && noteService.fileUrlIsNoteInCurrentFolder(url));
}

void UrlHandler::openUrl(QString urlString, const bool openInNewTab) {
    /* examples:
     * - <note://MyNote> opens the note "MyNote"
     * - <note://my-note-with-spaces-in-the-name> opens the note "My Note with
     * spaces in the name"
     * - <https://www.arcnotes.org> opens the web page
     * - <file:///path/to/my/note/folder/subfolder/My%20note.pdf> opens the note
     * "My note" in the subfolder "subfolder"
     * - <file:///path/to/my/file/ArcNotes.pdf> opens the file
     * "/path/to/my/file/ArcNotes.pdf" if the operating system supports that
     * handler
     */
    if (urlString.isEmpty()) {
        return;
    }

    // Handle wikilink: scheme early - it uses "wikilink:target" (no double slash)
    // so isValidUrl() would wrongly classify it as invalid and try to treat it
    // as a relative filename, producing a bogus file:// URL.
    if (urlString.startsWith(QStringLiteral("wikilink:"))) {
        handleWikiLinkUrl(urlString, openInNewTab);
        return;
    }

    bool urlWasNotValid = !ArcNotesMarkdownTextEdit::isValidUrl(urlString);
    QString fragment;
    if (urlWasNotValid) {
        if (_context == nullptr) {
            qWarning() << "Cannot resolve relative note url without UrlHandlerContext" << urlString;
            return;
        }
        fragment = _context->urlFragmentFromFileName(urlString);
        urlString = _context->urlFileUrlFromCurrentNoteFileName(urlString, true);
    } else {
        fragment = QUrl(urlString).fragment();
    }

    QUrl url = QUrl(urlString);
    const bool isExistingNoteFileUrl = _context != nullptr && _context->urlFileUrlIsExistingNoteInCurrentFolder(url);
    const bool isNoteFileUrl = _context != nullptr && _context->urlFileUrlIsNoteInCurrentFolder(url);
    const QString scheme = url.scheme();

    if (urlString.startsWith(QStringLiteral("file://..")) && !isExistingNoteFileUrl) {
        handleFileUrl(urlString);
    } else if (urlString.startsWith(QStringLiteral("file://attachments"))) {
        handleFileAttachmentUrl(urlString);
    } else if (scheme == QStringLiteral("noteid")) {
        handleNoteIdUrl(urlString, openInNewTab);
    } else if (scheme == QStringLiteral("wikilink")) {
        handleWikiLinkUrl(urlString, openInNewTab);
    } else if (scheme == QStringLiteral("note") || isNoteFileUrl) {
        handleNoteUrl(urlString, fragment, openInNewTab);
    } else if (scheme == QStringLiteral("checkbox")) {
        handleCheckboxUrl(urlString);
    } else if (scheme == QStringLiteral("file")) {
        const auto fileUrl = localFileUrlForDesktopOpen(urlString);
        auto res = QDesktopServices::openUrl(fileUrl);
        if (!res) {
            qWarning() << "Failed to open local file url" << fileUrl << urlString;
        }
    }
}

namespace {
struct UrlWikiLinkParts {
    QString heading;
    QString subfolderPath;
    QString noteName;
};

UrlWikiLinkParts parseUrlWikiLinkParts(QString target) {
    UrlWikiLinkParts parts;
    const int hashPos = target.indexOf(QChar('#'));
    if (hashPos >= 0) {
        parts.heading = target.mid(hashPos + 1).trimmed();
        target = target.left(hashPos).trimmed();
    }

    target = Utils::Misc::removeIfStartsWith(target, QStringLiteral("/"));
    const int slashPos = target.lastIndexOf(QChar('/'));
    if (slashPos >= 0) {
        parts.subfolderPath = target.left(slashPos).trimmed();
        parts.noteName = target.mid(slashPos + 1).trimmed();
    } else {
        parts.noteName = target.trimmed();
    }

    return parts;
}
}  // namespace

void UrlHandler::handleWikiLinkUrl(const QString& urlString, bool openInNewTab) {
    // Extract the target by stripping the scheme prefix and percent-decoding.
    // Using QUrl::host() is avoided because Qt normalises the host to lowercase,
    // which would corrupt note names that have uppercase letters.
    // Use "wikilink:" (no double-slash) so Qt treats the rest as a URL path
    // rather than an authority/host. The authority form "wikilink://name"
    // causes Qt to parse the note name as a hostname, applying hostname
    // validation (no spaces) and lowercasing, which corrupts note names.
    static const QString scheme = QStringLiteral("wikilink:");
    const QString rawTarget = urlString.startsWith(scheme) ? urlString.mid(scheme.length()) : urlString;
    const QString target = QUrl::fromPercentEncoding(rawTarget.toUtf8());

    if (_context == nullptr) {
        qWarning() << "Cannot handle wikilink without UrlHandlerContext" << urlString;
        return;
    }

    if (!_context->urlWikiLinkSupportEnabled()) {
        return;
    }

    const NoteData currentNote = _context->urlCurrentNote();
    const NoteData resolvedNote = _context->urlResolveWikiLink(target, currentNote.noteSubFolderId);

    if (resolvedNote.id > 0) {
        _context->urlOpenNote(resolvedNote, openInNewTab);

        const UrlWikiLinkParts parts = parseUrlWikiLinkParts(target);
        if (!parts.heading.isEmpty()) {
            QTextDocument* document = _context->urlActiveNoteDocument();
            QVector<Node> nodes;
            if (document != nullptr) {
                nodes = NavigationWidget::parseDocument(document);
            }
            for (const auto& node : nodes) {
                if (node.text.contains(parts.heading)) {
                    _context->urlJumpToEditorPosition(node.pos);
                    break;
                }
            }
        }
        return;
    }

    const UrlWikiLinkParts parts = parseUrlWikiLinkParts(target);
    if (parts.noteName.isEmpty()) {
        return;
    }

    QString prompt = QObject::tr("Note '%1' does not exist. Create it?").arg(parts.noteName);
    if (!parts.subfolderPath.isEmpty()) {
        prompt = QObject::tr("Note '%1' does not exist in '%2'. Create it?").arg(parts.noteName, parts.subfolderPath);
    }

    if (Utils::Gui::questionNoSkipOverride(nullptr, QObject::tr("Note was not found"), prompt,
                                           QStringLiteral("open-wikilink-create-note")) != QMessageBox::Yes) {
        return;
    }

    int targetSubFolderId = currentNote.noteSubFolderId;
    if (!parts.subfolderPath.isEmpty()) {
        targetSubFolderId = _context->urlNoteSubFolderIdByPath(parts.subfolderPath);
        if (targetSubFolderId <= 0) {
            targetSubFolderId = _context->urlEnsureNoteSubFolderPath(parts.subfolderPath);
        }

        if (targetSubFolderId > 0) {
            _context->urlRefreshNoteFolders();
            _context->urlJumpToNoteSubFolder(targetSubFolderId);
        }
    }

    if (targetSubFolderId > 0) {
        _context->urlSetActiveNoteSubFolder(targetSubFolderId);
    }

    _context->urlCreateNote(parts.noteName, false);
}

void UrlHandler::handleNoteIdUrl(const QString& urlString, bool openInNewTab) {
    static const QRegularExpression re(QStringLiteral(R"(^noteid:\/\/(\d+)$)"));
    QRegularExpressionMatch match = re.match(urlString);

    if (match.hasMatch()) {
        int noteId = match.captured(1).toInt();
        if (_context == nullptr) {
            qWarning() << "Cannot open note id url without UrlHandlerContext" << urlString;
            return;
        }
        NoteData note = _context->urlNoteById(noteId);
        if (note.id > 0) {
            _context->urlOpenNote(note, openInNewTab);
        }
    } else {
        qWarning() << "NoteIdUrlHandler malformed url: " << urlString;
    }
}

void UrlHandler::handleNoteUrl(const QString& urlString, const QString& fragment, bool openInNewTab) {
    qDebug() << __func__ << " - urlString:" << urlString << " - openInNewTab:" << openInNewTab;

    NoteData note;
    const QUrl url(urlString);
    if (_context == nullptr) {
        qWarning() << "Cannot handle note url without UrlHandlerContext" << urlString;
        return;
    }

    const NoteData currentNote = _context->urlCurrentNote();

    const bool isNoteFileUrl = _context->urlFileUrlIsNoteInCurrentFolder(url);

    if (isNoteFileUrl) {
        note = _context->urlNoteByFileUrl(url);
    } else {
        // try to fetch a note from the url string
        note = _context->urlNoteByUrlString(urlString);
    }

    // does this note really exist?
    if (note.id > 0) {
        qDebug() << __func__ << " - note fetched, id:" << note.id << " - openInNewTab:" << openInNewTab;
        if (openInNewTab) {
            // open note in a new tab
            qDebug() << __func__ << " - calling openNoteInTab";
            _context->urlOpenNote(note, true);
        } else {
            // set current note
            _context->urlOpenNote(note, false);
        }

        // Jump to the Markdown heading in the note that is represented by the url fragment
        if (!fragment.isEmpty()) {
            QTextDocument* document = _context->urlActiveNoteDocument();
            QVector<Node> nodes;
            if (document != nullptr) {
                nodes = NavigationWidget::parseDocument(document);
            }

            // Search in the nodes for the fragment
            for (const auto& node : nodes) {
                if (node.text.contains(fragment)) {
                    _context->urlJumpToEditorPosition(node.pos);
                    break;
                }
            }
        }
    } else {
        QString fileName;
        QUrl filePath;

        if (!isNoteFileUrl) {
            // if the name of the linked note only consists of numbers we cannot
            // use host() to get the filename, it would get converted to an
            // ip-address
            static const QRegularExpression re(QStringLiteral(R"(^\w+:\/\/(\d+)$)"));
            QRegularExpressionMatch match = re.match(urlString);
            fileName = match.hasMatch() ? match.captured(1) : url.host();

            // try to generate a useful title for the note
            fileName = Utils::Misc::toStartCase(fileName.replace(QStringLiteral("_"), QStringLiteral(" ")));
        } else {
            fileName = url.fileName();
            filePath = url.adjusted(QUrl::RemoveFilename);
        }

        // remove file extension
        QFileInfo fileInfo(fileName);
        fileName = fileInfo.baseName();
        QString relativeFilePath = _context->urlRelativePathForFileUrlInCurrentFolder(filePath);
        QString currentNoteRelativeSubFolderPath = _context->urlCurrentNoteSubFolderRelativePath();

        // remove the current relative sub-folder path from the relative path
        // of the future note to be able to create the correct path afterward
        if (!currentNoteRelativeSubFolderPath.isEmpty()) {
            relativeFilePath.remove(
                QRegularExpression("^" + QRegularExpression::escape(currentNoteRelativeSubFolderPath) + "\\/"));
        }

        // Open attachments with extensions that are used for notes externally
        if (relativeFilePath.contains(QStringLiteral("attachments"))) {
            if (QDesktopServices::openUrl(url)) {
                return;
            }
        }

        if (!relativeFilePath.isEmpty() && !_context->urlCurrentFolderHasSubfolders()) {
            Utils::Gui::warning(nullptr, QObject::tr("Note was not found"),
                                QObject::tr("Could not find note.<br />Unable to automatically "
                                            "create note at location, because subfolders are "
                                            "disabled for the current note folder."),
                                "cannot-create-note-not-has-subfolders");
            return;
        }

        QString promptQuestion;

        if (relativeFilePath.isEmpty()) {
            promptQuestion = QObject::tr(
                                 "Note was not found, create new note "
                                 "<strong>%1</strong>?")
                                 .arg(fileName);
        } else {
            promptQuestion = QObject::tr(
                                 "Note was not found, create new note "
                                 "<strong>%1</strong> at path <strong>%2</strong>?")
                                 .arg(fileName, relativeFilePath);
        }

        // ask if we want to create a new note if note wasn't found
        if (Utils::Gui::questionNoSkipOverride(nullptr, QObject::tr("Note was not found"), promptQuestion,
                                               QStringLiteral("open-url-create-note")) == QMessageBox::Yes) {
            int noteSubFolderId = currentNote.noteSubFolderId;
            bool subFolderCreationFailed(false);

            if (!relativeFilePath.isEmpty()) {
                for (const QString& folderName : relativeFilePath.split("/")) {
                    if (folderName.isEmpty()) {
                        break;
                    }

                    const int subFolderId = _context->urlEnsureChildNoteSubFolder(folderName, noteSubFolderId);
                    if (subFolderId <= 0) {
                        qWarning() << "Failed to create subfolder: " << folderName
                                   << "when attempting to create path: " << relativeFilePath;
                        subFolderCreationFailed = true;
                        break;
                    }

                    noteSubFolderId = subFolderId;
                    _context->urlSetActiveNoteSubFolder(noteSubFolderId);
                }
            }

            if (!subFolderCreationFailed) {
                if (!relativeFilePath.isEmpty()) {
                    _context->urlRefreshNoteFolders();
                    _context->urlJumpToNoteSubFolder(noteSubFolderId);
                }
                _context->urlCreateNote(fileName, false);
            } else {
                Utils::Gui::warning(nullptr, QObject::tr("Failed to create note"), QObject::tr("Note creation failed"),
                                    "note-create-failed");
            }
            return;
        }
    }
}

/**
 * Handles a checkbox:// url
 *
 * @param urlString
 */
void UrlHandler::handleCheckboxUrl(const QString& urlString) {
    // Check if read-only mode is enabled and allow to get out of it
    if (_context == nullptr || !_context->urlDoNoteEditingCheck()) {
        return;
    }

    auto* textEdit = _context->urlNoteTextEdit();
    if (textEdit == nullptr) {
        return;
    }

    const auto text = textEdit->toPlainText();
    const QUrl url(urlString);

    int index = url.host().mid(1).toInt();
    static const QRegularExpression re(R"((^|\n)\s*[-*+]\s\[([xX ]?)\])", QRegularExpression::CaseInsensitiveOption);
    int pos = 0;
    while (true) {
        QRegularExpressionMatch match;
        pos = text.indexOf(re, pos, &match);
        if (pos == -1)  // not found
            return;
        auto cursor = textEdit->textCursor();
        int matchedLength = match.capturedLength();
        qDebug() << __func__ << "match.capturedLength(): " << match.capturedLength();
        cursor.setPosition(pos + match.capturedLength() - 1);
        if (cursor.block().userState() == MarkdownHighlighter::HighlighterState::List) {
            if (index == 0) {
                auto ch = match.captured(2);
                if (ch.isEmpty())
                    cursor.insertText(QStringLiteral("x"));
                else {
                    cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
                    cursor.insertText(ch == QStringLiteral(" ") ? QStringLiteral("x") : QStringLiteral(" "));
                }

                // refresh instantly
                textEdit->setTextCursor(cursor);
                _context->urlRefreshNotePreview();
                break;
            }
            --index;
        }
        pos += matchedLength;
    }
}

void UrlHandler::handleFileUrl(QString urlString) {
    if (_context == nullptr) {
        qWarning() << "Cannot resolve relative file url without UrlHandlerContext" << urlString;
        return;
    }

    QString windowsSlash = QString();

#ifdef Q_OS_WIN32
    // we need another slash for Windows
    windowsSlash = QStringLiteral("/");
#endif

    urlString.replace(QLatin1String("file://.."), QStringLiteral("file://") + windowsSlash +
                                                      _context->urlCurrentNoteFolderPath() + QStringLiteral("/.."));

    QDesktopServices::openUrl(QUrl(urlString));
}

void UrlHandler::handleFileAttachmentUrl(QString urlString) {
    if (_context == nullptr) {
        qWarning() << "Cannot resolve attachment url without UrlHandlerContext" << urlString;
        return;
    }

    QString windowsSlash = QString();

#ifdef Q_OS_WIN32
    // we need another slash for Windows
    windowsSlash = QStringLiteral("/");
#endif

    urlString.replace(QLatin1String("file://attachments"), QStringLiteral("file://") + windowsSlash +
                                                               _context->urlCurrentNoteFolderPath() +
                                                               QStringLiteral("/attachments"));

    QDesktopServices::openUrl(QUrl(urlString));
}
