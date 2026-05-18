#pragma once

#include <QFont>
#include <QVariant>
#include <QWidget>

class FontColorWidget;
class QLineEdit;
class SettingsViewModel;

class EditorFontColorSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit EditorFontColorSettingsWidget(QWidget* parent = nullptr, SettingsViewModel* settingsViewModel = nullptr);
    ~EditorFontColorSettingsWidget() override;

    void readSettings();
    void storeSettings();
    void setWikiLinkItemsVisible(bool visible);

signals:
    void needRestart();

private slots:
    void on_noteTextEditButton_clicked();
    void on_noteTextEditCodeButton_clicked();
    void on_noteTextEditResetButton_clicked();
    void on_noteTextEditCodeResetButton_clicked();

private:
    FontColorWidget* _editorFontColorWidget = nullptr;
    QLineEdit* _noteTextEditFontLabel = nullptr;
    QLineEdit* _noteTextEditCodeFontLabel = nullptr;
    QFont noteTextEditFont;
    QFont noteTextEditCodeFont;
    SettingsViewModel* _settingsViewModel = nullptr;

    void buildUi();
    void setFontLabel(QLineEdit* label, const QFont& font);
    void storeFontSettings();
    QVariant settingValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(const QString& key, const QVariant& value);
};
