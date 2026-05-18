#include "notefoldersettingswidget.h"

#include <viewmodels/notefoldersettingsviewmodel.h>

#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "dialogs/filedialog.h"
#include "utils/gui.h"
#include "utils/misc.h"
#include "widgets/notefolderlistwidget.h"

NoteFolderSettingsWidget::NoteFolderSettingsWidget(QWidget* parent, NoteFolderSettingsViewModel* viewModel)
    : QWidget(parent), _viewModel(viewModel) {
    buildUi();
}

NoteFolderSettingsWidget::~NoteFolderSettingsWidget() = default;

void NoteFolderSettingsWidget::buildUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* groupBox = new QGroupBox(tr("Your note folders"), this);
    auto* groupLayout = new QHBoxLayout(groupBox);
    rootLayout->addWidget(groupBox);

    auto* selectFrame = new QFrame(groupBox);
    auto* selectLayout = new QVBoxLayout(selectFrame);
    selectLayout->setContentsMargins(0, 0, 0, 0);

    _noteFolderListWidget = new NoteFolderListWidget(selectFrame);
    _noteFolderListWidget->setObjectName(QStringLiteral("noteFolderListWidget"));
    _noteFolderListWidget->setDragDropMode(QAbstractItemView::InternalMove);
    _noteFolderListWidget->setDefaultDropAction(Qt::MoveAction);
    _noteFolderListWidget->setMinimumWidth(220);
    selectLayout->addWidget(_noteFolderListWidget, 1);

    auto* buttonLayout = new QHBoxLayout();
    _noteFolderAddButton = new QPushButton(tr("&Add folder"), selectFrame);
    _noteFolderAddButton->setObjectName(QStringLiteral("noteFolderAddButton"));
    _noteFolderRemoveButton = new QPushButton(tr("&Remove folder"), selectFrame);
    _noteFolderRemoveButton->setObjectName(QStringLiteral("noteFolderRemoveButton"));
    buttonLayout->addWidget(_noteFolderAddButton);
    buttonLayout->addWidget(_noteFolderRemoveButton);
    selectLayout->addLayout(buttonLayout);
    groupLayout->addWidget(selectFrame, 1);

    _noteFolderEditFrame = new QFrame(groupBox);
    _noteFolderEditFrame->setObjectName(QStringLiteral("noteFolderEditFrame"));
    auto* editLayout = new QVBoxLayout(_noteFolderEditFrame);
    editLayout->setContentsMargins(0, 0, 0, 0);

    _noteFolderVerticalSpacerFrame = new QFrame(_noteFolderEditFrame);
    _noteFolderVerticalSpacerFrame->setObjectName(QStringLiteral("noteFolderVerticalSpacerFrame"));
    _noteFolderVerticalSpacerFrame->setFrameShape(QFrame::NoFrame);
    editLayout->addWidget(_noteFolderVerticalSpacerFrame);

    auto* formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    _noteFolderNameLineEdit = new QLineEdit(_noteFolderEditFrame);
    _noteFolderNameLineEdit->setObjectName(QStringLiteral("noteFolderNameLineEdit"));
    _noteFolderNameLineEdit->setToolTip(tr("This name will be viewed in all the menus."));
    _noteFolderNameLineEdit->setPlaceholderText(tr("Note folder name"));
    formLayout->addRow(tr("Name:"), _noteFolderNameLineEdit);

    auto* pathFrame = new QFrame(_noteFolderEditFrame);
    auto* pathLayout = new QHBoxLayout(pathFrame);
    pathLayout->setContentsMargins(0, 0, 0, 0);
    _noteFolderLocalPathLineEdit = new QLineEdit(pathFrame);
    _noteFolderLocalPathLineEdit->setObjectName(QStringLiteral("noteFolderLocalPathLineEdit"));
    _noteFolderLocalPathLineEdit->setReadOnly(true);
    _noteFolderLocalPathLineEdit->setPlaceholderText(tr("Path where your notes are stored locally"));
    _noteFolderLocalPathButton = new QPushButton(pathFrame);
    _noteFolderLocalPathButton->setObjectName(QStringLiteral("noteFolderLocalPathButton"));
    _noteFolderLocalPathButton->setToolTip(tr("Click here to select your local note path"));
    _noteFolderLocalPathButton->setIcon(
        QIcon::fromTheme(QStringLiteral("document-open"), QIcon(QStringLiteral(":/icons/breeze-arcnotes/16x16/"
                                                                               "document-open.svg"))));
    _noteFolderLocalPathButton->setFixedWidth(34);
    pathLayout->addWidget(_noteFolderLocalPathLineEdit, 1);
    pathLayout->addWidget(_noteFolderLocalPathButton);
    formLayout->addRow(tr("Note folder path:"), pathFrame);
    editLayout->addLayout(formLayout);

    _noteFolderActiveCheckBox = new QCheckBox(tr("Use as active note folder"), _noteFolderEditFrame);
    _noteFolderActiveCheckBox->setObjectName(QStringLiteral("noteFolderActiveCheckBox"));
    editLayout->addWidget(_noteFolderActiveCheckBox);

    _noteFolderShowSubfoldersCheckBox = new QCheckBox(tr("Use note subfolders"), _noteFolderEditFrame);
    _noteFolderShowSubfoldersCheckBox->setObjectName(QStringLiteral("noteFolderShowSubfoldersCheckBox"));
    _noteFolderShowSubfoldersCheckBox->setToolTip(tr("Show notes from subfolders in this note folder."));
    editLayout->addWidget(_noteFolderShowSubfoldersCheckBox);

    _allowDifferentNoteFileNameCheckBox =
        new QCheckBox(tr("Allow note file name to be different from headline"), _noteFolderEditFrame);
    _allowDifferentNoteFileNameCheckBox->setObjectName(QStringLiteral("allowDifferentNoteFileNameCheckBox"));
    _allowDifferentNoteFileNameCheckBox->setToolTip(tr("This also allows note files to be renamed"));
    editLayout->addWidget(_allowDifferentNoteFileNameCheckBox);

    _noteFolderSubfolderSettingsFrame = new QGroupBox(tr("Subfolder visibility"), _noteFolderEditFrame);
    _noteFolderSubfolderSettingsFrame->setObjectName(QStringLiteral("noteFolderSubfolderSettingsFrame"));
    auto* subfolderLayout = new QVBoxLayout(_noteFolderSubfolderSettingsFrame);
    _noteFolderAllSubfoldersCheckBox = new QCheckBox(tr("All subfolders"), _noteFolderSubfolderSettingsFrame);
    _noteFolderAllSubfoldersCheckBox->setObjectName(QStringLiteral("noteFolderAllSubfoldersCheckBox"));
    _noteFolderAllSubfoldersCheckBox->setToolTip(
        tr("If checked, all subfolders will be shown. If unchecked, you can select which "
           "subfolders to show."));
    subfolderLayout->addWidget(_noteFolderAllSubfoldersCheckBox);

    _noteFolderSubfolderTreeWidget = new QTreeWidget(_noteFolderSubfolderSettingsFrame);
    _noteFolderSubfolderTreeWidget->setObjectName(QStringLiteral("noteFolderSubfolderTreeWidget"));
    _noteFolderSubfolderTreeWidget->setToolTip(
        tr("Select which subfolders to show. Unchecked subfolders and their children will be "
           "ignored."));
    _noteFolderSubfolderTreeWidget->setHeaderHidden(true);
    _noteFolderSubfolderTreeWidget->setRootIsDecorated(true);
    _noteFolderSubfolderTreeWidget->setColumnCount(1);
    subfolderLayout->addWidget(_noteFolderSubfolderTreeWidget, 1);
    editLayout->addWidget(_noteFolderSubfolderSettingsFrame, 1);
    editLayout->addStretch();

    groupLayout->addWidget(_noteFolderEditFrame, 2);

    connect(_noteFolderListWidget, &QListWidget::currentItemChanged, this,
            &NoteFolderSettingsWidget::on_noteFolderListWidget_currentItemChanged);
    connect(_noteFolderListWidget, &NoteFolderListWidget::folderOrderChanged, this,
            [this](const QVector<int>& folderIds) {
                if (_viewModel != nullptr) {
                    _viewModel->updateFolderPriorities(folderIds);
                }
            });
    connect(_noteFolderAddButton, &QPushButton::clicked, this,
            &NoteFolderSettingsWidget::on_noteFolderAddButton_clicked);
    connect(_noteFolderRemoveButton, &QPushButton::clicked, this,
            &NoteFolderSettingsWidget::on_noteFolderRemoveButton_clicked);
    connect(_noteFolderNameLineEdit, &QLineEdit::editingFinished, this,
            &NoteFolderSettingsWidget::on_noteFolderNameLineEdit_editingFinished);
    connect(_noteFolderLocalPathButton, &QPushButton::clicked, this,
            &NoteFolderSettingsWidget::on_noteFolderLocalPathButton_clicked);
    connect(_noteFolderActiveCheckBox, &QCheckBox::checkStateChanged, this,
            &NoteFolderSettingsWidget::on_noteFolderActiveCheckBox_stateChanged);
    connect(_noteFolderShowSubfoldersCheckBox, &QCheckBox::toggled, this,
            &NoteFolderSettingsWidget::on_noteFolderShowSubfoldersCheckBox_toggled);
    connect(_noteFolderAllSubfoldersCheckBox, &QCheckBox::toggled, this,
            &NoteFolderSettingsWidget::on_noteFolderAllSubfoldersCheckBox_toggled);
    connect(_allowDifferentNoteFileNameCheckBox, &QCheckBox::toggled, this,
            &NoteFolderSettingsWidget::on_allowDifferentNoteFileNameCheckBox_toggled);
}

