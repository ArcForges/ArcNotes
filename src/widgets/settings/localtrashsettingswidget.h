#pragma once

#include <QVariant>
#include <QWidget>
#include <QtGlobal>

class QCheckBox;
class QFrame;
class QGroupBox;
class QRadioButton;
class QSpinBox;
class SettingsViewModel;

class LocalTrashSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit LocalTrashSettingsWidget(QWidget* parent = nullptr, SettingsViewModel* settingsViewModel = nullptr);
    ~LocalTrashSettingsWidget() override;

    void readSettings();
    void storeSettings();

private slots:
    void on_localTrashEnabledCheckBox_toggled(bool checked);
    void on_localTrashClearCheckBox_toggled(bool checked);
    void on_noTrashRadioButton_toggled(bool checked);
    void on_systemTrashRadioButton_toggled(bool checked);
    void on_localTrashRadioButton_toggled(bool checked);

private:
    QGroupBox* _legacyTrashSupportGroupBox = nullptr;
    QGroupBox* _trashModeGroupBox = nullptr;
    QCheckBox* _localTrashEnabledCheckBox = nullptr;
    QRadioButton* _noTrashRadioButton = nullptr;
    QRadioButton* _systemTrashRadioButton = nullptr;
    QRadioButton* _localTrashRadioButton = nullptr;
    QGroupBox* _localTrashGroupBox = nullptr;
    QCheckBox* _localTrashClearCheckBox = nullptr;
    QFrame* _localTrashClearFrame = nullptr;
    QSpinBox* _localTrashClearTimeSpinBox = nullptr;
    SettingsViewModel* _settingsViewModel = nullptr;

    void buildUi();
    void updateTrashSettingsState();
    int trashModeFromSettings() const;
    QVariant settingValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(const QString& key, const QVariant& value);
};
