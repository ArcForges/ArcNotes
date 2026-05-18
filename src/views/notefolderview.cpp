#include "notefolderview.h"

#include <viewmodels/notefolderviewmodel.h>

NoteFolderView::NoteFolderView(QWidget* parent) : QListView(parent) {
    setObjectName(QStringLiteral("noteFolderView"));
}

void NoteFolderView::setViewModel(NoteFolderViewModel* viewModel) {
    _viewModel = viewModel;
    setModel(_viewModel == nullptr ? nullptr : _viewModel->model());
    if (_viewModel == nullptr) {
        return;
    }

    connect(this, &QListView::activated, this, [this](const QModelIndex& index) {
        if (_viewModel != nullptr && index.isValid()) {
            _viewModel->switchFolder(index.data(NoteFolderListModel::FolderIdRole).toInt());
        }
    });
}
