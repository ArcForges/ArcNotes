#pragma once

#include <core/data/notedata.h>

#include <QString>
#include <QUrl>

class ArcNotesMarkdownTextEdit;
class QTextDocument;

class UrlHandlerContext {
public:
    virtual ~UrlHandlerContext() = default;

    [[nodiscard]] virtual NoteData urlCurrentNote() const = 0;
    [[nodiscard]] virtual QString urlCurrentNoteFolderPath() const = 0;
    [[nodiscard]] virtual QString urlCurrentNoteSubFolderRelativePath() const = 0;
    [[nodiscard]] virtual QString urlFileUrlFromCurrentNoteFileName(const QString& fileName,
                                                                    bool withFragment) const = 0;
    [[nodiscard]] virtual QTextDocument* urlActiveNoteDocument() const = 0;
    [[nodiscard]] virtual ArcNotesMarkdownTextEdit* urlNoteTextEdit() const = 0;
    [[nodiscard]] virtual QString urlFragmentFromFileName(const QString& fileName) const = 0;
    [[nodiscard]] virtual bool urlFileUrlIsNoteInCurrentFolder(const QUrl& url) const = 0;
    [[nodiscard]] virtual bool urlFileUrlIsExistingNoteInCurrentFolder(const QUrl& url) const = 0;
    [[nodiscard]] virtual QString urlRelativePathForFileUrlInCurrentFolder(const QUrl& url) const = 0;
    [[nodiscard]] virtual NoteData urlNoteById(int noteId) const = 0;
    [[nodiscard]] virtual NoteData urlNoteByFileUrl(const QUrl& url) const = 0;
    [[nodiscard]] virtual NoteData urlNoteByUrlString(const QString& urlString) const = 0;
    [[nodiscard]] virtual bool urlWikiLinkSupportEnabled() const = 0;
    [[nodiscard]] virtual NoteData urlResolveWikiLink(const QString& target, int currentNoteSubFolderId) const = 0;
    [[nodiscard]] virtual bool urlCurrentFolderHasSubfolders() const = 0;
    [[nodiscard]] virtual int urlNoteSubFolderIdByPath(const QString& pathData) const = 0;
    virtual int urlEnsureNoteSubFolderPath(const QString& pathData) = 0;
    virtual int urlEnsureChildNoteSubFolder(const QString& name, int parentId) = 0;
    virtual void urlSetActiveNoteSubFolder(int noteSubFolderId) = 0;
    virtual void urlOpenNote(const NoteData& note, bool openInNewTab) = 0;
    virtual void urlCreateNote(const QString& name, bool withNameAppend) = 0;
    virtual void urlRefreshNoteFolders() = 0;
    virtual void urlJumpToNoteSubFolder(int noteSubFolderId) = 0;
    virtual void urlJumpToEditorPosition(int position) = 0;
    [[nodiscard]] virtual bool urlDoNoteEditingCheck() const = 0;
    virtual void urlRefreshNotePreview() = 0;
};

/*
 * Handles note urls
 *
 * examples:
 * - <note://MyNote> opens the note "MyNote"
 * - <note://my-note-with-spaces-in-the-name> opens the note "My Note with
 * spaces in the name"
 */
class UrlHandler {
public:
    explicit UrlHandler(UrlHandlerContext* context = nullptr);

    static bool isUrlSchemeLocal(const QUrl& url);
    static QUrl localFileUrlForDesktopOpen(const QString& urlString) {
        const QUrl url(urlString);
        const QString localFilePath = url.toLocalFile();
        return localFilePath.isEmpty() ? url : QUrl::fromLocalFile(localFilePath);
    }

    void openUrl(QString urlString, bool openInNewTab = false);

private:
    void handleWikiLinkUrl(const QString& urlString, bool openInNewTab = false);
    void handleNoteIdUrl(const QString& urlString, bool openInNewTab = false);
    void handleNoteUrl(const QString& urlString, const QString& fragment, bool openInNewTab = false);
    void handleCheckboxUrl(const QString& urlString);
    void handleFileUrl(QString urlString);
    void handleFileAttachmentUrl(QString urlString);

    UrlHandlerContext* _context = nullptr;
};
