#pragma once

#include <models/notesubfoldertreemodel.h>

#include <QObject>
#include <QString>

class AppState;
class CommandBus;

class NoteSubFolderViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int activeSubFolderId READ activeSubFolderId NOTIFY activeSubFolderChanged)
    Q_PROPERTY(bool showNotesFromAll READ showNotesFromAll WRITE setShowNotesFromAll NOTIFY showNotesFromAllChanged)

public:
    explicit NoteSubFolderViewModel(CommandBus* commandBus = nullptr, AppState* appState = nullptr,
                                    QObject* parent = nullptr);

    NoteSubFolderTreeModel* model();
    [[nodiscard]] const NoteSubFolderTreeModel* model() const;
    [[nodiscard]] int activeSubFolderId() const;
    [[nodiscard]] bool showNotesFromAll() const;

    void setCommandBus(CommandBus* commandBus);
    void setAppState(AppState* appState);

public slots:
    void setSubFolders(const QVector<NoteSubFolderData>& subFolders);
    void setShowNotesFromAll(bool enabled);
    void selectSubFolder(int subFolderId);
    void createSubFolder(const QString& name, int parentId = 0);
    void deleteSubFolder(int subFolderId);
    void renameSubFolder(int subFolderId, const QString& name);
    void moveSubFolder(int subFolderId, int destinationParentId);

signals:
    void activeSubFolderChanged(int subFolderId);
    void showNotesFromAllChanged(bool enabled);
    void commandFailed(const QString& message);

private:
    CommandBus* _commandBus = nullptr;
    AppState* _appState = nullptr;
    NoteSubFolderTreeModel _model;
    int _activeSubFolderId = 0;
    bool _showNotesFromAll = true;
};
