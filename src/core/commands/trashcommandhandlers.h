#pragma once

class CommandBus;
class TrashService;

class TrashCommandHandlers {
public:
    static void registerHandlers(CommandBus* bus, TrashService* trashService);
};
