#pragma once

#include <QTreeView>

class NoteSubFolderViewModel;

class NoteSubFolderView : public QTreeView {
    Q_OBJECT

public:
    explicit NoteSubFolderView(QWidget* parent = nullptr);

    void setViewModel(NoteSubFolderViewModel* viewModel);

private:
    NoteSubFolderViewModel* _viewModel = nullptr;
};
