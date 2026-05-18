#pragma once

#include <core/data/notefolderdata.h>

#include <QList>
#include <QVariant>

class NoteFolderRepository {
public:
    NoteFolderData findById(int id) const;
    QList<NoteFolderData> findAll() const;
    NoteFolderData current() const;
    bool save(const NoteFolderData& folder) const;
    NoteFolderData saveAndReturn(const NoteFolderData& folder) const;
    bool remove(int id) const;
    int countAll() const;
    int currentFolderId() const;
    void setCurrentFolderId(int id) const;
    bool migrateToNoteFolders() const;
    QVariant settingValue(int folderId, const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(int folderId, const QString& key, const QVariant& value) const;
};
