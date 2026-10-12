/* FreeBASIC SQLite 3 native qualification
 * File: sqlite3-native.c
 * Purpose: Establish native SQL NULL, empty text and error ownership results.
 * Responsibilities: Actual callbacks, SQLite allocation cleanup and connection close.
 * This file contains no substitute SQL implementation or persistent database.
 */
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

static int inspect(void *data, int count, char **values, char **names)
{
    int *rows = data;
    if (count != 3 || !values || !names || values[0] || !values[1] || *values[1] ||
        !values[2] || strcmp(values[2], "alive")) return 1;
    (*rows)++;
    return 0;
}
int main(void)
{
    sqlite3 *database = NULL;
    char *message = NULL;
    sqlite3_int64 baseline = sqlite3_memory_used();
    int rows = 0, status = 1;
    if (sqlite3_open(":memory:", &database) != SQLITE_OK) goto cleanup;
    if (sqlite3_exec(database, "select NULL as n,'' as e,'alive' as v", inspect, &rows, &message) != SQLITE_OK || rows != 1) goto cleanup;
    if (sqlite3_exec(database, "select * from missing_table", NULL, NULL, &message) != SQLITE_ERROR || !message) goto cleanup;
    sqlite3_free(message); message = NULL;
    status = 0;
cleanup:
    if (message) sqlite3_free(message);
    if (database && sqlite3_close(database) != SQLITE_OK) status = 2;
    if (sqlite3_memory_used() != baseline) status = 3;
    if (!status) puts("SQLITE3_NATIVE=PASS null=1 empty=1 text=1 error=1 cleanup=1");
    return status;
}
/* end of sqlite3-native.c */
