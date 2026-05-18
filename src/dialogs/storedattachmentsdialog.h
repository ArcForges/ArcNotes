#pragma once

#include <core/data/notedata.h>

#include "masterdialog.h"

class QCheckBox;
class QFrame;
class QLabel;
class QLineEdit;
class QProgressBar;
class QTreeWidget;
class QTreeWidgetItem;
class StoredFileDialogHost;

class StoredAttachmentsDialog : public MasterDialog {
    Q_OBJECT

public:
    explicit StoredAttachmentsDialog(QWidget* parent = nullptr);
    ~StoredAttachmentsDialog() override;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void on_fileTreeWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void on_deleteButton_clicked();
    void on_insertButton_clicked();
    void on_openFileButton_clicked();
    void on_openFolderButton_clicked();
    void on_fileTreeWidget_customContextMenuRequested(const QPoint& pos);
    void on_fileTreeWidget_itemDoubleClicked(QTreeWidgetItem* item, int column);
    void on_noteTreeWidget_itemDoubleClicked(QTreeWidgetItem* item, int column);
    void on_refreshButton_clicked();
    void on_fileTreeWidget_itemChanged(QTreeWidgetItem* item, int column);
    void on_searchLineEdit_textChanged(const QString& arg1);
    void on_noteTreeWidget_customContextMenuRequested(const QPoint& pos);
    void on_orphanedCheckBox_toggled(bool checked);
    void on_currentNoteCheckBox_toggled(bool checked);

private:
    StoredFileDialogHost* _host = nullptr;
    bool _orphanedAttachmentsOnly = false;
    bool _currentNoteOnly = false;
    QHash<QString, QVector<NoteData>> _fileNoteList;
    QLineEdit* _searchLineEdit = nullptr;
    QTreeWidget* _fileTreeWidget = nullptr;
    QProgressBar* _progressBar = nullptr;
    QCheckBox* _orphanedCheckBox = nullptr;
    QCheckBox* _currentNoteCheckBox = nullptr;
    QFrame* _infoFrame = nullptr;
    QLabel* _pathLabel = nullptr;
    QLabel* _typeLabel = nullptr;
    QLabel* _sizeLabel = nullptr;
    QFrame* _notesFrame = nullptr;
    QTreeWidget* _noteTreeWidget = nullptr;

    void buildUi();
    QString getFilePath(QTreeWidgetItem* item) const;
    void refreshAttachmentFiles();
    void loadCurrentFileDetails();
    void refreshAndJumpToFileName(const QString& fileName);
    void openCurrentNote();
};
