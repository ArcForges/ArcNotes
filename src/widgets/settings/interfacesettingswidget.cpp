#include "interfacesettingswidget.h"

#include <utils/gui.h>
#include <viewmodels/settingsviewmodel.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStyleFactory>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QVariant>

namespace {
QGroupBox* groupBox(const QString& title, QWidget* parent) {
    auto* box = new QGroupBox(title, parent);
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(6);
    return box;
}

int defaultTreeItemHeight(QWidget* parent) {
    QTreeWidget treeWidget(parent);
    auto* treeWidgetItem = new QTreeWidgetItem();
    treeWidget.addTopLevelItem(treeWidgetItem);
    return treeWidget.visualItemRect(treeWidgetItem).height();
}

bool defaultHideIconsInMenus() {
#ifdef Q_OS_MAC
    return true;
#else
    return false;
#endif
}
}  // namespace

InterfaceSettingsWidget::InterfaceSettingsWidget(QWidget* parent, SettingsViewModel* settingsViewModel)
    : QWidget(parent), _settingsViewModel(settingsViewModel) {
    buildUi();
}

InterfaceSettingsWidget::~InterfaceSettingsWidget() = default;

void InterfaceSettingsWidget::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* languageGroup = groupBox(tr("Interface language"), this);
    auto* languageLayout = qobject_cast<QVBoxLayout*>(languageGroup->layout());
    auto* helpTranslateLabel =
        new QLabel(tr("If you want to help to translate ArcNotes or update an existing translation please "
                      "read: <a href=\"%1\">How can I help to translate ArcNotes?</a>")
                       .arg(QStringLiteral("https://www.arcnotes.org/contributing/translation.html")),
                   languageGroup);
    helpTranslateLabel->setOpenExternalLinks(true);
    helpTranslateLabel->setWordWrap(true);
    auto* restartLabel =
        new QLabel(tr("If you change the language you have to restart the application for the changes to "
                      "take action."),
                   languageGroup);
    restartLabel->setWordWrap(true);
    _languageSearchLineEdit = new QLineEdit(languageGroup);
    _languageSearchLineEdit->setPlaceholderText(tr("Search language"));
    _languageListWidget = new QListWidget(languageGroup);
    _languageListWidget->setMinimumHeight(180);
    languageLayout->addWidget(helpTranslateLabel);
    languageLayout->addWidget(restartLabel);
    languageLayout->addWidget(_languageSearchLineEdit);
    languageLayout->addWidget(_languageListWidget);
    rootLayout->addWidget(languageGroup);

    auto* systemTrayGroup = groupBox(tr("System tray"), this);
    auto* systemTrayLayout = qobject_cast<QVBoxLayout*>(systemTrayGroup->layout());
    _showSystemTrayCheckBox = new QCheckBox(tr("Show system tray icon"), systemTrayGroup);
    _showSystemTrayCheckBox->setToolTip(tr("You need to restart the application to let this setting take effect"));
    _startHiddenCheckBox = new QCheckBox(tr("Start application hidden"), systemTrayGroup);
    _startHiddenCheckBox->setToolTip(tr("You need to restart the application to let this setting take effect"));
    systemTrayLayout->addWidget(_showSystemTrayCheckBox);
    systemTrayLayout->addWidget(_startHiddenCheckBox);
    rootLayout->addWidget(systemTrayGroup);

    auto* styleGroup = groupBox(tr("Interface style"), this);
    _interfaceStyleComboBox = new QComboBox(styleGroup);
    qobject_cast<QVBoxLayout*>(styleGroup->layout())->addWidget(_interfaceStyleComboBox);
    rootLayout->addWidget(styleGroup);

    auto* iconsGroup = groupBox(tr("Icons"), this);
    _hideIconsInMenusCheckBox = new QCheckBox(tr("Hide menu icons"), iconsGroup);
    _hideIconsInMenusCheckBox->setToolTip(
        tr("Hide icons in menus as well as the \"Find action\" dialog. Requires restart to "
           "take effect"));
    qobject_cast<QVBoxLayout*>(iconsGroup->layout())->addWidget(_hideIconsInMenusCheckBox);
    rootLayout->addWidget(iconsGroup);

    auto* itemSizesGroup = groupBox(tr("Item sizes in the main window"), this);
    auto* itemSizesLayout = new QFormLayout();
    _itemHeightSpinBox = new QSpinBox(itemSizesGroup);
    _itemHeightSpinBox->setRange(0, 200);
    _itemHeightSpinBox->setSuffix(tr(" px"));
    auto* itemHeightResetButton = new QPushButton(tr("Reset"), itemSizesGroup);
    auto* itemHeightLayout = new QHBoxLayout();
    itemHeightLayout->addWidget(_itemHeightSpinBox);
    itemHeightLayout->addWidget(itemHeightResetButton);
    itemHeightLayout->addStretch();
    itemSizesLayout->addRow(tr("List and tree item height:"), itemHeightLayout);

    _toolbarIconSizeSpinBox = new QSpinBox(itemSizesGroup);
    _toolbarIconSizeSpinBox->setRange(0, 200);
    _toolbarIconSizeSpinBox->setSuffix(tr(" px"));
    auto* toolbarIconSizeResetButton = new QPushButton(tr("Reset"), itemSizesGroup);
    auto* toolbarIconLayout = new QHBoxLayout();
    toolbarIconLayout->addWidget(_toolbarIconSizeSpinBox);
    toolbarIconLayout->addWidget(toolbarIconSizeResetButton);
    toolbarIconLayout->addStretch();
    itemSizesLayout->addRow(tr("Toolbar icon size:"), toolbarIconLayout);
    qobject_cast<QVBoxLayout*>(itemSizesGroup->layout())->addLayout(itemSizesLayout);
    rootLayout->addWidget(itemSizesGroup);

    _overrideInterfaceFontSizeGroupBox = groupBox(tr("Override interface font size"), this);
    _overrideInterfaceFontSizeGroupBox->setCheckable(true);
    auto* fontSizeForm = new QFormLayout();
    _interfaceFontSizeSpinBox = new QSpinBox(_overrideInterfaceFontSizeGroupBox);
    _interfaceFontSizeSpinBox->setRange(1, 200);
    _interfaceFontSizeSpinBox->setSuffix(tr(" pt"));
    fontSizeForm->addRow(tr("Interface font size:"), _interfaceFontSizeSpinBox);
    qobject_cast<QVBoxLayout*>(_overrideInterfaceFontSizeGroupBox->layout())->addLayout(fontSizeForm);
    rootLayout->addWidget(_overrideInterfaceFontSizeGroupBox);

    _overrideInterfaceScalingFactorGroupBox = groupBox(tr("Override interface scaling factor"), this);
    _overrideInterfaceScalingFactorGroupBox->setCheckable(true);
    auto* scalingForm = new QFormLayout();
    _interfaceScalingFactorSpinBox = new QSpinBox(_overrideInterfaceScalingFactorGroupBox);
    _interfaceScalingFactorSpinBox->setRange(10, 500);
    _interfaceScalingFactorSpinBox->setSuffix(tr(" %"));
    scalingForm->addRow(tr("Interface scaling factor:"), _interfaceScalingFactorSpinBox);
    qobject_cast<QVBoxLayout*>(_overrideInterfaceScalingFactorGroupBox->layout())->addLayout(scalingForm);
    rootLayout->addWidget(_overrideInterfaceScalingFactorGroupBox);

    auto* statusBarGroup = groupBox(tr("Status bar"), this);
    auto* statusBarLayout = qobject_cast<QVBoxLayout*>(statusBarGroup->layout());
    _showStatusBarNotePathCheckBox = new QCheckBox(tr("Show note path"), statusBarGroup);
    _showStatusBarRelativeNotePathCheckBox = new QCheckBox(tr("Only show relative note path"), statusBarGroup);
    statusBarLayout->addWidget(_showStatusBarNotePathCheckBox);
    statusBarLayout->addWidget(_showStatusBarRelativeNotePathCheckBox);
    rootLayout->addWidget(statusBarGroup);

    auto* distractionFreeModeGroup = groupBox(tr("Distraction free mode"), this);
    auto* dfmLayout = qobject_cast<QVBoxLayout*>(distractionFreeModeGroup->layout());
    _openDistractionFreeModeInFullScreenCheckBox =
        new QCheckBox(tr("Open distraction free mode in full-screen"), distractionFreeModeGroup);
    _hideStatusBarInDistractionFreeModeCheckBox =
        new QCheckBox(tr("Hide status bar in distraction free mode"), distractionFreeModeGroup);
    dfmLayout->addWidget(_openDistractionFreeModeInFullScreenCheckBox);
    dfmLayout->addWidget(_hideStatusBarInDistractionFreeModeCheckBox);
    rootLayout->addWidget(distractionFreeModeGroup);
    rootLayout->addStretch();

    populateLanguageList();

    connect(_languageSearchLineEdit, &QLineEdit::textChanged, this,
            &InterfaceSettingsWidget::on_languageSearchLineEdit_textChanged);
    connect(_interfaceStyleComboBox, &QComboBox::currentTextChanged, this,
            &InterfaceSettingsWidget::on_interfaceStyleComboBox_currentTextChanged);
    connect(_showSystemTrayCheckBox, &QCheckBox::toggled, this,
            &InterfaceSettingsWidget::on_showSystemTrayCheckBox_toggled);
    connect(_interfaceFontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &InterfaceSettingsWidget::on_interfaceFontSizeSpinBox_valueChanged);
    connect(_overrideInterfaceFontSizeGroupBox, &QGroupBox::toggled, this,
            &InterfaceSettingsWidget::on_overrideInterfaceFontSizeGroupBox_toggled);
    connect(_showStatusBarNotePathCheckBox, &QCheckBox::toggled, this,
            &InterfaceSettingsWidget::on_showStatusBarNotePathCheckBox_toggled);
    connect(_overrideInterfaceScalingFactorGroupBox, &QGroupBox::toggled, this,
            &InterfaceSettingsWidget::on_overrideInterfaceScalingFactorGroupBox_toggled);
    connect(_interfaceScalingFactorSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &InterfaceSettingsWidget::on_interfaceScalingFactorSpinBox_valueChanged);
    connect(itemHeightResetButton, &QPushButton::clicked, this,
            &InterfaceSettingsWidget::on_itemHeightResetButton_clicked);
    connect(toolbarIconSizeResetButton, &QPushButton::clicked, this,
            &InterfaceSettingsWidget::on_toolbarIconSizeResetButton_clicked);
}

