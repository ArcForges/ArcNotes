#include "navigationview.h"

#include <viewmodels/navigationviewmodel.h>

#include <QAbstractItemModel>
#include <QFrame>

NavigationView::NavigationView(QWidget* parent) : QTreeView(parent) {
    setObjectName(QStringLiteral("navigationView"));
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    setHeaderHidden(true);
}

void NavigationView::setViewModel(NavigationViewModel* viewModel) {
    _viewModel = viewModel;
    setModel(_viewModel == nullptr ? nullptr : _viewModel->model());
    if (_viewModel == nullptr) {
        return;
    }

    const auto selectHeading = [this](const QModelIndex& index) {
        if (_viewModel != nullptr && index.isValid()) {
            _viewModel->selectHeading(index.data(NavigationOutlineModel::PositionRole).toInt());
        }
    };
    connect(this, &QTreeView::clicked, this, selectHeading);
    connect(this, &QTreeView::activated, this, selectHeading);
    connect(model(), &QAbstractItemModel::modelReset, this, &QTreeView::expandAll);
    expandAll();
}
