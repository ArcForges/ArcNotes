#pragma once

#include <QListWidget>
#include <QTreeWidgetItem>
#include <QWidget>

#include "core/data/notefolderdata.h"

class QCheckBox;
class QFrame;
class QGroupBox;
class QLineEdit;
class QPushButton;
class QTreeWidget;
class NoteFolderListWidget;
class NoteFolderSettingsViewModel;

class NoteFolderSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit NoteFolderSettingsWidget(QWidget* parent = nullptr, NoteFolderSettingsViewModel* viewModel = nullptr);
    ~NoteFolderSettingsWidget() override;

    void initialize();
    void readSettings();
    void storeSettings();

signals:
    void storeSettingsRequested();

private slots:
    void on_noteFolderListWidget_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void on_noteFolderAddButton_clicked();
    void on_noteFolderRemoveButton_clicked();
    void on_noteFolderNameLineEdit_editingFinished();
    void on_noteFolderLocalPathButton_clicked();
    void on_noteFolderActiveCheckBox_stateChanged(Qt::CheckState arg1);
    void on_noteFolderShowSubfoldersCheckBox_toggled(bool checked);
    void on_noteFolderAllSubfoldersCheckBox_toggled(bool checked);
    void on_allowDifferentNoteFileNameCheckBox_toggled(bool checked);
    void onSubfolderTreeItemChanged(QTreeWidgetItem* item, int column);
    void saveSubfolderTreeSelection();

private:
    NoteFolderListWidget* _noteFolderListWidget = nullptr;
    QFrame* _noteFolderEditFrame = nullptr;
    QFrame* _noteFolderVerticalSpacerFrame = nullptr;
    QLineEdit* _noteFolderNameLineEdit = nullptr;
    QLineEdit* _noteFolderLocalPathLineEdit = nullptr;
    QPushButton* _noteFolderAddButton = nullptr;
    QPushButton* _noteFolderRemoveButton = nullptr;
    QPushButton* _noteFolderLocalPathButton = nullptr;
    QCheckBox* _noteFolderActiveCheckBox = nullptr;
    QCheckBox* _noteFolderShowSubfoldersCheckBox = nullptr;
    QCheckBox* _noteFolderAllSubfoldersCheckBox = nullptr;
    QCheckBox* _allowDifferentNoteFileNameCheckBox = nullptr;
    QGroupBox* _noteFolderSubfolderSettingsFrame = nullptr;
    QTreeWidget* _noteFolderSubfolderTreeWidget = nullptr;
    NoteFolderData _selectedNoteFolder;
    NoteFolderSettingsViewModel* _viewModel = nullptr;
    bool _updatingSubfolderTreeCheckStates = false;

    void buildUi();
    void updateSubfolderVisibility();
    void populateSubfolderTree();
    void populateSubfolderTreeFromDir(QTreeWidgetItem* parentItem, const QString& path, const QString& relativePath);
    void applySubfolderTreeCheckStates(QTreeWidget* tree, const QStringList& excludedPaths);
    void applyCheckStateToItem(QTreeWidgetItem* item, const QStringList& excludedPaths);
    void setSubfolderTreeChildrenCheckState(QTreeWidgetItem* item, Qt::CheckState checkState);
    void updateSubfolderTreeParentCheckStates(QTreeWidgetItem* item);
    Qt::CheckState subfolderTreeParentCheckState(QTreeWidgetItem* item);
    void collectExcludedSubfolderPaths(QTreeWidgetItem* item, QStringList& excludedPaths);
    bool selectedNoteFolderIsFetched() const;
};
