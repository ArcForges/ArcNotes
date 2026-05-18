#pragma once

#include <core/data/notefolderdata.h>

#include <QList>
#include <QObject>
#include <QVariant>
#include <QVector>

class CommandBus;
class NoteFolderRepository;
class NoteSubFolderRepository;

class NoteFolderSettingsViewModel : public QObject {
    Q_OBJECT

public:
    explicit NoteFolderSettingsViewModel(CommandBus* commandBus = nullptr,
                                         NoteFolderRepository* noteFolderRepository = nullptr,
                                         NoteSubFolderRepository* noteSubFolderRepository = nullptr,
                                         QObject* parent = nullptr);

    [[nodiscard]] QList<NoteFolderData> noteFolders() const;
    [[nodiscard]] NoteFolderData noteFolder(int folderId) const;
    [[nodiscard]] int noteFolderCount() const;
    [[nodiscard]] int currentFolderId() const;
    [[nodiscard]] QVariant noteFolderSetting(int folderId, const QString& key,
                                             const QVariant& defaultValue = QVariant()) const;
    [[nodiscard]] QString subFolderTreeExpandStateSettingsKey(int noteFolderId) const;
    [[nodiscard]] bool willSubFolderBeIgnored(const QString& folderName) const;
    [[nodiscard]] QString defaultIgnoredSubfoldersPattern() const;

public slots:
    NoteFolderData saveNoteFolder(const NoteFolderData& folder);
    bool removeNoteFolder(int folderId);
    bool setCurrentFolderId(int folderId);
    bool setNoteFolderSetting(int folderId, const QString& key, const QVariant& value);
    bool removePersistentSetting(const QString& key);
    bool updateFolderPriorities(const QVector<int>& folderIds);

signals:
    void commandFailed(const QString& message);

private:
    CommandBus* _commandBus = nullptr;
    NoteFolderRepository* _noteFolderRepository = nullptr;
    NoteSubFolderRepository* _noteSubFolderRepository = nullptr;
};
