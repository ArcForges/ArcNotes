#include "actiondialog.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QTreeWidget>
#include <QVBoxLayout>

ActionDialog::ActionDialog(QMenuBar* menuBar, QWidget* parent)
    : MasterDialog(parent), _menuBar(menuBar), _filterEdit(new QLineEdit(this)), _actionTree(new QTreeWidget(this)) {
    setWindowTitle(tr("Actions"));

    auto* layout = new QVBoxLayout(this);
    _filterEdit->setPlaceholderText(tr("Search actions"));
    layout->addWidget(_filterEdit);

    _actionTree->setColumnCount(2);
    _actionTree->setHeaderLabels({tr("Action"), tr("Shortcut")});
    _actionTree->installEventFilter(this);
    layout->addWidget(_actionTree);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    connect(_filterEdit, &QLineEdit::textChanged, this, &ActionDialog::refreshUi);
    refreshUi();
    afterSetupUI();
}

ActionDialog::~ActionDialog() = default;

void ActionDialog::refreshUi() {
    if (_actionTree == nullptr) {
        return;
    }

    _actionTree->clear();
    if (_menuBar == nullptr) {
        return;
    }

    for (QAction* action : _menuBar->actions()) {
        if (QMenu* menu = action->menu()) {
            buildActionTreeForMenu(menu);
        }
    }
    _actionTree->expandAll();
    _actionTree->resizeColumnToContents(0);
}

bool ActionDialog::eventFilter(QObject* obj, QEvent* event) {
    if (obj == _actionTree && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            accept();
            return true;
        }
    }

    return MasterDialog::eventFilter(obj, event);
}

void ActionDialog::buildActionTreeForMenu(QMenu* menu, QTreeWidgetItem* parentItem) {
    if (menu == nullptr || _actionTree == nullptr) {
        return;
    }

    const QString filter = _filterEdit == nullptr ? QString() : _filterEdit->text();
    auto* menuItem = parentItem == nullptr ? new QTreeWidgetItem(_actionTree) : new QTreeWidgetItem(parentItem);
    menuItem->setText(0, menu->title().remove(QLatin1Char('&')));

    for (QAction* action : menu->actions()) {
        if (QMenu* childMenu = action->menu()) {
            buildActionTreeForMenu(childMenu, menuItem);
            continue;
        }

        const QString text = action->text().remove(QLatin1Char('&'));
        if (!filter.isEmpty() && !text.contains(filter, Qt::CaseInsensitive)) {
            continue;
        }

        auto* actionItem = new QTreeWidgetItem(menuItem);
        actionItem->setText(0, text);
        actionItem->setText(1, action->shortcut().toString(QKeySequence::NativeText));
    }
}