void NoteFolderSettingsWidget::initialize() {
    _noteFolderListWidget->clear();
    _noteFolderEditFrame->setEnabled(_viewModel != nullptr && _viewModel->noteFolderCount() > 0);
    _noteFolderVerticalSpacerFrame->setVisible(true);

    const QList<NoteFolderData> noteFolders =
        _viewModel == nullptr ? QList<NoteFolderData>() : _viewModel->noteFolders();
    const int noteFoldersCount = noteFolders.count();

    for (const NoteFolderData& noteFolder : noteFolders) {
        auto* item = new QListWidgetItem(noteFolder.name);
        item->setData(Qt::UserRole, noteFolder.id);
        _noteFolderListWidget->addItem(item);

        if (_viewModel != nullptr && noteFolder.id == _viewModel->currentFolderId()) {
            _noteFolderListWidget->setCurrentItem(item);
        }
    }

    _noteFolderRemoveButton->setEnabled(noteFoldersCount > 1);
    _noteFolderLocalPathLineEdit->setPlaceholderText(Utils::Misc::defaultNotesPath());
}

void NoteFolderSettingsWidget::readSettings() {
    QListWidgetItem* noteFolderListItem = Utils::Gui::getListWidgetItemWithUserData(
        _noteFolderListWidget, _viewModel == nullptr ? 0 : _viewModel->currentFolderId());
    if (noteFolderListItem != nullptr) {
        _noteFolderListWidget->setCurrentItem(noteFolderListItem);
    }
}

