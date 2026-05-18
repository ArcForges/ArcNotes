#pragma once

#include <QVariant>
#include <QWidget>

class QCheckBox;
class QLineEdit;
class QSpinBox;
class SettingsViewModel;

class EditorSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit EditorSettingsWidget(QWidget* parent = nullptr, SettingsViewModel* settingsViewModel = nullptr);
    ~EditorSettingsWidget() override;

    void initialize();
    void readSettings();
    void storeSettings();

signals:
    void needRestart();
    void wikiLinkSupportToggled(bool checked);

private slots:
    void on_cursorWidthResetButton_clicked();
    void on_markdownHighlightingCheckBox_toggled(bool checked);
    void on_enableWikiLinkSupportCheckBox_toggled(bool checked);
    void on_showLineNumbersInEditorCheckBox_toggled(bool checked);
    void on_editorWidthInDFMOnlyCheckBox_toggled(bool checked);

private:
    QCheckBox* _markdownHighlightingCheckBox = nullptr;
    QCheckBox* _fullyHighlightedBlockquotesCheckBox = nullptr;
    QCheckBox* _autoBracketClosingCheckBox = nullptr;
    QCheckBox* _autoBracketRemovalCheckBox = nullptr;
    QCheckBox* _removeTrailingSpacesCheckBox = nullptr;
    QCheckBox* _showLineNumbersInEditorCheckBox = nullptr;
    QCheckBox* _highlightCurrentLineCheckBox = nullptr;
    QCheckBox* _headingFoldingCheckBox = nullptr;
    QCheckBox* _hideFormattingSyntaxCheckBox = nullptr;
    QCheckBox* _enableWikiLinkSupportCheckBox = nullptr;
    QCheckBox* _wikiLinkFileNameAutoSelectCheckBox = nullptr;
    QCheckBox* _hangingIndentCheckBox = nullptr;
    QCheckBox* _showMarkdownImagePreviewsCheckBox = nullptr;
    QCheckBox* _editorWidthInDFMOnlyCheckBox = nullptr;
    QCheckBox* _vimModeCheckBox = nullptr;
    QCheckBox* _disableCursorBlinkingCheckBox = nullptr;
    QCheckBox* _useTabIndentCheckBox = nullptr;
    QSpinBox* _indentSizeSpinBox = nullptr;
    QSpinBox* _cursorWidthSpinBox = nullptr;
    QLineEdit* _timeFormatLineEdit = nullptr;
    SettingsViewModel* _settingsViewModel = nullptr;

    void buildUi();
    QVariant settingValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(const QString& key, const QVariant& value);
};
