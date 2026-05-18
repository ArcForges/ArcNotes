#pragma once

#include <QTreeView>

class TagTreeViewModel;

class TagTreeView : public QTreeView {
    Q_OBJECT

public:
    explicit TagTreeView(QWidget* parent = nullptr);

    void setViewModel(TagTreeViewModel* viewModel);

private:
    TagTreeViewModel* _viewModel = nullptr;
};
