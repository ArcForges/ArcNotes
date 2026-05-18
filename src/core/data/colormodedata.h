#pragma once

#include <QHash>
#include <QMetaType>
#include <QString>

struct ColorModeData {
    QString id;
    QString name;
    bool darkMode = false;
    bool darkModeColors = false;
    bool darkModeTrayIcon = false;
    bool darkModeIconTheme = false;
    bool internalIconTheme = false;
    bool systemIconTheme = false;
    QString editorColorSchemaKey;

    bool operator==(const ColorModeData& other) const {
        return id == other.id && name == other.name && darkMode == other.darkMode &&
               darkModeColors == other.darkModeColors && darkModeTrayIcon == other.darkModeTrayIcon &&
               darkModeIconTheme == other.darkModeIconTheme && internalIconTheme == other.internalIconTheme &&
               systemIconTheme == other.systemIconTheme && editorColorSchemaKey == other.editorColorSchemaKey;
    }

    bool operator!=(const ColorModeData& other) const { return !(*this == other); }
};

inline size_t qHash(const ColorModeData& colorMode, size_t seed = 0) {
    return qHash(colorMode.id, seed);
}

Q_DECLARE_TYPEINFO(ColorModeData, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(ColorModeData)
