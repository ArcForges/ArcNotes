#pragma once

#include <QWidget>

class QListView;
class QLineEdit;
class SearchViewModel;

class SearchPanelView : public QWidget {
    Q_OBJECT

public:
    explicit SearchPanelView(QWidget* parent = nullptr);

    void setViewModel(SearchViewModel* viewModel);

private:
    SearchViewModel* _viewModel = nullptr;
    QLineEdit* _searchEdit = nullptr;
    QListView* _resultsView = nullptr;
};
