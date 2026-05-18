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

#include <QVariant>
#include <QWidget>

class QCheckBox;
class QFrame;
class ArcNotesMarkdownTextEdit;
class SettingsViewModel;

class DebugSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit DebugSettingsWidget(QWidget* parent = nullptr, SettingsViewModel* settingsViewModel = nullptr);
    ~DebugSettingsWidget() override;

    void initialize();
    void outputSettings();

signals:
    void aboutToOutputSettings();

private slots:
    void on_gitHubLineBreaksCheckBox_toggled(bool checked);
    void on_debugInfoAnonymizeCheckBox_toggled(bool checked);
    void on_saveDebugInfoButton_clicked();
    void on_copyDebugInfoButton_clicked();

private:
    ArcNotesMarkdownTextEdit* _debugInfoTextEdit = nullptr;
    QFrame* _debugInfoTextEditSearchFrame = nullptr;
    QCheckBox* _gitHubLineBreaksCheckBox = nullptr;
    QCheckBox* _debugInfoAnonymizeCheckBox = nullptr;
    SettingsViewModel* _settingsViewModel = nullptr;

    QVariant settingValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(const QString& key, const QVariant& value);
};
