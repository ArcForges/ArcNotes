#pragma once

class CommandBus;

class SettingsCommandHandlers {
public:
    static void registerHandlers(CommandBus* bus);
};
