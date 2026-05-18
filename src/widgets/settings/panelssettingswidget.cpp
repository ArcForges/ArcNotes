#include "panelssettingswidget.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QVariant>

#include "viewmodels/notefoldersettingsviewmodel.h"
#include "viewmodels/settingsviewmodel.h"

namespace {
constexpr int SortAlphabetical = 0;
constexpr int SortByLastChange = 1;
constexpr int OrderAscending = 0;
constexpr int OrderDescending = 1;

QGroupBox* groupBox(const QString& title, QWidget* parent) {
    auto* box = new QGroupBox(title, parent);
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(6);
    return box;
}

QGroupBox* orderGroupBox(const QString& title, QRadioButton** descending, QRadioButton** ascending, QWidget* parent) {
    auto* box = groupBox(title, parent);
    auto* layout = qobject_cast<QVBoxLayout*>(box->layout());
    *descending = new QRadioButton(QObject::tr("Descending"), box);
    *ascending = new QRadioButton(QObject::tr("Ascending"), box);
    layout->addWidget(*descending);
    layout->addWidget(*ascending);
    return box;
}
}  // namespace

PanelsSettingsWidget::PanelsSettingsWidget(QWidget* parent, SettingsViewModel* settingsViewModel,
                                           NoteFolderSettingsViewModel* noteFolderSettingsViewModel)
    : QWidget(parent),
      _settingsViewModel(settingsViewModel),
      _noteFolderSettingsViewModel(noteFolderSettingsViewModel) {
    buildUi();
}

PanelsSettingsWidget::~PanelsSettingsWidget() = default;

