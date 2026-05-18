#include "pendingcommandtracker.h"

PendingCommandTracker::PendingCommandTracker(QObject* parent) : QObject(parent) {}

bool PendingCommandTracker::isBusy(const QString& commandType) const {
    return _busyCounts.value(commandType) > 0;
}

bool PendingCommandTracker::isBusy(const QString& commandType, int targetId) const {
    return _targetBusyCounts.value(targetKey(commandType, targetId)) > 0;
}

void PendingCommandTracker::markBusy(const QString& commandType, int targetId) {
    const bool wasBusy = isBusy(commandType);
    _busyCounts[commandType] = _busyCounts.value(commandType) + 1;
    if (!wasBusy) {
        emit busyChanged(commandType, true);
    }

    if (targetId == 0) {
        return;
    }

    const QString key = targetKey(commandType, targetId);
    const bool targetWasBusy = isBusy(commandType, targetId);
    _targetBusyCounts[key] = _targetBusyCounts.value(key) + 1;
    if (!targetWasBusy) {
        emit targetBusyChanged(commandType, targetId, true);
    }
}

void PendingCommandTracker::clearBusy(const QString& commandType, int targetId) {
    const int count = qMax(0, _busyCounts.value(commandType) - 1);
    if (count == 0) {
        _busyCounts.remove(commandType);
        emit busyChanged(commandType, false);
    } else {
        _busyCounts.insert(commandType, count);
    }

    if (targetId == 0) {
        return;
    }

    const QString key = targetKey(commandType, targetId);
    const int targetCount = qMax(0, _targetBusyCounts.value(key) - 1);
    if (targetCount == 0) {
        _targetBusyCounts.remove(key);
        emit targetBusyChanged(commandType, targetId, false);
    } else {
        _targetBusyCounts.insert(key, targetCount);
    }
}

QString PendingCommandTracker::targetKey(const QString& commandType, int targetId) const {
    return commandType + QLatin1Char(':') + QString::number(targetId);
}
