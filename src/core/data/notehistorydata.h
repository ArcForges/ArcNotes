#pragma once

#include <QList>
#include <QMetaType>
#include <QString>

struct NoteHistoryItemData {
    QString noteName;
    QString noteSubFolderPathData;
    int cursorPosition = 0;
    float relativeScrollBarPosition = 0.0F;

    bool operator==(const NoteHistoryItemData& other) const {
        return noteName == other.noteName && noteSubFolderPathData == other.noteSubFolderPathData &&
               cursorPosition == other.cursorPosition && relativeScrollBarPosition == other.relativeScrollBarPosition;
    }

    bool operator!=(const NoteHistoryItemData& other) const { return !(*this == other); }
};

struct NoteHistoryData {
    QList<NoteHistoryItemData> items;
    int currentIndex = 0;
    NoteHistoryItemData currentItem;

    bool operator==(const NoteHistoryData& other) const {
        return items == other.items && currentIndex == other.currentIndex && currentItem == other.currentItem;
    }

    bool operator!=(const NoteHistoryData& other) const { return !(*this == other); }
};

Q_DECLARE_METATYPE(NoteHistoryItemData)
Q_DECLARE_METATYPE(NoteHistoryData)
