#pragma once

#include <core/data/editorsessionsnapshot.h>

#include <QObject>
#include <QString>

class EditorState : public QObject {
    Q_OBJECT
    Q_PROPERTY(int noteId READ noteId WRITE setNoteId NOTIFY noteIdChanged)
    Q_PROPERTY(bool isDirty READ isDirty WRITE setDirty NOTIFY dirtyChanged)
    Q_PROPERTY(bool isSaving READ isSaving WRITE setSaving NOTIFY savingChanged)
    Q_PROPERTY(bool hasConflict READ hasConflict WRITE setHasConflict NOTIFY hasConflictChanged)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged)
    Q_PROPERTY(int selectionStart READ selectionStart WRITE setSelectionStart NOTIFY selectionStartChanged)
    Q_PROPERTY(int selectionLength READ selectionLength WRITE setSelectionLength NOTIFY selectionLengthChanged)
    Q_PROPERTY(QString contentHash READ contentHash WRITE setContentHash NOTIFY contentHashChanged)
    Q_PROPERTY(QString baseChecksum READ baseChecksum WRITE setBaseChecksum NOTIFY baseChecksumChanged)
    Q_PROPERTY(bool isReadOnly READ isReadOnly WRITE setReadOnly NOTIFY readOnlyChanged)

public:
    explicit EditorState(QObject* parent = nullptr);

    [[nodiscard]] int noteId() const;
    [[nodiscard]] bool isDirty() const;
    [[nodiscard]] bool isSaving() const;
    [[nodiscard]] bool hasConflict() const;
    [[nodiscard]] int cursorPosition() const;
    [[nodiscard]] int selectionStart() const;
    [[nodiscard]] int selectionLength() const;
    [[nodiscard]] QString contentHash() const;
    [[nodiscard]] QString baseChecksum() const;
    [[nodiscard]] bool isReadOnly() const;

    [[nodiscard]] EditorSessionSnapshot toSnapshot() const;

public slots:
    void setNoteId(int noteId);
    void setDirty(bool dirty);
    void setSaving(bool saving);
    void setHasConflict(bool hasConflict);
    void setCursorPosition(int position);
    void setSelectionStart(int position);
    void setSelectionLength(int length);
    void setContentHash(const QString& hash);
    void setBaseChecksum(const QString& checksum);
    void setReadOnly(bool readOnly);
    void setFromSnapshot(const EditorSessionSnapshot& snapshot);
    void reset();

signals:
    void noteIdChanged(int noteId);
    void dirtyChanged(bool dirty);
    void savingChanged(bool saving);
    void hasConflictChanged(bool hasConflict);
    void cursorPositionChanged(int position);
    void selectionStartChanged(int position);
    void selectionLengthChanged(int length);
    void contentHashChanged(const QString& hash);
    void baseChecksumChanged(const QString& checksum);
    void readOnlyChanged(bool readOnly);

private:
    int _noteId = 0;
    bool _dirty = false;
    bool _saving = false;
    bool _hasConflict = false;
    int _cursorPosition = 0;
    int _selectionStart = 0;
    int _selectionLength = 0;
    QString _contentHash;
    QString _baseChecksum;
    bool _readOnly = false;
};
