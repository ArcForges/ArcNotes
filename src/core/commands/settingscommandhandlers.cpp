#include "settingscommandhandlers.h"

#include <core/data/commands.h>
#include <core/repositories/colormoderepository.h>
#include <core/repositories/databaserepository.h>
#include <core/repositories/notefolderrepository.h>
#include <core/repositories/settingsrepository.h>

#include "commandbus.h"

void SettingsCommandHandlers::registerHandlers(CommandBus* bus) {
    bus->registerHandler<SetSettingCommand>([](const SetSettingCommand& command) {
        SettingsRepository().setValue(command.key, command.value);
        return CommandResult::ok();
    });

    bus->registerHandler<RemoveSettingCommand>([](const RemoveSettingCommand& command) {
        SettingsRepository().remove(command.key);
        return CommandResult::ok();
    });

    bus->registerHandler<ClearSettingsCommand>([](const ClearSettingsCommand&) {
        SettingsRepository().clear();
        return CommandResult::ok();
    });

    bus->registerHandler<WriteSettingsArrayCommand>([](const WriteSettingsArrayCommand& command) {
        SettingsRepository().writeArray(command.arrayName, command.values);
        return CommandResult::ok();
    });

    bus->registerHandler<ReinitializeDatabaseCommand>([](const ReinitializeDatabaseCommand&) {
        DatabaseRepository().reinitializeDiskDatabase();
        (void)NoteFolderRepository().migrateToNoteFolders();
        return CommandResult::ok();
    });

    bus->registerHandler<CheckDatabaseIntegrityCommand>([](const CheckDatabaseIntegrityCommand&) {
        return CommandResult::ok(QString(), DatabaseRepository().checkDiskDatabaseIntegrity());
    });

    bus->registerHandler<RemoveDiskDatabaseCommand>([](const RemoveDiskDatabaseCommand&) {
        DatabaseRepository().removeDiskDatabase();
        return CommandResult::ok();
    });

    bus->registerHandler<EnsureBuiltInColorModesCommand>([](const EnsureBuiltInColorModesCommand&) {
        ColorModeRepository().ensureBuiltInModesExist();
        return CommandResult::ok();
    });

    bus->registerHandler<CreateColorModeCommand>([](const CreateColorModeCommand& command) {
        const ColorModeData colorMode = ColorModeRepository().createCustom(command.name);
        return CommandResult::ok(QString(), QVariant::fromValue(colorMode));
    });

    bus->registerHandler<SaveColorModeCommand>([](const SaveColorModeCommand& command) {
        return ColorModeRepository().save(command.colorMode)
                   ? CommandResult::ok()
                   : CommandResult::fail(QStringLiteral("Could not save color mode"));
    });

    bus->registerHandler<RemoveColorModeCommand>([](const RemoveColorModeCommand& command) {
        return ColorModeRepository().remove(command.colorModeId)
                   ? CommandResult::ok()
                   : CommandResult::fail(QStringLiteral("Could not remove color mode"));
    });

    bus->registerHandler<SetCurrentColorModeCommand>([](const SetCurrentColorModeCommand& command) {
        ColorModeRepository().setCurrentId(command.colorModeId);
        return CommandResult::ok();
    });
}
