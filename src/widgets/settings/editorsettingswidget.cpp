#include "editorsettingswidget.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QVariant>

#include "viewmodels/settingsviewmodel.h"

namespace {
QGroupBox* groupBox(const QString& title, QWidget* parent) {
    auto* box = new QGroupBox(title, parent);
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(6);
    return box;
}
}  // namespace

EditorSettingsWidget::EditorSettingsWidget(QWidget* parent, SettingsViewModel* settingsViewModel)
    : QWidget(parent), _settingsViewModel(settingsViewModel) {
    buildUi();
}

EditorSettingsWidget::~EditorSettingsWidget() = default;

void EditorSettingsWidget::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* highlightingGroup = groupBox(tr("Markdown highlighting"), this);
    auto* highlightingLayout = qobject_cast<QVBoxLayout*>(highlightingGroup->layout());
    _markdownHighlightingCheckBox = new QCheckBox(tr("Enable markdown highlighting"), highlightingGroup);
    _fullyHighlightedBlockquotesCheckBox = new QCheckBox(tr("Fully highlight blockquotes"), highlightingGroup);
    _highlightCurrentLineCheckBox = new QCheckBox(tr("Highlight current line"), highlightingGroup);
    _headingFoldingCheckBox = new QCheckBox(tr("Enable heading folding"), highlightingGroup);
    _hideFormattingSyntaxCheckBox = new QCheckBox(tr("Hide formatting syntax"), highlightingGroup);
    highlightingLayout->addWidget(_markdownHighlightingCheckBox);
    highlightingLayout->addWidget(_fullyHighlightedBlockquotesCheckBox);
    highlightingLayout->addWidget(_highlightCurrentLineCheckBox);
    highlightingLayout->addWidget(_headingFoldingCheckBox);
    highlightingLayout->addWidget(_hideFormattingSyntaxCheckBox);
    rootLayout->addWidget(highlightingGroup);

    auto* typingGroup = groupBox(tr("Typing"), this);
    auto* typingLayout = qobject_cast<QVBoxLayout*>(typingGroup->layout());
    _autoBracketClosingCheckBox = new QCheckBox(tr("Auto bracket closing"), typingGroup);
    _autoBracketRemovalCheckBox = new QCheckBox(tr("Auto bracket removal"), typingGroup);
    _removeTrailingSpacesCheckBox = new QCheckBox(tr("Remove trailing spaces"), typingGroup);
    _useTabIndentCheckBox = new QCheckBox(tr("Use tab character for indentation"), typingGroup);
    _hangingIndentCheckBox = new QCheckBox(tr("Use hanging indentation"), typingGroup);
    typingLayout->addWidget(_autoBracketClosingCheckBox);
    typingLayout->addWidget(_autoBracketRemovalCheckBox);
    typingLayout->addWidget(_removeTrailingSpacesCheckBox);
    typingLayout->addWidget(_useTabIndentCheckBox);
    typingLayout->addWidget(_hangingIndentCheckBox);

    auto* indentForm = new QFormLayout();
    _indentSizeSpinBox = new QSpinBox(typingGroup);
    _indentSizeSpinBox->setRange(1, 16);
    indentForm->addRow(tr("Indent size:"), _indentSizeSpinBox);
    typingLayout->addLayout(indentForm);
    rootLayout->addWidget(typingGroup);

    auto* editorGroup = groupBox(tr("Editor"), this);
    auto* editorLayout = qobject_cast<QVBoxLayout*>(editorGroup->layout());
    _showLineNumbersInEditorCheckBox = new QCheckBox(tr("Show line numbers"), editorGroup);
    _editorWidthInDFMOnlyCheckBox = new QCheckBox(tr("Only use editor width in distraction-free mode"), editorGroup);
    _showMarkdownImagePreviewsCheckBox = new QCheckBox(tr("Show markdown image previews"), editorGroup);
    _vimModeCheckBox = new QCheckBox(tr("Use Vim mode"), editorGroup);
    _disableCursorBlinkingCheckBox = new QCheckBox(tr("Disable cursor blinking"), editorGroup);
    editorLayout->addWidget(_showLineNumbersInEditorCheckBox);
    editorLayout->addWidget(_editorWidthInDFMOnlyCheckBox);
    editorLayout->addWidget(_showMarkdownImagePreviewsCheckBox);
    editorLayout->addWidget(_vimModeCheckBox);
    editorLayout->addWidget(_disableCursorBlinkingCheckBox);

    auto* cursorLayout = new QHBoxLayout();
    _cursorWidthSpinBox = new QSpinBox(editorGroup);
    _cursorWidthSpinBox->setRange(1, 20);
    auto* cursorWidthResetButton = new QPushButton(tr("Reset"), editorGroup);
    cursorLayout->addWidget(_cursorWidthSpinBox);
    cursorLayout->addWidget(cursorWidthResetButton);
    cursorLayout->addStretch();
    auto* editorForm = new QFormLayout();
    editorForm->addRow(tr("Cursor width:"), cursorLayout);
    _timeFormatLineEdit = new QLineEdit(editorGroup);
    editorForm->addRow(tr("Time format:"), _timeFormatLineEdit);
    editorLayout->addLayout(editorForm);
    rootLayout->addWidget(editorGroup);

    auto* wikiGroup = groupBox(tr("Wiki links"), this);
    auto* wikiLayout = qobject_cast<QVBoxLayout*>(wikiGroup->layout());
    _enableWikiLinkSupportCheckBox = new QCheckBox(tr("Enable wiki link support"), wikiGroup);
    _wikiLinkFileNameAutoSelectCheckBox =
        new QCheckBox(tr("Automatically select the note file name for wiki links"), wikiGroup);
    wikiLayout->addWidget(_enableWikiLinkSupportCheckBox);
    wikiLayout->addWidget(_wikiLinkFileNameAutoSelectCheckBox);
    rootLayout->addWidget(wikiGroup);
    rootLayout->addStretch();

    connect(cursorWidthResetButton, &QPushButton::clicked, this,
            &EditorSettingsWidget::on_cursorWidthResetButton_clicked);
    connect(_markdownHighlightingCheckBox, &QCheckBox::toggled, this,
            &EditorSettingsWidget::on_markdownHighlightingCheckBox_toggled);
    connect(_enableWikiLinkSupportCheckBox, &QCheckBox::toggled, this,
            &EditorSettingsWidget::on_enableWikiLinkSupportCheckBox_toggled);
    connect(_showLineNumbersInEditorCheckBox, &QCheckBox::toggled, this,
            &EditorSettingsWidget::on_showLineNumbersInEditorCheckBox_toggled);
    connect(_editorWidthInDFMOnlyCheckBox, &QCheckBox::toggled, this,
            &EditorSettingsWidget::on_editorWidthInDFMOnlyCheckBox_toggled);
}

