#pragma once

#include <QFont>
#include <QVariant>
#include <QWidget>

class QCheckBox;
class QGroupBox;
class QLineEdit;
class QSpinBox;
class SettingsViewModel;

class PreviewFontSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit PreviewFontSettingsWidget(QWidget* parent = nullptr, SettingsViewModel* settingsViewModel = nullptr);
    ~PreviewFontSettingsWidget() override;

    void readSettings();
    void storeSettings();

private slots:
    void on_noteTextViewButton_clicked();
    void on_noteTextViewCodeButton_clicked();
    void on_noteTextViewResetButton_clicked();
    void on_noteTextViewCodeResetButton_clicked();
    void on_noteTextViewUseEditorStylesCheckBox_toggled(bool checked);
    void on_noteTextViewRefreshDebounceTimeResetButton_clicked();

private:
    QCheckBox* _noteTextViewRTLCheckBox = nullptr;
    QCheckBox* _noteTextViewIgnoreCodeFontSizeCheckBox = nullptr;
    QCheckBox* _noteTextViewUnderlineCheckBox = nullptr;
    QCheckBox* _noteTextViewUseEditorStylesCheckBox = nullptr;
    QCheckBox* _useInternalExportStylingCheckBox = nullptr;
    QSpinBox* _noteTextViewRefreshDebounceTimeSpinBox = nullptr;
    QGroupBox* _previewFontsGroupBox = nullptr;
    QLineEdit* _noteTextViewFontLabel = nullptr;
    QLineEdit* _noteTextViewCodeFontLabel = nullptr;
    QFont noteTextViewFont;
    QFont noteTextViewCodeFont;
    SettingsViewModel* _settingsViewModel = nullptr;

    void buildUi();
    void setFontLabel(QLineEdit* label, const QFont& font);
    QVariant settingValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(const QString& key, const QVariant& value);
};
