/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Eventlog Server API Implementation
 *
 */

#include "includes.h"

#define DB_QUERY_CREATE_EVENTS_TABLE "CREATE TABLE lwievents               \
                         (EventRecordId integer PRIMARY KEY AUTOINCREMENT, \
                            EventTableCategoryId   varchar(128),            \
                            EventType     varchar(128),                      \
                            EventDateTime     integer,                       \
                            EventSource   varchar(128),                      \
                            EventCategory varchar(128),                      \
                            EventSourceId      integer,                      \
                            User          varchar(128),                      \
                            Computer      varchar(128),                      \
                            Description   TEXT,                              \
                            Data          varchar(128)                       \
                         )"


#define DB_QUERY_CREATE_UNIQUE_INDEX "CREATE UNIQUE INDEX lwindex_%s ON lwievents(%s)"

#define DB_QUERY_CREATE_INDEX "CREATE INDEX lwindex_%s ON lwievents(%s)"

#define DB_QUERY_ALL_WITH_LIMIT "SELECT EventRecordId,    \
                                    EventTableCategoryId, \
                                    EventType,            \
                                    EventDateTime,        \
                                    EventSource,          \
                                    EventCategory,        \
                                    EventSourceId,        \
                                    User,                 \
                                    Computer,             \
                                    Description,          \
                                    Data                  \
                             FROM     lwievents           \
                             ORDER BY EventRecordId ASC  \
                             LIMIT %ld OFFSET %ld"



#define DB_QUERY_WITH_LIMIT "SELECT EventRecordId,        \
                                    EventTableCategoryId, \
                                    EventType,            \
                                    EventDateTime,        \
                                    EventSource,          \
                                    EventCategory,        \
                                    EventSourceId,        \
                                    User,                 \
                                    Computer,             \
                                    Description,          \
                                    Data                  \
                             FROM     lwievents           \
                             WHERE  (%s)                  \
                             ORDER BY EventRecordId ASC  \
                             LIMIT %ld OFFSET %ld"

#define DB_QUERY_COUNT_ALL  "SELECT COUNT(EventRecordId)  \
                             FROM     lwievents"

#define DB_QUERY_COUNT      "SELECT COUNT(EventRecordId)  \
                             FROM     lwievents           \
                             WHERE  (%s)"

#define DB_QUERY_DROP_EVENTS_TABLE "DROP TABLE lwievents"

#define DB_QUERY_DELETE     "DELETE FROM     lwievents    \
                             WHERE  (%s)"

#define DB_QUERY_INSERT_EVENT "INSERT INTO lwievents         \
                                     (EventRecordId,         \
                                        EventTableCategoryId,  \
                                        EventType,             \
                                        EventDateTime,         \
                                        EventSource,           \
                                        EventCategory,         \
                                        EventSourceId,         \
                                        User,                  \
                                        Computer,              \
                                        Description,           \
                                        Data                   \
                                     )                         \
                                VALUES( NULL,                  \
                                        %Q,                    \
                                        %Q,                    \
                                        %d,                    \
                                        %Q,                    \
                                        %Q,                    \
                                        %d,                    \
                                        %Q,                    \
                                        %Q,                    \
                                        %Q,                    \
                                        %Q)"

//Delete the record over 'n' entries
#define DB_QUERY_DELETE_ABOVE_LIMIT "DELETE FROM     lwievents    \
                                     WHERE  EventRecordId > (%d)"

//To delete records 'n' days older than current date.
#define DB_QUERY_DELETE_OLDER_THAN  "DELETE FROM     lwievents    \
                                     WHERE  (datetime(EventDateTime,'unixepoch','localtime') < datetime('now','-%d day'))"

//To get count of records that are older  than 'n' days
#define DB_QUERY_COUNT_OLDER_THAN  "SELECT COUNT (EventRecordId) FROM  lwievents    \
                                     WHERE  (datetime(EventDateTime,'unixepoch','localtime') < datetime('now','-%d day'))"

