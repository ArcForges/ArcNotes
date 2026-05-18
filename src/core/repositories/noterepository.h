#pragma once

#include <core/data/notedata.h>

#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

class NoteRepository {
public:
    [[nodiscard]] NoteData findById(int id, const QString& connectionName = QStringLiteral("memory")) const;
    [[nodiscard]] NoteData findByName(const QString& name, int noteSubFolderId = -1) const;
    [[nodiscard]] NoteData findByName(const QRegularExpression& regExp, int noteSubFolderId = -1) const;
    [[nodiscard]] NoteData findByFileName(const QString& fileName, int noteSubFolderId = -1) const;
    [[nodiscard]] NoteData findByRelativeFileName(const NoteData& currentNote, const QString& fileName) const;
    [[nodiscard]] NoteData findByFileUrl(const QUrl& url) const;
    [[nodiscard]] NoteData findByUrlString(const QString& urlString) const;
    [[nodiscard]] QVector<NoteData> findAll(int limit = -1,
                                            const QString& connectionName = QStringLiteral("memory")) const;
    [[nodiscard]] QVector<NoteData> findBySubFolderId(int noteSubFolderId,
                                                      const QString& connectionName = QStringLiteral("memory")) const;
    [[nodiscard]] QVector<NoteData> search(const QString& text) const;
    [[nodiscard]] QStringList searchAsNameList(const QString& text, bool ignoreNoteSubFolder = false) const;
    [[nodiscard]] QString defaultNoteFileExtension() const;
    [[nodiscard]] QStringList noteFileExtensionList(const QString& prefix = QString()) const;
    [[nodiscard]] bool isWikiLinkSupportEnabled() const;
    [[nodiscard]] bool fileUrlIsNoteInCurrentFolder(const QUrl& url) const;
    [[nodiscard]] bool fileUrlIsExistingNoteInCurrentFolder(const QUrl& url) const;
    [[nodiscard]] QString decodeNoteUrl(QString url) const;
    [[nodiscard]] QString fragmentFromFileName(const QString& fileName) const;
    [[nodiscard]] QString relativePathForFileUrlInCurrentFolder(const QUrl& url) const;
    [[nodiscard]] QVector<int> searchInNotes(const QString& query, bool ignoreNoteSubFolder = false,
                                             int noteSubFolderId = -1,
                                             const QString& connectionName = QStringLiteral("memory")) const;
    [[nodiscard]] bool save(const NoteData& note) const;
    [[nodiscard]] bool remove(int id, bool withFile = false) const;
    [[nodiscard]] bool isFavorite(const NoteData& note) const;
    [[nodiscard]] bool setFavorite(const NoteData& note, bool favorite) const;
    [[nodiscard]] bool toggleFavorite(const NoteData& note) const;
    [[nodiscard]] bool deleteAll() const;
    [[nodiscard]] int countAll() const;
    [[nodiscard]] QVector<int> fetchAllIds(int limit = -1, int offset = -1) const;
};
