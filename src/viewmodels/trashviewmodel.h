#pragma once

#include <models/trashitemmodel.h>

#include <QObject>
#include <QString>

class CommandBus;

class TrashViewModel : public QObject {
    Q_OBJECT

public:
    explicit TrashViewModel(CommandBus* commandBus = nullptr, QObject* parent = nullptr);

    TrashItemModel* model();
    [[nodiscard]] const TrashItemModel* model() const;

    void setCommandBus(CommandBus* commandBus);

public slots:
    void setTrashItems(const QVector<TrashItemData>& trashItems);
    void restoreItem(int trashItemId);
    void clearTrash(bool expiredOnly = false);

signals:
    void commandFailed(const QString& message);

private:
    CommandBus* _commandBus = nullptr;
    TrashItemModel _model;
};
