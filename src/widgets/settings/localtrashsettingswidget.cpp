#include "localtrashsettingswidget.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QVariant>

#include "viewmodels/settingsviewmodel.h"

namespace {
constexpr int NoTrashMode = 0;
constexpr int SystemTrashMode = 1;
constexpr int LocalTrashMode = 2;

QGroupBox* groupBox(const QString& title, QWidget* parent) {
    auto* box = new QGroupBox(title, parent);
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(10, 8, 10, 10);
    return box;
}
}  // namespace

LocalTrashSettingsWidget::LocalTrashSettingsWidget(QWidget* parent, SettingsViewModel* settingsViewModel)
    : QWidget(parent), _settingsViewModel(settingsViewModel) {
    buildUi();

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    _legacyTrashSupportGroupBox->setVisible(false);
#else
    _trashModeGroupBox->setVisible(false);
#endif

    updateTrashSettingsState();
}

LocalTrashSettingsWidget::~LocalTrashSettingsWidget() = default;

void LocalTrashSettingsWidget::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    _trashModeGroupBox = groupBox(tr("Trash mode"), this);
    auto* modeLayout = qobject_cast<QVBoxLayout*>(_trashModeGroupBox->layout());
    _noTrashRadioButton = new QRadioButton(tr("No trash"), _trashModeGroupBox);
    _systemTrashRadioButton = new QRadioButton(tr("Use system trash"), _trashModeGroupBox);
    _localTrashRadioButton = new QRadioButton(tr("Use local trash"), _trashModeGroupBox);
    modeLayout->addWidget(_noTrashRadioButton);
    modeLayout->addWidget(_systemTrashRadioButton);
    modeLayout->addWidget(_localTrashRadioButton);
    rootLayout->addWidget(_trashModeGroupBox);

    _legacyTrashSupportGroupBox = groupBox(tr("Legacy local trash"), this);
    _localTrashEnabledCheckBox = new QCheckBox(tr("Enable local trash support"), _legacyTrashSupportGroupBox);
    qobject_cast<QVBoxLayout*>(_legacyTrashSupportGroupBox->layout())->addWidget(_localTrashEnabledCheckBox);
    rootLayout->addWidget(_legacyTrashSupportGroupBox);

    _localTrashGroupBox = groupBox(tr("Local trash"), this);
    auto* localTrashLayout = qobject_cast<QVBoxLayout*>(_localTrashGroupBox->layout());
    _localTrashClearCheckBox =
        new QCheckBox(tr("Automatically remove old items from local trash"), _localTrashGroupBox);
    localTrashLayout->addWidget(_localTrashClearCheckBox);
    _localTrashClearFrame = new QFrame(_localTrashGroupBox);
    auto* clearLayout = new QFormLayout(_localTrashClearFrame);
    clearLayout->setContentsMargins(24, 0, 0, 0);
    _localTrashClearTimeSpinBox = new QSpinBox(_localTrashClearFrame);
    _localTrashClearTimeSpinBox->setRange(1, 3650);
    _localTrashClearTimeSpinBox->setSuffix(tr(" days"));
    clearLayout->addRow(tr("Remove items older than:"), _localTrashClearTimeSpinBox);
    localTrashLayout->addWidget(_localTrashClearFrame);
    rootLayout->addWidget(_localTrashGroupBox);
    rootLayout->addStretch();

    connect(_localTrashEnabledCheckBox, &QCheckBox::toggled, this,
            &LocalTrashSettingsWidget::on_localTrashEnabledCheckBox_toggled);
    connect(_localTrashClearCheckBox, &QCheckBox::toggled, this,
            &LocalTrashSettingsWidget::on_localTrashClearCheckBox_toggled);
    connect(_noTrashRadioButton, &QRadioButton::toggled, this,
            &LocalTrashSettingsWidget::on_noTrashRadioButton_toggled);
    connect(_systemTrashRadioButton, &QRadioButton::toggled, this,
            &LocalTrashSettingsWidget::on_systemTrashRadioButton_toggled);
    connect(_localTrashRadioButton, &QRadioButton::toggled, this,
            &LocalTrashSettingsWidget::on_localTrashRadioButton_toggled);
}

