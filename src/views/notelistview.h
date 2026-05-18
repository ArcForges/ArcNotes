#pragma once

#include <QTreeView>

class NoteListViewModel;

class NoteListView : public QTreeView {
    Q_OBJECT

public:
    explicit NoteListView(QWidget* parent = nullptr);

    void setViewModel(NoteListViewModel* viewModel);

private:
    NoteListViewModel* _viewModel = nullptr;
};