void PanelsSettingsWidget::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* notesGroup = groupBox(tr("Notes panel"), this);
    auto* notesLayout = qobject_cast<QVBoxLayout*>(notesGroup->layout());
    _notesPanelSortAlphabeticalRadioButton = new QRadioButton(tr("Sort notes alphabetically"), notesGroup);
    _notesPanelSortByLastChangeRadioButton = new QRadioButton(tr("Sort notes by last change"), notesGroup);
    notesLayout->addWidget(_notesPanelSortAlphabeticalRadioButton);
    notesLayout->addWidget(_notesPanelSortByLastChangeRadioButton);
    _notesPanelOrderGroupBox = orderGroupBox(tr("Order"), &_notesPanelOrderDescendingRadioButton,
                                             &_notesPanelOrderAscendingRadioButton, notesGroup);
    notesLayout->addWidget(_notesPanelOrderGroupBox);
    _noteListPreviewCheckBox = new QCheckBox(tr("Show note preview"), notesGroup);
    _allowEmptyNotesCheckBox = new QCheckBox(tr("Allow empty notes"), notesGroup);
    _maxNoteFileSizeSpinBox = new QSpinBox(notesGroup);
    _maxNoteFileSizeSpinBox->setRange(1, 1024 * 1024);
    _maxNoteFileSizeSpinBox->setSuffix(tr(" KiB"));
    auto* maxFileLayout = new QFormLayout();
    maxFileLayout->addRow(tr("Maximum note file size:"), _maxNoteFileSizeSpinBox);
    notesLayout->addWidget(_noteListPreviewCheckBox);
    notesLayout->addWidget(_allowEmptyNotesCheckBox);
    notesLayout->addLayout(maxFileLayout);
    rootLayout->addWidget(notesGroup);

    auto* subfoldersGroup = groupBox(tr("Note subfolders panel"), this);
    auto* subfoldersLayout = qobject_cast<QVBoxLayout*>(subfoldersGroup->layout());
    _noteSubfoldersPanelHideSearchCheckBox = new QCheckBox(tr("Hide search"), subfoldersGroup);
    _noteSubfoldersPanelDisplayAsFullTreeCheckBox = new QCheckBox(tr("Display as full tree"), subfoldersGroup);
    _noteSubfoldersPanelShowNotesRecursivelyCheckBox = new QCheckBox(tr("Show notes recursively"), subfoldersGroup);
    _disableSavedSearchesAutoCompletionCheckBox =
        new QCheckBox(tr("Disable saved searches auto completion"), subfoldersGroup);
    _showMatchesCheckBox = new QCheckBox(tr("Show matches"), subfoldersGroup);
    _noteSearchPanelOpenCreatedNotesInNewTabCheckBox =
        new QCheckBox(tr("Open created notes in new tab"), subfoldersGroup);
    _noteSubfoldersPanelShowRootFolderNameCheckBox = new QCheckBox(tr("Show root folder name"), subfoldersGroup);
    _noteSubfoldersPanelShowFullPathCheckBox = new QCheckBox(tr("Show full path"), subfoldersGroup);
    _noteSubfoldersPanelTabsUnsetAllNotesSelectionCheckBox =
        new QCheckBox(tr("Unset All notes selection when using tabs"), subfoldersGroup);
    _noteSubfoldersPanelSortAlphabeticalRadioButton =
        new QRadioButton(tr("Sort subfolders alphabetically"), subfoldersGroup);
    _noteSubfoldersPanelSortByLastChangeRadioButton =
        new QRadioButton(tr("Sort subfolders by last change"), subfoldersGroup);
    subfoldersLayout->addWidget(_noteSubfoldersPanelHideSearchCheckBox);
    subfoldersLayout->addWidget(_noteSubfoldersPanelDisplayAsFullTreeCheckBox);
    subfoldersLayout->addWidget(_noteSubfoldersPanelShowNotesRecursivelyCheckBox);
    subfoldersLayout->addWidget(_disableSavedSearchesAutoCompletionCheckBox);
    subfoldersLayout->addWidget(_showMatchesCheckBox);
    subfoldersLayout->addWidget(_noteSearchPanelOpenCreatedNotesInNewTabCheckBox);
    subfoldersLayout->addWidget(_noteSubfoldersPanelShowRootFolderNameCheckBox);
    subfoldersLayout->addWidget(_noteSubfoldersPanelShowFullPathCheckBox);
    subfoldersLayout->addWidget(_noteSubfoldersPanelTabsUnsetAllNotesSelectionCheckBox);
    subfoldersLayout->addWidget(_noteSubfoldersPanelSortAlphabeticalRadioButton);
    subfoldersLayout->addWidget(_noteSubfoldersPanelSortByLastChangeRadioButton);
    _noteSubfoldersPanelOrderGroupBox = orderGroupBox(tr("Order"), &_noteSubfoldersPanelOrderDescendingRadioButton,
                                                      &_noteSubfoldersPanelOrderAscendingRadioButton, subfoldersGroup);
    subfoldersLayout->addWidget(_noteSubfoldersPanelOrderGroupBox);

    auto* ignoredSubfolderLayout = new QHBoxLayout();
    _ignoreNoteSubFoldersLineEdit = new QLineEdit(subfoldersGroup);
    auto* ignoreNoteSubFoldersResetButton = new QPushButton(tr("Reset"), subfoldersGroup);
    ignoredSubfolderLayout->addWidget(_ignoreNoteSubFoldersLineEdit);
    ignoredSubfolderLayout->addWidget(ignoreNoteSubFoldersResetButton);
    auto* ignoredFilesLayout = new QFormLayout();
    _ignoredNoteFilesLineEdit = new QLineEdit(subfoldersGroup);
    ignoredFilesLayout->addRow(tr("Ignored note subfolders:"), ignoredSubfolderLayout);
    ignoredFilesLayout->addRow(tr("Ignored note files:"), _ignoredNoteFilesLineEdit);
    subfoldersLayout->addLayout(ignoredFilesLayout);
    rootLayout->addWidget(subfoldersGroup);

    auto* tagsGroup = groupBox(tr("Tags panel"), this);
    auto* tagsLayout = qobject_cast<QVBoxLayout*>(tagsGroup->layout());
    _tagsPanelHideSearchCheckBox = new QCheckBox(tr("Hide search"), tagsGroup);
    _tagsPanelHideNoteCountCheckBox = new QCheckBox(tr("Hide note count"), tagsGroup);
    _taggingShowNotesRecursivelyCheckBox = new QCheckBox(tr("Show notes recursively"), tagsGroup);
    _tagsPanelSortAlphabeticalRadioButton = new QRadioButton(tr("Sort tags alphabetically"), tagsGroup);
    _tagsPanelSortByLastChangeRadioButton = new QRadioButton(tr("Sort tags by last change"), tagsGroup);
    tagsLayout->addWidget(_tagsPanelHideSearchCheckBox);
    tagsLayout->addWidget(_tagsPanelHideNoteCountCheckBox);
    tagsLayout->addWidget(_taggingShowNotesRecursivelyCheckBox);
    tagsLayout->addWidget(_tagsPanelSortAlphabeticalRadioButton);
    tagsLayout->addWidget(_tagsPanelSortByLastChangeRadioButton);
    _tagsPanelOrderGroupBox = orderGroupBox(tr("Order"), &_tagsPanelOrderDescendingRadioButton,
                                            &_tagsPanelOrderAscendingRadioButton, tagsGroup);
    tagsLayout->addWidget(_tagsPanelOrderGroupBox);
    rootLayout->addWidget(tagsGroup);

    auto* navigationGroup = groupBox(tr("Navigation panel"), this);
    auto* navigationLayout = qobject_cast<QVBoxLayout*>(navigationGroup->layout());
    _navigationPanelHideSearchCheckBox = new QCheckBox(tr("Hide search"), navigationGroup);
    _navigationPanelAutoSelectCheckBox = new QCheckBox(tr("Automatically select the current note"), navigationGroup);
    navigationLayout->addWidget(_navigationPanelHideSearchCheckBox);
    navigationLayout->addWidget(_navigationPanelAutoSelectCheckBox);
    rootLayout->addWidget(navigationGroup);

    auto* noteFolderPanelGroup = groupBox(tr("Note folder panel"), this);
    auto* noteFolderPanelLayout = qobject_cast<QVBoxLayout*>(noteFolderPanelGroup->layout());
    _enableNoteTreeCheckBox = new QCheckBox(tr("Enable note tree"), noteFolderPanelGroup);
    _noteEditCentralWidgetCheckBox = new QCheckBox(tr("Show note edit as central widget"), noteFolderPanelGroup);
    _restoreNoteTabsCheckBox = new QCheckBox(tr("Restore note tabs"), noteFolderPanelGroup);
    _hideTabCloseButtonCheckBox = new QCheckBox(tr("Hide close buttons on note tabs"), noteFolderPanelGroup);
    _noteFolderButtonsCheckBox = new QCheckBox(tr("Use note folder buttons"), noteFolderPanelGroup);
    noteFolderPanelLayout->addWidget(_enableNoteTreeCheckBox);
    noteFolderPanelLayout->addWidget(_noteEditCentralWidgetCheckBox);
    noteFolderPanelLayout->addWidget(_restoreNoteTabsCheckBox);
    noteFolderPanelLayout->addWidget(_hideTabCloseButtonCheckBox);
    noteFolderPanelLayout->addWidget(_noteFolderButtonsCheckBox);
    rootLayout->addWidget(noteFolderPanelGroup);
    rootLayout->addStretch();

    connect(ignoreNoteSubFoldersResetButton, &QPushButton::clicked, this,
            &PanelsSettingsWidget::on_ignoreNoteSubFoldersResetButton_clicked);
}

