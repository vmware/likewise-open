#include "includes.h"

DWORD
SamDbClose(
    HANDLE hDb
    )
{
    DWORD dwError = 0;

    sqlite3* pDbHandle = (sqlite3*)hDb;

    if (pDbHandle) {
       sqlite3_close(pDbHandle);
    }

    return dwError;
}
