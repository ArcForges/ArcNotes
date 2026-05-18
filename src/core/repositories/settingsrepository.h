#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QVector>

class SettingsRepository {
public:
    [[nodiscard]] bool contains(const QString& key) const;
    [[nodiscard]] QStringList allKeys() const;
    [[nodiscard]] QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value) const;
    void remove(const QString& key) const;
    void clear() const;
    [[nodiscard]] QVector<QVariantMap> arrayValues(const QString& arrayName, const QStringList& keys) const;
    void writeArray(const QString& arrayName, const QVector<QVariantMap>& values) const;
};