void EditorSettingsWidget::initialize() {
    connect(_fullyHighlightedBlockquotesCheckBox, &QCheckBox::toggled, this, &EditorSettingsWidget::needRestart);
    connect(_vimModeCheckBox, &QCheckBox::toggled, this, &EditorSettingsWidget::needRestart);
    connect(_disableCursorBlinkingCheckBox, &QCheckBox::toggled, this, &EditorSettingsWidget::needRestart);
}

void EditorSettingsWidget::readSettings() {
    _markdownHighlightingCheckBox->setChecked(
        settingValue(QStringLiteral("markdownHighlightingEnabled"), true).toBool());
    _fullyHighlightedBlockquotesCheckBox->setChecked(
        settingValue(QStringLiteral("fullyHighlightedBlockquotes")).toBool());
    _autoBracketClosingCheckBox->setChecked(settingValue(QStringLiteral("Editor/autoBracketClosing"), true).toBool());
    _autoBracketRemovalCheckBox->setChecked(settingValue(QStringLiteral("Editor/autoBracketRemoval"), true).toBool());
    _removeTrailingSpacesCheckBox->setChecked(settingValue(QStringLiteral("Editor/removeTrailingSpaces")).toBool());
    _showLineNumbersInEditorCheckBox->setChecked(settingValue(QStringLiteral("Editor/showLineNumbers")).toBool());
    _highlightCurrentLineCheckBox->setChecked(
        settingValue(QStringLiteral("Editor/highlightCurrentLine"), true).toBool());
    _headingFoldingCheckBox->setChecked(settingValue(QStringLiteral("Editor/headingFolding"), false).toBool());
    _hideFormattingSyntaxCheckBox->setChecked(
        settingValue(QStringLiteral("Editor/hideFormattingSyntax"), false).toBool());
    _enableWikiLinkSupportCheckBox->setChecked(settingValue(QStringLiteral("Editor/wikiLinkSupport"), false).toBool());
    on_enableWikiLinkSupportCheckBox_toggled(_enableWikiLinkSupportCheckBox->isChecked());
    _wikiLinkFileNameAutoSelectCheckBox->setChecked(
        settingValue(QStringLiteral("Editor/wikiLinkFileNameAutoSelect"), false).toBool());
    _hangingIndentCheckBox->setChecked(settingValue(QStringLiteral("Editor/hangingIndent"), false).toBool());
    _showMarkdownImagePreviewsCheckBox->setChecked(
        settingValue(QStringLiteral("Editor/showMarkdownImagePreviews"), true).toBool());
    _editorWidthInDFMOnlyCheckBox->setChecked(
        settingValue(QStringLiteral("Editor/editorWidthInDFMOnly"), true).toBool());
    _vimModeCheckBox->setChecked(settingValue(QStringLiteral("Editor/vimMode")).toBool());
    _disableCursorBlinkingCheckBox->setChecked(settingValue(QStringLiteral("Editor/disableCursorBlinking")).toBool());
    _useTabIndentCheckBox->setChecked(settingValue(QStringLiteral("Editor/useTabIndent")).toBool());
    _indentSizeSpinBox->setValue(settingValue(QStringLiteral("Editor/indentSize"), 4).toInt());
    _cursorWidthSpinBox->setValue(settingValue(QStringLiteral("cursorWidth"), 1).toInt());
    _timeFormatLineEdit->setText(settingValue(QStringLiteral("insertTimeFormat")).toString());
}

