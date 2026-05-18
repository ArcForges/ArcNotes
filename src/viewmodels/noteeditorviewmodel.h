#pragma once

#include <core/data/notedata.h>

#include <QObject>
#include <QString>
#include <QVariant>

class AppState;
class CommandBus;
class Debouncer;
class EditorState;

class NoteEditorViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int noteId READ noteId NOTIFY noteIdChanged)
    Q_PROPERTY(QString noteTitle READ noteTitle NOTIFY noteTitleChanged)
    Q_PROPERTY(QString noteText READ noteText WRITE textChanged NOTIFY noteTextChanged)
    Q_PROPERTY(bool isDirty READ isDirty NOTIFY dirtyChanged)
    Q_PROPERTY(bool isReadOnly READ isReadOnly NOTIFY readOnlyChanged)
    Q_PROPERTY(bool isSaving READ isSaving NOTIFY savingChanged)
    Q_PROPERTY(bool hasConflict READ hasConflict NOTIFY conflictChanged)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged)

public:
    explicit NoteEditorViewModel(CommandBus* commandBus = nullptr, AppState* appState = nullptr,
                                 EditorState* editorState = nullptr, QObject* parent = nullptr);
    ~NoteEditorViewModel() override;

    int noteId() const;
    QString noteTitle() const;
    QString noteText() const;
    bool isDirty() const;
    bool isReadOnly() const;
    bool isSaving() const;
    bool hasConflict() const;
    int cursorPosition() const;

    void setCommandBus(CommandBus* commandBus);
    void setAppState(AppState* appState);
    void setEditorState(EditorState* editorState);

public slots:
    void loadNote(const NoteData& note);
    void textChanged(const QString& text);
    void save();
    void formatBold();
    void formatItalic();
    void formatHeading(int level);
    void insertLink(const QString& url, const QString& title);
    void insertImage(const QString& sourcePath, const QString& title = QString());
    void resolveConflict(const QString& resolution);
    void setCursorPosition(int position);

signals:
    void noteIdChanged(int noteId);
    void noteTitleChanged(const QString& title);
    void noteTextChanged(const QString& text);
    void dirtyChanged(bool dirty);
    void readOnlyChanged(bool readOnly);
    void savingChanged(bool saving);
    void conflictChanged(bool conflict);
    void cursorPositionChanged(int position);
    void formatRequested(const QString& format, const QVariant& argument);
    void linkInsertionRequested(const QString& url, const QString& title);
    void conflictResolutionRequested(const QString& resolution);
    void commandFailed(const QString& message);

private:
    void syncFromEditorState();

    CommandBus* _commandBus = nullptr;
    AppState* _appState = nullptr;
    EditorState* _editorState = nullptr;
    Debouncer* _saveDebouncer = nullptr;
    int _noteId = 0;
    QString _noteTitle;
    QString _noteText;
    QString _baseChecksum;
    bool _dirty = false;
    bool _readOnly = false;
    bool _saving = false;
    bool _hasConflict = false;
    int _cursorPosition = 0;
};