void InterfaceSettingsWidget::populateLanguageList() {
    struct LanguageItem {
        const char* name;
        const char* locale;
        const char* toolTip;
    };
    static const LanguageItem languages[] = {
        {"Automatic", "", "Automatic"},
        {"English", "en", "English"},
        {"English (British)", "en_GB", "English (British)"},
        {"Deutsch", "de", "German"},
        {"Español", "es", "Spanish"},
        {"Français", "fr", "French"},
        {"Italiano", "it", "Italian"},
        {"Nederlands", "nl", "Dutch"},
        {"Polski", "pl", "Polish"},
        {"Português", "pt", "Portuguese"},
        {"Português do Brasil", "pt_BR", "Portuguese Brazil"},
        {"Русский", "ru", "Russian"},
        {"Українська мова", "uk", "Ukrainian"},
        {"简化字", "zh_CN", "Chinese Simplified"},
        {"正體字", "zh_TW", "Chinese Traditional"},
        {"日本語", "ja", "Japanese"},
        {"한국어", "ko", "Korean"},
        {"Türkçe", "tr", "Turkish"},
        {"العَرَبِيَّة", "ar", "Arabic"},
    };

    for (const LanguageItem& language : languages) {
        auto* item = new QListWidgetItem(QString::fromUtf8(language.name));
        item->setWhatsThis(QString::fromLatin1(language.locale));
        item->setToolTip(QString::fromUtf8(language.toolTip));
        _languageListWidget->addItem(item);
    }
}

