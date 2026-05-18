#include "previewfontsettingswidget.h"

#include <QCheckBox>
#include <QFontDatabase>
#include <QFontDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTextBrowser>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVariant>

#include "utils/gui.h"
#include "viewmodels/settingsviewmodel.h"

namespace {
QGroupBox* groupBox(const QString& title, QWidget* parent) {
    auto* box = new QGroupBox(title, parent);
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(10, 8, 10, 10);
    return box;
}
}  // namespace

PreviewFontSettingsWidget::PreviewFontSettingsWidget(QWidget* parent, SettingsViewModel* settingsViewModel)
    : QWidget(parent), _settingsViewModel(settingsViewModel) {
    buildUi();
}

PreviewFontSettingsWidget::~PreviewFontSettingsWidget() = default;

void PreviewFontSettingsWidget::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* renderingGroup = groupBox(tr("Rendering"), this);
    auto* renderingLayout = qobject_cast<QVBoxLayout*>(renderingGroup->layout());
    _noteTextViewRTLCheckBox = new QCheckBox(tr("Right to left text direction"), renderingGroup);
    _noteTextViewIgnoreCodeFontSizeCheckBox = new QCheckBox(tr("Ignore code font size"), renderingGroup);
    _noteTextViewUnderlineCheckBox = new QCheckBox(tr("Underline links"), renderingGroup);
    _noteTextViewUseEditorStylesCheckBox = new QCheckBox(tr("Use editor styles for preview"), renderingGroup);
    renderingLayout->addWidget(_noteTextViewRTLCheckBox);
    renderingLayout->addWidget(_noteTextViewIgnoreCodeFontSizeCheckBox);
    renderingLayout->addWidget(_noteTextViewUnderlineCheckBox);
    renderingLayout->addWidget(_noteTextViewUseEditorStylesCheckBox);

    auto* debounceLayout = new QHBoxLayout();
    _noteTextViewRefreshDebounceTimeSpinBox = new QSpinBox(renderingGroup);
    _noteTextViewRefreshDebounceTimeSpinBox->setRange(0, 10000);
    _noteTextViewRefreshDebounceTimeSpinBox->setSuffix(tr(" ms"));
    auto* debounceResetButton = new QPushButton(tr("Reset"), renderingGroup);
    debounceLayout->addWidget(_noteTextViewRefreshDebounceTimeSpinBox);
    debounceLayout->addWidget(debounceResetButton);
    debounceLayout->addStretch();
    auto* renderingForm = new QFormLayout();
    renderingForm->addRow(tr("Refresh debounce time:"), debounceLayout);
    renderingLayout->addLayout(renderingForm);
    rootLayout->addWidget(renderingGroup);

    auto* exportGroup = groupBox(tr("Exporting"), this);
    _useInternalExportStylingCheckBox = new QCheckBox(tr("Use internal export styling"), exportGroup);
    qobject_cast<QVBoxLayout*>(exportGroup->layout())->addWidget(_useInternalExportStylingCheckBox);
    rootLayout->addWidget(exportGroup);

    _previewFontsGroupBox = groupBox(tr("Preview fonts"), this);
    auto* fontForm = new QFormLayout();
    _noteTextViewFontLabel = new QLineEdit(_previewFontsGroupBox);
    _noteTextViewFontLabel->setReadOnly(true);
    auto* fontButton = new QPushButton(tr("Choose..."), _previewFontsGroupBox);
    auto* fontResetButton = new QPushButton(tr("Reset"), _previewFontsGroupBox);
    auto* fontLayout = new QHBoxLayout();
    fontLayout->addWidget(_noteTextViewFontLabel, 1);
    fontLayout->addWidget(fontButton);
    fontLayout->addWidget(fontResetButton);
    fontForm->addRow(tr("Preview font:"), fontLayout);

    _noteTextViewCodeFontLabel = new QLineEdit(_previewFontsGroupBox);
    _noteTextViewCodeFontLabel->setReadOnly(true);
    auto* codeFontButton = new QPushButton(tr("Choose..."), _previewFontsGroupBox);
    auto* codeFontResetButton = new QPushButton(tr("Reset"), _previewFontsGroupBox);
    auto* codeFontLayout = new QHBoxLayout();
    codeFontLayout->addWidget(_noteTextViewCodeFontLabel, 1);
    codeFontLayout->addWidget(codeFontButton);
    codeFontLayout->addWidget(codeFontResetButton);
    fontForm->addRow(tr("Code font:"), codeFontLayout);
    qobject_cast<QVBoxLayout*>(_previewFontsGroupBox->layout())->addLayout(fontForm);
    rootLayout->addWidget(_previewFontsGroupBox);
    rootLayout->addStretch();

    connect(fontButton, &QPushButton::clicked, this, &PreviewFontSettingsWidget::on_noteTextViewButton_clicked);
    connect(codeFontButton, &QPushButton::clicked, this, &PreviewFontSettingsWidget::on_noteTextViewCodeButton_clicked);
    connect(fontResetButton, &QPushButton::clicked, this,
            &PreviewFontSettingsWidget::on_noteTextViewResetButton_clicked);
    connect(codeFontResetButton, &QPushButton::clicked, this,
            &PreviewFontSettingsWidget::on_noteTextViewCodeResetButton_clicked);
    connect(_noteTextViewUseEditorStylesCheckBox, &QCheckBox::toggled, this,
            &PreviewFontSettingsWidget::on_noteTextViewUseEditorStylesCheckBox_toggled);
    connect(debounceResetButton, &QPushButton::clicked, this,
            &PreviewFontSettingsWidget::on_noteTextViewRefreshDebounceTimeResetButton_clicked);
}

