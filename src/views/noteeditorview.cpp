#include "noteeditorview.h"

#include <viewmodels/noteeditorviewmodel.h>
#include <widgets/arcnotesmarkdowntextedit.h>

#include <QFrame>
#include <QSignalBlocker>
#include <QVBoxLayout>

NoteEditorView::NoteEditorView(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    _editor = new ArcNotesMarkdownTextEdit(this);
    _editor->setObjectName(QStringLiteral("noteTextEdit"));
    _editor->setContextMenuPolicy(Qt::CustomContextMenu);
    _editor->setFrameShape(QFrame::NoFrame);
    _editor->setCursorWidth(1);
    layout->addWidget(_editor);

    auto* searchFrame = new QFrame(this);
    searchFrame->setObjectName(QStringLiteral("noteTextEditSearchFrame"));
    searchFrame->setFrameShape(QFrame::NoFrame);
    searchFrame->setLineWidth(0);
    layout->addWidget(searchFrame);
    _editor->initSearchFrame(searchFrame);
}

void NoteEditorView::setViewModel(NoteEditorViewModel* viewModel) {
    if (_viewModel == viewModel) {
        return;
    }

    if (_viewModel != nullptr) {
        disconnect(_viewModel, nullptr, this, nullptr);
        disconnect(_editor, nullptr, _viewModel, nullptr);
    }

    _viewModel = viewModel;
    if (_viewModel == nullptr) {
        return;
    }

    applyViewModelText(_viewModel->noteText());
    connect(_viewModel, &NoteEditorViewModel::noteTextChanged, this, &NoteEditorView::applyViewModelText);
    connect(_editor, &QPlainTextEdit::textChanged, this, [this]() {
        if (_viewModel != nullptr) {
            _viewModel->textChanged(_editor->toPlainText());
        }
    });
}

ArcNotesMarkdownTextEdit* NoteEditorView::editor() const {
    return _editor;
}

void NoteEditorView::applyViewModelText(const QString& text) {
    if (_editor->toPlainText() == text) {
        return;
    }
    const QSignalBlocker blocker(_editor);
    _editor->setText(text);
}
