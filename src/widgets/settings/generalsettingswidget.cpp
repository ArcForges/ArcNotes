#include "generalsettingswidget.h"

#include <utils/gui.h>
#include <utils/misc.h>
#include <viewmodels/settingsviewmodel.h>

#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>

#include "dialogs/filedialog.h"
#include "release.h"
#include "widgets/settings/debugoptionsettingswidget.h"

namespace {
QGroupBox* createGroup(const QString& title, QWidget* parent) {
    auto* box = new QGroupBox(title, parent);
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(6);
    return box;
}

QPushButton* iconButton(const QString& toolTip, const QString& iconName, QWidget* parent) {
    auto* button = new QPushButton(parent);
    button->setToolTip(toolTip);
    button->setIcon(
        QIcon::fromTheme(iconName, QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/%1.svg").arg(iconName))));
    button->setFixedWidth(32);
    return button;
}
}  // namespace

GeneralSettingsWidget::GeneralSettingsWidget(QWidget* parent, SettingsViewModel* settingsViewModel)
    : QWidget(parent), _settingsViewModel(settingsViewModel) {
    buildUi();
}

GeneralSettingsWidget::~GeneralSettingsWidget() = default;

void GeneralSettingsWidget::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* notesGroup = createGroup(tr("Notes"), this);
    auto* notesLayout = qobject_cast<QVBoxLayout*>(notesGroup->layout());

    _acceptAllExternalModificationsCheckBox =
        new QCheckBox(tr("Accept all external modifications of the current note"), notesGroup);
    _acceptAllExternalModificationsCheckBox->setToolTip(
        tr("Check this if you want to accept all external modifications while you are editing "
           "the current note."));
    _notifyAllExternalModificationsCheckBox =
        new QCheckBox(tr("Notify about all external modifications of the current note"), notesGroup);
    _notifyAllExternalModificationsCheckBox->setToolTip(
        tr("If checked you will always be notified about external modifications, even if the "
           "current note was not edited in the last minute."));
    _ignoreAllExternalModificationsCheckBox =
        new QCheckBox(tr("Ignore all external modifications of the current note"), notesGroup);
    _ignoreAllExternalModificationsCheckBox->setToolTip(
        tr("Check this if you want to ignore all external modifications while you are editing "
           "the current note."));
    _newNoteAskHeadlineCheckBox =
        new QCheckBox(tr("The shortcut to create a new note asks for a headline"), notesGroup);
    _newNoteAskHeadlineCheckBox->setToolTip(tr("By default the headline will generated automatically"));
    _ignoreAllExternalNoteFolderChangesCheckBox =
        new QCheckBox(tr("Ignore all external note folder changes"), notesGroup);
    _ignoreAllExternalNoteFolderChangesCheckBox->setToolTip(tr("Do this at your own risk!"));
    _enableNoteChecksumChecks =
        new QCheckBox(tr("Use checksums to detect external modifications to unsaved notes"), notesGroup);
    _enableNoteChecksumChecks->setToolTip(tr("Do this at your own risk!"));
    _useUNIXNewlineCheckBox = new QCheckBox(tr("Use UNIX newline instead of native newline characters"), notesGroup);
    _useUNIXNewlineCheckBox->setToolTip(
        tr("You can use this for example under Windows if you have troubles with newlines in "
           "the Nextcloud / ownCloud notes app"));
    _legacyLinkingCheckBox = new QCheckBox(tr("Use legacy way to link to notes, images and attachments"), notesGroup);
    _legacyLinkingCheckBox->setToolTip(QStringLiteral("note://, file://media, file://attachments"));
    _restoreCursorPositionCheckBox = new QCheckBox(tr("Restore cursor position when opening a note"), notesGroup);
    _restoreCursorPositionCheckBox->setToolTip(
        tr("When opening notes the cursor position inside the note will now be restored to the "
           "position when the note was last visited in the current session"));
    _restoreLastNoteAtStartupCheckBox = new QCheckBox(tr("Open last accessed note at application startup"), notesGroup);
    _restoreLastNoteAtStartupCheckBox->setToolTip(
        tr("Disable this if you want to launch the application without opening a note"));
    _automaticNoteFolderDatabaseClosingCheckBox =
        new QCheckBox(tr("Automatically close the note folder database"), notesGroup);
    _automaticNoteFolderDatabaseClosingCheckBox->setToolTip(
        tr("Automatically close the note folder database to prevent problems with sync tools"));
    _imageScaleDownCheckBox = new QCheckBox(tr("Scale images down when inserted into notes"), notesGroup);

    notesLayout->addWidget(_acceptAllExternalModificationsCheckBox);
    notesLayout->addWidget(_notifyAllExternalModificationsCheckBox);
    notesLayout->addWidget(_ignoreAllExternalModificationsCheckBox);
    notesLayout->addWidget(_newNoteAskHeadlineCheckBox);
    notesLayout->addWidget(_ignoreAllExternalNoteFolderChangesCheckBox);
    notesLayout->addWidget(_enableNoteChecksumChecks);
    notesLayout->addWidget(_useUNIXNewlineCheckBox);
    notesLayout->addWidget(_legacyLinkingCheckBox);
    notesLayout->addWidget(_restoreCursorPositionCheckBox);
    notesLayout->addWidget(_restoreLastNoteAtStartupCheckBox);
    notesLayout->addWidget(_automaticNoteFolderDatabaseClosingCheckBox);
    notesLayout->addWidget(_imageScaleDownCheckBox);

    _imageScalingFrame = new QFrame(notesGroup);
    auto* imageLayout = new QFormLayout(_imageScalingFrame);
    imageLayout->setContentsMargins(24, 0, 0, 0);
    _maximumImageWidthSpinBox = new QSpinBox(_imageScalingFrame);
    _maximumImageWidthSpinBox->setRange(1, 99999);
    _maximumImageHeightSpinBox = new QSpinBox(_imageScalingFrame);
    _maximumImageHeightSpinBox->setRange(1, 99999);
    imageLayout->addRow(tr("Maximum width:"), _maximumImageWidthSpinBox);
    imageLayout->addRow(tr("Maximum height:"), _maximumImageHeightSpinBox);
    notesLayout->addWidget(_imageScalingFrame);

    auto* noteSaveLayout = new QHBoxLayout();
    auto* noteSaveLabel = new QLabel(tr("note save interval time [sec]"), notesGroup);
    noteSaveLabel->setToolTip(
        tr("You might run into sync troubles with older versions of "
           "Nextcloud / ownCloud sync when going far below 10 sec."));
    _noteSaveIntervalTime = new QSpinBox(notesGroup);
    _noteSaveIntervalTime->setRange(0, 9999);
    _noteSaveIntervalTime->setToolTip(noteSaveLabel->toolTip());
    noteSaveLayout->addWidget(noteSaveLabel);
    noteSaveLayout->addWidget(_noteSaveIntervalTime);
    noteSaveLayout->addStretch();
    notesLayout->addLayout(noteSaveLayout);
    rootLayout->addWidget(notesGroup);

    auto* readOnlyModeGroup = createGroup(tr("Read-only mode"), this);
    auto* readOnlyModeLayout = qobject_cast<QVBoxLayout*>(readOnlyModeGroup->layout());
    _enableReadOnlyModeCheckBox = new QCheckBox(tr("Enable read-only mode"), readOnlyModeGroup);
    _enableReadOnlyModeCheckBox->setToolTip(tr("Enable or disable the read-only mode feature"));
    readOnlyModeLayout->addWidget(_enableReadOnlyModeCheckBox);
    _readOnlyModeSettingsFrame = new QFrame(readOnlyModeGroup);
    auto* readOnlySettingsLayout = new QHBoxLayout(_readOnlyModeSettingsFrame);
    readOnlySettingsLayout->setContentsMargins(24, 0, 0, 0);
    _startInReadOnlyModeCheckBox = new QCheckBox(tr("Start application in read-only mode"), _readOnlyModeSettingsFrame);
    _autoReadOnlyModeCheckBox =
        new QCheckBox(tr("When inactive, enter read-only mode automatically after:"), _readOnlyModeSettingsFrame);
    _autoReadOnlyModeTimeoutSpinBox = new QSpinBox(_readOnlyModeSettingsFrame);
    _autoReadOnlyModeTimeoutSpinBox->setRange(1, 9999);
    _autoReadOnlyModeTimeoutSpinBox->setSuffix(tr(" sec"));
    readOnlySettingsLayout->addWidget(_startInReadOnlyModeCheckBox);
    readOnlySettingsLayout->addWidget(_autoReadOnlyModeCheckBox);
    readOnlySettingsLayout->addWidget(_autoReadOnlyModeTimeoutSpinBox);
    readOnlySettingsLayout->addStretch();
    readOnlyModeLayout->addWidget(_readOnlyModeSettingsFrame);
    rootLayout->addWidget(readOnlyModeGroup);

    _appInstanceGroupBox = createGroup(tr("App instance"), this);
    auto* appInstanceLayout = qobject_cast<QVBoxLayout*>(_appInstanceGroupBox->layout());
    _allowOnlyOneAppInstanceCheckBox =
        new QCheckBox(tr("Only allow one app instance at the same time"), _appInstanceGroupBox);
    appInstanceLayout->addWidget(_allowOnlyOneAppInstanceCheckBox);
    rootLayout->addWidget(_appInstanceGroupBox);

    auto* searchEngineGroup = createGroup(tr("Search engine"), this);
    _searchEngineSelectionComboBox = new QComboBox(searchEngineGroup);
    _searchEngineSelectionComboBox->setEditable(false);
    qobject_cast<QVBoxLayout*>(searchEngineGroup->layout())->addWidget(_searchEngineSelectionComboBox);
    rootLayout->addWidget(searchEngineGroup);

    auto* messageBoxesGroup = createGroup(tr("Message boxes"), this);
    auto* resetMessageBoxesButton = new QPushButton(tr("Reset message boxes"), messageBoxesGroup);
    resetMessageBoxesButton->setToolTip(tr("Forget all \"Don't ask again\" choices in message boxes"));
    qobject_cast<QVBoxLayout*>(messageBoxesGroup->layout())->addWidget(resetMessageBoxesButton);
    rootLayout->addWidget(messageBoxesGroup);

    auto* extensionsGroup = createGroup(tr("Note file extensions"), this);
    auto* extensionsLayout = qobject_cast<QVBoxLayout*>(extensionsGroup->layout());
    auto* extensionsHelp =
        new QLabel(tr("Files with the listed file extensions will be recognized as notes, the selected file "
                      "extension will be used for new notes."),
                   extensionsGroup);
    extensionsHelp->setWordWrap(true);
    extensionsLayout->addWidget(extensionsHelp);

    auto* extensionsContentLayout = new QHBoxLayout();
    _defaultNoteFileExtensionListWidget = new QListWidget(extensionsGroup);
    _defaultNoteFileExtensionListWidget->setObjectName(QStringLiteral("defaultNoteFileExtensionListWidget"));
    _defaultNoteFileExtensionListWidget->setMinimumHeight(90);
    extensionsContentLayout->addWidget(_defaultNoteFileExtensionListWidget, 1);
    auto* extensionButtonLayout = new QVBoxLayout();
    auto* addCustomNoteFileExtensionButton =
        iconButton(tr("Add a new note file extension"), QStringLiteral("list-add"), extensionsGroup);
    addCustomNoteFileExtensionButton->setObjectName(QStringLiteral("addCustomNoteFileExtensionButton"));
    _removeCustomNoteFileExtensionButton =
        iconButton(tr("Remove selected note file extension"), QStringLiteral("list-remove"), extensionsGroup);
    _removeCustomNoteFileExtensionButton->setObjectName(QStringLiteral("removeCustomNoteFileExtensionButton"));
    extensionButtonLayout->addWidget(addCustomNoteFileExtensionButton);
    extensionButtonLayout->addWidget(_removeCustomNoteFileExtensionButton);
    extensionButtonLayout->addStretch();
    extensionsContentLayout->addLayout(extensionButtonLayout);
    extensionsLayout->addLayout(extensionsContentLayout);
    rootLayout->addWidget(extensionsGroup);

    auto* externalEditorGroup = createGroup(tr("External editor"), this);
    auto* externalEditorLayout = new QFormLayout();
    _externalEditorPathLineEdit = new QLineEdit(externalEditorGroup);
    _externalEditorPathLineEdit->setPlaceholderText(tr("Path to external editor"));
    auto* editorPathFrame = new QFrame(externalEditorGroup);
    auto* editorPathLayout = new QHBoxLayout(editorPathFrame);
    editorPathLayout->setContentsMargins(0, 0, 0, 0);
    auto* setExternalEditorPathToolButton = new QToolButton(editorPathFrame);
    setExternalEditorPathToolButton->setObjectName(QStringLiteral("setExternalEditorPathToolButton"));
    setExternalEditorPathToolButton->setToolTip(tr("set external editor"));
    setExternalEditorPathToolButton->setIcon(QIcon::fromTheme(
        QStringLiteral("document-open"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/document-open.svg"))));
    editorPathLayout->addWidget(_externalEditorPathLineEdit, 1);
    editorPathLayout->addWidget(setExternalEditorPathToolButton);
    externalEditorLayout->addRow(tr("Path to external editor:"), editorPathFrame);
    qobject_cast<QVBoxLayout*>(externalEditorGroup->layout())->addLayout(externalEditorLayout);
    rootLayout->addWidget(externalEditorGroup);

    auto* clearAppDataGroup = createGroup(tr("Clear application data and exit"), this);
    auto* clearAppDataLayout = qobject_cast<QVBoxLayout*>(clearAppDataGroup->layout());
    auto* clearLabel = new QLabel(tr("This button allows you to clear all settings, remove the local todo database and "
                                     "exit ArcNotes."),
                                  clearAppDataGroup);
    clearLabel->setWordWrap(true);
    auto* clearAppDataAndExitButton = new QPushButton(tr("Clear app data and exit"), clearAppDataGroup);
    clearAppDataLayout->addWidget(clearLabel);
    clearAppDataLayout->addWidget(clearAppDataAndExitButton);
    rootLayout->addWidget(clearAppDataGroup);

    auto* settingsGroup = createGroup(tr("Settings"), this);
    auto* settingsLayout = new QHBoxLayout();
    auto* importSettingsButton = new QPushButton(tr("Import settings"), settingsGroup);
    auto* exportSettingsButton = new QPushButton(tr("Export settings"), settingsGroup);
    settingsLayout->addWidget(importSettingsButton);
    settingsLayout->addWidget(exportSettingsButton);
    settingsLayout->addStretch();
    qobject_cast<QVBoxLayout*>(settingsGroup->layout())->addLayout(settingsLayout);
    rootLayout->addWidget(settingsGroup);

    auto* databaseGroup = createGroup(tr("Database"), this);
    auto* databaseLayout = qobject_cast<QVBoxLayout*>(databaseGroup->layout());
    auto* databaseLabel =
        new QLabel(tr("Be warned that reinitializing the database will also remove your cached todo items "
                      "and configured note folders!"),
                   databaseGroup);
    databaseLabel->setWordWrap(true);
    auto* databaseButtonLayout = new QHBoxLayout();
    auto* reinitializeDatabaseButton = new QPushButton(tr("Reinitialize Database"), databaseGroup);
    auto* databaseIntegrityCheckButton = new QPushButton(tr("Check integrity"), databaseGroup);
    databaseIntegrityCheckButton->setToolTip(tr("Checks the disk database integrity and fixes problems"));
    databaseButtonLayout->addWidget(reinitializeDatabaseButton);
    databaseButtonLayout->addWidget(databaseIntegrityCheckButton);
    databaseButtonLayout->addStretch();
    databaseLayout->addWidget(databaseLabel);
    databaseLayout->addLayout(databaseButtonLayout);
    rootLayout->addWidget(databaseGroup);
    rootLayout->addStretch();

    connect(_imageScaleDownCheckBox, &QCheckBox::toggled, this,
            &GeneralSettingsWidget::on_imageScaleDownCheckBox_toggled);
    connect(_enableReadOnlyModeCheckBox, &QCheckBox::toggled, this,
            &GeneralSettingsWidget::on_enableReadOnlyModeCheckBox_toggled);
    connect(reinitializeDatabaseButton, &QPushButton::clicked, this,
            &GeneralSettingsWidget::on_reinitializeDatabaseButton_clicked);
    connect(databaseIntegrityCheckButton, &QPushButton::clicked, this,
            &GeneralSettingsWidget::on_databaseIntegrityCheckButton_clicked);
    connect(clearAppDataAndExitButton, &QPushButton::clicked, this,
            &GeneralSettingsWidget::on_clearAppDataAndExitButton_clicked);
    connect(setExternalEditorPathToolButton, &QToolButton::clicked, this,
            &GeneralSettingsWidget::on_setExternalEditorPathToolButton_clicked);
    connect(addCustomNoteFileExtensionButton, &QPushButton::clicked, this,
            &GeneralSettingsWidget::on_addCustomNoteFileExtensionButton_clicked);
    connect(_removeCustomNoteFileExtensionButton, &QPushButton::clicked, this,
            &GeneralSettingsWidget::on_removeCustomNoteFileExtensionButton_clicked);
    connect(_defaultNoteFileExtensionListWidget, &QListWidget::itemChanged, this,
            &GeneralSettingsWidget::on_defaultNoteFileExtensionListWidget_itemChanged);
    connect(_defaultNoteFileExtensionListWidget, &QListWidget::itemSelectionChanged, this,
            &GeneralSettingsWidget::on_defaultNoteFileExtensionListWidget_itemSelectionChanged);
    connect(resetMessageBoxesButton, &QPushButton::clicked, this,
            &GeneralSettingsWidget::on_resetMessageBoxesButton_clicked);
    connect(importSettingsButton, &QPushButton::clicked, this, &GeneralSettingsWidget::on_importSettingsButton_clicked);
    connect(exportSettingsButton, &QPushButton::clicked, this, &GeneralSettingsWidget::on_exportSettingsButton_clicked);
}

void GeneralSettingsWidget::initialize() {
    _noteSaveIntervalTime->setToolTip(_noteSaveIntervalTime->toolTip());
    _removeCustomNoteFileExtensionButton->setDisabled(true);

#ifndef Q_OS_WIN32
    _automaticNoteFolderDatabaseClosingCheckBox->hide();
#endif

#ifdef Q_OS_MAC
    _appInstanceGroupBox->setVisible(false);
    _allowOnlyOneAppInstanceCheckBox->setChecked(false);
#endif

    _noteNotificationButtonGroup = new QButtonGroup(this);
    _noteNotificationButtonGroup->addButton(_notifyAllExternalModificationsCheckBox);
    _noteNotificationButtonGroup->addButton(_ignoreAllExternalModificationsCheckBox);
    _noteNotificationButtonGroup->addButton(_acceptAllExternalModificationsCheckBox);

    _noteNotificationNoneCheckBox = new QCheckBox(this);
    _noteNotificationNoneCheckBox->setHidden(true);
    _noteNotificationButtonGroup->addButton(_noteNotificationNoneCheckBox);
    connect(_noteNotificationButtonGroup, &QButtonGroup::buttonPressed, this,
            &GeneralSettingsWidget::noteNotificationButtonGroupPressed);

    initSearchEngineComboBox();
}

void GeneralSettingsWidget::readSettings() {
    _externalEditorPathLineEdit->setText(Utils::Misc::prependPortableDataPathIfNeeded(
        settingValue(QStringLiteral("externalEditorPath")).toString(), true));

    _notifyAllExternalModificationsCheckBox->setChecked(
        settingValue(QStringLiteral("notifyAllExternalModifications")).toBool());
    _ignoreAllExternalModificationsCheckBox->setChecked(
        settingValue(QStringLiteral("ignoreAllExternalModifications")).toBool());
    _acceptAllExternalModificationsCheckBox->setChecked(
        settingValue(QStringLiteral("acceptAllExternalModifications")).toBool());
    _ignoreAllExternalNoteFolderChangesCheckBox->setChecked(
        settingValue(QStringLiteral("ignoreAllExternalNoteFolderChanges")).toBool());
    _enableNoteChecksumChecks->setChecked(settingValue(QStringLiteral("enableNoteChecksumChecks"), false).toBool());
    _newNoteAskHeadlineCheckBox->setChecked(settingValue(QStringLiteral("newNoteAskHeadline")).toBool());
    _useUNIXNewlineCheckBox->setChecked(settingValue(QStringLiteral("useUNIXNewline")).toBool());

#ifdef Q_OS_MAC
    const bool restoreCursorPositionDefault = false;
#else
    const bool restoreCursorPositionDefault = true;
#endif
    _restoreCursorPositionCheckBox->setChecked(
        settingValue(QStringLiteral("restoreCursorPosition"), restoreCursorPositionDefault).toBool());
    _restoreLastNoteAtStartupCheckBox->setChecked(
        settingValue(QStringLiteral("restoreLastNoteAtStartup"), true).toBool());
    _noteSaveIntervalTime->setValue(settingValue(QStringLiteral("noteSaveIntervalTime"), 10).toInt());
    _allowOnlyOneAppInstanceCheckBox->setChecked(settingValue(QStringLiteral("allowOnlyOneAppInstance")).toBool());

    const QSignalBlocker blocker(_defaultNoteFileExtensionListWidget);
    Q_UNUSED(blocker)
    _defaultNoteFileExtensionListWidget->clear();
    const QStringList noteFileExtensions = noteFileExtensionList();
    for (const QString& fileExtension : noteFileExtensions) {
        addCustomNoteFileExtension(fileExtension);
    }

    const auto noteFileExtensionItems =
        _defaultNoteFileExtensionListWidget->findItems(defaultNoteFileExtension(), Qt::MatchExactly);
    if (noteFileExtensionItems.count() > 0) {
        _defaultNoteFileExtensionListWidget->setCurrentItem(noteFileExtensionItems.at(0));
    }

    const bool scaleImageDown = settingValue(QStringLiteral("imageScaleDown"), false).toBool();
    _maximumImageHeightSpinBox->setValue(settingValue(QStringLiteral("imageScaleDownMaximumHeight"), 1024).toInt());
    _maximumImageWidthSpinBox->setValue(settingValue(QStringLiteral("imageScaleDownMaximumWidth"), 1024).toInt());
    _imageScaleDownCheckBox->setChecked(scaleImageDown);
    _imageScalingFrame->setVisible(scaleImageDown);

    _automaticNoteFolderDatabaseClosingCheckBox->setChecked(
        settingValue(QStringLiteral("automaticNoteFolderDatabaseClosing")).toBool());
    _legacyLinkingCheckBox->setChecked(settingValue(QStringLiteral("legacyLinking")).toBool());

    const bool enableReadOnlyMode = settingValue(QStringLiteral("enableReadOnlyMode"), true).toBool();
    _enableReadOnlyModeCheckBox->setChecked(enableReadOnlyMode);
    _readOnlyModeSettingsFrame->setEnabled(enableReadOnlyMode);

    _startInReadOnlyModeCheckBox->setChecked(settingValue(QStringLiteral("startInReadOnlyMode")).toBool());
    _autoReadOnlyModeCheckBox->setChecked(settingValue(QStringLiteral("autoReadOnlyMode")).toBool());
    _autoReadOnlyModeTimeoutSpinBox->setValue(settingValue(QStringLiteral("autoReadOnlyModeTimeout"), 30).toInt());

    initSearchEngineComboBox();
}

void GeneralSettingsWidget::storeSettings() {
    setSettingValue(QStringLiteral("notifyAllExternalModifications"),
                    _notifyAllExternalModificationsCheckBox->isChecked());
    setSettingValue(QStringLiteral("ignoreAllExternalModifications"),
                    _ignoreAllExternalModificationsCheckBox->isChecked());
    setSettingValue(QStringLiteral("acceptAllExternalModifications"),
                    _acceptAllExternalModificationsCheckBox->isChecked());
    setSettingValue(QStringLiteral("ignoreAllExternalNoteFolderChanges"),
                    _ignoreAllExternalNoteFolderChangesCheckBox->isChecked());
    setSettingValue(QStringLiteral("enableNoteChecksumChecks"), _enableNoteChecksumChecks->isChecked());
    setSettingValue(QStringLiteral("newNoteAskHeadline"), _newNoteAskHeadlineCheckBox->isChecked());
    setSettingValue(QStringLiteral("useUNIXNewline"), _useUNIXNewlineCheckBox->isChecked());
    setSettingValue(QStringLiteral("restoreCursorPosition"), _restoreCursorPositionCheckBox->isChecked());
    setSettingValue(QStringLiteral("restoreLastNoteAtStartup"), _restoreLastNoteAtStartupCheckBox->isChecked());
    setSettingValue(QStringLiteral("noteSaveIntervalTime"), _noteSaveIntervalTime->value());

    if (_defaultNoteFileExtensionListWidget->currentItem() != nullptr) {
        setSettingValue(QStringLiteral("defaultNoteFileExtension"),
                        _defaultNoteFileExtensionListWidget->currentItem()->text());
    }

    QStringList noteFileExtensionList;
    for (int i = 0; i < _defaultNoteFileExtensionListWidget->count(); i++) {
        QListWidgetItem* item = _defaultNoteFileExtensionListWidget->item(i);
        noteFileExtensionList.append(item->text());
    }
    noteFileExtensionList.removeDuplicates();
    setSettingValue(QStringLiteral("noteFileExtensionList"), noteFileExtensionList);

    setSettingValue(QStringLiteral("externalEditorPath"),
                    Utils::Misc::makePathRelativeToPortableDataPathIfNeeded(_externalEditorPathLineEdit->text()));

    setSettingValue(QStringLiteral("allowOnlyOneAppInstance"), _allowOnlyOneAppInstanceCheckBox->isChecked());
    setSettingValue(QStringLiteral("imageScaleDown"), _imageScaleDownCheckBox->isChecked());
    setSettingValue(QStringLiteral("imageScaleDownMaximumHeight"), _maximumImageHeightSpinBox->value());
    setSettingValue(QStringLiteral("imageScaleDownMaximumWidth"), _maximumImageWidthSpinBox->value());
    setSettingValue(QStringLiteral("SearchEngineId"), _searchEngineSelectionComboBox->currentData().toInt());
    setSettingValue(QStringLiteral("automaticNoteFolderDatabaseClosing"),
                    _automaticNoteFolderDatabaseClosingCheckBox->isChecked());
    setSettingValue(QStringLiteral("legacyLinking"), _legacyLinkingCheckBox->isChecked());
    setSettingValue(QStringLiteral("enableReadOnlyMode"), _enableReadOnlyModeCheckBox->isChecked());
    setSettingValue(QStringLiteral("startInReadOnlyMode"), _startInReadOnlyModeCheckBox->isChecked());
    setSettingValue(QStringLiteral("autoReadOnlyMode"), _autoReadOnlyModeCheckBox->isChecked());
    setSettingValue(QStringLiteral("autoReadOnlyModeTimeout"), _autoReadOnlyModeTimeoutSpinBox->value());
}

void GeneralSettingsWidget::setAllowOnlyOneAppInstance(bool checked) {
    _allowOnlyOneAppInstanceCheckBox->setChecked(checked);
}

void GeneralSettingsWidget::initSearchEngineComboBox() {
    const QHash<int, Utils::Misc::SearchEngine> searchEngines = Utils::Misc::getSearchEnginesHashMap();

    _searchEngineSelectionComboBox->clear();
    const QList<int> searchEngineIds = Utils::Misc::getSearchEnginesIds();
    for (int id : searchEngineIds) {
        const Utils::Misc::SearchEngine searchEngine = searchEngines[id];
        _searchEngineSelectionComboBox->addItem(searchEngine.name, id);
    }

    const int savedEngineId =
        settingValue(QStringLiteral("SearchEngineId"), Utils::Misc::getDefaultSearchEngineId()).toInt();
    int savedEngineIndex = _searchEngineSelectionComboBox->findData(savedEngineId);
    savedEngineIndex = savedEngineIndex == -1 ? 0 : savedEngineIndex;
    _searchEngineSelectionComboBox->setCurrentIndex(savedEngineIndex);
}

void GeneralSettingsWidget::on_reinitializeDatabaseButton_clicked() {
    if (QMessageBox::question(this, tr("Database"),
                              tr("Do you really want to clear the local database? "
                                 "This will also remove your configured note "
                                 "folders and your cached todo items!"),
                              QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes) {
        reinitializeDatabase();

        Utils::Gui::information(this, tr("Database"),
                                tr("The Database was reinitialized. Please restart the application now!"),
                                QStringLiteral("database-reinitialized"));
    }
}

void GeneralSettingsWidget::on_databaseIntegrityCheckButton_clicked() {
    if (checkDatabaseIntegrity()) {
        Utils::Gui::information(this, tr("Database"), tr("The integrity of the disk database is valid."),
                                QStringLiteral("database-integrity-check-valid"));
    } else {
        Utils::Gui::warning(this, tr("Database"), tr("The integrity of the disk database is not valid!"),
                            QStringLiteral("database-integrity-check-not-valid"));
    }
}

void GeneralSettingsWidget::on_clearAppDataAndExitButton_clicked() {
    if (QMessageBox::question(this, tr("Clear app data and exit"),
                              tr("Do you really want to clear all settings, remove the "
                                 "database and exit ArcNotes?\n\n"
                                 "Your notes will stay intact!"),
                              QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes) {
        clearSettings();
        removeDiskDatabase();
        DebugOptionSettingsWidget::removeLogFile();
        qApp->setProperty("clearAppDataAndExit", true);
        qApp->quit();
    }
}

void GeneralSettingsWidget::on_setExternalEditorPathToolButton_clicked() {
    QString path = _externalEditorPathLineEdit->text();
    QString dirPath = path;

    if (!path.isEmpty()) {
        dirPath = QFileInfo(path).dir().path();
    }

    if (path.isEmpty() && Utils::Misc::isInPortableMode()) {
        dirPath = Utils::Misc::portableDataPath();
    }

    QStringList mimeTypeFilters;
    mimeTypeFilters << QStringLiteral("application/x-executable") << QStringLiteral("application/octet-stream");

    FileDialog dialog(QStringLiteral("ExternalEditor"));
    if (!dirPath.isEmpty()) {
        dialog.setDirectory(dirPath);
    }
    if (!path.isEmpty()) {
        dialog.selectFile(path);
    }

    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.setWindowTitle(tr("Select editor application"));
    const int ret = dialog.exec();

    if (ret == QDialog::Accepted) {
        const QStringList fileNames = dialog.selectedFiles();
        if (!fileNames.empty()) {
            _externalEditorPathLineEdit->setText(fileNames.at(0));
        }
    }
}

void GeneralSettingsWidget::on_addCustomNoteFileExtensionButton_clicked() {
    bool ok;
    QString fileExtension;
    fileExtension = QInputDialog::getText(this, tr("File extension"), tr("Please enter a new note file extension:"),
                                          QLineEdit::Normal, fileExtension, &ok);

    if (!ok) {
        return;
    }

    fileExtension = Utils::Misc::removeIfStartsWith(std::move(fileExtension), QStringLiteral("."));
    QListWidgetItem* item = addCustomNoteFileExtension(fileExtension);

    if (item != nullptr) {
        _defaultNoteFileExtensionListWidget->setCurrentItem(item);
    }
}

QListWidgetItem* GeneralSettingsWidget::addCustomNoteFileExtension(const QString& fileExtension) {
    const QString trimmed = fileExtension.trimmed();

    if (trimmed.isEmpty() || _defaultNoteFileExtensionListWidget->findItems(trimmed, Qt::MatchExactly).count() > 0) {
        return nullptr;
    }

    auto* item = new QListWidgetItem(trimmed);
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    if (trimmed == QLatin1String("md")) {
        item->setToolTip(tr("Markdown file"));
    } else if (trimmed == QLatin1String("txt")) {
        item->setToolTip(tr("Plain text file"));
    }

    _defaultNoteFileExtensionListWidget->addItem(item);

    return item;
}

void GeneralSettingsWidget::on_removeCustomNoteFileExtensionButton_clicked() {
    if (_defaultNoteFileExtensionListWidget->count() <= 1) {
        return;
    }

    auto* item = _defaultNoteFileExtensionListWidget->currentItem();
    if (item == nullptr) {
        return;
    }

    if (Utils::Gui::question(this, tr("Remove note file extension"),
                             tr("Do you really want to remove the note file extension "
                                "<strong>%1</strong>? You will not see files with this "
                                "extension in the note list any more!")
                                 .arg(item->text()),
                             QStringLiteral("remove-note-file-extension")) != QMessageBox::Yes) {
        return;
    }

    delete item;
    _removeCustomNoteFileExtensionButton->setEnabled(_defaultNoteFileExtensionListWidget->count() > 1);
}

void GeneralSettingsWidget::on_defaultNoteFileExtensionListWidget_itemChanged(QListWidgetItem* item) {
    const QString fileExtension = Utils::Misc::removeIfStartsWith(item->text(), QStringLiteral(".")).trimmed();

    if (fileExtension != item->text()) {
        item->setText(fileExtension);
    }
}

void GeneralSettingsWidget::on_defaultNoteFileExtensionListWidget_itemSelectionChanged() {
    _removeCustomNoteFileExtensionButton->setEnabled(_defaultNoteFileExtensionListWidget->count() > 1);
}

void GeneralSettingsWidget::on_resetMessageBoxesButton_clicked() {
    if (QMessageBox::question(this, tr("Reset message boxes"),
                              tr("Do you really want to reset the overrides of all message "
                                 "boxes?")) == QMessageBox::Yes) {
        removeSettingValue(QStringLiteral("MessageBoxOverride"));
    }
}

void GeneralSettingsWidget::on_exportSettingsButton_clicked() {
    FileDialog dialog(QStringLiteral("SettingsExport"));
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter(tr("INI files") + QStringLiteral(" (*.ini)"));
    dialog.setWindowTitle(tr("Export settings"));
    dialog.selectFile(QStringLiteral("ArcNotes-settings.ini"));
    const int ret = dialog.exec();

    if (ret == QDialog::Accepted) {
        QString fileName = dialog.selectedFile();

        if (!fileName.isEmpty()) {
            if (QFileInfo(fileName).suffix().isEmpty()) {
                fileName.append(QStringLiteral(".ini"));
            }

            QSettings exportSettings(fileName, QSettings::IniFormat);
            exportSettings.clear();
            exportSettings.setValue(QStringLiteral("SettingsExport/platform"), QStringLiteral(PLATFORM));

            const QStringList keys = settingKeys();
            for (const QString& key : keys) {
                exportSettings.setValue(key, settingValue(key));
            }
        }
    }
}

void GeneralSettingsWidget::on_importSettingsButton_clicked() {
    const QString title = tr("Import settings");
    const QString text = tr("Do you really want to import settings? Your current "
                            "settings will get removed and not every setting may "
                            "get restored, like the note folder settings. "
                            "You also will need to adjust some settings, especially "
                            "across platforms, but your notes will stay intact!") +
                         QLatin1String("\n\n") + tr("The application will be restarted after the import.") +
                         Utils::Misc::appendSingleAppInstanceTextIfNeeded();

    if (QMessageBox::question(this, title, text, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ==
        QMessageBox::No) {
        return;
    }

    FileDialog dialog(QStringLiteral("SettingsExport"));
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setNameFilter(tr("INI files") + QStringLiteral(" (*.ini)"));
    dialog.setWindowTitle(tr("Import settings"));
    const int ret = dialog.exec();

    if (ret != QDialog::Accepted) {
        return;
    }

    const QString fileName = dialog.selectedFile();
    QSettings importSettings(fileName, QSettings::IniFormat);
    clearSettings();
    removeDiskDatabase();

    const QStringList keys = importSettings.allKeys();
    for (const QString& key : keys) {
        const QVariant value = importSettings.value(key);
        setSettingValue(key, value);
    }

    qApp->setProperty("clearAppDataAndExit", true);
    Utils::Misc::restartApplication();
}

void GeneralSettingsWidget::on_imageScaleDownCheckBox_toggled(bool checked) {
    _imageScalingFrame->setVisible(checked);
}

void GeneralSettingsWidget::on_enableReadOnlyModeCheckBox_toggled(bool checked) {
    _readOnlyModeSettingsFrame->setEnabled(checked);
}

void GeneralSettingsWidget::noteNotificationButtonGroupPressed(QAbstractButton* button) const {
    if (button->isChecked()) {
        QTimer::singleShot(100, this, &GeneralSettingsWidget::noteNotificationNoneCheckBoxCheck);
    }
}

void GeneralSettingsWidget::noteNotificationNoneCheckBoxCheck() {
    _noteNotificationNoneCheckBox->setChecked(true);
}

QString GeneralSettingsWidget::defaultNoteFileExtension() const {
    return settingValue(QStringLiteral("defaultNoteFileExtension"), QStringLiteral("md")).toString();
}

QStringList GeneralSettingsWidget::noteFileExtensionList() const {
    QStringList list = settingValue(QStringLiteral("noteFileExtensionList")).toStringList();
    list.removeDuplicates();
    if (list.isEmpty()) {
        list << defaultNoteFileExtension();
    }
    return list;
}

QStringList GeneralSettingsWidget::settingKeys() const {
    return _settingsViewModel == nullptr ? QStringList() : _settingsViewModel->persistentSettingKeys();
}

QVariant GeneralSettingsWidget::settingValue(const QString& key, const QVariant& defaultValue) const {
    return _settingsViewModel == nullptr ? defaultValue : _settingsViewModel->persistentSetting(key, defaultValue);
}

void GeneralSettingsWidget::setSettingValue(const QString& key, const QVariant& value) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->setPersistentSetting(key, value);
    }
}

void GeneralSettingsWidget::removeSettingValue(const QString& key) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->removePersistentSetting(key);
    }
}

void GeneralSettingsWidget::clearSettings() {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->clearPersistentSettings();
    }
}

bool GeneralSettingsWidget::reinitializeDatabase() {
    return _settingsViewModel != nullptr && _settingsViewModel->reinitializeDatabase();
}

bool GeneralSettingsWidget::checkDatabaseIntegrity() {
    return _settingsViewModel != nullptr && _settingsViewModel->checkDatabaseIntegrity();
}

void GeneralSettingsWidget::removeDiskDatabase() {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->removeDiskDatabase();
    }
}
