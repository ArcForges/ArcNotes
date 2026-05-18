#pragma once

#include <core/data/notedata.h>

#include <QHash>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

class LinkDialogHost {
public:
    virtual ~LinkDialogHost() = default;

    [[nodiscard]] virtual QVector<NoteData> linkDialogAllNotes() const = 0;
    [[nodiscard]] virtual NoteData linkDialogNoteById(int noteId) const = 0;
    [[nodiscard]] virtual QStringList linkDialogSearchNoteNames(const QString& query) const = 0;
    [[nodiscard]] virtual QHash<QString, QStringList> linkDialogTagNamesByNoteFilePath() const = 0;
    [[nodiscard]] virtual QString linkDialogRelativeFilePathFromCurrentNote(const QString& path) const = 0;
    [[nodiscard]] virtual bool linkDialogShowSubfolders() const = 0;
    [[nodiscard]] virtual bool linkDialogWikiLinkSupportEnabled() const = 0;
    [[nodiscard]] virtual bool linkDialogDarkModeColors() const = 0;
    [[nodiscard]] virtual QUrl linkDialogLastSelectedFileUrl() const = 0;
    virtual void linkDialogSetLastSelectedFileUrl(const QString& fileUrlString) = 0;
    [[nodiscard]] virtual QUrl linkDialogLastSelectedDirectoryUrl() const = 0;
    virtual void linkDialogSetLastSelectedDirectoryUrl(const QString& directoryUrlString) = 0;
};
