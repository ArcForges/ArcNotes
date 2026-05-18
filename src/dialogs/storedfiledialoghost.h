#pragma once

#include <core/data/notedata.h>

#include <QString>
#include <QStringList>
#include <QVector>

class StoredFileDialogHost {
public:
    virtual ~StoredFileDialogHost() = default;

    [[nodiscard]] virtual NoteData storedFileDialogCurrentNote() const = 0;
    [[nodiscard]] virtual QVector<NoteData> storedFileDialogAllNotes() const = 0;
    [[nodiscard]] virtual QString storedFileDialogMediaPath() const = 0;
    [[nodiscard]] virtual QString storedFileDialogAttachmentsPath() const = 0;
    [[nodiscard]] virtual QStringList storedFileDialogMediaFiles(const NoteData& note) const = 0;
    [[nodiscard]] virtual QStringList storedFileDialogAttachmentFiles(const NoteData& note) const = 0;
    [[nodiscard]] virtual QString storedFileDialogMediaUrlStringForFileName(const NoteData& note,
                                                                            const QString& fileName) const = 0;
    [[nodiscard]] virtual QString storedFileDialogAttachmentUrlStringForFileName(const NoteData& note,
                                                                                 const QString& fileName) const = 0;
    virtual bool storedFileDialogRenameMediaReferences(const QVector<int>& noteIds, const QString& oldFileName,
                                                       const QString& newFileName) = 0;
    virtual bool storedFileDialogRenameAttachmentReferences(const QVector<int>& noteIds, const QString& oldFileName,
                                                            const QString& newFileName) = 0;
    virtual void storedFileDialogInsertText(const QString& text) = 0;
    virtual void storedFileDialogOpenNoteInTab(int noteId) = 0;
    virtual void storedFileDialogReloadCurrentNote(bool force) = 0;
};
