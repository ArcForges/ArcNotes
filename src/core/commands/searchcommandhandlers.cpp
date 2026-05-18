#include "searchcommandhandlers.h"

#include <core/data/commands.h>
#include <core/services/searchservice.h>
#include <core/state/searchstate.h>

#include "commandbus.h"

void SearchCommandHandlers::registerHandlers(CommandBus* bus, SearchService* searchService, SearchState* searchState) {
    bus->registerHandler<SearchNotesCommand>([searchService, searchState](const SearchNotesCommand& command) {
        if (searchState != nullptr) {
            searchState->setQuery(command.query);
            searchState->setSearchMode(command.contentSearch ? QStringLiteral("content") : QStringLiteral("name"));
            searchState->setSearching(true);
        }
        const QVector<int> noteIds =
            searchService->searchInNotes(command.query, command.subFolderId < 0, command.subFolderId);
        if (searchState != nullptr) {
            searchState->setResultNoteIds(noteIds);
            searchState->setSearching(false);
        }
        return CommandResult::ok(QString(), QVariant::fromValue(noteIds));
    });
}
