#include "searchservice.h"

#include <QRegularExpression>

SearchService::SearchService(NoteRepository* noteRepository)
    : _noteRepository(noteRepository != nullptr ? noteRepository : &_ownedNoteRepository) {}

namespace {
QString removeNameSearchPrefix(QString searchTerm) {
    static const QRegularExpression re(QStringLiteral("^(name:|n:)"));
    return searchTerm.remove(re);
}
}  // namespace

QVector<NoteData> SearchService::searchNotes(const QString& text) const {
    return _noteRepository->search(text);
}

QVector<int> SearchService::searchInNotes(const QString& query, bool ignoreNoteSubFolder, int noteSubFolderId) const {
    return _noteRepository->searchInNotes(query, ignoreNoteSubFolder, noteSubFolderId);
}

QStringList SearchService::buildQueryStringList(QString searchString, bool escapeForRegularExpression,
                                                bool removeSearchPrefix) const {
    QStringList queryStrings;

    static const QRegularExpression re(QStringLiteral("\"([^\"]+)\""));
    QRegularExpressionMatchIterator iterator = re.globalMatch(searchString);
    while (iterator.hasNext()) {
        const QRegularExpressionMatch match = iterator.next();
        QString text = match.captured(1);

        if (escapeForRegularExpression) {
            text = QRegularExpression::escape(text);
        }

        queryStrings.append(text);
        searchString.remove(match.captured(0));
    }

    searchString.remove(QChar('"'));
    searchString = searchString.simplified();

    const QStringList searchStringList = searchString.split(QChar(' '));
    queryStrings.reserve(queryStrings.size() + searchStringList.size());
    for (QString text : searchStringList) {
        if (removeSearchPrefix && isNameSearch(text)) {
            text = removeNameSearchPrefix(text);
        }

        queryStrings.append(escapeForRegularExpression ? QRegularExpression::escape(text) : text);
    }

    queryStrings.removeAll(QLatin1String(""));
    queryStrings.removeDuplicates();

    return queryStrings;
}

bool SearchService::isNameSearch(const QString& searchTerm) const {
    return searchTerm.startsWith(QStringLiteral("name:")) || searchTerm.startsWith(QStringLiteral("n:"));
}
