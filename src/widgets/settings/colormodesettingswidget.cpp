/*
 * Copyright (c) 2014-2026 Patrizio Bekerle -- <patrizio@bekerle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 */

#include "colormodesettingswidget.h"

#include <utils/gui.h>
#include <utils/schema.h>
#include <viewmodels/settingsviewmodel.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QVariant>

ColorModeSettingsWidget::ColorModeSettingsWidget(QWidget* parent, SettingsViewModel* settingsViewModel)
    : QWidget(parent), _settingsViewModel(settingsViewModel) {
    auto* rootLayout = new QGridLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* colorModesGroupBox = new QGroupBox(tr("Color modes"), this);
    rootLayout->addWidget(colorModesGroupBox, 0, 0);

    auto* innerLayout = new QGridLayout(colorModesGroupBox);

    auto* selectFrame = new QFrame(colorModesGroupBox);
    selectFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    selectFrame->setFrameShape(QFrame::NoFrame);
    auto* selectLayout = new QGridLayout(selectFrame);
    selectLayout->setContentsMargins(0, 0, 0, 0);
    selectLayout->setSpacing(9);
    innerLayout->addWidget(selectFrame, 0, 0);

    _colorModeListWidget = new QListWidget(selectFrame);
    _colorModeListWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    selectLayout->addWidget(_colorModeListWidget, 0, 0);

    auto* buttonFrame = new QFrame(selectFrame);
    buttonFrame->setFrameShape(QFrame::NoFrame);
    auto* buttonLayout = new QGridLayout(buttonFrame);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    selectLayout->addWidget(buttonFrame, 1, 0);

    buttonLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 0);
    auto* colorModeAddButton = new QPushButton(tr("&Add color mode"), buttonFrame);
    colorModeAddButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add"),
                                                 QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/list-add.svg"))));
    buttonLayout->addWidget(colorModeAddButton, 0, 1);

    _colorModeRemoveButton = new QPushButton(tr("&Remove color mode"), buttonFrame);
    _colorModeRemoveButton->setIcon(QIcon::fromTheme(
        QStringLiteral("edit-delete"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/edit-delete.svg"))));
    buttonLayout->addWidget(_colorModeRemoveButton, 0, 2);

    _colorModeEditFrame = new QFrame(colorModesGroupBox);
    _colorModeEditFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    _colorModeEditFrame->setFrameShape(QFrame::NoFrame);
    auto* editLayout = new QGridLayout(_colorModeEditFrame);
    innerLayout->addWidget(_colorModeEditFrame, 0, 3);

    editLayout->addWidget(new QLabel(tr("Name:"), _colorModeEditFrame), 0, 0, 1, 2);
    _colorModeNameLineEdit = new QLineEdit(_colorModeEditFrame);
    _colorModeNameLineEdit->setToolTip(tr("Name of the color mode"));
    _colorModeNameLineEdit->setPlaceholderText(tr("Color mode name"));
    _colorModeNameLineEdit->setClearButtonEnabled(true);
    editLayout->addWidget(_colorModeNameLineEdit, 1, 0, 1, 2);

    _colorModeActiveCheckBox = new QCheckBox(tr("Use as active color mode"), _colorModeEditFrame);
    editLayout->addWidget(_colorModeActiveCheckBox, 2, 0, 1, 2);

    auto* darkModeGroupBox = new QGroupBox(tr("Dark mode"), _colorModeEditFrame);
    auto* darkModeLayout = new QGridLayout(darkModeGroupBox);
    editLayout->addWidget(darkModeGroupBox, 3, 0, 1, 2);

    _colorModeDarkModeCheckBox = new QCheckBox(tr("Enable dark mode"), darkModeGroupBox);
    _colorModeDarkModeCheckBox->setToolTip(
        tr("This enables the dark mode, interface and icons will be modified. You need to "
           "restart the application to let these changes take action."));
    darkModeLayout->addWidget(_colorModeDarkModeCheckBox, 0, 0);

    _colorModeDarkModeColorsCheckBox =
        new QCheckBox(tr("Optimize preview colors for dark mode desktop themes"), darkModeGroupBox);
    _colorModeDarkModeColorsCheckBox->setToolTip(
        tr("Only some colors will be adapted, your desktop theme will control the rest. You "
           "need to restart the application to let these changes take action."));
    darkModeLayout->addWidget(_colorModeDarkModeColorsCheckBox, 1, 0);

    _colorModeDarkModeTrayIconCheckBox =
        new QCheckBox(tr("Enable dark mode application icon and tray icon"), darkModeGroupBox);
    _colorModeDarkModeTrayIconCheckBox->setToolTip(
        tr("You may need to restart the application to let these changes take effect"));
    darkModeLayout->addWidget(_colorModeDarkModeTrayIconCheckBox, 2, 0);

    auto* iconsGroupBox = new QGroupBox(tr("Icons"), _colorModeEditFrame);
    auto* iconsLayout = new QGridLayout(iconsGroupBox);
    editLayout->addWidget(iconsGroupBox, 4, 0, 1, 2);

    _colorModeInternalIconThemeCheckBox =
        new QCheckBox(tr("Use internal icon theme instead of system icon theme"), iconsGroupBox);
    _colorModeInternalIconThemeCheckBox->setToolTip(
        tr("You need to restart the application to let this setting take effect"));
    iconsLayout->addWidget(_colorModeInternalIconThemeCheckBox, 0, 0);

    _colorModeSystemIconThemeCheckBox = new QCheckBox(tr("Enforce system icon theme"), iconsGroupBox);
    _colorModeSystemIconThemeCheckBox->setToolTip(
        tr("You need to restart the application to let this setting take effect"));
    iconsLayout->addWidget(_colorModeSystemIconThemeCheckBox, 1, 0);

    _colorModeDarkModeIconThemeCheckBox = new QCheckBox(tr("Enable dark mode icon theme"), iconsGroupBox);
    _colorModeDarkModeIconThemeCheckBox->setToolTip(
        tr("You may need to restart the application to let these changes take effect"));
    iconsLayout->addWidget(_colorModeDarkModeIconThemeCheckBox, 2, 0);

    auto* editorSchemaGroupBox = new QGroupBox(tr("Editor color schema"), _colorModeEditFrame);
    auto* editorSchemaLayout = new QGridLayout(editorSchemaGroupBox);
    editLayout->addWidget(editorSchemaGroupBox, 5, 0, 1, 2);

    _colorModeEditorColorSchemaComboBox = new QComboBox(editorSchemaGroupBox);
    _colorModeEditorColorSchemaComboBox->setToolTip(tr("Select the editor color schema for this color mode"));
    editorSchemaLayout->addWidget(_colorModeEditorColorSchemaComboBox, 0, 0);

    editLayout->addItem(new QSpacerItem(20, 5, QSizePolicy::Minimum, QSizePolicy::Expanding), 6, 0);

    connect(_colorModeListWidget, &QListWidget::currentItemChanged, this,
            &ColorModeSettingsWidget::on_colorModeListWidget_currentItemChanged);
    connect(colorModeAddButton, &QPushButton::clicked, this, &ColorModeSettingsWidget::on_colorModeAddButton_clicked);
    connect(_colorModeRemoveButton, &QPushButton::clicked, this,
            &ColorModeSettingsWidget::on_colorModeRemoveButton_clicked);
    connect(_colorModeNameLineEdit, &QLineEdit::editingFinished, this,
            &ColorModeSettingsWidget::on_colorModeNameLineEdit_editingFinished);
    connect(_colorModeActiveCheckBox, &QCheckBox::checkStateChanged, this,
            &ColorModeSettingsWidget::on_colorModeActiveCheckBox_stateChanged);
    connect(_colorModeDarkModeCheckBox, &QCheckBox::toggled, this,
            &ColorModeSettingsWidget::on_colorModeDarkModeCheckBox_toggled);
    connect(_colorModeDarkModeColorsCheckBox, &QCheckBox::toggled, this,
            &ColorModeSettingsWidget::on_colorModeDarkModeColorsCheckBox_toggled);
    connect(_colorModeDarkModeTrayIconCheckBox, &QCheckBox::toggled, this,
            &ColorModeSettingsWidget::on_colorModeDarkModeTrayIconCheckBox_toggled);
    connect(_colorModeDarkModeIconThemeCheckBox, &QCheckBox::toggled, this,
            &ColorModeSettingsWidget::on_colorModeDarkModeIconThemeCheckBox_toggled);
    connect(_colorModeInternalIconThemeCheckBox, &QCheckBox::toggled, this,
            &ColorModeSettingsWidget::on_colorModeInternalIconThemeCheckBox_toggled);
    connect(_colorModeSystemIconThemeCheckBox, &QCheckBox::toggled, this,
            &ColorModeSettingsWidget::on_colorModeSystemIconThemeCheckBox_toggled);
    connect(_colorModeEditorColorSchemaComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ColorModeSettingsWidget::on_colorModeEditorColorSchemaComboBox_currentIndexChanged);

    ensureBuiltInColorModes();
    _initialColorModeId = currentColorModeId();
}

ColorModeSettingsWidget::~ColorModeSettingsWidget() = default;

QString ColorModeSettingsWidget::initialColorModeId() const {
    return _initialColorModeId;
}

void ColorModeSettingsWidget::initEditorSchemaComboBox() {
    _colorModeEditorColorSchemaComboBox->clear();

    const QStringList& defaultSchemaKeys = Utils::Schema::schemaSettings->defaultSchemaKeys();
    QMap<QString, QString> defaultSchemaNameKeys;

    const QSettings& defaultSchemaSettings = Utils::Schema::schemaSettings->defaultSchemaSettings();

    for (const QString& schemaKey : defaultSchemaKeys) {
        QString name = defaultSchemaSettings.value(schemaKey + QStringLiteral("/Name")).toString();

        if (schemaKey == QStringLiteral("EditorColorSchema-6033d61b-cb96-46d5-a3a8-20d5172017eb")) {
            name = QStringLiteral("  ") + name;
        }

        if (schemaKey == QStringLiteral("EditorColorSchema-cdbf28fc-1ddc-4d13-bb21-6a4043316a2f")) {
            name = QStringLiteral(" ") + name;
        }

        defaultSchemaNameKeys.insert(name, schemaKey);
    }

    for (const QString& schemaKey : defaultSchemaNameKeys.values()) {
        const QString name = defaultSchemaSettings.value(schemaKey + QStringLiteral("/Name")).toString();
        _colorModeEditorColorSchemaComboBox->addItem(name.trimmed(), schemaKey);
    }

    const QStringList schemes = settingValue(QStringLiteral("Editor/ColorSchemes")).toStringList();
    for (const QString& schemaKey : schemes) {
        const QString name = settingValue(schemaKey + QStringLiteral("/Name")).toString();
        _colorModeEditorColorSchemaComboBox->addItem(name, schemaKey);
    }
}

void ColorModeSettingsWidget::initialize() {
    initEditorSchemaComboBox();

    _colorModeEditFrame->setEnabled(false);
    _colorModeListWidget->clear();

    const QList<ColorModeData> colorModes = availableColorModes();

    for (const ColorModeData& mode : colorModes) {
        auto* item = new QListWidgetItem(mode.name);
        item->setData(Qt::UserRole, mode.id);
        _colorModeListWidget->addItem(item);

        if (mode.id == currentColorModeId()) {
            _colorModeListWidget->setCurrentItem(item);
        }
    }

    if (_colorModeListWidget->currentRow() == -1 && _colorModeListWidget->count() > 0) {
        _colorModeListWidget->setCurrentRow(0);
    }
}

void ColorModeSettingsWidget::applyColorModeSettings() {
    const ColorModeData mode = currentColorMode();

    setSettingValue(QStringLiteral("darkMode"), mode.darkMode);
    setSettingValue(QStringLiteral("darkModeColors"), mode.darkModeColors);
    setSettingValue(QStringLiteral("darkModeTrayIcon"), mode.darkModeTrayIcon);
    setSettingValue(QStringLiteral("darkModeIconTheme"), mode.darkModeIconTheme);
    setSettingValue(QStringLiteral("internalIconTheme"), mode.internalIconTheme);
    setSettingValue(QStringLiteral("systemIconTheme"), mode.systemIconTheme);

    if (!mode.editorColorSchemaKey.isEmpty()) {
        setSettingValue(QStringLiteral("Editor/CurrentSchemaKey"), mode.editorColorSchemaKey);
    }

    Utils::Gui::fixDarkModeIcons(this);
    Utils::Gui::applyDarkModeSettings();
}

void ColorModeSettingsWidget::on_colorModeListWidget_currentItemChanged(QListWidgetItem* current,
                                                                        QListWidgetItem* previous) {
    Q_UNUSED(previous)

    if (current == nullptr) {
        _colorModeEditFrame->setEnabled(false);
        return;
    }

    _colorModeEditFrame->setEnabled(true);

    const QString colorModeId = current->data(Qt::UserRole).toString();
    _selectedColorMode = colorModeById(colorModeId);

    const QSignalBlocker nameBlocker(_colorModeNameLineEdit);
    Q_UNUSED(nameBlocker)
    const QSignalBlocker activeBlocker(_colorModeActiveCheckBox);
    Q_UNUSED(activeBlocker)
    const QSignalBlocker darkModeBlocker(_colorModeDarkModeCheckBox);
    Q_UNUSED(darkModeBlocker)
    const QSignalBlocker darkModeColorsBlocker(_colorModeDarkModeColorsCheckBox);
    Q_UNUSED(darkModeColorsBlocker)
    const QSignalBlocker darkModeTrayIconBlocker(_colorModeDarkModeTrayIconCheckBox);
    Q_UNUSED(darkModeTrayIconBlocker)
    const QSignalBlocker darkModeIconThemeBlocker(_colorModeDarkModeIconThemeCheckBox);
    Q_UNUSED(darkModeIconThemeBlocker)
    const QSignalBlocker internalIconThemeBlocker(_colorModeInternalIconThemeCheckBox);
    Q_UNUSED(internalIconThemeBlocker)
    const QSignalBlocker systemIconThemeBlocker(_colorModeSystemIconThemeCheckBox);
    Q_UNUSED(systemIconThemeBlocker)
    const QSignalBlocker schemaBlocker(_colorModeEditorColorSchemaComboBox);
    Q_UNUSED(schemaBlocker)

    _colorModeNameLineEdit->setText(_selectedColorMode.name);
    _colorModeActiveCheckBox->setChecked(_selectedColorMode.id == currentColorModeId());

    _colorModeDarkModeCheckBox->setChecked(_selectedColorMode.darkMode);
    _colorModeDarkModeColorsCheckBox->setChecked(_selectedColorMode.darkModeColors);
    _colorModeDarkModeTrayIconCheckBox->setChecked(_selectedColorMode.darkModeTrayIcon);
    _colorModeDarkModeIconThemeCheckBox->setChecked(_selectedColorMode.darkModeIconTheme);

    _colorModeInternalIconThemeCheckBox->setChecked(_selectedColorMode.internalIconTheme);
    _colorModeSystemIconThemeCheckBox->setChecked(_selectedColorMode.systemIconTheme);

    const QString schemaKey = _selectedColorMode.editorColorSchemaKey;
    for (int i = 0; i < _colorModeEditorColorSchemaComboBox->count(); i++) {
        if (_colorModeEditorColorSchemaComboBox->itemData(i).toString() == schemaKey) {
            _colorModeEditorColorSchemaComboBox->setCurrentIndex(i);
            break;
        }
    }

    _colorModeNameLineEdit->setReadOnly(selectedColorModeIsBuiltIn());
    _colorModeRemoveButton->setEnabled(!selectedColorModeIsBuiltIn());

#ifndef Q_OS_LINUX
    _colorModeSystemIconThemeCheckBox->setHidden(true);
    _colorModeSystemIconThemeCheckBox->setChecked(false);
#endif
}

void ColorModeSettingsWidget::on_colorModeAddButton_clicked() {
    ColorModeData newMode = createColorMode(tr("New color mode"));
    if (newMode.id.isEmpty()) {
        return;
    }

    auto* item = new QListWidgetItem(newMode.name);
    item->setData(Qt::UserRole, newMode.id);
    _colorModeListWidget->addItem(item);

    _colorModeListWidget->setCurrentRow(_colorModeListWidget->count() - 1);
    _colorModeNameLineEdit->setFocus();
    _colorModeNameLineEdit->selectAll();
}

void ColorModeSettingsWidget::on_colorModeRemoveButton_clicked() {
    if (!hasSelectedColorMode() || selectedColorModeIsBuiltIn()) {
        return;
    }

    if (Utils::Gui::question(this, tr("Remove color mode"),
                             tr("Remove the color mode <strong>%1</strong>?").arg(_selectedColorMode.name),
                             QStringLiteral("remove-color-mode")) == QMessageBox::Yes) {
        const bool wasCurrent = _selectedColorMode.id == currentColorModeId();

        removeColorMode(_selectedColorMode.id);
        delete _colorModeListWidget->takeItem(_colorModeListWidget->currentRow());

        if (wasCurrent) {
            setCurrentColorModeId(lightColorModeId());
            applyColorModeSettings();
        }
    }
}

void ColorModeSettingsWidget::on_colorModeNameLineEdit_editingFinished() {
    if (!hasSelectedColorMode() || selectedColorModeIsBuiltIn()) {
        return;
    }

    QString text = _colorModeNameLineEdit->text().remove(QStringLiteral("\n")).trimmed();
    text.truncate(50);

    if (text.isEmpty()) {
        text = tr("Color mode");
    }

    _selectedColorMode.name = text;
    saveColorMode(_selectedColorMode);

    _colorModeListWidget->currentItem()->setText(text);
}

void ColorModeSettingsWidget::on_colorModeActiveCheckBox_stateChanged(Qt::CheckState arg1) {
    Q_UNUSED(arg1)

    if (!hasSelectedColorMode()) {
        return;
    }

    if (!_colorModeActiveCheckBox->isChecked()) {
        const QSignalBlocker blocker(_colorModeActiveCheckBox);
        Q_UNUSED(blocker)
        _colorModeActiveCheckBox->setChecked(true);
    } else {
        setCurrentColorModeId(_selectedColorMode.id);
        applyColorModeSettings();
    }
}

void ColorModeSettingsWidget::on_colorModeDarkModeCheckBox_toggled(bool checked) {
    if (!hasSelectedColorMode()) {
        return;
    }

    _selectedColorMode.darkMode = checked;

    if (checked) {
        const QSignalBlocker colorsBlocker(_colorModeDarkModeColorsCheckBox);
        Q_UNUSED(colorsBlocker)
        _colorModeDarkModeColorsCheckBox->setChecked(true);
        _selectedColorMode.darkModeColors = true;

        const QSignalBlocker iconBlocker(_colorModeDarkModeIconThemeCheckBox);
        Q_UNUSED(iconBlocker)
        _colorModeDarkModeIconThemeCheckBox->setChecked(true);
        _selectedColorMode.darkModeIconTheme = true;
    }

    saveColorMode(_selectedColorMode);

    if (_selectedColorMode.id == currentColorModeId()) {
        applyColorModeSettings();
    }
}

void ColorModeSettingsWidget::on_colorModeDarkModeColorsCheckBox_toggled(bool checked) {
    if (!hasSelectedColorMode()) {
        return;
    }

    _selectedColorMode.darkModeColors = checked;
    saveColorMode(_selectedColorMode);

    if (_selectedColorMode.id == currentColorModeId()) {
        applyColorModeSettings();
    }
}

void ColorModeSettingsWidget::on_colorModeDarkModeTrayIconCheckBox_toggled(bool checked) {
    if (!hasSelectedColorMode()) {
        return;
    }

    _selectedColorMode.darkModeTrayIcon = checked;
    saveColorMode(_selectedColorMode);

    if (_selectedColorMode.id == currentColorModeId()) {
        applyColorModeSettings();
    }
}

void ColorModeSettingsWidget::on_colorModeDarkModeIconThemeCheckBox_toggled(bool checked) {
    if (!hasSelectedColorMode()) {
        return;
    }

    _selectedColorMode.darkModeIconTheme = checked;
    saveColorMode(_selectedColorMode);

    if (_selectedColorMode.id == currentColorModeId()) {
        applyColorModeSettings();
    }
}

void ColorModeSettingsWidget::on_colorModeInternalIconThemeCheckBox_toggled(bool checked) {
    if (!hasSelectedColorMode()) {
        return;
    }

    _selectedColorMode.internalIconTheme = checked;

    if (checked) {
        const QSignalBlocker blocker(_colorModeSystemIconThemeCheckBox);
        Q_UNUSED(blocker)
        _colorModeSystemIconThemeCheckBox->setChecked(false);
        _selectedColorMode.systemIconTheme = false;
        _colorModeSystemIconThemeCheckBox->setEnabled(false);
    } else {
        _colorModeSystemIconThemeCheckBox->setEnabled(true);
    }

    saveColorMode(_selectedColorMode);

    if (_selectedColorMode.id == currentColorModeId()) {
        applyColorModeSettings();
    }
}

void ColorModeSettingsWidget::on_colorModeSystemIconThemeCheckBox_toggled(bool checked) {
    if (!hasSelectedColorMode()) {
        return;
    }

    _selectedColorMode.systemIconTheme = checked;

    if (checked) {
        const QSignalBlocker blocker(_colorModeInternalIconThemeCheckBox);
        Q_UNUSED(blocker)
        _colorModeInternalIconThemeCheckBox->setChecked(false);
        _selectedColorMode.internalIconTheme = false;
        _colorModeInternalIconThemeCheckBox->setEnabled(false);
        _colorModeDarkModeIconThemeCheckBox->setEnabled(false);
    } else {
        _colorModeInternalIconThemeCheckBox->setEnabled(true);
        _colorModeDarkModeIconThemeCheckBox->setEnabled(true);
    }

    saveColorMode(_selectedColorMode);

    if (_selectedColorMode.id == currentColorModeId()) {
        applyColorModeSettings();
    }
}

void ColorModeSettingsWidget::on_colorModeEditorColorSchemaComboBox_currentIndexChanged(int index) {
    if (index < 0 || !hasSelectedColorMode()) {
        return;
    }

    const QString schemaKey = _colorModeEditorColorSchemaComboBox->itemData(index).toString();
    _selectedColorMode.editorColorSchemaKey = schemaKey;
    saveColorMode(_selectedColorMode);

    if (_selectedColorMode.id == currentColorModeId()) {
        applyColorModeSettings();
    }
}

bool ColorModeSettingsWidget::hasSelectedColorMode() const {
    return !_selectedColorMode.id.isEmpty();
}

bool ColorModeSettingsWidget::selectedColorModeIsBuiltIn() const {
    return _settingsViewModel != nullptr && _settingsViewModel->isBuiltInColorModeId(_selectedColorMode.id);
}

QList<ColorModeData> ColorModeSettingsWidget::availableColorModes() const {
    return _settingsViewModel == nullptr ? QList<ColorModeData>() : _settingsViewModel->colorModes();
}

ColorModeData ColorModeSettingsWidget::colorModeById(const QString& id) const {
    return _settingsViewModel == nullptr ? ColorModeData() : _settingsViewModel->colorModeById(id);
}

ColorModeData ColorModeSettingsWidget::currentColorMode() const {
    return _settingsViewModel == nullptr ? ColorModeData() : _settingsViewModel->currentColorMode();
}

QString ColorModeSettingsWidget::currentColorModeId() const {
    return _settingsViewModel == nullptr ? QString() : _settingsViewModel->currentColorModeId();
}

QString ColorModeSettingsWidget::lightColorModeId() const {
    return _settingsViewModel == nullptr ? QString() : _settingsViewModel->lightColorModeId();
}

void ColorModeSettingsWidget::ensureBuiltInColorModes() {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->ensureBuiltInColorModes();
    }
}

ColorModeData ColorModeSettingsWidget::createColorMode(const QString& name) {
    return _settingsViewModel == nullptr ? ColorModeData() : _settingsViewModel->createColorMode(name);
}

bool ColorModeSettingsWidget::saveColorMode(const ColorModeData& colorMode) {
    return _settingsViewModel != nullptr && _settingsViewModel->saveColorMode(colorMode);
}

bool ColorModeSettingsWidget::removeColorMode(const QString& id) {
    return _settingsViewModel != nullptr && _settingsViewModel->removeColorMode(id);
}

bool ColorModeSettingsWidget::setCurrentColorModeId(const QString& id) {
    return _settingsViewModel != nullptr && _settingsViewModel->setCurrentColorModeId(id);
}

QVariant ColorModeSettingsWidget::settingValue(const QString& key, const QVariant& defaultValue) const {
    return _settingsViewModel == nullptr ? defaultValue : _settingsViewModel->persistentSetting(key, defaultValue);
}

void ColorModeSettingsWidget::setSettingValue(const QString& key, const QVariant& value) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->setPersistentSetting(key, value);
    }
}
