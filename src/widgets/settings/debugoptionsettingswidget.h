#pragma once

#include <QVariant>
#include <QWidget>

class QCheckBox;
class QFrame;
class QLabel;
class SettingsViewModel;

class DebugOptionSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit DebugOptionSettingsWidget(QWidget* parent = nullptr, SettingsViewModel* settingsViewModel = nullptr);
    ~DebugOptionSettingsWidget() override;

    void initialize();
    void readSettings();
    void storeSettings();

    static void removeLogFile();

private slots:
    void on_fileLoggingCheckBox_toggled(bool checked);
    void on_clearLogFileButton_clicked();

private:
    QCheckBox* _fileLoggingCheckBox = nullptr;
    QFrame* _logFileFrame = nullptr;
    QLabel* _logFileLabel = nullptr;
    SettingsViewModel* _settingsViewModel = nullptr;

    void buildUi();
    QVariant settingValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(const QString& key, const QVariant& value);
};