void NoteFolderSettingsWidget::storeSettings() {}

void NoteFolderSettingsWidget::on_noteFolderListWidget_currentItemChanged(QListWidgetItem* current,
                                                                          QListWidgetItem* previous) {
    Q_UNUSED(previous)

    if (current == nullptr) {
        _selectedNoteFolder = NoteFolderData();
        _noteFolderEditFrame->setEnabled(false);
        return;
    }

    const int noteFolderId = current->data(Qt::UserRole).toInt();
    _selectedNoteFolder = _viewModel == nullptr ? NoteFolderData() : _viewModel->noteFolder(noteFolderId);
    _noteFolderEditFrame->setEnabled(selectedNoteFolderIsFetched());
    if (selectedNoteFolderIsFetched()) {
        _noteFolderNameLineEdit->setText(_selectedNoteFolder.name);
        _noteFolderLocalPathLineEdit->setText(_selectedNoteFolder.localPath);
        _noteFolderShowSubfoldersCheckBox->setChecked(_selectedNoteFolder.showSubfolders);
        _noteFolderAllSubfoldersCheckBox->setChecked(_selectedNoteFolder.allSubfolders);
        _allowDifferentNoteFileNameCheckBox->setChecked(
            (_viewModel == nullptr
                 ? QVariant()
                 : _viewModel->noteFolderSetting(_selectedNoteFolder.id, QStringLiteral("allowDifferentNoteFileName")))
                .toBool());
        const QSignalBlocker blocker(_noteFolderActiveCheckBox);
        Q_UNUSED(blocker)
        _noteFolderActiveCheckBox->setChecked(_selectedNoteFolder.id ==
                                              (_viewModel == nullptr ? 0 : _viewModel->currentFolderId()));
    }

    updateSubfolderVisibility();
}