//To sort the record depending upon the date
#define DB_QUERY_SORT_ON_DATE       "SELECT (*) FROM  lwievents    \
                                     ORDER BY EventDateTime desc "

//To get records 'n' days older than current date
#define DB_QUERY_GET_OLDER_THAN  "SELECT (*) FROM  lwievents    \
                                    WHERE  (datetime(EventDateTime,'unixepoch','localtime') < datetime('now','-%d day'))"
//Function prototype
static
DWORD
BuildEventLogRecordList(
    PSTR *,
    DWORD ,
    DWORD ,
    EVENT_LOG_RECORD** 
    );

static
DWORD
SrvCheckSqlFilter(
    PSTR pszFilter
    );

//public interface

DWORD
SrvInitEventDatabase()
{
    pthread_rwlock_init(&g_dbLock, NULL);
    return 0;
}

DWORD
SrvShutdownEventDatabase()
{
    return 0;
}

DWORD
SrvOpenEventDatabase(
    PHANDLE phDB
    )
{
    DWORD dwError = 0;
    PEVENTLOG_CONTEXT pEventLogCtx = NULL;
    sqlite3* pSqliteHandle = NULL;

    dwError = sqlite3_open(EVENTLOG_DB, &pSqliteHandle);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAllocateMemory(sizeof(EVENTLOG_CONTEXT),
                                (PVOID*)&pEventLogCtx);
    BAIL_ON_EVT_ERROR(dwError);

    pEventLogCtx->pDbHandle = pSqliteHandle;
    pSqliteHandle = NULL;

    *phDB = (HANDLE)pEventLogCtx;

    return dwError;

    error:

    if (pSqliteHandle)
        sqlite3_close(pSqliteHandle);

    if (pEventLogCtx)
        EVTFreeMemory(pEventLogCtx);

    return dwError;
}

DWORD
SrvCloseEventDatabase(
    HANDLE hDB
    )
{
    DWORD dwError =0;
    PEVENTLOG_CONTEXT pContext = (PEVENTLOG_CONTEXT)(hDB);

    if (pContext) {

        if (pContext->pDbHandle != NULL) {
            sqlite3_close(pContext->pDbHandle);
        }
        EVT_LOG_VERBOSE("Freeing the context.................");
        EVTFreeMemory(pContext);
    }

    return dwError;
}