void InterfaceSettingsWidget::initialize() {
#ifdef Q_OS_MAC
    _showSystemTrayCheckBox->setText(tr("Show menu bar item"));
#endif

    connect(_languageListWidget, &QListWidget::itemSelectionChanged, this, &InterfaceSettingsWidget::needRestart);
    connect(_hideIconsInMenusCheckBox, &QCheckBox::toggled, this, &InterfaceSettingsWidget::needRestart);
    connect(_showSystemTrayCheckBox, &QCheckBox::toggled, this, &InterfaceSettingsWidget::needRestart);
    connect(_startHiddenCheckBox, &QCheckBox::toggled, this, &InterfaceSettingsWidget::needRestart);
}

void InterfaceSettingsWidget::readSettings() {
    {
        const QSignalBlocker b1(_overrideInterfaceFontSizeGroupBox);
        const QSignalBlocker b2(_interfaceFontSizeSpinBox);
        Q_UNUSED(b1)
        Q_UNUSED(b2)
        _overrideInterfaceFontSizeGroupBox->setChecked(
            settingValue(QStringLiteral("overrideInterfaceFontSize"), false).toBool());
        _interfaceFontSizeSpinBox->setValue(settingValue(QStringLiteral("interfaceFontSize"), 11).toInt());
    }

    {
        const QSignalBlocker b3(_overrideInterfaceScalingFactorGroupBox);
        const QSignalBlocker b4(_interfaceScalingFactorSpinBox);
        Q_UNUSED(b3)
        Q_UNUSED(b4)
        _overrideInterfaceScalingFactorGroupBox->setChecked(
            settingValue(QStringLiteral("overrideInterfaceScalingFactor"), false).toBool());
        _interfaceScalingFactorSpinBox->setValue(settingValue(QStringLiteral("interfaceScalingFactor"), 100).toInt());
    }

    _toolbarIconSizeSpinBox->setValue(settingValue(QStringLiteral("MainWindow/mainToolBar.iconSize")).toInt());
    _itemHeightSpinBox->setValue(settingValue(QStringLiteral("itemHeight"), defaultTreeItemHeight(this)).toInt());

    const QString savedLanguage = settingValue(QStringLiteral("interfaceLanguage")).toString();
    for (int i = 0; i < _languageListWidget->count(); ++i) {
        QListWidgetItem* item = _languageListWidget->item(i);
        if (item->whatsThis() == savedLanguage) {
            _languageListWidget->setCurrentItem(item);
            break;
        }
    }

    _hideIconsInMenusCheckBox->setChecked(
        settingValue(QStringLiteral("hideIconsInMenus"), defaultHideIconsInMenus()).toBool());
    _showStatusBarNotePathCheckBox->setChecked(settingValue(QStringLiteral("showStatusBarNotePath"), true).toBool());
    _showStatusBarRelativeNotePathCheckBox->setChecked(
        settingValue(QStringLiteral("showStatusBarRelativeNotePath")).toBool());
    _showStatusBarRelativeNotePathCheckBox->setEnabled(_showStatusBarNotePathCheckBox->isChecked());
    _hideStatusBarInDistractionFreeModeCheckBox->setChecked(
        settingValue(QStringLiteral("DistractionFreeMode/hideStatusBar")).toBool());
    _openDistractionFreeModeInFullScreenCheckBox->setChecked(
        settingValue(QStringLiteral("DistractionFreeMode/openInFullScreen"), true).toBool());

    {
        const QSignalBlocker b5(_showSystemTrayCheckBox);
        Q_UNUSED(b5)
        const bool showSystemTray = settingValue(QStringLiteral("ShowSystemTray")).toBool();
        _showSystemTrayCheckBox->setChecked(showSystemTray);
        _startHiddenCheckBox->setEnabled(showSystemTray);
        _startHiddenCheckBox->setChecked(settingValue(QStringLiteral("StartHidden")).toBool());
        if (!showSystemTray) {
            _startHiddenCheckBox->setChecked(false);
        }
    }

    loadInterfaceStyleComboBox();
}

