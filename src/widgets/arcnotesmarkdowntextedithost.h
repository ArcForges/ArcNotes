#pragma once

#include <core/data/notedata.h>

#include <QString>
#include <QUrl>
#include <QVariant>
#include <QVector>

class QAction;
class QMimeData;
class QTextDocument;

class ArcNotesMarkdownTextEditHost {
public:
    virtual ~ArcNotesMarkdownTextEditHost() = default;

    [[nodiscard]] virtual bool editorIsInDistractionFreeMode() const = 0;
    virtual void editorShowStatusMessage(const QString& message, const QString& symbol, int timeout) = 0;
    virtual QAction* editorAction(const QString& objectName) = 0;
    [[nodiscard]] virtual NoteData editorCurrentNote() const = 0;
    [[nodiscard]] virtual QVector<NoteData> editorAllNotes() const = 0;
    [[nodiscard]] virtual QString editorCurrentNoteFolderPath() const = 0;
    [[nodiscard]] virtual QString editorRenderTextToHtml(const QString& text, bool forExport) const = 0;
    [[nodiscard]] virtual bool editorWikiLinkSupportEnabled() const = 0;
    [[nodiscard]] virtual bool editorFileUrlIsNoteInCurrentFolder(const QUrl& url) const = 0;
    [[nodiscard]] virtual QVariant editorSettingValue(const QString& key,
                                                      const QVariant& defaultValue = QVariant()) const = 0;
    virtual void editorSetSettingValue(const QString& key, const QVariant& value) = 0;
    [[nodiscard]] virtual QString editorSelectedText() const = 0;
    [[nodiscard]] virtual int editorMaxImageWidth() const = 0;
    virtual void editorPrintDocument(QTextDocument* document, bool selectedOnly) = 0;
    virtual void editorExportNoteAsPDF(QTextDocument* document, bool selectedOnly) = 0;
    virtual void editorHandleMimeData(const QMimeData* mimeData) = 0;
    [[nodiscard]] virtual bool editorDoNoteEditingCheck() const = 0;
    virtual void editorAllowNoteEditing() = 0;
    virtual void editorDisallowNoteEditing() = 0;
};
