#include "colormoderepository.h"

#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QUuid>

#include "settingsrepository.h"

namespace {
const QString kLightModeId = QStringLiteral("ColorMode-light");
const QString kDarkModeId = QStringLiteral("ColorMode-dark");
const QString kLightEditorSchemaKey = QStringLiteral("EditorColorSchema-6033d61b-cb96-46d5-a3a8-20d5172017eb");
const QString kDarkEditorSchemaKey = QStringLiteral("EditorColorSchema-cdbf28fc-1ddc-4d13-bb21-6a4043316a2f");

QString settingsGroupKey(const QString& id) {
    return QStringLiteral("ColorModes/") + id;
}

bool isBuiltInIdValue(const QString& id) {
    return id == kLightModeId || id == kDarkModeId;
}

ColorModeData readMode(const SettingsRepository& settings, const QString& id) {
    const QString group = settingsGroupKey(id);

    ColorModeData mode;
    mode.id = id;
    mode.name = settings.value(group + QStringLiteral("/name")).toString();
    mode.darkMode = settings.value(group + QStringLiteral("/darkMode")).toBool();
    mode.darkModeColors = settings.value(group + QStringLiteral("/darkModeColors")).toBool();
    mode.darkModeTrayIcon = settings.value(group + QStringLiteral("/darkModeTrayIcon")).toBool();
    mode.darkModeIconTheme = settings.value(group + QStringLiteral("/darkModeIconTheme")).toBool();
    mode.internalIconTheme = settings.value(group + QStringLiteral("/internalIconTheme")).toBool();
    mode.systemIconTheme = settings.value(group + QStringLiteral("/systemIconTheme")).toBool();
    mode.editorColorSchemaKey = settings.value(group + QStringLiteral("/editorColorSchemaKey")).toString();
    return mode;
}
}  // namespace

ColorModeData ColorModeRepository::findById(const QString& id) const {
    return readMode(SettingsRepository(), id);
}

QList<ColorModeData> ColorModeRepository::findAll() const {
    const SettingsRepository settings;
    const QStringList ids = settings.value(QStringLiteral("ColorModes/ids")).toStringList();
    QList<ColorModeData> modes;
    modes.reserve(ids.size());
    for (const QString& id : ids) {
        modes.append(readMode(settings, id));
    }
    return modes;
}

ColorModeData ColorModeRepository::current() const {
    const QString id = currentId();
    return findById(id.isEmpty() ? kLightModeId : id);
}

bool ColorModeRepository::save(const ColorModeData& colorMode) const {
    if (colorMode.id.isEmpty()) {
        qWarning() << "Cannot store color mode with empty id";
        return false;
    }

    SettingsRepository settings;
    const QString group = settingsGroupKey(colorMode.id);
    settings.setValue(group + QStringLiteral("/name"), colorMode.name);
    settings.setValue(group + QStringLiteral("/darkMode"), colorMode.darkMode);
    settings.setValue(group + QStringLiteral("/darkModeColors"), colorMode.darkModeColors);
    settings.setValue(group + QStringLiteral("/darkModeTrayIcon"), colorMode.darkModeTrayIcon);
    settings.setValue(group + QStringLiteral("/darkModeIconTheme"), colorMode.darkModeIconTheme);
    settings.setValue(group + QStringLiteral("/internalIconTheme"), colorMode.internalIconTheme);
    settings.setValue(group + QStringLiteral("/systemIconTheme"), colorMode.systemIconTheme);
    settings.setValue(group + QStringLiteral("/editorColorSchemaKey"), colorMode.editorColorSchemaKey);

    QStringList ids = settings.value(QStringLiteral("ColorModes/ids")).toStringList();
    if (!ids.contains(colorMode.id)) {
        ids.append(colorMode.id);
        settings.setValue(QStringLiteral("ColorModes/ids"), ids);
    }

    return true;
}

