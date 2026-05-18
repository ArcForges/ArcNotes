#include "debugoptionsettingswidget.h"

#include <utils/gui.h>
#include <utils/misc.h>
#include <viewmodels/settingsviewmodel.h>

#include <QCheckBox>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>

DebugOptionSettingsWidget::DebugOptionSettingsWidget(QWidget* parent, SettingsViewModel* settingsViewModel)
    : QWidget(parent), _settingsViewModel(settingsViewModel) {
    buildUi();
}

DebugOptionSettingsWidget::~DebugOptionSettingsWidget() = default;

void DebugOptionSettingsWidget::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* loggingGroup = new QGroupBox(tr("Logging"), this);
    auto* loggingLayout = new QVBoxLayout(loggingGroup);
    _fileLoggingCheckBox = new QCheckBox(tr("Enable file logging"), loggingGroup);
    loggingLayout->addWidget(_fileLoggingCheckBox);

    _logFileFrame = new QFrame(loggingGroup);
    auto* logFileLayout = new QVBoxLayout(_logFileFrame);
    _logFileLabel = new QLabel(_logFileFrame);
    _logFileLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    _logFileLabel->setWordWrap(true);
    auto* clearLogFileButton = new QPushButton(tr("Clear log file"), _logFileFrame);
    logFileLayout->addWidget(_logFileLabel);
    logFileLayout->addWidget(clearLogFileButton);
    loggingLayout->addWidget(_logFileFrame);

    rootLayout->addWidget(loggingGroup);
    rootLayout->addStretch();

    connect(_fileLoggingCheckBox, &QCheckBox::toggled, this,
            &DebugOptionSettingsWidget::on_fileLoggingCheckBox_toggled);
    connect(clearLogFileButton, &QPushButton::clicked, this, &DebugOptionSettingsWidget::on_clearLogFileButton_clicked);
}

void DebugOptionSettingsWidget::initialize() {
    _logFileLabel->setText(QDir::toNativeSeparators(Utils::Misc::logFilePath()));
}

void DebugOptionSettingsWidget::readSettings() {
    _fileLoggingCheckBox->setChecked(settingValue(QStringLiteral("Debug/fileLogging")).toBool());
    on_fileLoggingCheckBox_toggled(_fileLoggingCheckBox->isChecked());
}

void DebugOptionSettingsWidget::storeSettings() {
    setSettingValue(QStringLiteral("Debug/fileLogging"), _fileLoggingCheckBox->isChecked());
}

void DebugOptionSettingsWidget::removeLogFile() {
    QFile file(Utils::Misc::logFilePath());
    if (file.exists()) {
        const bool result = file.remove();
        const QString text = result ? QStringLiteral("Removed") : QStringLiteral("Could not remove");

        qWarning() << text + QStringLiteral(" log file: ") << file.fileName();
    }
}

void DebugOptionSettingsWidget::on_fileLoggingCheckBox_toggled(bool checked) {
    _logFileFrame->setVisible(checked);
}

void DebugOptionSettingsWidget::on_clearLogFileButton_clicked() {
    removeLogFile();

    Utils::Gui::information(this, tr("Log file cleared"),
                            tr("The log file <strong>%1</strong> was cleared.").arg(Utils::Misc::logFilePath()),
                            QStringLiteral("log-file-cleared"));
}

QVariant DebugOptionSettingsWidget::settingValue(const QString& key, const QVariant& defaultValue) const {
    return _settingsViewModel == nullptr ? defaultValue : _settingsViewModel->persistentSetting(key, defaultValue);
}

void DebugOptionSettingsWidget::setSettingValue(const QString& key, const QVariant& value) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->setPersistentSetting(key, value);
    }
}