void LocalTrashSettingsWidget::readSettings() {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    switch (trashModeFromSettings()) {
        case NoTrashMode:
            _noTrashRadioButton->setChecked(true);
            break;
        case SystemTrashMode:
            _systemTrashRadioButton->setChecked(true);
            break;
        case LocalTrashMode:
            _localTrashRadioButton->setChecked(true);
            break;
        default:
            _localTrashRadioButton->setChecked(true);
            break;
    }
#else
    _localTrashEnabledCheckBox->setChecked(settingValue(QStringLiteral("localTrash/supportEnabled"), true).toBool());
#endif

    _localTrashClearCheckBox->setChecked(settingValue(QStringLiteral("localTrash/autoCleanupEnabled"), true).toBool());
    _localTrashClearTimeSpinBox->setValue(settingValue(QStringLiteral("localTrash/autoCleanupDays"), 30).toInt());

    updateTrashSettingsState();
}

void LocalTrashSettingsWidget::storeSettings() {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    int mode = NoTrashMode;

    if (_systemTrashRadioButton->isChecked()) {
        mode = SystemTrashMode;
    } else if (_localTrashRadioButton->isChecked()) {
        mode = LocalTrashMode;
    }

    setSettingValue(QStringLiteral("localTrash/mode"), mode);
    setSettingValue(QStringLiteral("localTrash/supportEnabled"), mode == LocalTrashMode);
#else
    setSettingValue(QStringLiteral("localTrash/supportEnabled"), _localTrashEnabledCheckBox->isChecked());
#endif

    setSettingValue(QStringLiteral("localTrash/autoCleanupEnabled"), _localTrashClearCheckBox->isChecked());
    setSettingValue(QStringLiteral("localTrash/autoCleanupDays"), _localTrashClearTimeSpinBox->value());
}

void LocalTrashSettingsWidget::on_localTrashEnabledCheckBox_toggled(bool checked) {
    Q_UNUSED(checked)
    updateTrashSettingsState();
}

void LocalTrashSettingsWidget::on_localTrashClearCheckBox_toggled(bool checked) {
    Q_UNUSED(checked)
    updateTrashSettingsState();
}

void LocalTrashSettingsWidget::on_noTrashRadioButton_toggled(bool checked) {
    Q_UNUSED(checked)
    updateTrashSettingsState();
}

void LocalTrashSettingsWidget::on_systemTrashRadioButton_toggled(bool checked) {
    Q_UNUSED(checked)
    updateTrashSettingsState();
}

void LocalTrashSettingsWidget::on_localTrashRadioButton_toggled(bool checked) {
    Q_UNUSED(checked)
    updateTrashSettingsState();
}

void LocalTrashSettingsWidget::updateTrashSettingsState() {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    const bool localTrashEnabled = _localTrashRadioButton->isChecked();
#else
    const bool localTrashEnabled = _localTrashEnabledCheckBox->isChecked();
#endif

    _localTrashGroupBox->setEnabled(localTrashEnabled);
    _localTrashClearCheckBox->setEnabled(localTrashEnabled);
    _localTrashClearFrame->setEnabled(localTrashEnabled && _localTrashClearCheckBox->isChecked());
}

int LocalTrashSettingsWidget::trashModeFromSettings() const {
    const QVariant mode = settingValue(QStringLiteral("localTrash/mode"));
    if (mode.isValid()) {
        switch (mode.toInt()) {
            case NoTrashMode:
            case SystemTrashMode:
            case LocalTrashMode:
                return mode.toInt();
            default:
                break;
        }
    }

    return settingValue(QStringLiteral("localTrash/supportEnabled"), true).toBool() ? LocalTrashMode : NoTrashMode;
}

QVariant LocalTrashSettingsWidget::settingValue(const QString& key, const QVariant& defaultValue) const {
    return _settingsViewModel == nullptr ? defaultValue : _settingsViewModel->persistentSetting(key, defaultValue);
}

void LocalTrashSettingsWidget::setSettingValue(const QString& key, const QVariant& value) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->setPersistentSetting(key, value);
    }
}
