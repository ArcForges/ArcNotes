#include "trashcommandhandlers.h"

#include <core/data/commands.h>
#include <core/services/trashservice.h>

#include "commandbus.h"

void TrashCommandHandlers::registerHandlers(CommandBus* bus, TrashService* trashService) {
    bus->registerHandler<RestoreTrashCommand>([trashService](const RestoreTrashCommand& command) {
        bool ok = true;
        for (int trashItemId : command.trashItemIds) {
            ok = trashService->restoreTrashItem(trashItemId) && ok;
        }
        return ok ? CommandResult::ok() : CommandResult::fail(QObject::tr("Trash restore failed."));
    });

    bus->registerHandler<ClearTrashCommand>([trashService](const ClearTrashCommand&) {
        return trashService->clearTrash() ? CommandResult::ok()
                                          : CommandResult::fail(QObject::tr("Trash clear failed."));
    });

    bus->registerHandler<RemoveTrashCommand>([trashService](const RemoveTrashCommand& command) {
        bool ok = true;
        for (int trashItemId : command.trashItemIds) {
            ok = trashService->removeTrashItem(trashItemId) && ok;
        }
        return ok ? CommandResult::ok() : CommandResult::fail(QObject::tr("Trash removal failed."));
    });
}
