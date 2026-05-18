#include "notesubfolderview.h"

#include <viewmodels/notesubfolderviewmodel.h>

#include <QAbstractItemView>
#include <QFrame>
#include <QItemSelectionModel>

NoteSubFolderView::NoteSubFolderView(QWidget* parent) : QTreeView(parent) {
    setObjectName(QStringLiteral("noteSubFolderView"));
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setHeaderHidden(true);
}

void NoteSubFolderView::setViewModel(NoteSubFolderViewModel* viewModel) {
    _viewModel = viewModel;
    setModel(_viewModel == nullptr ? nullptr : _viewModel->model());
    if (_viewModel == nullptr) {
        return;
    }

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current) {
        if (_viewModel != nullptr && current.isValid()) {
            _viewModel->selectSubFolder(current.data(NoteSubFolderTreeModel::SubFolderIdRole).toInt());
        }
    });
    connect(this, &QTreeView::activated, this, [this](const QModelIndex& index) {
        if (_viewModel != nullptr && index.isValid()) {
            _viewModel->selectSubFolder(index.data(NoteSubFolderTreeModel::SubFolderIdRole).toInt());
        }
    });
}
