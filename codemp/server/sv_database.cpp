#include "server.h"
extern "C" {
#include "sqlite3.h"
}

#define DB_FILENAME				"enhanced.db"

namespace DB {
	static sqlite3 *diskDb = NULL;
	static sqlite3 *dbPtr = NULL;

	static void ErrorCallback(void *ctx, int code, const char *msg) {
		Com_Printf("SQL error (code %d): %s\n", code, msg);
	}

	void Load(void)
	{
		if (dbPtr) {
			return;
		}

		// db options

		sqlite3_config(SQLITE_CONFIG_SINGLETHREAD); // we don't need multi threading
		sqlite3_config(SQLITE_CONFIG_MEMSTATUS, 0); // we don't need allocation statistics
		sqlite3_config(SQLITE_CONFIG_LOG, ErrorCallback, NULL); // error logging

		// initialize db

		int rc = sqlite3_initialize();

		if (rc != SQLITE_OK) {
			Com_Error(ERR_DROP, "Failed to initialize SQLite3 (code: %d)\n", rc);
			return;
		}

		Com_Printf("SQLite version: %s\n", sqlite3_libversion());

		rc = sqlite3_open_v2(DB_FILENAME, &diskDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

		if (rc != SQLITE_OK) {
			Com_Error(ERR_DROP, "Failed to open database file " DB_FILENAME " (code: %d)\n", rc);
			return;
		}

		Com_Printf("Successfully opened database file " DB_FILENAME "\n");

		if (g_inMemoryDb->integer) {
			Com_Printf("Using in-memory database\n");

			// open db in memory
			sqlite3 *memoryDb = NULL;
			rc = sqlite3_open_v2(":memory:", &memoryDb, SQLITE_OPEN_READWRITE, NULL);

			if (rc == SQLITE_OK) {
				sqlite3_backup *backup = sqlite3_backup_init(memoryDb, "main", diskDb, "main");
				if (backup) {
					rc = sqlite3_backup_step(backup, -1);
					if (rc == SQLITE_DONE) {
						rc = sqlite3_backup_finish(backup);
						if (rc == SQLITE_OK) {
							dbPtr = memoryDb;
						}
					}
				}
			}

			if (!dbPtr) {
				Com_Printf("ERROR: Failed to load database into memory!\n");
			}
		}

		// use disk db by default in any case
		if (!dbPtr) {
			Com_Printf("Using on-disk database\n");
			dbPtr = diskDb;
		}
	}

	void Unload(void)
	{
		int rc;

		if (dbPtr && diskDb && dbPtr != diskDb) {
			Com_Printf("Saving in-memory database changes to disk\n");

			// we are using in memory db, save changes to disk
			bool success = false;
			sqlite3_backup *backup = sqlite3_backup_init(diskDb, "main", dbPtr, "main");
			if (backup) {
				rc = sqlite3_backup_step(backup, -1);
				if (rc == SQLITE_DONE) {
					rc = sqlite3_backup_finish(backup);
					if (rc == SQLITE_OK) {
						success = true;
					}
				}
			}

			if (!success) {
				Com_Printf("WARNING: Failed to backup in-memory database! Changes from this session have NOT been saved!\n");
			}

			sqlite3_close(dbPtr);
		}

		if (diskDb) {
			sqlite3_close(diskDb);
			diskDb = dbPtr = NULL;
		}
	}

	void *GetDbPtr(void) {
		return dbPtr;
	}

	int PrepareV2(const char *zSql, int nBytes, void **ppStmt, const char **pzTail) {
		return sqlite3_prepare_v2(dbPtr, zSql, nBytes, (sqlite3_stmt **)ppStmt, pzTail);
	}

	int Step(void *stmt) {
		return sqlite3_step((sqlite3_stmt *)stmt);
	}

	int Finalize(void *stmt) {
		return sqlite3_finalize((sqlite3_stmt *)stmt);
	}

	int Exec(const char *sql, int (*callback)(void *, int, char **, char **), void *callbackarg, char **errmsg) {
		return sqlite3_exec(dbPtr, sql, callback, callbackarg, errmsg);
	}
}