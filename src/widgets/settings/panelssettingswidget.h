#pragma once

#include <QVariant>
#include <QWidget>

class QCheckBox;
class QGroupBox;
class QLineEdit;
class QRadioButton;
class QSpinBox;
class NoteFolderSettingsViewModel;
class SettingsViewModel;

class PanelsSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit PanelsSettingsWidget(QWidget* parent = nullptr, SettingsViewModel* settingsViewModel = nullptr,
                                  NoteFolderSettingsViewModel* noteFolderSettingsViewModel = nullptr);
    ~PanelsSettingsWidget() override;

    void initialize();
    void readSettings();
    void storeSettings();

signals:
    void needRestart();

private slots:
    void on_ignoreNoteSubFoldersResetButton_clicked();

private:
    QRadioButton* _notesPanelSortAlphabeticalRadioButton = nullptr;
    QRadioButton* _notesPanelSortByLastChangeRadioButton = nullptr;
    QGroupBox* _notesPanelOrderGroupBox = nullptr;
    QRadioButton* _notesPanelOrderDescendingRadioButton = nullptr;
    QRadioButton* _notesPanelOrderAscendingRadioButton = nullptr;
    QCheckBox* _noteSubfoldersPanelHideSearchCheckBox = nullptr;
    QCheckBox* _noteSubfoldersPanelDisplayAsFullTreeCheckBox = nullptr;
    QCheckBox* _noteSubfoldersPanelShowNotesRecursivelyCheckBox = nullptr;
    QCheckBox* _disableSavedSearchesAutoCompletionCheckBox = nullptr;
    QCheckBox* _showMatchesCheckBox = nullptr;
    QCheckBox* _noteSearchPanelOpenCreatedNotesInNewTabCheckBox = nullptr;
    QCheckBox* _noteSubfoldersPanelShowRootFolderNameCheckBox = nullptr;
    QCheckBox* _noteSubfoldersPanelShowFullPathCheckBox = nullptr;
    QCheckBox* _noteSubfoldersPanelTabsUnsetAllNotesSelectionCheckBox = nullptr;
    QRadioButton* _noteSubfoldersPanelSortAlphabeticalRadioButton = nullptr;
    QRadioButton* _noteSubfoldersPanelSortByLastChangeRadioButton = nullptr;
    QGroupBox* _noteSubfoldersPanelOrderGroupBox = nullptr;
    QRadioButton* _noteSubfoldersPanelOrderDescendingRadioButton = nullptr;
    QRadioButton* _noteSubfoldersPanelOrderAscendingRadioButton = nullptr;
    QLineEdit* _ignoreNoteSubFoldersLineEdit = nullptr;
    QLineEdit* _ignoredNoteFilesLineEdit = nullptr;
    QCheckBox* _tagsPanelHideSearchCheckBox = nullptr;
    QCheckBox* _tagsPanelHideNoteCountCheckBox = nullptr;
    QCheckBox* _taggingShowNotesRecursivelyCheckBox = nullptr;
    QCheckBox* _noteListPreviewCheckBox = nullptr;
    QCheckBox* _allowEmptyNotesCheckBox = nullptr;
    QSpinBox* _maxNoteFileSizeSpinBox = nullptr;
    QRadioButton* _tagsPanelSortAlphabeticalRadioButton = nullptr;
    QRadioButton* _tagsPanelSortByLastChangeRadioButton = nullptr;
    QGroupBox* _tagsPanelOrderGroupBox = nullptr;
    QRadioButton* _tagsPanelOrderDescendingRadioButton = nullptr;
    QRadioButton* _tagsPanelOrderAscendingRadioButton = nullptr;
    QCheckBox* _navigationPanelHideSearchCheckBox = nullptr;
    QCheckBox* _navigationPanelAutoSelectCheckBox = nullptr;
    QCheckBox* _enableNoteTreeCheckBox = nullptr;
    QCheckBox* _noteEditCentralWidgetCheckBox = nullptr;
    QCheckBox* _restoreNoteTabsCheckBox = nullptr;
    QCheckBox* _hideTabCloseButtonCheckBox = nullptr;
    QCheckBox* _noteFolderButtonsCheckBox = nullptr;
    SettingsViewModel* _settingsViewModel = nullptr;
    NoteFolderSettingsViewModel* _noteFolderSettingsViewModel = nullptr;

    void buildUi();
    QString defaultIgnoredSubfoldersPattern() const;
    QVariant settingValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(const QString& key, const QVariant& value);
};