void NoteFolderSettingsWidget::on_noteFolderAddButton_clicked() {
    const QString currentPath = _selectedNoteFolder.localPath;

    _selectedNoteFolder = NoteFolderData();
    _selectedNoteFolder.name = tr("new folder");
    _selectedNoteFolder.localPath = currentPath;
    _selectedNoteFolder.priority = _noteFolderListWidget->count();
    _selectedNoteFolder = _viewModel == nullptr ? NoteFolderData() : _viewModel->saveNoteFolder(_selectedNoteFolder);

    if (selectedNoteFolderIsFetched()) {
        auto* item = new QListWidgetItem(_selectedNoteFolder.name);
        item->setData(Qt::UserRole, _selectedNoteFolder.id);
        _noteFolderListWidget->addItem(item);
        _noteFolderListWidget->setCurrentRow(_noteFolderListWidget->count() - 1);
        _noteFolderRemoveButton->setEnabled(true);
        _noteFolderNameLineEdit->setFocus();
        _noteFolderNameLineEdit->selectAll();
    }
}

void NoteFolderSettingsWidget::on_noteFolderRemoveButton_clicked() {
    if (_noteFolderListWidget->count() < 2) {
        return;
    }

    if (Utils::Gui::question(this, tr("Remove note folder"),
                             tr("Remove the current note folder <strong>%1</strong>?").arg(_selectedNoteFolder.name),
                             QStringLiteral("remove-note-folder")) == QMessageBox::Yes) {
        QString settingsKey = QStringLiteral("savedSearches/noteFolder-") + QString::number(_selectedNoteFolder.id);
        if (_viewModel != nullptr) {
            _viewModel->removePersistentSetting(settingsKey);
        }

        settingsKey =
            _viewModel == nullptr ? QString() : _viewModel->subFolderTreeExpandStateSettingsKey(_selectedNoteFolder.id);
        if (_viewModel != nullptr) {
            _viewModel->removePersistentSetting(settingsKey);
        }

        if (_viewModel == nullptr || !_viewModel->removeNoteFolder(_selectedNoteFolder.id)) {
            return;
        }

        delete _noteFolderListWidget->takeItem(_noteFolderListWidget->currentRow());
        _noteFolderRemoveButton->setEnabled(_noteFolderListWidget->count() > 1);

        emit storeSettingsRequested();
    }
}

void NoteFolderSettingsWidget::on_noteFolderNameLineEdit_editingFinished() {
    if (!selectedNoteFolderIsFetched()) {
        return;
    }

    QString text = _noteFolderNameLineEdit->text().remove(QStringLiteral("\n")).trimmed();
    text.truncate(50);

    if (text.isEmpty()) {
        const QString localPath = _noteFolderLocalPathLineEdit->text();
        text = QDir(localPath).dirName();
    }

    _selectedNoteFolder.name = text;
    _selectedNoteFolder = _viewModel == nullptr ? NoteFolderData() : _viewModel->saveNoteFolder(_selectedNoteFolder);

    if (_noteFolderListWidget->currentItem() != nullptr) {
        _noteFolderListWidget->currentItem()->setText(text);
    }
}

void NoteFolderSettingsWidget::on_noteFolderLocalPathButton_clicked() {
    const QString dir =
        QFileDialog::getExistingDirectory(this, tr("Please select the folder where your notes will get stored to"),
                                          _selectedNoteFolder.localPath, QFileDialog::ShowDirsOnly);

    const QDir d(dir);
    if (d.exists() && !dir.isEmpty()) {
        _noteFolderLocalPathLineEdit->setText(dir);
        _selectedNoteFolder.localPath = dir;
        _selectedNoteFolder =
            _viewModel == nullptr ? NoteFolderData() : _viewModel->saveNoteFolder(_selectedNoteFolder);
    }
}