void PreviewFontSettingsWidget::readSettings() {
    _noteTextViewRTLCheckBox->setChecked(settingValue(QStringLiteral("MainWindow/noteTextView.rtl")).toBool());
    _noteTextViewIgnoreCodeFontSizeCheckBox->setChecked(
        settingValue(QStringLiteral("MainWindow/noteTextView.ignoreCodeFontSize"), true).toBool());
    _noteTextViewUnderlineCheckBox->setChecked(
        settingValue(QStringLiteral("MainWindow/noteTextView.underline"), true).toBool());
    _noteTextViewUseEditorStylesCheckBox->setChecked(
        settingValue(QStringLiteral("MainWindow/noteTextView.useEditorStyles"), true).toBool());
    _noteTextViewRefreshDebounceTimeSpinBox->setValue(
        settingValue(QStringLiteral("MainWindow/noteTextView.refreshDebounceTime"), 600).toInt());
    _useInternalExportStylingCheckBox->setChecked(
        settingValue(QStringLiteral("MainWindow/noteTextView.useInternalExportStyling"), true).toBool());

    QString fontString = settingValue(QStringLiteral("MainWindow/noteTextView.font")).toString();
    if (fontString.isEmpty()) {
        QTextEdit textEdit;
        fontString = textEdit.font().toString();
        setSettingValue(QStringLiteral("MainWindow/noteTextView.font"), fontString);
    }

    noteTextViewFont.fromString(fontString);
    setFontLabel(_noteTextViewFontLabel, noteTextViewFont);

    fontString = settingValue(QStringLiteral("MainWindow/noteTextView.code.font")).toString();
    if (fontString.isEmpty()) {
        on_noteTextViewCodeResetButton_clicked();
        fontString = noteTextViewCodeFont.toString();
        setSettingValue(QStringLiteral("MainWindow/noteTextView.code.font"), fontString);
    } else {
        noteTextViewCodeFont.fromString(fontString);
    }

    setFontLabel(_noteTextViewCodeFontLabel, noteTextViewCodeFont);
    on_noteTextViewUseEditorStylesCheckBox_toggled(_noteTextViewUseEditorStylesCheckBox->isChecked());
}

void PreviewFontSettingsWidget::storeSettings() {
    setSettingValue(QStringLiteral("MainWindow/noteTextView.rtl"), _noteTextViewRTLCheckBox->isChecked());
    setSettingValue(QStringLiteral("MainWindow/noteTextView.ignoreCodeFontSize"),
                    _noteTextViewIgnoreCodeFontSizeCheckBox->isChecked());
    setSettingValue(QStringLiteral("MainWindow/noteTextView.underline"), _noteTextViewUnderlineCheckBox->isChecked());
    setSettingValue(QStringLiteral("MainWindow/noteTextView.useEditorStyles"),
                    _noteTextViewUseEditorStylesCheckBox->isChecked());
    setSettingValue(QStringLiteral("MainWindow/noteTextView.useInternalExportStyling"),
                    _useInternalExportStylingCheckBox->isChecked());
    setSettingValue(QStringLiteral("MainWindow/noteTextView.refreshDebounceTime"),
                    _noteTextViewRefreshDebounceTimeSpinBox->value());
    setSettingValue(QStringLiteral("MainWindow/noteTextView.font"), noteTextViewFont.toString());
    setSettingValue(QStringLiteral("MainWindow/noteTextView.code.font"), noteTextViewCodeFont.toString());
}

void PreviewFontSettingsWidget::on_noteTextViewButton_clicked() {
    bool ok;
    QFont font = Utils::Gui::fontDialogGetFont(&ok, noteTextViewFont, this);
    if (ok) {
        noteTextViewFont = font;
        setFontLabel(_noteTextViewFontLabel, noteTextViewFont);
    }
}

void PreviewFontSettingsWidget::on_noteTextViewCodeButton_clicked() {
    bool ok;
    QFont font =
        Utils::Gui::fontDialogGetFont(&ok, noteTextViewCodeFont, this, QString(), QFontDialog::MonospacedFonts);
    if (ok) {
        noteTextViewCodeFont = font;
        setFontLabel(_noteTextViewCodeFontLabel, noteTextViewCodeFont);
    }
}

void PreviewFontSettingsWidget::on_noteTextViewResetButton_clicked() {
    QTextBrowser textView;
    noteTextViewFont = textView.font();
    setFontLabel(_noteTextViewFontLabel, noteTextViewFont);
}

void PreviewFontSettingsWidget::on_noteTextViewCodeResetButton_clicked() {
    noteTextViewCodeFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    setFontLabel(_noteTextViewCodeFontLabel, noteTextViewCodeFont);
}

void PreviewFontSettingsWidget::on_noteTextViewUseEditorStylesCheckBox_toggled(bool checked) {
    _previewFontsGroupBox->setDisabled(checked);
}

void PreviewFontSettingsWidget::on_noteTextViewRefreshDebounceTimeResetButton_clicked() {
    _noteTextViewRefreshDebounceTimeSpinBox->setValue(600);
}

void PreviewFontSettingsWidget::setFontLabel(QLineEdit* label, const QFont& font) {
    label->setText(font.family() + QStringLiteral(" (") + QString::number(font.pointSize()) + QStringLiteral(")"));
    label->setFont(font);
}

QVariant PreviewFontSettingsWidget::settingValue(const QString& key, const QVariant& defaultValue) const {
    return _settingsViewModel == nullptr ? defaultValue : _settingsViewModel->persistentSetting(key, defaultValue);
}

void PreviewFontSettingsWidget::setSettingValue(const QString& key, const QVariant& value) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->setPersistentSetting(key, value);
    }
}
