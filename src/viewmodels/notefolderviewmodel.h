#pragma once

#include <models/notefolderlistmodel.h>

#include <QObject>
#include <QString>

class AppState;
class CommandBus;

class NoteFolderViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int currentFolderId READ currentFolderId NOTIFY currentFolderChanged)
    Q_PROPERTY(QString currentFolderName READ currentFolderName NOTIFY currentFolderChanged)

public:
    explicit NoteFolderViewModel(CommandBus* commandBus = nullptr, AppState* appState = nullptr,
                                 QObject* parent = nullptr);

    NoteFolderListModel* model();
    [[nodiscard]] const NoteFolderListModel* model() const;
    [[nodiscard]] int currentFolderId() const;
    [[nodiscard]] QString currentFolderName() const;

    void setCommandBus(CommandBus* commandBus);
    void setAppState(AppState* appState);

public slots:
    void setFolders(const QVector<NoteFolderData>& folders);
    void switchFolder(int folderId);

signals:
    void currentFolderChanged();
    void commandFailed(const QString& message);

private:
    void syncFromState();

    CommandBus* _commandBus = nullptr;
    AppState* _appState = nullptr;
    NoteFolderListModel _model;
    NoteFolderData _currentFolder;
};