void InterfaceSettingsWidget::storeSettings() {
    setSettingValue(QStringLiteral("overrideInterfaceFontSize"), _overrideInterfaceFontSizeGroupBox->isChecked());
    setSettingValue(QStringLiteral("interfaceFontSize"), _interfaceFontSizeSpinBox->value());
    setSettingValue(QStringLiteral("overrideInterfaceScalingFactor"),
                    _overrideInterfaceScalingFactorGroupBox->isChecked());
    setSettingValue(QStringLiteral("interfaceScalingFactor"), _interfaceScalingFactorSpinBox->value());
    setSettingValue(QStringLiteral("itemHeight"), _itemHeightSpinBox->value());
    setSettingValue(QStringLiteral("MainWindow/mainToolBar.iconSize"), _toolbarIconSizeSpinBox->value());

    QString interfaceLanguage;
    QListWidgetItem* currentItem = _languageListWidget->currentItem();
    if (currentItem != nullptr) {
        interfaceLanguage = currentItem->whatsThis();
    }
    setSettingValue(QStringLiteral("interfaceLanguage"), interfaceLanguage);

    setSettingValue(QStringLiteral("hideIconsInMenus"), _hideIconsInMenusCheckBox->isChecked());
    setSettingValue(QStringLiteral("showStatusBarNotePath"), _showStatusBarNotePathCheckBox->isChecked());
    setSettingValue(QStringLiteral("showStatusBarRelativeNotePath"),
                    _showStatusBarRelativeNotePathCheckBox->isChecked());
    setSettingValue(QStringLiteral("DistractionFreeMode/hideStatusBar"),
                    _hideStatusBarInDistractionFreeModeCheckBox->isChecked());
    setSettingValue(QStringLiteral("DistractionFreeMode/openInFullScreen"),
                    _openDistractionFreeModeInFullScreenCheckBox->isChecked());

    if (_interfaceStyleComboBox->currentIndex() > 0) {
        setSettingValue(QStringLiteral("interfaceStyle"), _interfaceStyleComboBox->currentText());
    } else {
        removeSettingValue(QStringLiteral("interfaceStyle"));
    }

    setSettingValue(QStringLiteral("ShowSystemTray"), _showSystemTrayCheckBox->isChecked());
    setSettingValue(QStringLiteral("StartHidden"), _startHiddenCheckBox->isChecked());
}

