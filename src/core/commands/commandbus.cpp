#include "commandbus.h"

#include <QUuid>

#include "pendingcommandtracker.h"

CommandBus::CommandBus(QObject* parent) : QObject(parent), _pendingTracker(new PendingCommandTracker(this)) {}

CommandBus::~CommandBus() = default;

PendingCommandTracker* CommandBus::pendingTracker() const {
    return _pendingTracker;
}

QString CommandBus::nextCommandId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