void PanelsSettingsWidget::initialize() {
    connect(_notesPanelSortAlphabeticalRadioButton, &QRadioButton::toggled, _notesPanelOrderGroupBox,
            &QGroupBox::setEnabled);
    connect(_noteSubfoldersPanelShowRootFolderNameCheckBox, &QCheckBox::toggled,
            _noteSubfoldersPanelShowFullPathCheckBox, &QCheckBox::setEnabled);
    connect(_noteSubfoldersPanelSortAlphabeticalRadioButton, &QRadioButton::toggled, _noteSubfoldersPanelOrderGroupBox,
            &QGroupBox::setEnabled);
    connect(_tagsPanelSortAlphabeticalRadioButton, &QRadioButton::toggled, _tagsPanelOrderGroupBox,
            &QGroupBox::setEnabled);

    if (!_noteListPreviewCheckBox->text().contains(QLatin1String("(experimental)"))) {
        _noteListPreviewCheckBox->setText(_noteListPreviewCheckBox->text() + QStringLiteral(" (experimental)"));
    }

    connect(_noteFolderButtonsCheckBox, &QCheckBox::toggled, this, &PanelsSettingsWidget::needRestart);
    connect(_noteListPreviewCheckBox, &QCheckBox::toggled, this, &PanelsSettingsWidget::needRestart);
    connect(_maxNoteFileSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &PanelsSettingsWidget::needRestart);
    connect(_enableNoteTreeCheckBox, &QCheckBox::toggled, this, &PanelsSettingsWidget::needRestart);
    connect(_ignoreNoteSubFoldersLineEdit, &QLineEdit::textChanged, this, &PanelsSettingsWidget::needRestart);
}

