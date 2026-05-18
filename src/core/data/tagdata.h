#pragma once

#include <QColor>
#include <QHash>
#include <QMetaType>
#include <QString>

struct TagData {
    int id = 0;
    QString name;
    int parentId = 0;
    int priority = 0;
    QColor color;

    bool operator==(const TagData& other) const {
        return id == other.id && name == other.name && parentId == other.parentId && priority == other.priority &&
               color == other.color;
    }

    bool operator!=(const TagData& other) const { return !(*this == other); }
};

inline size_t qHash(const TagData& tag, size_t seed = 0) {
    return qHash(tag.id, seed);
}

Q_DECLARE_TYPEINFO(TagData, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(TagData)
