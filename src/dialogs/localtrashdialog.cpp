#include "localtrashdialog.h"

#include <utils/gui.h>
#include <viewmodels/viewmodellocator.h>

#include <QDebug>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "widgets/arcnotesmarkdowntextedit.h"

class LocalTrashTreeWidgetItem : public QTreeWidgetItem {
public:
    explicit LocalTrashTreeWidgetItem(QTreeWidget* parent) : QTreeWidgetItem(parent) {}

private:
    bool operator<(const QTreeWidgetItem& other) const override {
        const int column = treeWidget()->sortColumn();

        if (column == 1) {
            return data(column, Qt::UserRole).toLongLong() < other.data(column, Qt::UserRole).toLongLong();
        }

        return text(column).toLower() < other.text(column).toLower();
    }
};

LocalTrashDialog::LocalTrashDialog(QWidget* parent)
    : MasterDialog(parent), _host(dynamic_cast<LocalTrashDialogHost*>(parent)) {
    buildUi();
    afterSetupUI();
    setupMainSplitter();

    QPushButton* button;
    _buttonBox->clear();

    button = new QPushButton(tr("Restore"), this);
    button->setToolTip(tr("Restore selected notes"));
    button->setProperty("ActionRole", Restore);
    button->setDefault(false);
    button->setIcon(
        QIcon::fromTheme(QStringLiteral("view-restore"), QIcon(":/icons/breeze-arcnotes/16x16/view-restore.svg")));
    _buttonBox->addButton(button, QDialogButtonBox::ActionRole);

    button = new QPushButton(tr("Remove"), this);
    button->setToolTip(tr("Remove selected notes"));
    button->setProperty("ActionRole", Remove);
    button->setDefault(false);
    button->setIcon(
        QIcon::fromTheme(QStringLiteral("edit-delete"), QIcon(":/icons/breeze-arcnotes/16x16/edit-delete.svg")));
    _buttonBox->addButton(button, QDialogButtonBox::ActionRole);

    connect(_buttonBox, &QDialogButtonBox::clicked, this, &LocalTrashDialog::dialogButtonClicked);
    connect(this, &QDialog::finished, this, &LocalTrashDialog::storeSettings);
    connect(_trashTreeWidget, &QTreeWidget::currentItemChanged, this,
            &LocalTrashDialog::on_trashTreeWidget_currentItemChanged);
    connect(_searchLineEdit, &QLineEdit::textChanged, this, &LocalTrashDialog::on_searchLineEdit_textChanged);

    Utils::Gui::initTreeWidgetHeaderOrderPersistence(_trashTreeWidget,
                                                     QStringLiteral("LocalTrashDialog/trashTreeWidgetHeaderOrder"));

    loadTrashedNotes();
}

LocalTrashDialog::~LocalTrashDialog() = default;

void LocalTrashDialog::buildUi() {
    setWindowTitle(tr("Local trash"));
    resize(900, 600);

    auto* rootLayout = new QVBoxLayout(this);
    _listFrame = new QFrame(this);
    auto* listLayout = new QVBoxLayout(_listFrame);
    listLayout->setContentsMargins(0, 0, 0, 0);

    _searchLineEdit = new QLineEdit(_listFrame);
    _searchLineEdit->setPlaceholderText(tr("Search"));
    listLayout->addWidget(_searchLineEdit);

    _trashTreeWidget = new QTreeWidget(_listFrame);
    _trashTreeWidget->setColumnCount(2);
    _trashTreeWidget->setHeaderLabels(QStringList() << tr("File") << tr("Date"));
    _trashTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _trashTreeWidget->setSortingEnabled(true);
    listLayout->addWidget(_trashTreeWidget, 1);

    _noteBrowserFrame = new QFrame(this);
    auto* browserLayout = new QVBoxLayout(_noteBrowserFrame);
    browserLayout->setContentsMargins(0, 0, 0, 0);
    _noteBrowser = new ArcNotesMarkdownTextEdit(_noteBrowserFrame);
    _noteBrowser->setReadOnly(true);
    browserLayout->addWidget(_noteBrowser, 1);

    _buttonBox = new QDialogButtonBox(this);

    _trashSplitter = new QSplitter(this);
    rootLayout->addWidget(_trashSplitter, 1);
    rootLayout->addWidget(_buttonBox);
}

void LocalTrashDialog::setupMainSplitter() {
    _trashSplitter->addWidget(_listFrame);
    _trashSplitter->addWidget(_noteBrowserFrame);

    SettingsViewModel* viewModel = ViewModelLocator::settingsViewModel();
    const QByteArray state =
        viewModel == nullptr ? QByteArray()
                             : viewModel->persistentSetting(QStringLiteral("localTrashSplitterSizes")).toByteArray();
    _trashSplitter->restoreState(state);
}

