#pragma once

#include <models/notelistmodel.h>

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>

class AppState;
class CommandBus;

class NoteListViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged)
    Q_PROPERTY(QString sortField READ sortField WRITE setSortField NOTIFY sortFieldChanged)
    Q_PROPERTY(int noteCount READ noteCount NOTIFY noteCountChanged)
    Q_PROPERTY(QVariantList selectedNoteIds READ selectedNoteIds WRITE setSelectedNoteIds NOTIFY selectedNoteIdsChanged)

public:
    explicit NoteListViewModel(CommandBus* commandBus = nullptr, AppState* appState = nullptr,
                               QObject* parent = nullptr);

    NoteListModel* model();
    [[nodiscard]] const NoteListModel* model() const;

    [[nodiscard]] QString sortOrder() const;
    [[nodiscard]] QString sortField() const;
    [[nodiscard]] int noteCount() const;
    [[nodiscard]] QVariantList selectedNoteIds() const;

    void setCommandBus(CommandBus* commandBus);
    void setAppState(AppState* appState);

public slots:
    void setNotes(const QVector<NoteData>& notes);
    void setSortOrder(const QString& sortOrder);
    void setSortField(const QString& sortField);
    void setSelectedNoteIds(const QVariantList& noteIds);
    void selectNote(int noteId);
    void openNote(int noteId);
    void createNote(const QString& name, int folderId = 0, int subFolderId = 0);
    void renameNote(int noteId, const QString& name);
    void deleteSelectedNotes(bool moveToTrash = true);
    void sortBy(const QString& field, const QString& order);
    void refresh();

signals:
    void sortOrderChanged(const QString& sortOrder);
    void sortFieldChanged(const QString& sortField);
    void noteCountChanged(int noteCount);
    void selectedNoteIdsChanged(const QVariantList& noteIds);
    void noteSelected(int noteId);
    void refreshRequested();
    void commandFailed(const QString& message);

private:
    [[nodiscard]] QVector<int> selectedNoteIdVector() const;
    [[nodiscard]] QVariantList toVariantList(const QVector<int>& noteIds) const;

    CommandBus* _commandBus = nullptr;
    AppState* _appState = nullptr;
    NoteListModel _model;
    QString _sortOrder = QStringLiteral("ascending");
    QString _sortField = QStringLiteral("modified");
    QVector<int> _selectedNoteIds;
};
