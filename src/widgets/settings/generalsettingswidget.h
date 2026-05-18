#pragma once

#include <QVariant>
#include <QWidget>

class QAbstractButton;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QFrame;
class QGroupBox;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QSpinBox;
class SettingsViewModel;
class QToolButton;

class GeneralSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit GeneralSettingsWidget(QWidget* parent = nullptr, SettingsViewModel* settingsViewModel = nullptr);
    ~GeneralSettingsWidget() override;

    void initialize();
    void readSettings();
    void storeSettings();

public slots:
    void setAllowOnlyOneAppInstance(bool checked);

signals:
    void needRestart();

private slots:
    void on_reinitializeDatabaseButton_clicked();
    void on_databaseIntegrityCheckButton_clicked();
    void on_clearAppDataAndExitButton_clicked();
    void on_setExternalEditorPathToolButton_clicked();
    void on_addCustomNoteFileExtensionButton_clicked();
    void on_removeCustomNoteFileExtensionButton_clicked();
    void on_defaultNoteFileExtensionListWidget_itemChanged(QListWidgetItem* item);
    void on_defaultNoteFileExtensionListWidget_itemSelectionChanged();
    void on_resetMessageBoxesButton_clicked();
    void on_exportSettingsButton_clicked();
    void on_importSettingsButton_clicked();
    void on_imageScaleDownCheckBox_toggled(bool checked);
    void on_enableReadOnlyModeCheckBox_toggled(bool checked);
    void noteNotificationButtonGroupPressed(QAbstractButton* button) const;
    void noteNotificationNoneCheckBoxCheck();

private:
    QCheckBox* _acceptAllExternalModificationsCheckBox = nullptr;
    QCheckBox* _notifyAllExternalModificationsCheckBox = nullptr;
    QCheckBox* _ignoreAllExternalModificationsCheckBox = nullptr;
    QCheckBox* _ignoreAllExternalNoteFolderChangesCheckBox = nullptr;
    QCheckBox* _enableNoteChecksumChecks = nullptr;
    QCheckBox* _newNoteAskHeadlineCheckBox = nullptr;
    QCheckBox* _useUNIXNewlineCheckBox = nullptr;
    QCheckBox* _restoreCursorPositionCheckBox = nullptr;
    QCheckBox* _restoreLastNoteAtStartupCheckBox = nullptr;
    QCheckBox* _automaticNoteFolderDatabaseClosingCheckBox = nullptr;
    QCheckBox* _legacyLinkingCheckBox = nullptr;
    QCheckBox* _imageScaleDownCheckBox = nullptr;
    QCheckBox* _enableReadOnlyModeCheckBox = nullptr;
    QCheckBox* _startInReadOnlyModeCheckBox = nullptr;
    QCheckBox* _autoReadOnlyModeCheckBox = nullptr;
    QCheckBox* _allowOnlyOneAppInstanceCheckBox = nullptr;
    QSpinBox* _noteSaveIntervalTime = nullptr;
    QSpinBox* _maximumImageHeightSpinBox = nullptr;
    QSpinBox* _maximumImageWidthSpinBox = nullptr;
    QSpinBox* _autoReadOnlyModeTimeoutSpinBox = nullptr;
    QFrame* _imageScalingFrame = nullptr;
    QFrame* _readOnlyModeSettingsFrame = nullptr;
    QGroupBox* _appInstanceGroupBox = nullptr;
    QComboBox* _searchEngineSelectionComboBox = nullptr;
    QListWidget* _defaultNoteFileExtensionListWidget = nullptr;
    QPushButton* _removeCustomNoteFileExtensionButton = nullptr;
    QLineEdit* _externalEditorPathLineEdit = nullptr;
    QButtonGroup* _noteNotificationButtonGroup = nullptr;
    QCheckBox* _noteNotificationNoneCheckBox = nullptr;
    SettingsViewModel* _settingsViewModel = nullptr;

    void buildUi();
    QListWidgetItem* addCustomNoteFileExtension(const QString& fileExtension);
    void initSearchEngineComboBox();
    QString defaultNoteFileExtension() const;
    QStringList noteFileExtensionList() const;
    QStringList settingKeys() const;
    QVariant settingValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(const QString& key, const QVariant& value);
    void removeSettingValue(const QString& key);
    void clearSettings();
    bool reinitializeDatabase();
    bool checkDatabaseIntegrity();
    void removeDiskDatabase();
};