void InterfaceSettingsWidget::updateSearchIcons() {
    const bool darkMode = settingValue(QStringLiteral("darkMode")).toBool();

    const QString searchIconFileName =
        darkMode ? QStringLiteral("search-notes-dark.svg") : QStringLiteral("search-notes.svg");
    static const QRegularExpression searchIconRegex(QStringLiteral("background-image: url\\(:.+\\);"));
    const QString searchIconStyle = QStringLiteral("background-image: url(:/images/%1);").arg(searchIconFileName);

    QString styleSheet = _languageSearchLineEdit->styleSheet();
    styleSheet.replace(searchIconRegex, searchIconStyle);
    _languageSearchLineEdit->setStyleSheet(styleSheet);
}

void InterfaceSettingsWidget::loadInterfaceStyleComboBox() {
    const QSignalBlocker blocker(_interfaceStyleComboBox);
    Q_UNUSED(blocker)

    _interfaceStyleComboBox->clear();
    _interfaceStyleComboBox->addItem(tr("Automatic (needs restart)"));

    for (const QString& style : QStyleFactory::keys()) {
        _interfaceStyleComboBox->addItem(style);
    }

    const QString interfaceStyle = settingValue(QStringLiteral("interfaceStyle")).toString();

    if (!interfaceStyle.isEmpty()) {
        _interfaceStyleComboBox->setCurrentText(interfaceStyle);
    } else {
        _interfaceStyleComboBox->setCurrentIndex(0);
    }

    Utils::Gui::applyInterfaceStyle();
}

