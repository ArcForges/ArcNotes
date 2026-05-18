#include "databaserepository.h"

#include <services/databaseservice.h>

bool DatabaseRepository::reinitializeDiskDatabase() const {
    return DatabaseService::reinitializeDiskDatabase();
}

bool DatabaseRepository::removeDiskDatabase() const {
    return DatabaseService::removeDiskDatabase();
}

bool DatabaseRepository::checkDiskDatabaseIntegrity() const {
    return DatabaseService::checkDiskDatabaseIntegrity();
}
