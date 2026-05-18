#pragma once

#include <core/data/notefolderdata.h>

#include <QList>
#include <QVariant>

class NoteFolderRepository {
public:
    [[nodiscard]] NoteFolderData findById(int id) const;
    [[nodiscard]] QList<NoteFolderData> findAll() const;
    [[nodiscard]] NoteFolderData current() const;
    [[nodiscard]] bool save(const NoteFolderData& folder) const;
    [[nodiscard]] NoteFolderData saveAndReturn(const NoteFolderData& folder) const;
    [[nodiscard]] bool remove(int id) const;
    [[nodiscard]] int countAll() const;
    [[nodiscard]] int currentFolderId() const;
    void setCurrentFolderId(int id) const;
    [[nodiscard]] bool migrateToNoteFolders() const;
    [[nodiscard]] QVariant settingValue(int folderId, const QString& key,
                                        const QVariant& defaultValue = QVariant()) const;
    void setSettingValue(int folderId, const QString& key, const QVariant& value) const;
};