void InterfaceSettingsWidget::on_interfaceStyleComboBox_currentTextChanged(const QString& arg1) {
    Utils::Gui::applyInterfaceStyle(arg1);

    if (_interfaceStyleComboBox->currentIndex() == 0) {
        emit needRestart();
    }
}

void InterfaceSettingsWidget::on_showSystemTrayCheckBox_toggled(bool checked) {
#ifndef Q_OS_MAC
    if (checked) {
        emit systemTrayToggled(true);
    }
#endif

    _startHiddenCheckBox->setEnabled(checked);

    if (!checked) {
        _startHiddenCheckBox->setChecked(false);
    }
}

void InterfaceSettingsWidget::on_interfaceFontSizeSpinBox_valueChanged(int arg1) {
    setSettingValue(QStringLiteral("interfaceFontSize"), arg1);
    Utils::Gui::updateInterfaceFontSize(arg1);
}

void InterfaceSettingsWidget::on_overrideInterfaceFontSizeGroupBox_toggled(bool arg1) {
    setSettingValue(QStringLiteral("overrideInterfaceFontSize"), arg1);
    Utils::Gui::updateInterfaceFontSize();
}

void InterfaceSettingsWidget::on_languageSearchLineEdit_textChanged(const QString& arg1) {
    Utils::Gui::searchForTextInListWidget(_languageListWidget, arg1, true);
}

void InterfaceSettingsWidget::on_showStatusBarNotePathCheckBox_toggled(bool checked) {
    _showStatusBarRelativeNotePathCheckBox->setEnabled(checked);
}

void InterfaceSettingsWidget::on_overrideInterfaceScalingFactorGroupBox_toggled(bool arg1) {
    if (!arg1) {
        Utils::Gui::information(this, tr("Override interface scaling factor"),
                                tr("If you had this setting enabled, you now need to restart the application manually "
                                   "so the previous environment variable that overrides the scale factor is not in "
                                   "your environment again."),
                                QStringLiteral("settings-override-interface-scale-factor-off"));
    } else {
        emit needRestart();
    }
}

void InterfaceSettingsWidget::on_interfaceScalingFactorSpinBox_valueChanged(int arg1) {
    Q_UNUSED(arg1);
    emit needRestart();
}

void InterfaceSettingsWidget::on_itemHeightResetButton_clicked() {
    _itemHeightSpinBox->setValue(defaultTreeItemHeight(this));
}

void InterfaceSettingsWidget::on_toolbarIconSizeResetButton_clicked() {
    QToolBar toolbar(this);
    _toolbarIconSizeSpinBox->setValue(toolbar.iconSize().height());
}

QVariant InterfaceSettingsWidget::settingValue(const QString& key, const QVariant& defaultValue) const {
    return _settingsViewModel == nullptr ? defaultValue : _settingsViewModel->persistentSetting(key, defaultValue);
}

void InterfaceSettingsWidget::setSettingValue(const QString& key, const QVariant& value) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->setPersistentSetting(key, value);
    }
}

void InterfaceSettingsWidget::removeSettingValue(const QString& key) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->removePersistentSetting(key);
    }
}