bool ColorModeRepository::remove(const QString& id) const {
    if (isBuiltInId(id)) {
        qWarning() << "Cannot remove built-in color mode:" << id;
        return false;
    }

    SettingsRepository settings;
    settings.remove(settingsGroupKey(id));

    QStringList ids = settings.value(QStringLiteral("ColorModes/ids")).toStringList();
    ids.removeAll(id);
    settings.setValue(QStringLiteral("ColorModes/ids"), ids);
    return true;
}

QString ColorModeRepository::currentId() const {
    return SettingsRepository().value(QStringLiteral("ColorModes/currentId"), kLightModeId).toString();
}

void ColorModeRepository::setCurrentId(const QString& id) const {
    SettingsRepository().setValue(QStringLiteral("ColorModes/currentId"), id);
}

void ColorModeRepository::ensureBuiltInModesExist() const {
    SettingsRepository settings;
    QStringList ids = settings.value(QStringLiteral("ColorModes/ids")).toStringList();
    const bool firstMigration = ids.isEmpty();

    const bool existingDarkMode = settings.value(QStringLiteral("darkMode")).toBool();
    const bool existingDarkModeColors = settings.value(QStringLiteral("darkModeColors")).toBool();
    const bool existingDarkModeTrayIcon = settings.value(QStringLiteral("darkModeTrayIcon")).toBool();
    const bool existingDarkModeIconTheme =
        settings.value(QStringLiteral("darkModeIconTheme"), existingDarkMode).toBool();
    const bool existingInternalIconTheme = settings.value(QStringLiteral("internalIconTheme")).toBool();
    const bool existingSystemIconTheme = settings.value(QStringLiteral("systemIconTheme")).toBool();
    const QString existingSchemaKey = settings.value(QStringLiteral("Editor/CurrentSchemaKey")).toString();

    if (!ids.contains(kLightModeId)) {
        ColorModeData light;
        light.id = kLightModeId;
        light.name = QObject::tr("Light");
        light.editorColorSchemaKey = kLightEditorSchemaKey;
        if (firstMigration && !existingDarkMode) {
            light.internalIconTheme = existingInternalIconTheme;
            light.systemIconTheme = existingSystemIconTheme;
            if (!existingSchemaKey.isEmpty()) {
                light.editorColorSchemaKey = existingSchemaKey;
            }
        }
        save(light);
    }

    if (!ids.contains(kDarkModeId)) {
        ColorModeData dark;
        dark.id = kDarkModeId;
        dark.name = QObject::tr("Dark");
        dark.darkMode = true;
        dark.darkModeColors = true;
        dark.darkModeTrayIcon = true;
        dark.darkModeIconTheme = true;
        dark.editorColorSchemaKey = kDarkEditorSchemaKey;
        if (firstMigration && existingDarkMode) {
            dark.darkModeColors = existingDarkModeColors;
            dark.darkModeTrayIcon = existingDarkModeTrayIcon;
            dark.darkModeIconTheme = existingDarkModeIconTheme;
            dark.internalIconTheme = existingInternalIconTheme;
            dark.systemIconTheme = existingSystemIconTheme;
            if (!existingSchemaKey.isEmpty()) {
                dark.editorColorSchemaKey = existingSchemaKey;
            }
        }
        save(dark);
    }

    if (settings.value(QStringLiteral("ColorModes/currentId")).toString().isEmpty()) {
        settings.setValue(QStringLiteral("ColorModes/currentId"), existingDarkMode ? kDarkModeId : kLightModeId);
    }
}

ColorModeData ColorModeRepository::createCustom(const QString& name) const {
    ColorModeData mode;
    mode.id =
        QStringLiteral("ColorMode-") + QUuid::createUuid().toString().remove(QLatin1Char('{')).remove(QLatin1Char('}'));
    mode.name = name;
    mode.editorColorSchemaKey = kLightEditorSchemaKey;
    return mode;
}

bool ColorModeRepository::isBuiltInId(const QString& id) const {
    return isBuiltInIdValue(id);
}

QString ColorModeRepository::lightModeId() const {
    return kLightModeId;
}

QString ColorModeRepository::darkModeId() const {
    return kDarkModeId;
}
