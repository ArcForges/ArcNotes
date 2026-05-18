#include "searchpanelview.h"

#include <viewmodels/searchviewmodel.h>

#include <QLineEdit>
#include <QListView>
#include <QVBoxLayout>

SearchPanelView::SearchPanelView(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    _searchEdit = new QLineEdit(this);
    _searchEdit->setObjectName(QStringLiteral("searchEdit"));
    _resultsView = new QListView(this);
    _resultsView->setObjectName(QStringLiteral("searchResultsView"));
    layout->addWidget(_searchEdit);
    layout->addWidget(_resultsView);
}

void SearchPanelView::setViewModel(SearchViewModel *viewModel) {
    _viewModel = viewModel;
    _resultsView->setModel(_viewModel == nullptr ? nullptr : _viewModel->model());
    if (_viewModel == nullptr) {
        return;
    }

    connect(_searchEdit, &QLineEdit::textChanged, _viewModel, &SearchViewModel::search);
    connect(_viewModel, &SearchViewModel::queryChanged, _searchEdit, &QLineEdit::setText);
}
