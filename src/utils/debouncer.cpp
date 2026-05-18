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

#include "debouncer.h"

#include <QHash>
#include <QTimer>

Debouncer::Debouncer(QObject* parent) : QObject(parent) {}

Debouncer::~Debouncer() {
    cancelAll();
}

void Debouncer::debounce(const QString& key, std::function<void()> callback, int delayMs) {
    if (key.isEmpty()) {
        return;
    }

    QTimer* timer = _timers.value(key, nullptr);
    if (timer == nullptr) {
        timer = new QTimer(this);
        timer->setSingleShot(true);
        _timers.insert(key, timer);

        QObject::connect(timer, &QTimer::timeout, this, [this, key]() {
            QTimer* finishedTimer = _timers.take(key);
            const std::function<void()> callback = _callbacks.take(key);

            if (finishedTimer != nullptr) {
                finishedTimer->deleteLater();
            }

            if (callback) {
                callback();
            }
        });
    }

    _callbacks.insert(key, std::move(callback));
    timer->start(qMax(0, delayMs));
}

void Debouncer::cancel(const QString& key) {
    QTimer* timer = _timers.take(key);
    _callbacks.remove(key);

    if (timer == nullptr) {
        return;
    }

    timer->stop();
    timer->deleteLater();
}

void Debouncer::cancelAll() {
    const QList<QString> keys = _timers.keys();
    for (const QString& key : keys) {
        cancel(key);
    }
    _callbacks.clear();
}

bool Debouncer::isPending(const QString& key) const {
    return _timers.contains(key);
}