void NoteFolderSettingsWidget::on_noteFolderActiveCheckBox_stateChanged(Qt::CheckState arg1) {
    Q_UNUSED(arg1)

    if (!_noteFolderActiveCheckBox->isChecked()) {
        const QSignalBlocker blocker(_noteFolderActiveCheckBox);
        Q_UNUSED(blocker)
        _noteFolderActiveCheckBox->setChecked(true);
    } else if (selectedNoteFolderIsFetched()) {
        if (_viewModel != nullptr) {
            _viewModel->setCurrentFolderId(_selectedNoteFolder.id);
        }
        emit storeSettingsRequested();
    }
}

void NoteFolderSettingsWidget::on_noteFolderShowSubfoldersCheckBox_toggled(bool checked) {
    if (!selectedNoteFolderIsFetched()) {
        return;
    }

    _selectedNoteFolder.showSubfolders = checked;

    if (!checked) {
        _selectedNoteFolder.activeNoteSubFolderData.clear();
    }

    _selectedNoteFolder = _viewModel == nullptr ? NoteFolderData() : _viewModel->saveNoteFolder(_selectedNoteFolder);
    updateSubfolderVisibility();
}

void NoteFolderSettingsWidget::on_noteFolderAllSubfoldersCheckBox_toggled(bool checked) {
    if (!selectedNoteFolderIsFetched()) {
        return;
    }

    _selectedNoteFolder.allSubfolders = checked;
    _selectedNoteFolder = _viewModel == nullptr ? NoteFolderData() : _viewModel->saveNoteFolder(_selectedNoteFolder);
    updateSubfolderVisibility();
}

void NoteFolderSettingsWidget::updateSubfolderVisibility() {
    const bool showSubfolders = _noteFolderShowSubfoldersCheckBox->isChecked();
    const bool allSubfolders = _noteFolderAllSubfoldersCheckBox->isChecked();

    _noteFolderSubfolderSettingsFrame->setVisible(showSubfolders);
    _noteFolderSubfolderTreeWidget->setVisible(showSubfolders && !allSubfolders);

    if (showSubfolders && !allSubfolders) {
        populateSubfolderTree();
    }
}

void NoteFolderSettingsWidget::populateSubfolderTree() {
    _noteFolderSubfolderTreeWidget->clear();

    const QString localPath = _selectedNoteFolder.localPath;
    if (localPath.isEmpty() || !QDir(localPath).exists()) {
        return;
    }

    disconnect(_noteFolderSubfolderTreeWidget, &QTreeWidget::itemChanged, this,
               &NoteFolderSettingsWidget::onSubfolderTreeItemChanged);

    const QStringList excludedPaths = _selectedNoteFolder.excludedSubfolderPaths;
    populateSubfolderTreeFromDir(nullptr, localPath, QString());
    applySubfolderTreeCheckStates(_noteFolderSubfolderTreeWidget, excludedPaths);
    _noteFolderSubfolderTreeWidget->expandAll();

    connect(_noteFolderSubfolderTreeWidget, &QTreeWidget::itemChanged, this,
            &NoteFolderSettingsWidget::onSubfolderTreeItemChanged);
}

void NoteFolderSettingsWidget::populateSubfolderTreeFromDir(QTreeWidgetItem* parentItem, const QString& path,
                                                            const QString& relativePath) {
    QDir dir(path);
    const QStringList folders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden, QDir::Name);

    for (const QString& folder : folders) {
        if (_viewModel != nullptr && _viewModel->willSubFolderBeIgnored(folder)) {
            continue;
        }

        const QString childRelPath = relativePath.isEmpty() ? folder : relativePath + QLatin1Char('/') + folder;
        const QString childFullPath = path + QDir::separator() + folder;

        auto* item = new QTreeWidgetItem();
        item->setText(0, folder);
        item->setData(0, Qt::UserRole, childRelPath);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate);

        if (parentItem != nullptr) {
            parentItem->addChild(item);
        } else {
            _noteFolderSubfolderTreeWidget->addTopLevelItem(item);
        }

        populateSubfolderTreeFromDir(item, childFullPath, childRelPath);
    }
}

void NoteFolderSettingsWidget::applySubfolderTreeCheckStates(QTreeWidget* tree, const QStringList& excludedPaths) {
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        applyCheckStateToItem(tree->topLevelItem(i), excludedPaths);
    }
}

