#include "editorfontcolorsettingswidget.h"

#include <utils/gui.h>
#include <viewmodels/settingsviewmodel.h>

#include <QFontDatabase>
#include <QFontDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVariant>

#include "widgets/fontcolorwidget.h"

namespace {
QGroupBox* groupBox(const QString& title, QWidget* parent) {
    auto* box = new QGroupBox(title, parent);
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(10, 8, 10, 10);
    return box;
}
}  // namespace

EditorFontColorSettingsWidget::EditorFontColorSettingsWidget(QWidget* parent, SettingsViewModel* settingsViewModel)
    : QWidget(parent), _settingsViewModel(settingsViewModel) {
    buildUi();

    connect(_editorFontColorWidget, &FontColorWidget::schemaChanged, this,
            []() { Utils::Gui::applyDarkModeSettings(); });
}

EditorFontColorSettingsWidget::~EditorFontColorSettingsWidget() = default;

void EditorFontColorSettingsWidget::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* fontGroup = groupBox(tr("Editor fonts"), this);
    auto* fontForm = new QFormLayout();

    _noteTextEditFontLabel = new QLineEdit(fontGroup);
    _noteTextEditFontLabel->setReadOnly(true);
    auto* fontButton = new QPushButton(tr("Choose..."), fontGroup);
    auto* fontResetButton = new QPushButton(tr("Reset"), fontGroup);
    auto* fontLayout = new QHBoxLayout();
    fontLayout->addWidget(_noteTextEditFontLabel, 1);
    fontLayout->addWidget(fontButton);
    fontLayout->addWidget(fontResetButton);
    fontForm->addRow(tr("Editor font:"), fontLayout);

    _noteTextEditCodeFontLabel = new QLineEdit(fontGroup);
    _noteTextEditCodeFontLabel->setReadOnly(true);
    auto* codeFontButton = new QPushButton(tr("Choose..."), fontGroup);
    auto* codeFontResetButton = new QPushButton(tr("Reset"), fontGroup);
    auto* codeFontLayout = new QHBoxLayout();
    codeFontLayout->addWidget(_noteTextEditCodeFontLabel, 1);
    codeFontLayout->addWidget(codeFontButton);
    codeFontLayout->addWidget(codeFontResetButton);
    fontForm->addRow(tr("Editor code font:"), codeFontLayout);
    qobject_cast<QVBoxLayout*>(fontGroup->layout())->addLayout(fontForm);
    rootLayout->addWidget(fontGroup);

    _editorFontColorWidget = new FontColorWidget(this, _settingsViewModel);
    rootLayout->addWidget(_editorFontColorWidget, 1);

    connect(fontButton, &QPushButton::clicked, this, &EditorFontColorSettingsWidget::on_noteTextEditButton_clicked);
    connect(codeFontButton, &QPushButton::clicked, this,
            &EditorFontColorSettingsWidget::on_noteTextEditCodeButton_clicked);
    connect(fontResetButton, &QPushButton::clicked, this,
            &EditorFontColorSettingsWidget::on_noteTextEditResetButton_clicked);
    connect(codeFontResetButton, &QPushButton::clicked, this,
            &EditorFontColorSettingsWidget::on_noteTextEditCodeResetButton_clicked);
}

void EditorFontColorSettingsWidget::readSettings() {
    noteTextEditFont.fromString(settingValue(QStringLiteral("MainWindow/noteTextEdit.font")).toString());
    setFontLabel(_noteTextEditFontLabel, noteTextEditFont);

    noteTextEditCodeFont.fromString(settingValue(QStringLiteral("MainWindow/noteTextEdit.code.font")).toString());
    setFontLabel(_noteTextEditCodeFontLabel, noteTextEditCodeFont);
}

void EditorFontColorSettingsWidget::storeSettings() {
    storeFontSettings();
}

void EditorFontColorSettingsWidget::setWikiLinkItemsVisible(bool visible) {
    _editorFontColorWidget->setWikiLinkItemsVisible(visible);
}

void EditorFontColorSettingsWidget::storeFontSettings() {
    setSettingValue(QStringLiteral("MainWindow/noteTextEdit.font"), noteTextEditFont.toString());
    setSettingValue(QStringLiteral("MainWindow/noteTextEdit.code.font"), noteTextEditCodeFont.toString());
}

void EditorFontColorSettingsWidget::setFontLabel(QLineEdit* label, const QFont& font) {
    label->setText(font.family() + QStringLiteral(" (") + QString::number(font.pointSize()) + QStringLiteral(")"));
    label->setFont(font);
}

void EditorFontColorSettingsWidget::on_noteTextEditButton_clicked() {
    bool ok;
    QFont font = Utils::Gui::fontDialogGetFont(&ok, noteTextEditFont, this);

    if (ok) {
        noteTextEditFont = font;
        setFontLabel(_noteTextEditFontLabel, noteTextEditFont);
        storeFontSettings();
        emit needRestart();
        _editorFontColorWidget->updateAllTextItems();
    }
}

void EditorFontColorSettingsWidget::on_noteTextEditCodeButton_clicked() {
    bool ok;
    QFont font =
        Utils::Gui::fontDialogGetFont(&ok, noteTextEditCodeFont, this, QString(), QFontDialog::MonospacedFonts);

    if (ok) {
        noteTextEditCodeFont = font;
        setFontLabel(_noteTextEditCodeFontLabel, noteTextEditCodeFont);
        storeFontSettings();
        emit needRestart();
        _editorFontColorWidget->updateAllTextItems();
    }
}

void EditorFontColorSettingsWidget::on_noteTextEditResetButton_clicked() {
    QTextEdit textEdit;
    noteTextEditFont = textEdit.font();
    setFontLabel(_noteTextEditFontLabel, noteTextEditFont);
    storeFontSettings();
    emit needRestart();
    _editorFontColorWidget->updateAllTextItems();
}

void EditorFontColorSettingsWidget::on_noteTextEditCodeResetButton_clicked() {
    noteTextEditCodeFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    setFontLabel(_noteTextEditCodeFontLabel, noteTextEditCodeFont);
    storeFontSettings();
    emit needRestart();
    _editorFontColorWidget->updateAllTextItems();
}

QVariant EditorFontColorSettingsWidget::settingValue(const QString& key, const QVariant& defaultValue) const {
    return _settingsViewModel == nullptr ? defaultValue : _settingsViewModel->persistentSetting(key, defaultValue);
}

void EditorFontColorSettingsWidget::setSettingValue(const QString& key, const QVariant& value) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->setPersistentSetting(key, value);
    }
}
