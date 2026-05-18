#pragma once

#include <core/data/colormodedata.h>
#include <core/data/notefolderdata.h>

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QVector>

struct CreateNoteCommand {
    static constexpr const char *Type = "notes.create";
    QString name;
    int folderId = 0;
    int subFolderId = 0;
};

struct OpenNoteCommand {
    static constexpr const char *Type = "notes.open";
    int noteId = 0;
};

struct SaveNoteCommand {
    static constexpr const char *Type = "notes.save";
    int noteId = 0;
    QString text;
    QString baseChecksum;
};

struct DeleteNoteCommand {
    static constexpr const char *Type = "notes.delete";
    QVector<int> noteIds;
    bool moveToTrash = true;
};

struct RenameNoteCommand {
    static constexpr const char *Type = "notes.rename";
    int noteId = 0;
    QString newName;
};

struct MoveNoteCommand {
    static constexpr const char *Type = "notes.move";
    QVector<int> noteIds;
    int targetFolderId = 0;
    int targetSubFolderId = 0;
};

struct DuplicateNoteCommand {
    static constexpr const char *Type = "notes.duplicate";
    int noteId = 0;
    int targetFolderId = 0;
    int targetSubFolderId = 0;
};

struct ToggleFavoriteNoteCommand {
    static constexpr const char *Type = "notes.toggleFavorite";
    int noteId = 0;
};

struct SearchNotesCommand {
    static constexpr const char *Type = "notes.search";
    QString query;
    int subFolderId = -1;
    bool contentSearch = true;
};

struct TagNoteCommand {
    static constexpr const char *Type = "notes.tag";
    QVector<int> noteIds;
    int tagId = 0;
};

struct UntagNoteCommand {
    static constexpr const char *Type = "notes.untag";
    QVector<int> noteIds;
    int tagId = 0;
};

struct CreateTagCommand {
    static constexpr const char *Type = "tags.create";
    QString name;
    int parentId = 0;
};

struct DeleteTagCommand {
    static constexpr const char *Type = "tags.delete";
    int tagId = 0;
};

struct RenameTagCommand {
    static constexpr const char *Type = "tags.rename";
    int tagId = 0;
    QString newName;
};

struct MoveTagCommand {
    static constexpr const char *Type = "tags.move";
    int tagId = 0;
    int parentId = 0;
};

struct SetTagColorCommand {
    static constexpr const char *Type = "tags.setColor";
    int tagId = 0;
    QVariant color;
};

struct SwitchFolderCommand {
    static constexpr const char *Type = "folders.switch";
    int folderId = 0;
};

struct SaveNoteFolderCommand {
    static constexpr const char *Type = "folders.save";
    NoteFolderData folder;
};

struct RemoveNoteFolderCommand {
    static constexpr const char *Type = "folders.remove";
    int folderId = 0;
};

struct SetCurrentNoteFolderCommand {
    static constexpr const char *Type = "folders.setCurrent";
    int folderId = 0;
};

struct SetNoteFolderSettingCommand {
    static constexpr const char *Type = "folders.setSetting";
    int folderId = 0;
    QString key;
    QVariant value;
};

struct UpdateNoteFolderPrioritiesCommand {
    static constexpr const char *Type = "folders.updatePriorities";
    QVector<int> folderIds;
};

struct CreateSubFolderCommand {
    static constexpr const char *Type = "subfolders.create";
    QString name;
    int parentId = 0;
};

struct SetActiveSubFolderCommand {
    static constexpr const char *Type = "subfolders.setActive";
    int subFolderId = 0;
};

struct DeleteSubFolderCommand {
    static constexpr const char *Type = "subfolders.delete";
    int subFolderId = 0;
    bool deleteNotes = false;
};

struct RenameSubFolderCommand {
    static constexpr const char *Type = "subfolders.rename";
    int subFolderId = 0;
    QString name;
};

struct MoveSubFolderCommand {
    static constexpr const char *Type = "subfolders.move";
    int subFolderId = 0;
    int destinationParentId = 0;
};

struct RestoreTrashCommand {
    static constexpr const char *Type = "notes.restore";
    QVector<int> trashItemIds;
};

struct ClearTrashCommand {
    static constexpr const char *Type = "trash.clear";
    bool expiredOnly = false;
};

struct RemoveTrashCommand {
    static constexpr const char *Type = "trash.remove";
    QVector<int> trashItemIds;
};

struct SetSettingCommand {
    static constexpr const char *Type = "settings.set";
    QString key;
    QVariant value;
};

struct RemoveSettingCommand {
    static constexpr const char *Type = "settings.remove";
    QString key;
};

struct ClearSettingsCommand {
    static constexpr const char *Type = "settings.clear";
};

struct WriteSettingsArrayCommand {
    static constexpr const char *Type = "settings.writeArray";
    QString arrayName;
    QVector<QVariantMap> values;
};

struct ReinitializeDatabaseCommand {
    static constexpr const char *Type = "database.reinitialize";
};

struct CheckDatabaseIntegrityCommand {
    static constexpr const char *Type = "database.checkIntegrity";
};

struct RemoveDiskDatabaseCommand {
    static constexpr const char *Type = "database.removeDisk";
};

struct EnsureBuiltInColorModesCommand {
    static constexpr const char *Type = "colormodes.ensureBuiltIns";
};

struct CreateColorModeCommand {
    static constexpr const char *Type = "colormodes.create";
    QString name;
};

struct SaveColorModeCommand {
    static constexpr const char *Type = "colormodes.save";
    ColorModeData colorMode;
};

struct RemoveColorModeCommand {
    static constexpr const char *Type = "colormodes.remove";
    QString colorModeId;
};

struct SetCurrentColorModeCommand {
    static constexpr const char *Type = "colormodes.setCurrent";
    QString colorModeId;
};

struct ExportNoteCommand {
    static constexpr const char *Type = "notes.export";
    QVector<int> noteIds;
    QString destinationPath;
    QString format;
    bool includeAttachments = false;
    QHash<QString, QVariant> options;
};

struct InsertMediaCommand {
    static constexpr const char *Type = "media.insert";
    int noteId = 0;
    QString sourcePath;
    QString title;
    bool returnUrlOnly = false;
};

struct InsertAttachmentCommand {
    static constexpr const char *Type = "attachments.insert";
    int noteId = 0;
    QString sourcePath;
    QString title;
    QString fileName;
    bool returnUrlOnly = false;
};

struct RenameMediaReferencesCommand {
    static constexpr const char *Type = "media.renameReferences";
    QVector<int> noteIds;
    QString oldFileName;
    QString newFileName;
};

struct RenameAttachmentReferencesCommand {
    static constexpr const char *Type = "attachments.renameReferences";
    QVector<int> noteIds;
    QString oldFileName;
    QString newFileName;
};