void NoteFolderSettingsWidget::applyCheckStateToItem(QTreeWidgetItem* item, const QStringList& excludedPaths) {
    const QString path = item->data(0, Qt::UserRole).toString();

    bool excluded = false;
    for (const QString& excludedPath : excludedPaths) {
        if (path == excludedPath || path.startsWith(excludedPath + QLatin1Char('/'))) {
            excluded = true;
            break;
        }
    }

    item->setCheckState(0, excluded ? Qt::Unchecked : Qt::Checked);

    for (int i = 0; i < item->childCount(); ++i) {
        applyCheckStateToItem(item->child(i), excludedPaths);
    }

    if (!excluded && item->childCount() > 0) {
        item->setCheckState(0, subfolderTreeParentCheckState(item));
    }
}

void NoteFolderSettingsWidget::onSubfolderTreeItemChanged(QTreeWidgetItem* item, int column) {
    if (_updatingSubfolderTreeCheckStates || column != 0) {
        return;
    }

    _updatingSubfolderTreeCheckStates = true;

    const Qt::CheckState checkState = item->checkState(0);
    if (checkState == Qt::Checked || checkState == Qt::Unchecked) {
        setSubfolderTreeChildrenCheckState(item, checkState);
    }

    updateSubfolderTreeParentCheckStates(item->parent());

    _updatingSubfolderTreeCheckStates = false;
    saveSubfolderTreeSelection();
}

void NoteFolderSettingsWidget::setSubfolderTreeChildrenCheckState(QTreeWidgetItem* item, Qt::CheckState checkState) {
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem* child = item->child(i);
        child->setCheckState(0, checkState);
        setSubfolderTreeChildrenCheckState(child, checkState);
    }
}

void NoteFolderSettingsWidget::updateSubfolderTreeParentCheckStates(QTreeWidgetItem* item) {
    while (item != nullptr) {
        item->setCheckState(0, subfolderTreeParentCheckState(item));
        item = item->parent();
    }
}

Qt::CheckState NoteFolderSettingsWidget::subfolderTreeParentCheckState(QTreeWidgetItem* item) {
    bool hasCheckedChild = false;
    bool hasUncheckedChild = false;

    for (int i = 0; i < item->childCount(); ++i) {
        const Qt::CheckState childCheckState = item->child(i)->checkState(0);
        hasCheckedChild = hasCheckedChild || childCheckState != Qt::Unchecked;
        hasUncheckedChild = hasUncheckedChild || childCheckState != Qt::Checked;
    }

    if (!hasUncheckedChild) {
        return Qt::Checked;
    }

    if (!hasCheckedChild && item->checkState(0) == Qt::Unchecked) {
        return Qt::Unchecked;
    }

    return Qt::PartiallyChecked;
}

void NoteFolderSettingsWidget::saveSubfolderTreeSelection() {
    QStringList excludedPaths;

    for (int i = 0; i < _noteFolderSubfolderTreeWidget->topLevelItemCount(); ++i) {
        collectExcludedSubfolderPaths(_noteFolderSubfolderTreeWidget->topLevelItem(i), excludedPaths);
    }

    _selectedNoteFolder.excludedSubfolderPaths = excludedPaths;
    _selectedNoteFolder = _viewModel == nullptr ? NoteFolderData() : _viewModel->saveNoteFolder(_selectedNoteFolder);
}

void NoteFolderSettingsWidget::collectExcludedSubfolderPaths(QTreeWidgetItem* item, QStringList& excludedPaths) {
    const QString relativePath = item->data(0, Qt::UserRole).toString();

    if (item->checkState(0) == Qt::Unchecked) {
        excludedPaths.append(relativePath);
        return;
    }

    for (int i = 0; i < item->childCount(); ++i) {
        collectExcludedSubfolderPaths(item->child(i), excludedPaths);
    }
}

void NoteFolderSettingsWidget::on_allowDifferentNoteFileNameCheckBox_toggled(bool checked) {
    if (selectedNoteFolderIsFetched() && _viewModel != nullptr) {
        _viewModel->setNoteFolderSetting(_selectedNoteFolder.id, QStringLiteral("allowDifferentNoteFileName"), checked);
    }
}

bool NoteFolderSettingsWidget::selectedNoteFolderIsFetched() const {
    return _selectedNoteFolder.id > 0;
}