static
DWORD
SrvCheckSqlFilter(
    PSTR pszFilter
    )
{
    enum {
        COMMAND,
        SINGLE_QUOTE,
        DOUBLE_QUOTE,
        BACK_QUOTE,
    } mode = COMMAND;
    DWORD dwError = 0;
    DWORD dwIndex = 0;

    while (pszFilter[dwIndex])
    {
        switch(mode)
        {
            case COMMAND:
                switch (pszFilter[dwIndex])
                {
                    case ';':
                        dwError = EINVAL;
                        BAIL_ON_EVT_ERROR(dwError);
                        break;
                    case '\'':
                        mode = SINGLE_QUOTE;
                        break;
                    case '\"':
                        mode = DOUBLE_QUOTE;
                        break;
                    case '`':
                        mode = BACK_QUOTE;
                        break;
                }
                break;
            case SINGLE_QUOTE:
                switch (pszFilter[dwIndex])
                {
                    case '\'':
                        mode = COMMAND;
                        break;
                }
                break;
            case DOUBLE_QUOTE:
                switch (pszFilter[dwIndex])
                {
                    case '\"':
                        mode = COMMAND;
                        break;
                }
                break;
            case BACK_QUOTE:
                switch (pszFilter[dwIndex])
                {
                    case '`':
                        mode = COMMAND;
                        break;
                }
                break;
        }
        dwIndex++;
    }

    if (mode != COMMAND)
    {
        dwError = EINVAL;
        BAIL_ON_EVT_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
SrvEventLogCount(
    HANDLE hDB,
    PSTR sqlFilter,
    PDWORD pdwNumMatched
    )
{

    DWORD dwError = 0;
    PSTR  pszQuery = NULL;
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;
    ENTER_RW_READER_LOCK;

    if (sqlFilter == NULL) {
        dwError = EVTAllocateStringPrintf(
                        &pszQuery,
                        DB_QUERY_COUNT_ALL);
        BAIL_ON_EVT_ERROR(dwError);
    }
    else {
        dwError = SrvCheckSqlFilter(sqlFilter);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = EVTAllocateStringPrintf(
                        &pszQuery,
                        DB_QUERY_COUNT,
                        sqlFilter);
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = SrvQueryEventLog(
                    hDB,
                    pszQuery,
                    &nRows,
                    &nCols,
                    &ppszResult);
    BAIL_ON_EVT_ERROR(dwError);

    if (nRows == 1)
    {
        *pdwNumMatched = (DWORD) atoi(ppszResult[1]);
    }
    else
    {
        EVT_LOG_VERBOSE("Could not find count of event logs in database");
        dwError = EINVAL;
        BAIL_ON_EVT_ERROR(dwError);
    }

 cleanup:
    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }
    EVT_SAFE_FREE_STRING(pszQuery);

    LEAVE_RW_READER_LOCK;
    return dwError;

 error:
    *pdwNumMatched = 0;
    goto cleanup;
}

DWORD
SrvReadEventLog(
    HANDLE hDB,
    DWORD  dwStartingRowId,
    DWORD  nRecordsPerPage,
    PSTR   sqlFilter,
    PDWORD  pdwNumReturned,
    EVENT_LOG_RECORD** eventRecords
    )
{

    DWORD dwError = 0;
    PSTR  pszQuery = NULL;
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;
    ENTER_RW_READER_LOCK;

    if (IsNullOrEmptyString(sqlFilter)) {
        dwError = EVTAllocateStringPrintf(
                        &pszQuery,
                        DB_QUERY_ALL_WITH_LIMIT,
                        (long)nRecordsPerPage,
                        (long)dwStartingRowId);
        BAIL_ON_EVT_ERROR(dwError);
    }
    else
    {
        dwError = SrvCheckSqlFilter(sqlFilter);
        BAIL_ON_EVT_ERROR(dwError);

        dwError = EVTAllocateStringPrintf(
                        &pszQuery,
                        DB_QUERY_WITH_LIMIT,
                        sqlFilter,
                        (long)nRecordsPerPage,
                        (long)dwStartingRowId);
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = SrvQueryEventLog(hDB, pszQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_EVT_ERROR(dwError);

    if (nRows > 0) {

        dwError = BuildEventLogRecordList(ppszResult,
                                            nRows,
                                            nCols,
                                            eventRecords);
        BAIL_ON_EVT_ERROR(dwError);


    } else {

        EVT_LOG_VERBOSE("No event logs found in the database");
    }
    *pdwNumReturned = nRows;


 cleanup:
    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }
    EVT_SAFE_FREE_STRING(pszQuery);
    LEAVE_RW_READER_LOCK;
    return dwError;
 error:
    *pdwNumReturned = 0;
    *eventRecords = NULL;
    goto cleanup;

}


DWORD
SrvWriteEventLog(
    HANDLE hDB,
    PEVENT_LOG_RECORD pEventRecord
    )
{

    EVT_LOG_VERBOSE("server::evtdb.c SrvWriteEventLog(pszComputer=%s, hDB=%.16X)\n",
                    (IsNullOrEmptyString(pEventRecord->pszComputer) ? "" : pEventRecord->pszComputer), hDB);

    DWORD dwError = 0;
    PSTR pszQuery = NULL;    
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;

    ENTER_RW_WRITER_LOCK;
    
    pszQuery = sqlite3_mprintf(
               DB_QUERY_INSERT_EVENT,
               IsNullOrEmptyString(pEventRecord->pszEventTableCategoryId) ? "" : pEventRecord->pszEventTableCategoryId,
               IsNullOrEmptyString(pEventRecord->pszEventType) ? "" : pEventRecord->pszEventType,
               pEventRecord->dwEventDateTime,
               IsNullOrEmptyString(pEventRecord->pszEventSource) ? "" : pEventRecord->pszEventSource,
               IsNullOrEmptyString(pEventRecord->pszEventCategory) ? "" : pEventRecord->pszEventCategory,
               pEventRecord->dwEventSourceId,
               IsNullOrEmptyString(pEventRecord->pszUser) ? "" : pEventRecord->pszUser,
               IsNullOrEmptyString(pEventRecord->pszComputer) ? "" : pEventRecord->pszComputer,
               IsNullOrEmptyString(pEventRecord->pszDescription) ? "" : pEventRecord->pszDescription,
               IsNullOrEmptyString(pEventRecord->pszData) ? "" : pEventRecord->pszData);
    if (!pszQuery)
    {
        dwError = ENOMEM;
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = SrvQueryEventLog(hDB, pszQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_EVT_ERROR(dwError);

    EVT_LOG_VERBOSE("server::evtdb.c SrvWriteEventLog() finished\n");

 cleanup: 
    
    if (pszQuery){
        sqlite3_free(pszQuery);
    }
    
    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }
    LEAVE_RW_WRITER_LOCK;
    
    return dwError;
 error:
    goto cleanup;

}

/* A routine to trim the database*/
static
DWORD
SrvMaintainDB(
    HANDLE hDB,
    PBOOLEAN pbSafeInsert
    )
{
    DWORD dwError = 0;
    DWORD dwRecordCount = 0;
    DWORD dwCountOlderThan = 0;
    DWORD dwMaxRecords = 0;
    DWORD dwMaxAge = 0;
    DWORD dwMaxLogSize = 0;
    DWORD dwActualSize = 0;

    EVT_LOG_VERBOSE("In Maintain DB ...............");
    //Get Max records,max age and max log size from the global list
    dwError = EVTGetMaxRecords(&dwMaxRecords);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTGetMaxAge(&dwMaxAge);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTGetMaxLogSize(&dwMaxLogSize);
    BAIL_ON_EVT_ERROR(dwError);

    //Get the count of the records
    dwError = SrvEventLogCount(hDB, NULL, &dwRecordCount);
    BAIL_ON_EVT_ERROR(dwError);

    //Get the count of the records which are older than 'n' days
    dwError = SrvEventLogCountOlderThan(hDB, dwMaxAge, &dwCountOlderThan);
    BAIL_ON_EVT_ERROR(dwError);

    EVT_LOG_VERBOSE("EventLog record count older than curdate = %d",dwCountOlderThan);
    EVT_LOG_VERBOSE("EventLog Record count = %d",dwRecordCount);

    //Get the File size
    dwError = EVTGetFileSize(EVENTLOG_DB,&dwActualSize);
    BAIL_ON_EVT_ERROR(dwError);

    //Regular house keeping
    if (dwCountOlderThan) {
        EVT_LOG_VERBOSE("Deleting the record as older than current date");
        dwError = SrvDeleteOlderThanCurDate(hDB, dwMaxAge);
        BAIL_ON_EVT_ERROR(dwError);

        //Since already some records got deleted get the latest record count
        dwError = SrvEventLogCount(hDB, NULL, &dwRecordCount);
        BAIL_ON_EVT_ERROR(dwError);
    }

    EVT_LOG_VERBOSE("Record Count = %d ",dwCountOlderThan);
    //If the record count is greater than the Max Records
    if (dwRecordCount > dwMaxRecords) {

        EVT_LOG_VERBOSE("DB size exceeds the Max Records Size");
        EVT_LOG_VERBOSE("Going to trim DB.........");

        //If records not aged out,then delete the records above Max Records
        dwError = SrvDeleteAboveLimitFromEventLog(hDB, dwMaxRecords);
        BAIL_ON_EVT_ERROR(dwError);
    }

    EVT_LOG_VERBOSE("Actual Log size = %d ",dwActualSize);
    if(dwActualSize >= dwMaxLogSize) {
        EVT_LOG_VERBOSE("Log Size is exceeds the maximum limit set");

        goto error;
    }

    //Safe to insert record,set bSafeInsert to TRUE
    *pbSafeInsert = TRUE;

    EVT_LOG_VERBOSE("Pruned DB returning");

cleanup:

	return dwError;

error:

    *pbSafeInsert = FALSE;
    goto cleanup;
}

DWORD
SrvWriteToDB(
    HANDLE hDB,
    PEVENT_LOG_RECORD pEventRecord
    )
{
    DWORD dwError = 0;
    BOOLEAN bSafeInsert = TRUE;

    //Trim the DB
    EVT_LOG_VERBOSE("Going to trim database .....");
    dwError = SrvMaintainDB(hDB,&bSafeInsert);
    BAIL_ON_EVT_ERROR(dwError);

    if(bSafeInsert) {
        //Write the eventlog
        dwError = SrvWriteEventLog(hDB,pEventRecord);
        BAIL_ON_EVT_ERROR(dwError);
    }

error:

    return dwError;

}


DWORD
SrvClearEventLog(
    HANDLE hDB
    )
{
    DWORD dwError = 0;
    CHAR  szQuery[8092];
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;
    ENTER_RW_WRITER_LOCK;


    sprintf(szQuery, DB_QUERY_DROP_EVENTS_TABLE);

    dwError = SrvQueryEventLog(hDB, szQuery, &nRows, &nCols, &ppszResult);
    if (dwError != 1) BAIL_ON_EVT_ERROR(dwError);

    sprintf(szQuery, DB_QUERY_CREATE_EVENTS_TABLE);
    dwError = SrvQueryEventLog(hDB, szQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_EVT_ERROR(dwError);

    sprintf(szQuery, DB_QUERY_CREATE_UNIQUE_INDEX, "recordId", "EventRecordId");
    dwError = SrvQueryEventLog(hDB, szQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_EVT_ERROR(dwError);

    sprintf(szQuery, DB_QUERY_CREATE_INDEX, "tableCategoryId", "EventTableCategoryId");
    dwError = SrvQueryEventLog(hDB, szQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_EVT_ERROR(dwError);

    sprintf(szQuery, DB_QUERY_CREATE_INDEX, "dateTime", "EventDateTime");
    dwError = SrvQueryEventLog(hDB, szQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_EVT_ERROR(dwError);

 cleanup:
    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }
    LEAVE_RW_WRITER_LOCK;
    return dwError;
 error:
    goto cleanup;

}

DWORD
SrvDeleteFromEventLog(
    HANDLE hDB,
    PSTR sqlFilter
    )
{
    DWORD dwError = 0;
    PSTR  pszQuery = NULL;
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;
    ENTER_RW_WRITER_LOCK;

    if (sqlFilter == NULL) {
        goto cleanup;
    }

    dwError = SrvCheckSqlFilter(sqlFilter);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTAllocateStringPrintf(
                    &pszQuery,
                    DB_QUERY_DELETE,
                    sqlFilter);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = SrvQueryEventLog(hDB, pszQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_EVT_ERROR(dwError);

 cleanup:
    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }
    EVT_SAFE_FREE_STRING(pszQuery);
    LEAVE_RW_WRITER_LOCK;
    return dwError;
 error:
    goto cleanup;

}

//To get count of records that are older  than 'n' days.
DWORD
SrvEventLogCountOlderThan(
    HANDLE hDB,
    DWORD dwOlderThan,
    PDWORD pdwNumMatched
    )
{

    DWORD dwError = 0;
    CHAR  szQuery[8092];
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;
    ENTER_RW_READER_LOCK;

    sprintf(szQuery, DB_QUERY_COUNT_OLDER_THAN, dwOlderThan);

    dwError = SrvQueryEventLog(hDB, szQuery, &nRows, &nCols, &ppszResult);

    if (nRows == 1) {
        *pdwNumMatched = (DWORD) atoi(ppszResult[1]);
        BAIL_ON_EVT_ERROR(dwError);
    } else {

        EVT_LOG_VERBOSE("Could not find count of event logs in database");
    }

 cleanup:
    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }
    LEAVE_RW_READER_LOCK;
    return dwError;
 error:
    goto cleanup;

}

//Delete the record over 'n' entries
DWORD
SrvDeleteAboveLimitFromEventLog(
    HANDLE hDB,
    DWORD dwOlderThan
    )
{

    DWORD dwError = 0;
    CHAR  szQuery[8092];
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;

    ENTER_RW_WRITER_LOCK;

    sprintf(szQuery, DB_QUERY_DELETE_ABOVE_LIMIT, dwOlderThan);

    dwError = SrvQueryEventLog(hDB, szQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_EVT_ERROR(dwError);

 cleanup:
    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }
    LEAVE_RW_WRITER_LOCK;
    return dwError;
 error:
    goto cleanup;

}

//To delete records 'n' days older than current date.
DWORD
SrvDeleteOlderThanCurDate(
    HANDLE hDB,
    DWORD dwOlderThan
    )
{
    DWORD dwError = 0;
    CHAR  szQuery[8092];
    DWORD nRows = 0;
    DWORD nCols = 0;
    PSTR* ppszResult = NULL;

    ENTER_RW_WRITER_LOCK;

    sprintf(szQuery, DB_QUERY_DELETE_OLDER_THAN, dwOlderThan);

    dwError = SrvQueryEventLog(hDB, szQuery, &nRows, &nCols, &ppszResult);
    BAIL_ON_EVT_ERROR(dwError);

 cleanup:
    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }
    LEAVE_RW_WRITER_LOCK;
    return dwError;
 error:
    goto cleanup;

}

//helper functions

/*
 * Warning: this function should be surrounded by a lock;
 * sqlite3_free_table(ppszResult) should be called after the
 * function call, within the critical section.
 */
DWORD
SrvQueryEventLog(
    HANDLE hDB,
    PSTR szQuery,
    PDWORD pdwNumRows,
    PDWORD pdwNumCols,
    PSTR** pppszResult
    )
{
    DWORD dwError = 0;
    PEVENTLOG_CONTEXT pContext = NULL;
    PSTR  pszError = NULL;

    INT numRowsLocal = 0;
    INT numColsLocal = 0;

    if (!hDB) {
        dwError = EINVAL;
        BAIL_ON_EVT_ERROR(dwError);
    }

    pContext = (PEVENTLOG_CONTEXT)((long)hDB);
    if (!pContext->pDbHandle) {
        dwError = EVT_ERROR_INVALID_DB_HANDLE;
        BAIL_ON_EVT_ERROR(dwError);
    }

    EVT_LOG_INFO("evtdb: SrvQueryEventLog: query={%s}\n\n", szQuery);



    dwError = sqlite3_get_table(pContext->pDbHandle,
                                szQuery,
                                pppszResult,
                                (INT*) &numRowsLocal,
                                (INT*) &numColsLocal,
                                (PSTR*) &pszError
                                );

    if (dwError) {
        if (!IsNullOrEmptyString(pszError)) {
            EVT_LOG_ERROR(pszError);
        }
        BAIL_ON_EVT_ERROR(dwError);
    }

    *pdwNumRows = (DWORD)numRowsLocal;
    *pdwNumCols = (DWORD)numColsLocal;

 error:

    return dwError;

}

static
DWORD
BuildEventLogRecordList(
    PSTR * ppszResult,
    DWORD  nRows,
    DWORD  nCols,
    EVENT_LOG_RECORD** eventLogRecords
    )
{
    DWORD dwError = 0;
    EVENT_LOG_RECORD* pRecord = NULL;
    INT iCol = 0;
    INT iRow = 0;
    INT iVal = 0;

    if (eventLogRecords == NULL) {
        return -1;
    }

    if (nRows < 1) {
        return -1;
    }

    if (ppszResult == NULL || IsNullOrEmptyString(ppszResult[0])) {
        return -1;
    }

    EVENT_LOG_RECORD* records = *eventLogRecords;

    for (iVal = 0; iVal < EVENT_DB_COL_SENTINEL; iVal++) {
        //TODO: find something useful to do with the header information.
        EVT_LOG_DEBUG("evtdb.c: BuildEventLogRecordList: HEADER col=%d str=%s\n", iVal, ppszResult[iVal]);
    }

    for (iRow = 0; iRow < nRows; iRow++)
    {
        pRecord = &(records[iRow]);

        for (iCol = 0; iCol < nCols; iCol++)
        {

            if (iRow < (nRows -1) || iCol < (nCols - 1))
            {
                EVT_LOG_DEBUG("server::evtdb.c BuildEventLogRecordList(r%d, c%d): CURR:%s  NEXT:%s\n",
                    iRow, iCol, ppszResult[iVal], ppszResult[iVal+1]);
            }

            else {
                EVT_LOG_DEBUG("server::evtdb.c BuildEventLogRecordList(r%d, c%d): CURR:%s\n",
                    iRow, iCol, ppszResult[iVal]);
            }

            switch(iCol)
            {
                case EventTableCategoryId:
                {
                    if (IsNullOrEmptyString(ppszResult[iVal]))
                    {
                        pRecord->pszEventTableCategoryId = NULL;
                    }
                    else
                    {
                        dwError = RPCAllocateString( ppszResult[iVal],
                                      (PSTR*)(&pRecord->pszEventTableCategoryId));
                        BAIL_ON_EVT_ERROR(dwError);
                    }
                }
                break;
                case EventRecordId:
                {
                    pRecord->dwEventRecordId = atoi(ppszResult[iVal]);
                }
                break;
                case EventType:
                {
                    if (IsNullOrEmptyString(ppszResult[iVal]))
                    {
                        pRecord->pszEventType = NULL;
                    }
                    else
                    {
                        dwError = RPCAllocateString( ppszResult[iVal],
                                      (PSTR*)(&pRecord->pszEventType));
                        BAIL_ON_EVT_ERROR(dwError);
                    }
                }
                break;
                case EventDateTime:
                {
                    pRecord->dwEventDateTime = atoi(ppszResult[iVal]);
                }
                break;
                case EventSource:
                {

                    if (IsNullOrEmptyString(ppszResult[iVal]))
                    {
                        pRecord->pszEventSource = NULL;
                    }
                    else
                    {
                        dwError = RPCAllocateString( ppszResult[iVal],
                                      (PSTR*)(&pRecord->pszEventSource));
                        BAIL_ON_EVT_ERROR(dwError);
                    }
                }
                break;
                case EventCategory:
                {
                    if (IsNullOrEmptyString(ppszResult[iVal]))
                    {
                        pRecord->pszEventCategory = NULL;
                    }
                    else
                    {
                        dwError = RPCAllocateString( ppszResult[iVal],
                                      (PSTR*)(&pRecord->pszEventCategory));
                        BAIL_ON_EVT_ERROR(dwError);
                    }
                }
                break;
                case EventSourceId:
                {
                    pRecord->dwEventSourceId = atoi(ppszResult[iVal]);
                }
                break;
                case User:
                {
                    if (IsNullOrEmptyString(ppszResult[iVal]))
                    {
                        pRecord->pszUser = NULL;
                    }
                    else
                    {
                        dwError = RPCAllocateString( ppszResult[iVal],
                                      (PSTR*)(&pRecord->pszUser));
                        BAIL_ON_EVT_ERROR(dwError);
                    }
                }
                break;
                case Computer:
                {
                    if (IsNullOrEmptyString(ppszResult[iVal]))
                    {
                        pRecord->pszComputer = NULL;
                    }
                    else
                    {
                        dwError = RPCAllocateString( ppszResult[iVal],
                                      (PSTR*)(&pRecord->pszComputer));

                        BAIL_ON_EVT_ERROR(dwError);
                    }
                }
                break;
                case Description:
                {
                    if (IsNullOrEmptyString(ppszResult[iVal]))
                    {
                        pRecord->pszDescription = NULL;
                    }
                    else
                    {
                        dwError = RPCAllocateString( ppszResult[iVal],
                                      (PSTR*)(&pRecord->pszDescription));

                        BAIL_ON_EVT_ERROR(dwError);
                    }
                }
                break;
                case Data:
                {
                    if (IsNullOrEmptyString(ppszResult[iVal]))
                    {
                        pRecord->pszData = NULL;
                    }
                    else
                    {
                        dwError = RPCAllocateString( ppszResult[iVal],
                                      (PSTR*)(&pRecord->pszData));

                        BAIL_ON_EVT_ERROR(dwError);
                    }
                }
                break;
            }
            iVal++;
        }
    }


    EVT_LOG_VERBOSE("server::evtdb.c BuildEventLogRecordList() finished, nRows=%d, dwError=%d\n",
            nRows, dwError);

error:

    return dwError;
}



DWORD
SrvCreateDB(BOOLEAN replaceDB)
{
    DWORD dwError = 0;
    sqlite3* pSqliteHandle = NULL;
    PSTR pszError = NULL;
    BOOLEAN bExists = FALSE;

    CHAR szQuery[1024];

    dwError = EVTCheckFileExists(EVENTLOG_DB, &bExists);
    BAIL_ON_EVT_ERROR(dwError);

    if (bExists) {
    if (replaceDB) {
        dwError = EVTRemoveFile(EVENTLOG_DB);
        BAIL_ON_EVT_ERROR(dwError);
    }
    else return 0;
    }

    dwError = EVTCheckDirectoryExists(EVENTLOG_DB_DIR, &bExists);
    BAIL_ON_EVT_ERROR(dwError);

    if (!bExists) {
        dwError = EVTCreateDirectory(EVENTLOG_DB_DIR, S_IRWXU);
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = sqlite3_open(EVENTLOG_DB, &pSqliteHandle);
    BAIL_ON_EVT_ERROR(dwError);
   
    dwError = sqlite3_exec(pSqliteHandle,
                            DB_QUERY_CREATE_EVENTS_TABLE,                           
                            NULL,
                            NULL,
                            &pszError);
    BAIL_ON_EVT_ERROR(dwError);

    sprintf(szQuery, DB_QUERY_CREATE_UNIQUE_INDEX, "recordId", "EventRecordId");
    dwError = sqlite3_exec(pSqliteHandle,
                            szQuery,
                            NULL,
                            NULL,
                            &pszError);
    BAIL_ON_EVT_ERROR(dwError);

    sprintf(szQuery, DB_QUERY_CREATE_INDEX, "tableCategoryId", "EventTableCategoryId");
    dwError = sqlite3_exec(pSqliteHandle,
                            szQuery,
                            NULL,
                            NULL,
                            &pszError);
    BAIL_ON_EVT_ERROR(dwError);

    sprintf(szQuery, DB_QUERY_CREATE_INDEX, "dateTime", "EventDateTime");
    dwError = sqlite3_exec(pSqliteHandle,
                            szQuery,
                            NULL,
                            NULL,
                            &pszError);
    BAIL_ON_EVT_ERROR(dwError);

    EVT_LOG_VERBOSE("New database file %s with TABLE lwievents successfully created.\n", EVENTLOG_DB);

cleanup:

    if (pSqliteHandle)
        sqlite3_close(pSqliteHandle);

    return dwError;

error:

    if (!IsNullOrEmptyString(pszError))
        EVT_LOG_ERROR(pszError);

    goto cleanup;
}
