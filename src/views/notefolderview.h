#pragma once

#include <QListView>

class NoteFolderViewModel;

class NoteFolderView : public QListView {
    Q_OBJECT

public:
    explicit NoteFolderView(QWidget* parent = nullptr);

    void setViewModel(NoteFolderViewModel* viewModel);

private:
    NoteFolderViewModel* _viewModel = nullptr;
};
