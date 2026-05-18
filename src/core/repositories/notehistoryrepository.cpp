#include "notehistoryrepository.h"

#include <core/repositories/notefolderrepository.h>
#include <core/repositories/settingsrepository.h>

#include <QDataStream>
#include <algorithm>
#include <utility>

class NoteHistoryItem {
public:
    NoteHistoryItem() = default;
    explicit NoteHistoryItem(QString noteName, QString noteSubFolderPathData, int cursorPosition,
                             float relativeScrollBarPosition)
        : _noteName(std::move(noteName)),
          _noteSubFolderPathData(std::move(noteSubFolderPathData)),
          _cursorPosition(cursorPosition),
          _relativeScrollBarPosition(relativeScrollBarPosition) {}

    [[nodiscard]] QString noteName() const { return _noteName; }
    [[nodiscard]] QString noteSubFolderPathData() const { return _noteSubFolderPathData; }
    [[nodiscard]] int cursorPosition() const { return _cursorPosition; }
    [[nodiscard]] float relativeScrollBarPosition() const { return _relativeScrollBarPosition; }

    [[nodiscard]] NoteHistoryItemData toData() const {
        NoteHistoryItemData data;
        data.noteName = _noteName;
        data.noteSubFolderPathData = _noteSubFolderPathData;
        data.cursorPosition = _cursorPosition;
        data.relativeScrollBarPosition = _relativeScrollBarPosition;
        return data;
    }

private:
    QString _noteName;
    QString _noteSubFolderPathData;
    int _cursorPosition = 0;
    float _relativeScrollBarPosition = 0.0F;
};

QDataStream& operator<<(QDataStream& out, const NoteHistoryItem& item) {
    out << item.noteName() << item.noteSubFolderPathData() << item.cursorPosition() << item.relativeScrollBarPosition();
    return out;
}

QDataStream& operator>>(QDataStream& in, NoteHistoryItem& item) {
    QString noteName;
    QString noteSubFolderPathData;
    int cursorPosition;
    float relativeScrollBarPosition;

    in >> noteName >> noteSubFolderPathData >> cursorPosition >> relativeScrollBarPosition;
    item = NoteHistoryItem(noteName, noteSubFolderPathData, cursorPosition, relativeScrollBarPosition);
    return in;
}

Q_DECLARE_METATYPE(NoteHistoryItem)

namespace {
QString historyKey(int noteFolderId) {
    return QStringLiteral("NoteHistory-") + QString::number(noteFolderId);
}

QString historyIndexKey(int noteFolderId) {
    return QStringLiteral("NoteHistoryCurrentIndex-") + QString::number(noteFolderId);
}

QVariantMap historyItemToMap(const NoteHistoryItemData& item) {
    QVariantMap map;
    map.insert(QStringLiteral("noteName"), item.noteName);
    map.insert(QStringLiteral("noteSubFolderPathData"), item.noteSubFolderPathData);
    map.insert(QStringLiteral("cursorPosition"), item.cursorPosition);
    map.insert(QStringLiteral("relativeScrollBarPosition"), item.relativeScrollBarPosition);
    return map;
}

NoteHistoryItemData historyItemFromMap(const QVariantMap& map) {
    NoteHistoryItemData item;
    item.noteName = map.value(QStringLiteral("noteName")).toString();
    item.noteSubFolderPathData = map.value(QStringLiteral("noteSubFolderPathData")).toString();
    item.cursorPosition = map.value(QStringLiteral("cursorPosition")).toInt();
    item.relativeScrollBarPosition = map.value(QStringLiteral("relativeScrollBarPosition")).toFloat();
    return item;
}
}  // namespace

void NoteHistoryRepository::registerLegacyMetaTypes() {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qRegisterMetaTypeStreamOperators<NoteHistoryItem>("NoteHistoryItem");
#endif
    qRegisterMetaType<NoteHistoryItem>("NoteHistoryItem");
}

QVariant NoteHistoryRepository::variantFromItemData(const NoteHistoryItemData& item) {
    return historyItemToMap(item);
}

NoteHistoryItemData NoteHistoryRepository::itemDataFromVariant(const QVariant& variant) {
    if (variant.canConvert<NoteHistoryItem>()) {
        return variant.value<NoteHistoryItem>().toData();
    }

    return historyItemFromMap(variant.toMap());
}

NoteHistoryData NoteHistoryRepository::restoreForCurrentFolder() const {
    NoteHistoryData history;
    const int currentNoteFolderId = NoteFolderRepository().currentFolderId();
    const SettingsRepository settings;
    const QVariantList items = settings.value(historyKey(currentNoteFolderId)).toList();

    if (items.isEmpty()) {
        return history;
    }

    int maxIndex = -1;
    for (const QVariant& variant : items) {
        const NoteHistoryItemData item = itemDataFromVariant(variant);
        if (!item.noteName.isEmpty()) {
            history.items.append(item);
            maxIndex++;
        }
    }

    const int newCurrentIndex = settings.value(historyIndexKey(currentNoteFolderId)).toInt();
    if (newCurrentIndex > 0 && newCurrentIndex <= maxIndex) {
        history.currentIndex = newCurrentIndex;
    }

    if (history.items.count() > history.currentIndex) {
        history.currentItem = history.items.at(history.currentIndex);
    }

    return history;
}

void NoteHistoryRepository::storeForCurrentFolder(const NoteHistoryData& historyData) const {
    const int currentNoteFolderId = NoteFolderRepository().currentFolderId();
    const int itemCount = historyData.items.count();
    const int maxCount = std::min<int>(itemCount, 200);

    if (maxCount == 0) {
        return;
    }

    QVariantList items;
    int newCurrentIndex = 0;
    int count = 0;
    for (int i = itemCount - maxCount; i < itemCount; i++) {
        items.append(historyItemToMap(historyData.items.at(i)));
        if (i == historyData.currentIndex) {
            newCurrentIndex = count;
        }
        count++;
    }

    const SettingsRepository settings;
    settings.setValue(historyKey(currentNoteFolderId), items);
    settings.setValue(historyIndexKey(currentNoteFolderId), newCurrentIndex);
}
