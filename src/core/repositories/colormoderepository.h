#pragma once

#include <core/data/colormodedata.h>

#include <QList>
#include <QString>

class ColorModeRepository {
public:
    ColorModeData findById(const QString& id) const;
    QList<ColorModeData> findAll() const;
    ColorModeData current() const;
    bool save(const ColorModeData& colorMode) const;
    bool remove(const QString& id) const;
    QString currentId() const;
    void setCurrentId(const QString& id) const;
    void ensureBuiltInModesExist() const;
    ColorModeData createCustom(const QString& name) const;
    bool isBuiltInId(const QString& id) const;
    QString lightModeId() const;
    QString darkModeId() const;
};
