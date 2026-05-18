#pragma once

#include <QStringList>

#include "localtrashdialoghost.h"
#include "masterdialog.h"

class QAbstractButton;
class QDialogButtonBox;
class QFrame;
class QLineEdit;
class QSplitter;
class QTreeWidget;
class QTreeWidgetItem;
class ArcNotesMarkdownTextEdit;

class LocalTrashDialog : public MasterDialog {
    Q_OBJECT

public:
    enum ButtonRole { Unset, Restore, Remove, Cancel };

    explicit LocalTrashDialog(QWidget* parent = nullptr);
    ~LocalTrashDialog() override;

private slots:
    void storeSettings();
    void dialogButtonClicked(QAbstractButton* button);
    void on_trashTreeWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void on_searchLineEdit_textChanged(const QString& arg1);

private:
    QSplitter* _trashSplitter = nullptr;
    QFrame* _listFrame = nullptr;
    QFrame* _noteBrowserFrame = nullptr;
    QLineEdit* _searchLineEdit = nullptr;
    QTreeWidget* _trashTreeWidget = nullptr;
    ArcNotesMarkdownTextEdit* _noteBrowser = nullptr;
    QDialogButtonBox* _buttonBox = nullptr;
    LocalTrashDialogHost* _host = nullptr;

    void buildUi();
    void setupMainSplitter();
    void loadTrashedNotes();
    void restoreSelectedTrashItems();
    void removeSelectedTrashItems();
};
