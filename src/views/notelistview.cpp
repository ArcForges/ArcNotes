#include "notelistview.h"

#include <viewmodels/notelistviewmodel.h>

#include <QAbstractItemView>
#include <QFrame>
#include <QItemSelectionModel>
#include <QVariantList>

NoteListView::NoteListView(QWidget* parent) : QTreeView(parent) {
    setObjectName(QStringLiteral("noteListView"));
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setRootIsDecorated(false);
    setHeaderHidden(true);
    setAlternatingRowColors(false);
}

void NoteListView::setViewModel(NoteListViewModel* viewModel) {
    _viewModel = viewModel;
    setModel(_viewModel == nullptr ? nullptr : _viewModel->model());
    if (_viewModel == nullptr) {
        return;
    }

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current) {
        if (_viewModel == nullptr || !current.isValid()) {
            return;
        }
        const int noteId = current.data(NoteListModel::NoteIdRole).toInt();
        _viewModel->selectNote(noteId);
    });
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        if (_viewModel == nullptr || selectionModel() == nullptr) {
            return;
        }

        QVariantList noteIds;
        const QModelIndexList rows = selectionModel()->selectedRows();
        noteIds.reserve(rows.count());
        for (const QModelIndex& index : rows) {
            const int noteId = index.data(NoteListModel::NoteIdRole).toInt();
            if (noteId > 0) {
                noteIds.append(noteId);
            }
        }
        _viewModel->setSelectedNoteIds(noteIds);
    });
}
