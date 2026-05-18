#pragma once

#include <QVariant>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class SettingsViewModel;

class InterfaceSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit InterfaceSettingsWidget(QWidget* parent = nullptr, SettingsViewModel* settingsViewModel = nullptr);
    ~InterfaceSettingsWidget() override;

    void initialize();
    void readSettings();
    void storeSettings();
    void updateSearchIcons();

signals:
    void needRestart();
    void systemTrayToggled(bool checked);

private slots:
    void on_interfaceStyleComboBox_currentTextChanged(const QString& arg1);
    void on_showSystemTrayCheckBox_toggled(bool checked);
    void on_interfaceFontSizeSpinBox_valueChanged(int arg1);
    void on_overrideInterfaceFontSizeGroupBox_toggled(bool arg1);
    void on_languageSearchLineEdit_textChanged(const QString& arg1);
    void on_showStatusBarNotePathCheckBox_toggled(bool checked);
    void on_overrideInterfaceScalingFactorGroupBox_toggled(bool arg1);
    void on_interfaceScalingFactorSpinBox_valueChanged(int arg1);
    void on_itemHeightResetButton_clicked();
    void on_toolbarIconSizeResetButton_clicked();

private:
    QGroupBox* _overrideInterfaceFontSizeGroupBox = nullptr;
    QSpinBox* _interfaceFontSizeSpinBox = nullptr;
    QGroupBox* _overrideInterfaceScalingFactorGroupBox = nullptr;
    QSpinBox* _interfaceScalingFactorSpinBox = nullptr;
    QSpinBox* _toolbarIconSizeSpinBox = nullptr;
    QSpinBox* _itemHeightSpinBox = nullptr;
    QListWidget* _languageListWidget = nullptr;
    QLineEdit* _languageSearchLineEdit = nullptr;
    QCheckBox* _hideIconsInMenusCheckBox = nullptr;
    QCheckBox* _showStatusBarNotePathCheckBox = nullptr;
    QCheckBox* _showStatusBarRelativeNotePathCheckBox = nullptr;
    QCheckBox* _hideStatusBarInDistractionFreeModeCheckBox = nullptr;
    QCheckBox* _openDistractionFreeModeInFullScreenCheckBox = nullptr;
    QCheckBox* _showSystemTrayCheckBox = nullptr;
    QCheckBox* _startHiddenCheckBox = nullptr;
    QComboBox* _interfaceStyleComboBox = nullptr;
    SettingsViewModel* _settingsViewModel = nullptr;

    void buildUi();
    void populateLanguageList();
    void loadInterfaceStyleComboBox();
    QVariant settingValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(const QString& key, const QVariant& value);
    void removeSettingValue(const QString& key);
};
