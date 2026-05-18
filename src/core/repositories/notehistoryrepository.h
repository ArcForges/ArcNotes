#pragma once

#include <core/data/notehistorydata.h>

#include <QVariant>

class NoteHistoryRepository {
public:
    static void registerLegacyMetaTypes();
    static QVariant variantFromItemData(const NoteHistoryItemData& item);
    static NoteHistoryItemData itemDataFromVariant(const QVariant& variant);

    NoteHistoryData restoreForCurrentFolder() const;
    void storeForCurrentFolder(const NoteHistoryData& history) const;
};
