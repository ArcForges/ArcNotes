#pragma once

#include <QVariant>
#include <QWidget>

class QComboBox;
class QGraphicsView;
class QLabel;
class QPushButton;
class QSettings;
class SettingsViewModel;

class LayoutPresetWidget : public QWidget {
    Q_OBJECT

Q_SIGNALS:
    void layoutStored(const QString& layoutUuid);

public:
    explicit LayoutPresetWidget(QWidget* parent = nullptr, SettingsViewModel* settingsViewModel = nullptr);
    ~LayoutPresetWidget() override;

    void resizeLayoutPresetImage() const;
    void setManualSettingsStoring(bool enabled);
    void storeLayoutPreset();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void on_layoutPresetComboBox_currentIndexChanged(int index);
    void on_useLayoutPresetPushButton_clicked();

private:
    QComboBox* _layoutPresetComboBox = nullptr;
    QLabel* _layoutPresetDescriptionLabel = nullptr;
    QGraphicsView* _layoutPresetGraphicsView = nullptr;
    QPushButton* _useLayoutPresetPushButton = nullptr;
    QSettings* _layoutPresetSettings = nullptr;
    SettingsViewModel* _settingsViewModel = nullptr;
    bool _manualSettingsStoring = false;

    void loadLayoutPresets();
    void updateCurrentLayoutPreset();
    static QString getLayoutPresetName(const QString& layoutPresetIdentifier);
    static QString getLayoutPresetDescription(const QString& layoutPresetIdentifier);
    QVariant settingValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(const QString& key, const QVariant& value);
};