void PanelsSettingsWidget::readSettings() {
    if (settingValue(QStringLiteral("notesPanelSort"), SortByLastChange).toInt() == SortAlphabetical) {
        _notesPanelSortAlphabeticalRadioButton->setChecked(true);
        _notesPanelOrderGroupBox->setEnabled(true);
    } else {
        _notesPanelSortByLastChangeRadioButton->setChecked(true);
        _notesPanelOrderGroupBox->setEnabled(false);
    }
    settingValue(QStringLiteral("notesPanelOrder")).toInt() == OrderDescending
        ? _notesPanelOrderDescendingRadioButton->setChecked(true)
        : _notesPanelOrderAscendingRadioButton->setChecked(true);

    _noteSubfoldersPanelHideSearchCheckBox->setChecked(
        settingValue(QStringLiteral("noteSubfoldersPanelHideSearch")).toBool());
    _noteSubfoldersPanelDisplayAsFullTreeCheckBox->setChecked(
        settingValue(QStringLiteral("noteSubfoldersPanelDisplayAsFullTree"), true).toBool());
    _noteSubfoldersPanelShowNotesRecursivelyCheckBox->setChecked(
        settingValue(QStringLiteral("noteSubfoldersPanelShowNotesRecursively")).toBool());
    _disableSavedSearchesAutoCompletionCheckBox->setChecked(
        settingValue(QStringLiteral("disableSavedSearchesAutoCompletion")).toBool());
    _showMatchesCheckBox->setChecked(settingValue(QStringLiteral("showMatches"), true).toBool());
    _noteSearchPanelOpenCreatedNotesInNewTabCheckBox->setChecked(
        settingValue(QStringLiteral("noteSearchPanelOpenCreatedNotesInNewTab")).toBool());

    if (settingValue(QStringLiteral("noteSubfoldersPanelShowRootFolderName"), true).toBool()) {
        _noteSubfoldersPanelShowRootFolderNameCheckBox->setChecked(true);
        _noteSubfoldersPanelShowFullPathCheckBox->setEnabled(true);
    } else {
        _noteSubfoldersPanelShowRootFolderNameCheckBox->setChecked(false);
        _noteSubfoldersPanelShowFullPathCheckBox->setEnabled(false);
    }

    _noteSubfoldersPanelTabsUnsetAllNotesSelectionCheckBox->setChecked(
        settingValue(QStringLiteral("noteSubfoldersPanelTabsUnsetAllNotesSelection")).toBool());
    _noteSubfoldersPanelShowFullPathCheckBox->setChecked(
        settingValue(QStringLiteral("noteSubfoldersPanelShowFullPath")).toBool());

    if (settingValue(QStringLiteral("noteSubfoldersPanelSort")).toInt() == SortAlphabetical) {
        _noteSubfoldersPanelSortAlphabeticalRadioButton->setChecked(true);
        _noteSubfoldersPanelOrderGroupBox->setEnabled(true);
    } else {
        _noteSubfoldersPanelSortByLastChangeRadioButton->setChecked(true);
        _noteSubfoldersPanelOrderGroupBox->setEnabled(false);
    }

    settingValue(QStringLiteral("noteSubfoldersPanelOrder")).toInt() == OrderDescending
        ? _noteSubfoldersPanelOrderDescendingRadioButton->setChecked(true)
        : _noteSubfoldersPanelOrderAscendingRadioButton->setChecked(true);

    _tagsPanelHideSearchCheckBox->setChecked(settingValue(QStringLiteral("tagsPanelHideSearch")).toBool());
    _tagsPanelHideNoteCountCheckBox->setChecked(settingValue(QStringLiteral("tagsPanelHideNoteCount"), false).toBool());
    _taggingShowNotesRecursivelyCheckBox->setChecked(
        settingValue(QStringLiteral("taggingShowNotesRecursively")).toBool());
    _noteListPreviewCheckBox->setChecked(settingValue(QStringLiteral("noteListPreview")).toBool());
    _allowEmptyNotesCheckBox->setChecked(settingValue(QStringLiteral("allowEmptyNotes"), true).toBool());
    _maxNoteFileSizeSpinBox->setValue(settingValue(QStringLiteral("maxNoteFileSize"), 1048576).toInt() / 1024);

    if (settingValue(QStringLiteral("tagsPanelSort")).toInt() == SortAlphabetical) {
        _tagsPanelSortAlphabeticalRadioButton->setChecked(true);
        _tagsPanelOrderGroupBox->setEnabled(true);
    } else {
        _tagsPanelSortByLastChangeRadioButton->setChecked(true);
        _tagsPanelOrderGroupBox->setEnabled(false);
    }

    settingValue(QStringLiteral("tagsPanelOrder")).toInt() == OrderDescending
        ? _tagsPanelOrderDescendingRadioButton->setChecked(true)
        : _tagsPanelOrderAscendingRadioButton->setChecked(true);

    _ignoreNoteSubFoldersLineEdit->setText(
        settingValue(QStringLiteral("ignoreNoteSubFolders"), defaultIgnoredSubfoldersPattern()).toString());
    _ignoredNoteFilesLineEdit->setText(settingValue(QStringLiteral("ignoredNoteFiles")).toString());

    _navigationPanelHideSearchCheckBox->setChecked(settingValue(QStringLiteral("navigationPanelHideSearch")).toBool());
    _navigationPanelAutoSelectCheckBox->setChecked(
        settingValue(QStringLiteral("navigationPanelAutoSelect"), true).toBool());
    _enableNoteTreeCheckBox->setChecked(settingValue(QStringLiteral("enableNoteTree")).toBool());

    _noteEditCentralWidgetCheckBox->setChecked(settingValue(QStringLiteral("noteEditIsCentralWidget"), true).toBool());
    _restoreNoteTabsCheckBox->setChecked(settingValue(QStringLiteral("restoreNoteTabs"), true).toBool());
    _hideTabCloseButtonCheckBox->setChecked(settingValue(QStringLiteral("hideTabCloseButton")).toBool());
    _noteFolderButtonsCheckBox->setChecked(settingValue(QStringLiteral("useNoteFolderButtons")).toBool());
}

