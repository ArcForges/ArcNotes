#pragma once

#include <QTreeView>

class NavigationViewModel;

class NavigationView : public QTreeView {
    Q_OBJECT

public:
    explicit NavigationView(QWidget* parent = nullptr);

    void setViewModel(NavigationViewModel* viewModel);

private:
    NavigationViewModel* _viewModel = nullptr;
};
