#pragma once

#include <QWidget>

class NoteEditorViewModel;
class ArcNotesMarkdownTextEdit;

class NoteEditorView : public QWidget {
    Q_OBJECT

public:
    explicit NoteEditorView(QWidget* parent = nullptr);

    void setViewModel(NoteEditorViewModel* viewModel);
    ArcNotesMarkdownTextEdit* editor() const;

private slots:
    void applyViewModelText(const QString& text);

private:
    NoteEditorViewModel* _viewModel = nullptr;
    ArcNotesMarkdownTextEdit* _editor = nullptr;
};