void PanelsSettingsWidget::storeSettings() {
    _notesPanelSortAlphabeticalRadioButton->isChecked()
        ? setSettingValue(QStringLiteral("notesPanelSort"), SortAlphabetical)
        : setSettingValue(QStringLiteral("notesPanelSort"), SortByLastChange);
    _notesPanelOrderDescendingRadioButton->isChecked()
        ? setSettingValue(QStringLiteral("notesPanelOrder"), OrderDescending)
        : setSettingValue(QStringLiteral("notesPanelOrder"), OrderAscending);

    setSettingValue(QStringLiteral("noteSubfoldersPanelHideSearch"),
                    _noteSubfoldersPanelHideSearchCheckBox->isChecked());
    setSettingValue(QStringLiteral("noteSubfoldersPanelDisplayAsFullTree"),
                    _noteSubfoldersPanelDisplayAsFullTreeCheckBox->isChecked());
    setSettingValue(QStringLiteral("noteSubfoldersPanelShowRootFolderName"),
                    _noteSubfoldersPanelShowRootFolderNameCheckBox->isChecked());
    setSettingValue(QStringLiteral("noteSubfoldersPanelShowNotesRecursively"),
                    _noteSubfoldersPanelShowNotesRecursivelyCheckBox->isChecked());
    setSettingValue(QStringLiteral("disableSavedSearchesAutoCompletion"),
                    _disableSavedSearchesAutoCompletionCheckBox->isChecked());
    setSettingValue(QStringLiteral("showMatches"), _showMatchesCheckBox->isChecked());
    setSettingValue(QStringLiteral("noteSearchPanelOpenCreatedNotesInNewTab"),
                    _noteSearchPanelOpenCreatedNotesInNewTabCheckBox->isChecked());
    setSettingValue(QStringLiteral("noteSubfoldersPanelShowFullPath"),
                    _noteSubfoldersPanelShowFullPathCheckBox->isChecked());
    setSettingValue(QStringLiteral("noteSubfoldersPanelTabsUnsetAllNotesSelection"),
                    _noteSubfoldersPanelTabsUnsetAllNotesSelectionCheckBox->isChecked());

    _noteSubfoldersPanelSortAlphabeticalRadioButton->isChecked()
        ? setSettingValue(QStringLiteral("noteSubfoldersPanelSort"), SortAlphabetical)
        : setSettingValue(QStringLiteral("noteSubfoldersPanelSort"), SortByLastChange);
    _noteSubfoldersPanelOrderDescendingRadioButton->isChecked()
        ? setSettingValue(QStringLiteral("noteSubfoldersPanelOrder"), OrderDescending)
        : setSettingValue(QStringLiteral("noteSubfoldersPanelOrder"), OrderAscending);

    const QSignalBlocker blocker(_ignoreNoteSubFoldersLineEdit);
    setSettingValue(QStringLiteral("ignoreNoteSubFolders"), _ignoreNoteSubFoldersLineEdit->text());

    const QSignalBlocker blocker2(_ignoredNoteFilesLineEdit);
    setSettingValue(QStringLiteral("ignoredNoteFiles"), _ignoredNoteFilesLineEdit->text());

    setSettingValue(QStringLiteral("tagsPanelHideSearch"), _tagsPanelHideSearchCheckBox->isChecked());
    setSettingValue(QStringLiteral("tagsPanelHideNoteCount"), _tagsPanelHideNoteCountCheckBox->isChecked());
    setSettingValue(QStringLiteral("taggingShowNotesRecursively"), _taggingShowNotesRecursivelyCheckBox->isChecked());
    setSettingValue(QStringLiteral("noteListPreview"), _noteListPreviewCheckBox->isChecked());
    setSettingValue(QStringLiteral("allowEmptyNotes"), _allowEmptyNotesCheckBox->isChecked());
    setSettingValue(QStringLiteral("maxNoteFileSize"), _maxNoteFileSizeSpinBox->value() * 1024);

    _tagsPanelSortAlphabeticalRadioButton->isChecked()
        ? setSettingValue(QStringLiteral("tagsPanelSort"), SortAlphabetical)
        : setSettingValue(QStringLiteral("tagsPanelSort"), SortByLastChange);
    _tagsPanelOrderDescendingRadioButton->isChecked()
        ? setSettingValue(QStringLiteral("tagsPanelOrder"), OrderDescending)
        : setSettingValue(QStringLiteral("tagsPanelOrder"), OrderAscending);

    setSettingValue(QStringLiteral("navigationPanelHideSearch"), _navigationPanelHideSearchCheckBox->isChecked());
    setSettingValue(QStringLiteral("navigationPanelAutoSelect"), _navigationPanelAutoSelectCheckBox->isChecked());
    setSettingValue(QStringLiteral("enableNoteTree"), _enableNoteTreeCheckBox->isChecked());
    setSettingValue(QStringLiteral("noteEditIsCentralWidget"), _noteEditCentralWidgetCheckBox->isChecked());
    setSettingValue(QStringLiteral("restoreNoteTabs"), _restoreNoteTabsCheckBox->isChecked());
    setSettingValue(QStringLiteral("hideTabCloseButton"), _hideTabCloseButtonCheckBox->isChecked());
    setSettingValue(QStringLiteral("useNoteFolderButtons"), _noteFolderButtonsCheckBox->isChecked());
}

void PanelsSettingsWidget::on_ignoreNoteSubFoldersResetButton_clicked() {
    _ignoreNoteSubFoldersLineEdit->setText(defaultIgnoredSubfoldersPattern());
}

QString PanelsSettingsWidget::defaultIgnoredSubfoldersPattern() const {
    return _noteFolderSettingsViewModel == nullptr ? QString()
                                                   : _noteFolderSettingsViewModel->defaultIgnoredSubfoldersPattern();
}

QVariant PanelsSettingsWidget::settingValue(const QString& key, const QVariant& defaultValue) const {
    return _settingsViewModel == nullptr ? defaultValue : _settingsViewModel->persistentSetting(key, defaultValue);
}

void PanelsSettingsWidget::setSettingValue(const QString& key, const QVariant& value) {
    if (_settingsViewModel != nullptr) {
        _settingsViewModel->setPersistentSetting(key, value);
    }
}