void LocalTrashDialog::loadTrashedNotes() {
    const QVector<TrashItemData> trashItems = _host == nullptr ? QVector<TrashItemData>() : _host->localTrashItems();
    _trashTreeWidget->clear();
    _noteBrowser->clear();

    for (const TrashItemData& trashItem : trashItems) {
        auto* item = new LocalTrashTreeWidgetItem(_trashTreeWidget);
        item->setText(0, trashItem.fileName);
        item->setText(1, trashItem.created.toString());
        item->setData(0, Qt::UserRole, trashItem.id);
        item->setData(1, Qt::UserRole, trashItem.created.toMSecsSinceEpoch());

        QString toolTipText =
            tr("File will be restored to: %1").arg(_host->localTrashItemRestorationPath(trashItem.id));

        if (!_host->localTrashItemFileExists(trashItem.id)) {
            item->setIcon(0, QIcon::fromTheme(QStringLiteral("edit-delete"),
                                              QIcon(":/icons/breeze-arcnotes/16x16/edit-delete.svg")));
            toolTipText =
                tr("File <strong>%1</strong> isn't readable and can't be restored!").arg(trashItem.fullNoteFilePath);
        }

        item->setToolTip(0, toolTipText);
        item->setToolTip(1, toolTipText);
        _trashTreeWidget->addTopLevelItem(item);
    }

    _trashTreeWidget->sortItems(1, Qt::SortOrder::DescendingOrder);

    if (Utils::Gui::hasTreeWidgetHeaderLayout(_trashTreeWidget)) {
        Utils::Gui::restoreTreeWidgetHeaderLayout(_trashTreeWidget);
    } else {
        _trashTreeWidget->resizeColumnToContents(0);
    }
}

void LocalTrashDialog::storeSettings() {
    if (SettingsViewModel* viewModel = ViewModelLocator::settingsViewModel()) {
        viewModel->setPersistentSetting(QStringLiteral("localTrashSplitterSizes"), _trashSplitter->saveState());
    }
}

void LocalTrashDialog::dialogButtonClicked(QAbstractButton* button) {
    const int actionRole = button->property("ActionRole").toInt();

    switch (actionRole) {
        case Remove:
            removeSelectedTrashItems();
            break;
        case Restore:
            restoreSelectedTrashItems();
            break;
        default:
            close();
            break;
    }
}

void LocalTrashDialog::on_trashTreeWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous) {
    Q_UNUSED(previous);
    if (current == nullptr) {
        _noteBrowser->clear();
        return;
    }

    const int trashItemId = current->data(0, Qt::UserRole).toInt();
    _noteBrowser->setPlainText(_host == nullptr ? QString() : _host->localTrashItemText(trashItemId));
}

void LocalTrashDialog::restoreSelectedTrashItems() {
    const int selectedItemsCount = _trashTreeWidget->selectedItems().count();

    if (selectedItemsCount == 0) {
        return;
    }

    if (Utils::Gui::question(this, tr("Restore selected notes"),
                             tr("Restore <strong>%n</strong> selected note(s)?", "", selectedItemsCount),
                             QStringLiteral("local-trash-restore-notes")) == QMessageBox::Yes) {
        const QSignalBlocker blocker(_trashTreeWidget);
        Q_UNUSED(blocker)

        int restoreCount = 0;
        QVector<int> ids;
        for (QTreeWidgetItem* item : _trashTreeWidget->selectedItems()) {
            ids.append(item->data(0, Qt::UserRole).toInt());
        }

        if (_host != nullptr && _host->localTrashRestoreItems(ids)) {
            restoreCount = ids.count();
        } else {
            qDebug() << "Selected trash item restore failed";
        }

        Utils::Gui::information(this, tr("Notes restored"),
                                tr("<strong>%n</strong> note(s) were restored", "", restoreCount),
                                QStringLiteral("local-trash-notes-restored"));

        if (restoreCount > 0) {
            loadTrashedNotes();
        }
    }
}

void LocalTrashDialog::removeSelectedTrashItems() {
    const int selectedItemsCount = _trashTreeWidget->selectedItems().count();

    if (selectedItemsCount == 0) {
        return;
    }

    if (Utils::Gui::question(this, tr("Remove selected notes"),
                             tr("Remove <strong>%n</strong> selected note(s)?", "", selectedItemsCount),
                             QStringLiteral("local-trash-remove-notes")) == QMessageBox::Yes) {
        const QSignalBlocker blocker(_trashTreeWidget);
        Q_UNUSED(blocker)

        int removeCount = 0;
        QVector<int> ids;
        for (QTreeWidgetItem* item : _trashTreeWidget->selectedItems()) {
            ids.append(item->data(0, Qt::UserRole).toInt());
        }

        if (_host != nullptr && _host->localTrashRemoveItems(ids)) {
            removeCount = ids.count();
        } else {
            qDebug() << "Selected trash item removal failed";
        }

        Utils::Gui::information(this, tr("Notes removed"),
                                tr("<strong>%n</strong> note(s) were removed", "", removeCount),
                                QStringLiteral("local-trash-notes-removed"));

        if (removeCount > 0) {
            loadTrashedNotes();
        }
    }
}

void LocalTrashDialog::on_searchLineEdit_textChanged(const QString& arg1) {
    Utils::Gui::searchForTextInTreeWidget(_trashTreeWidget, arg1);
}
