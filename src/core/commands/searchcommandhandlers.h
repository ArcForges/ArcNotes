#pragma once

class CommandBus;
class SearchState;
class SearchService;

class SearchCommandHandlers {
public:
    static void registerHandlers(CommandBus* bus, SearchService* searchService, SearchState* searchState = nullptr);
};
