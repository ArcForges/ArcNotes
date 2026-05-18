#pragma once

#include <core/data/colormodedata.h>

#include <QList>
#include <QString>

class ColorModeRepository {
public:
    [[nodiscard]] ColorModeData findById(const QString& id) const;
    [[nodiscard]] QList<ColorModeData> findAll() const;
    [[nodiscard]] ColorModeData current() const;
    [[nodiscard]] bool save(const ColorModeData& colorMode) const;
    [[nodiscard]] bool remove(const QString& id) const;
    [[nodiscard]] QString currentId() const;
    void setCurrentId(const QString& id) const;
    void ensureBuiltInModesExist() const;
    [[nodiscard]] ColorModeData createCustom(const QString& name) const;
    [[nodiscard]] bool isBuiltInId(const QString& id) const;
    [[nodiscard]] QString lightModeId() const;
    [[nodiscard]] QString darkModeId() const;
};
