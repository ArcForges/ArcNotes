#pragma once

#include <core/data/notedata.h>

#include "masterdialog.h"

class NotePreviewWidget;
class QFrame;
class ArcNotesMarkdownTextEdit;
class QTabWidget;
class QUrl;
class NoteDialogHost;

class NoteDialog : public MasterDialog {
    Q_OBJECT

public:
    explicit NoteDialog(QWidget* parent = nullptr);
    ~NoteDialog() override;
    void setNoteFolderPath(const QString& noteFolderPath);
    void setNote(const NoteData& note);

Q_SIGNALS:
    void jumpToNoteRequested(int noteId);

private slots:
    void on_noteTextView_anchorClicked(const QUrl& url);
    void on_tabWidget_currentChanged(int index);
    void onReloadButtonClicked();
    void onJumpToNoteButtonClicked();

private:
    NoteData _note;
    QString _noteFolderPath;
    NoteDialogHost* _host = nullptr;
    ArcNotesMarkdownTextEdit* _textEdit = nullptr;
    NotePreviewWidget* _noteTextView = nullptr;
    QFrame* _searchFrame = nullptr;
    QTabWidget* _tabWidget = nullptr;
};
