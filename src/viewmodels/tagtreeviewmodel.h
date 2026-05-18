#pragma once

#include <models/tagtreemodel.h>

#include <QObject>
#include <QVariantList>
#include <QVector>

class AppState;
class CommandBus;

class TagTreeViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int activeTagId READ activeTagId WRITE selectTag NOTIFY activeTagIdChanged)
    Q_PROPERTY(int tagCount READ tagCount NOTIFY tagCountChanged)

public:
    explicit TagTreeViewModel(CommandBus* commandBus = nullptr, AppState* appState = nullptr,
                              QObject* parent = nullptr);

    TagTreeModel* model();
    const TagTreeModel* model() const;
    int activeTagId() const;
    int tagCount() const;

    void setCommandBus(CommandBus* commandBus);
    void setAppState(AppState* appState);

public slots:
    void setTags(const QVector<TagData>& tags, const QHash<int, int>& noteCounts = {});
    void setSelectedNoteIds(const QVariantList& noteIds);
    void selectTag(int tagId);
    void createTag(const QString& name, int parentId = 0);
    void deleteTag(int tagId);
    void renameTag(int tagId, const QString& name);
    void moveTag(int tagId, int parentId);
    void tagSelectedNotes(int tagId);
    void untagSelectedNotes(int tagId);

signals:
    void activeTagIdChanged(int tagId);
    void tagCountChanged(int tagCount);
    void commandFailed(const QString& message);

private:
    QVector<int> selectedNoteIdVector() const;

    CommandBus* _commandBus = nullptr;
    AppState* _appState = nullptr;
    TagTreeModel _model;
    int _activeTagId = 0;
    QVector<int> _selectedNoteIds;
};
