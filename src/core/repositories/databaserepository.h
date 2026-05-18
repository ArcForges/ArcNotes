#pragma once

class DatabaseRepository {
public:
    bool reinitializeDiskDatabase() const;
    bool removeDiskDatabase() const;
    bool checkDiskDatabaseIntegrity() const;
};
