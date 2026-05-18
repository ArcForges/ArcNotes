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

#pragma once

#include <core/data/colormodedata.h>

#include <QVariant>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QFrame;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class SettingsViewModel;

class ColorModeSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit ColorModeSettingsWidget(QWidget* parent = nullptr, SettingsViewModel* settingsViewModel = nullptr);
    ~ColorModeSettingsWidget() override;

    void initialize();
    QString initialColorModeId() const;

private slots:
    void on_colorModeListWidget_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void on_colorModeAddButton_clicked();
    void on_colorModeRemoveButton_clicked();
    void on_colorModeNameLineEdit_editingFinished();
    void on_colorModeActiveCheckBox_stateChanged(Qt::CheckState arg1);
    void on_colorModeDarkModeCheckBox_toggled(bool checked);
    void on_colorModeDarkModeColorsCheckBox_toggled(bool checked);
    void on_colorModeDarkModeTrayIconCheckBox_toggled(bool checked);
    void on_colorModeDarkModeIconThemeCheckBox_toggled(bool checked);
    void on_colorModeInternalIconThemeCheckBox_toggled(bool checked);
    void on_colorModeSystemIconThemeCheckBox_toggled(bool checked);
    void on_colorModeEditorColorSchemaComboBox_currentIndexChanged(int index);

private:
    QListWidget* _colorModeListWidget = nullptr;
    QFrame* _colorModeEditFrame = nullptr;
    QPushButton* _colorModeRemoveButton = nullptr;
    QLineEdit* _colorModeNameLineEdit = nullptr;
    QCheckBox* _colorModeActiveCheckBox = nullptr;
    QCheckBox* _colorModeDarkModeCheckBox = nullptr;
    QCheckBox* _colorModeDarkModeColorsCheckBox = nullptr;
    QCheckBox* _colorModeDarkModeTrayIconCheckBox = nullptr;
    QCheckBox* _colorModeDarkModeIconThemeCheckBox = nullptr;
    QCheckBox* _colorModeInternalIconThemeCheckBox = nullptr;
    QCheckBox* _colorModeSystemIconThemeCheckBox = nullptr;
    QComboBox* _colorModeEditorColorSchemaComboBox = nullptr;
    ColorModeData _selectedColorMode;
    SettingsViewModel* _settingsViewModel = nullptr;
    QString _initialColorModeId;

    void initEditorSchemaComboBox();
    void applyColorModeSettings();
    bool hasSelectedColorMode() const;
    bool selectedColorModeIsBuiltIn() const;
    QList<ColorModeData> availableColorModes() const;
    ColorModeData colorModeById(const QString& id) const;
    ColorModeData currentColorMode() const;
    QString currentColorModeId() const;
    QString lightColorModeId() const;
    void ensureBuiltInColorModes();
    ColorModeData createColorMode(const QString& name);
    bool saveColorMode(const ColorModeData& colorMode);
    bool removeColorMode(const QString& id);
    bool setCurrentColorModeId(const QString& id);
    QVariant settingValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(const QString& key, const QVariant& value);
};
