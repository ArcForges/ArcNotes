/*
 * Copyright (c) 2014-2026 Patrizio Bekerle -- <patrizio@bekerle.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 */

#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <functional>

class QTimer;

class Debouncer : public QObject {
    Q_OBJECT

public:
    explicit Debouncer(QObject* parent = nullptr);
    ~Debouncer() override;

    void debounce(const QString& key, std::function<void()> callback, int delayMs);
    void cancel(const QString& key);
    void cancelAll();
    bool isPending(const QString& key) const;

private:
    QHash<QString, QTimer*> _timers;
    QHash<QString, std::function<void()>> _callbacks;
};