void EditorSettingsWidget::storeSettings() {
    setSettingValue(QStringLiteral("markdownHighlightingEnabled"), _markdownHighlightingCheckBox->isChecked());
    setSettingValue(QStringLiteral("fullyHighlightedBlockquotes"), _fullyHighlightedBlockquotesCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/autoBracketClosing"), _autoBracketClosingCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/autoBracketRemoval"), _autoBracketRemovalCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/removeTrailingSpaces"), _removeTrailingSpacesCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/showLineNumbers"), _showLineNumbersInEditorCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/highlightCurrentLine"), _highlightCurrentLineCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/headingFolding"), _headingFoldingCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/hideFormattingSyntax"), _hideFormattingSyntaxCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/wikiLinkSupport"), _enableWikiLinkSupportCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/wikiLinkFileNameAutoSelect"),
                    _wikiLinkFileNameAutoSelectCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/hangingIndent"), _hangingIndentCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/showMarkdownImagePreviews"),
                    _showMarkdownImagePreviewsCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/editorWidthInDFMOnly"), _editorWidthInDFMOnlyCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/vimMode"), _vimModeCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/disableCursorBlinking"), _disableCursorBlinkingCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/useTabIndent"), _useTabIndentCheckBox->isChecked());
    setSettingValue(QStringLiteral("Editor/indentSize"), _indentSizeSpinBox->value());
    setSettingValue(QStringLiteral("cursorWidth"), _cursorWidthSpinBox->value());
    setSettingValue(QStringLiteral("insertTimeFormat"), _timeFormatLineEdit->text());
}

void EditorSettingsWidget::on_cursorWidthResetButton_clicked() {
    _cursorWidthSpinBox->setValue(1);
}

void EditorSettingsWidget::on_markdownHighlightingCheckBox_toggled(bool checked) {
    _fullyHighlightedBlockquotesCheckBox->setEnabled(checked);
}

void EditorSettingsWidget::on_enableWikiLinkSupportCheckBox_toggled(bool checked) {
    _wikiLinkFileNameAutoSelectCheckBox->setEnabled(checked);
    emit wikiLinkSupportToggled(checked);
}

void EditorSettingsWidget::on_showLineNumbersInEditorCheckBox_toggled(bool checked) {
    if (checked && !_editorWidthInDFMOnlyCheckBox->isChecked()) {
        const QSignalBlocker blocker(_editorWidthInDFMOnlyCheckBox);
        _editorWidthInDFMOnlyCheckBox->setChecked(true);
    }
}

void EditorSettingsWidget::on_editorWidthInDFMOnlyCheckBox_toggled(bool checked) {
    if (!checked && _showLineNumbersInEditorCheckBox->isChecked()) {
        const QSignalBlocker blocker(_showLineNumbersInEditorCheckBox);
        _showLineNumbersInEditorCheckBox->setChecked(false);
    }
}

QVariant EditorSettingsWidget::settingValue(const QString& key, const QVariant& defaultValue) const {
    return _settingsViewModel == nullptr ? defaultValue : _settingsViewModel->persistentSetting(key, defaultValue);
}

void EditorSettingsWidget::setSettingValue(const QString& key, const QVariant& value) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->setPersistentSetting(key, value);
    }
}
