#include "settingsrepository.h"

#include <services/settingsservice.h>

bool SettingsRepository::contains(const QString& key) const {
    return SettingsService().contains(key);
}

QStringList SettingsRepository::allKeys() const {
    return SettingsService().allKeys();
}

QVariant SettingsRepository::value(const QString& key, const QVariant& defaultValue) const {
    return SettingsService().value(key, defaultValue);
}

void SettingsRepository::setValue(const QString& key, const QVariant& value) const {
    SettingsService().setValue(key, value);
}

void SettingsRepository::remove(const QString& key) const {
    SettingsService().remove(key);
}

void SettingsRepository::clear() const {
    SettingsService().clear();
}

QVector<QVariantMap> SettingsRepository::arrayValues(const QString& arrayName, const QStringList& keys) const {
    SettingsService settings;
    QVector<QVariantMap> values;
    const int count = settings.beginReadArray(arrayName);
    values.reserve(count);
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        QVariantMap item;
        for (const QString& key : keys) {
            item.insert(key, settings.value(key));
        }
        values.append(item);
    }
    settings.endArray();
    return values;
}

void SettingsRepository::writeArray(const QString& arrayName, const QVector<QVariantMap>& values) const {
    SettingsService settings;
    settings.beginWriteArray(arrayName, values.size());
    for (int i = 0; i < values.size(); ++i) {
        settings.setArrayIndex(i);
        const QVariantMap& item = values.at(i);
        for (auto it = item.cbegin(); it != item.cend(); ++it) {
            settings.setValue(it.key(), it.value());
        }
    }
    settings.endArray();
}
