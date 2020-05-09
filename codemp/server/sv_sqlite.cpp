#include "server.h"

#include "sqlite3.h"

void SV_SQLiteInit() {
	int rc;

	rc = sqlite3_initialize();

	if (rc != SQLITE_OK) {
		Com_Printf("(SV_SQLiteInit) sqlite3_initialize: %s\n", sqlite3_errstr(rc));
		return;
	}

	Com_Printf("SQLite version: %s\n", sqlite3_libversion());
}

void SV_SQLiteShutdown() {
	sqlite3_shutdown();
}