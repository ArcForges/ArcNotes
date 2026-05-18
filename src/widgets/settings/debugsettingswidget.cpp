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

#include "debugsettingswidget.h"

#include <widgets/arcnotesmarkdowntextedit.h>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDebug>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QTextStream>
#include <QVariant>

#include "dialogs/filedialog.h"
#include "utils/gui.h"
#include "utils/misc.h"
#include "viewmodels/settingsviewmodel.h"

DebugSettingsWidget::DebugSettingsWidget(QWidget* parent, SettingsViewModel* settingsViewModel)
    : QWidget(parent), _settingsViewModel(settingsViewModel) {
    auto* rootLayout = new QGridLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* groupBox = new QGroupBox(tr("Debug information"), this);
    rootLayout->addWidget(groupBox, 0, 0);

    auto* groupLayout = new QGridLayout(groupBox);

    auto* descriptionLabel =
        new QLabel(tr("<html><head/><body><p>You can <span style=\" font-weight:600;\">copy and "
                      "paste</span> this text (or parts of this text) in an <span style=\" "
                      "font-weight:600;\">issue</span> on the <a "
                      "href=\"https://github.com/pbek/ArcNotes/issues\">ArcNotes issues page</a> if "
                      "you have <span style=\" font-weight:600;\">questions or troubles</span> with "
                      "ArcNotes. Data that is too sensitive is hidden in this information.</p></body></html>"),
                   groupBox);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setOpenExternalLinks(true);
    groupLayout->addWidget(descriptionLabel, 0, 0, 1, 7);

    auto* copyOrSaveLabel =
        new QLabel(tr("<html><head/><body><p>Copy or save the generated debug information when you need "
                      "to inspect the application environment.</p></body></html>"),
                   groupBox);
    groupLayout->addWidget(copyOrSaveLabel, 1, 0, 1, 7);

    _debugInfoTextEdit = new ArcNotesMarkdownTextEdit(groupBox);
    _debugInfoTextEdit->setReadOnly(true);
    _debugInfoTextEdit->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    groupLayout->addWidget(_debugInfoTextEdit, 2, 0, 1, 7);

    _debugInfoTextEditSearchFrame = new QFrame(groupBox);
    _debugInfoTextEditSearchFrame->setFrameShape(QFrame::NoFrame);
    _debugInfoTextEditSearchFrame->setLineWidth(0);
    groupLayout->addWidget(_debugInfoTextEditSearchFrame, 4, 0, 1, 7);

    _gitHubLineBreaksCheckBox = new QCheckBox(tr("Use GitHub line breaks"), groupBox);
    _gitHubLineBreaksCheckBox->setToolTip(
        tr("GitHub doesn't need two spaces at the end of the line to do a line break"));
    _gitHubLineBreaksCheckBox->setChecked(true);
    groupLayout->addWidget(_gitHubLineBreaksCheckBox, 5, 0, 1, 7);

    _debugInfoAnonymizeCheckBox = new QCheckBox(tr("Anonymize personal information"), groupBox);
    _debugInfoAnonymizeCheckBox->setToolTip(
        tr("Hide usernames in paths and anonymize other personal identifiers in the generated "
           "debug information"));
    groupLayout->addWidget(_debugInfoAnonymizeCheckBox, 6, 0, 1, 7);

    auto* copyDebugInfoButton = new QPushButton(tr("&Copy debug information to clipboard"), groupBox);
    copyDebugInfoButton->setToolTip(tr("Please use this in the issue tracker"));
    copyDebugInfoButton->setIcon(QIcon::fromTheme(
        QStringLiteral("edit-copy"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/edit-copy.svg"))));
    groupLayout->addWidget(copyDebugInfoButton, 7, 0);

    auto* saveDebugInfoButton = new QPushButton(tr("&Save debug information"), groupBox);
    saveDebugInfoButton->setToolTip(tr("Please don't use this in the issue tracker"));
    saveDebugInfoButton->setIcon(QIcon::fromTheme(
        QStringLiteral("document-save"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/document-save.svg"))));
    groupLayout->addWidget(saveDebugInfoButton, 7, 1);
    groupLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 7, 2, 1, 5);

    connect(_gitHubLineBreaksCheckBox, &QCheckBox::toggled, this,
            &DebugSettingsWidget::on_gitHubLineBreaksCheckBox_toggled);
    connect(_debugInfoAnonymizeCheckBox, &QCheckBox::toggled, this,
            &DebugSettingsWidget::on_debugInfoAnonymizeCheckBox_toggled);
    connect(saveDebugInfoButton, &QPushButton::clicked, this, &DebugSettingsWidget::on_saveDebugInfoButton_clicked);
    connect(copyDebugInfoButton, &QPushButton::clicked, this, &DebugSettingsWidget::on_copyDebugInfoButton_clicked);
}

DebugSettingsWidget::~DebugSettingsWidget() = default;

void DebugSettingsWidget::initialize() {
    _debugInfoTextEdit->initSearchFrame(_debugInfoTextEditSearchFrame);

    _debugInfoAnonymizeCheckBox->setChecked(settingValue(QStringLiteral("debugInfoAnonymize")).toBool());
}

void DebugSettingsWidget::outputSettings() {
    emit aboutToOutputSettings();

    const QString output = Utils::Misc::generateDebugInformation(_gitHubLineBreaksCheckBox->isChecked(),
                                                                 _debugInfoAnonymizeCheckBox->isChecked());

    _debugInfoTextEdit->setPlainText(output);
}

void DebugSettingsWidget::on_gitHubLineBreaksCheckBox_toggled(bool checked) {
    Q_UNUSED(checked)
    outputSettings();
}

void DebugSettingsWidget::on_debugInfoAnonymizeCheckBox_toggled(bool checked) {
    setSettingValue(QStringLiteral("debugInfoAnonymize"), checked);
    outputSettings();
}

void DebugSettingsWidget::on_saveDebugInfoButton_clicked() {
    Utils::Gui::information(this, tr("Debug information"),
                            tr("Please don't use this in the issue tracker, "
                               "copy the debug information text directly into the issue."),
                            QStringLiteral("debug-save"));

    FileDialog dialog(QStringLiteral("SaveDebugInfo"));
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter(tr("Markdown files") + QStringLiteral(" (*.md)"));
    dialog.setWindowTitle(tr("Save debug information"));
    dialog.selectFile(QStringLiteral("ArcNotes Debug Information.md"));
    const int ret = dialog.exec();

    if (ret == QDialog::Accepted) {
        const QString fileName = dialog.selectedFile();
        QFile file(fileName);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << file.errorString();
            return;
        }

        QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setCodec("UTF-8");
#endif
        out << _debugInfoTextEdit->toPlainText();
        file.flush();
        file.close();
    }
}

void DebugSettingsWidget::on_copyDebugInfoButton_clicked() {
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(_debugInfoTextEdit->toPlainText());

    Utils::Gui::information(this, tr("Debug information"), tr("The debug information was copied to the clipboard."),
                            QStringLiteral("debug-clipboard"));
}

QVariant DebugSettingsWidget::settingValue(const QString& key, const QVariant& defaultValue) const {
    return _settingsViewModel == nullptr ? defaultValue : _settingsViewModel->persistentSetting(key, defaultValue);
}

void DebugSettingsWidget::setSettingValue(const QString& key, const QVariant& value) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->setPersistentSetting(key, value);
    }
}
