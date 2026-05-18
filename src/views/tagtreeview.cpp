#include "tagtreeview.h"

#include <viewmodels/tagtreeviewmodel.h>

#include <QAbstractItemView>
#include <QFrame>
#include <QHeaderView>
#include <QItemSelectionModel>

TagTreeView::TagTreeView(QWidget* parent) : QTreeView(parent) {
    setObjectName(QStringLiteral("tagTreeView"));
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setHeaderHidden(true);
}

void TagTreeView::setViewModel(TagTreeViewModel* viewModel) {
    _viewModel = viewModel;
    setModel(_viewModel == nullptr ? nullptr : _viewModel->model());
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    setRootIsDecorated(true);
    if (_viewModel == nullptr) {
        return;
    }

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current) {
        if (_viewModel == nullptr || !current.isValid()) {
            return;
        }
        _viewModel->selectTag(current.data(TagTreeModel::TagIdRole).toInt());
    });
    const auto selectActiveTag = [this]() {
        if (_viewModel == nullptr || model() == nullptr || _viewModel->activeTagId() == 0) {
            return;
        }
        const QModelIndexList matches = model()->match(model()->index(0, 0), TagTreeModel::TagIdRole,
                                                       _viewModel->activeTagId(), 1, Qt::MatchRecursive);
        if (!matches.isEmpty()) {
            setCurrentIndex(matches.first());
        }
    };
    connect(model(), &QAbstractItemModel::modelReset, this, selectActiveTag);
    connect(_viewModel, &TagTreeViewModel::activeTagIdChanged, this, selectActiveTag);
    selectActiveTag();
}
