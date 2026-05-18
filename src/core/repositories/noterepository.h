#pragma once

#include <core/data/notedata.h>

#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

class NoteRepository {
public:
    NoteData findById(int id, const QString& connectionName = QStringLiteral("memory")) const;
    NoteData findByName(const QString& name, int noteSubFolderId = -1) const;
    NoteData findByName(const QRegularExpression& regExp, int noteSubFolderId = -1) const;
    NoteData findByFileName(const QString& fileName, int noteSubFolderId = -1) const;
    NoteData findByRelativeFileName(const NoteData& currentNote, const QString& fileName) const;
    NoteData findByFileUrl(const QUrl& url) const;
    NoteData findByUrlString(const QString& urlString) const;
    QVector<NoteData> findAll(int limit = -1, const QString& connectionName = QStringLiteral("memory")) const;
    QVector<NoteData> findBySubFolderId(int noteSubFolderId,
                                        const QString& connectionName = QStringLiteral("memory")) const;
    QVector<NoteData> search(const QString& text) const;
    QStringList searchAsNameList(const QString& text, bool ignoreNoteSubFolder = false) const;
    QString defaultNoteFileExtension() const;
    QStringList noteFileExtensionList(const QString& prefix = QString()) const;
    bool isWikiLinkSupportEnabled() const;
    bool fileUrlIsNoteInCurrentFolder(const QUrl& url) const;
    bool fileUrlIsExistingNoteInCurrentFolder(const QUrl& url) const;
    QString decodeNoteUrl(QString url) const;
    QString fragmentFromFileName(const QString& fileName) const;
    QString relativePathForFileUrlInCurrentFolder(const QUrl& url) const;
    QVector<int> searchInNotes(const QString& query, bool ignoreNoteSubFolder = false, int noteSubFolderId = -1,
                               const QString& connectionName = QStringLiteral("memory")) const;
    bool save(const NoteData& note) const;
    bool remove(int id, bool withFile = false) const;
    bool isFavorite(const NoteData& note) const;
    bool setFavorite(const NoteData& note, bool favorite) const;
    bool toggleFavorite(const NoteData& note) const;
    bool deleteAll() const;
    int countAll() const;
    QVector<int> fetchAllIds(int limit = -1, int offset = -1) const;
};
