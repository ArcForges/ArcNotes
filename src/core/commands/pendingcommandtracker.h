#pragma once

#include <QHash>
#include <QObject>
#include <QString>

class PendingCommandTracker : public QObject {
    Q_OBJECT

public:
    explicit PendingCommandTracker(QObject* parent = nullptr);

    bool isBusy(const QString& commandType) const;
    bool isBusy(const QString& commandType, int targetId) const;
    void markBusy(const QString& commandType, int targetId = 0);
    void clearBusy(const QString& commandType, int targetId = 0);

signals:
    void busyChanged(const QString& commandType, bool busy);
    void targetBusyChanged(const QString& commandType, int targetId, bool busy);

private:
    QString targetKey(const QString& commandType, int targetId) const;

    QHash<QString, int> _busyCounts;
    QHash<QString, int> _targetBusyCounts;
};
