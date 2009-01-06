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
 * Eventlog parser to parse log files from Microsoft Event Viewer.
 *
 */
#include "includes.h"

//Character indices to use in PRINT_TABLE option.
//This allows printing a user-readable table to stdout that 80 characters wide.
#define TABLOC_ID 0
#define TABLOC_TYPE     (TABLOC_ID+7)
#define TABLOC_DATE     (TABLOC_TYPE+20)
#define TABLOC_TIME     (TABLOC_DATE+16)
#define TABLOC_SOURCE   (TABLOC_TIME+14)
#define TABLOC_CATEGORY (TABLOC_SOURCE+25)
#define TABLOC_DATA     (TABLOC_CATEGORY+16)
#define TABLOC_BORDER " | "

//
// When you save the log file on windows, the following seems
// to be the default order of fields.
//
typedef enum
{
    EVENT_TABLE_CATEGORY_ID = 0,
    EVENT_DATE, //1
    EVENT_TIME, //2
    EVENT_SOURCE, //3
    EVENT_TYPE, //4
    EVENT_CATEGORY, //5
    EVENT_SOURCE_ID, //6
    EVENT_USER, //7
    EVENT_COMPUTER, //8
    EVENT_DESCRIPTION, //9
    EVENT_FIELD_TYPE_SENTINEL
} EventFieldType;

typedef enum
{
    TOKEN_NONE          = 0,
    TOKEN_STRING        = 1,
    TOKEN_QUOTED_STRING = 2,
    TOKEN_NEWLINE       = 3,
    TOKEN_EOF           = 4
} EventTokenId;

/* Lexer states */
#define BEGIN_STATE   0
#define LEX_CHAR      1
#define LEX_COMMA     2
#define LEX_DBL_QUOTE 3
#define LEX_EOF       4
#define LEX_NEWLINE   5
#define END_STATE     6

#define NULL_STR    "<null>"
#define MAXTOKENLEN 1024

typedef enum
{   
    Consume,
    Pushback,
    Skip
} EVTLexAction;

typedef struct _STATE_ENTRY {
    DWORD        dwNextState;
    EVTLexAction  dwAction;
    EventTokenId tokenId;
} STATE_ENTRY, *PSTATE_ENTRY;

typedef struct _EVENT_TOKEN {
    EventTokenId dwToken;
    DWORD        dwLength;
    CHAR         szToken[MAXTOKENLEN+1];
} EVENT_TOKEN, *PEVENT_TOKEN;

typedef struct _LEXER_STATE {
    FILE* fp;
    DWORD dwLine;
    DWORD dwCol;
    BOOLEAN     bLastToken;
    EVENT_TOKEN lastToken;
} LEXER_STATE, *PLEXER_STATE;

STATE_ENTRY StateTable[][7] =
{
    /* BEGIN_STATE := 0 */
    {
        {  BEGIN_STATE, Consume, TOKEN_NONE},     /* BEGIN_STATE   */
        {     LEX_CHAR, Consume, TOKEN_NONE},     /* LEX_CHAR      */
        {    END_STATE, Skip,    TOKEN_STRING},   /* LEX_COMMA     */
        {LEX_DBL_QUOTE, Skip,    TOKEN_NONE},     /* LEX_DBL_QUOTE */
        {    END_STATE, Skip,    TOKEN_EOF},      /* LEX_EOF       */
        {    END_STATE, Consume, TOKEN_NEWLINE},  /* LEX_NEWLINE   */
    },
    /* LEX_CHAR := 1 */
    {
        {  BEGIN_STATE, Consume,  TOKEN_NONE},    /* BEGIN_STATE   */
        {     LEX_CHAR, Consume,  TOKEN_NONE},    /* LEX_CHAR      */
        {    END_STATE, Skip,     TOKEN_STRING},  /* LEX_COMMA     */
        {     LEX_CHAR, Consume,  TOKEN_NONE},    /* LEX_DBL_QUOTE */
        {    END_STATE, Pushback, TOKEN_STRING},  /* LEX_EOF       */
        {    END_STATE, Pushback, TOKEN_STRING},  /* LEX_NEWLINE   */
    },
    /* LEX_COMMA := 2 */
    {
        {  BEGIN_STATE,  Consume, TOKEN_NONE},    /* BEGIN_STATE   */
        {    END_STATE, Pushback, TOKEN_NONE},    /* LEX_CHAR      */
        {    END_STATE, Pushback, TOKEN_NONE},    /* LEX_COMMA     */
        {    END_STATE, Pushback, TOKEN_NONE},    /* LEX_DBL_QUOTE */
        {    END_STATE, Skip,     TOKEN_EOF},     /* LEX_EOF       */
        {    END_STATE, Pushback, TOKEN_NONE},    /* LEX_NEWLINE   */
    },
    /* LEX_DBL_QUOTE := 3 */
    {
        {  BEGIN_STATE, Consume, TOKEN_NONE},          /* BEGIN_STATE   */
        {LEX_DBL_QUOTE, Consume, TOKEN_NONE},          /* LEX_CHAR      */
        {LEX_DBL_QUOTE, Consume, TOKEN_NONE},          /* LEX_COMMA     */
        {    END_STATE, Skip,    TOKEN_QUOTED_STRING}, /* LEX_DBL_QUOTE */
        {    END_STATE, Pushback,TOKEN_QUOTED_STRING}, /* LEX_EOF       */
        {LEX_DBL_QUOTE, Consume, TOKEN_NONE},          /* LEX_NEWLINE   */
    },
    /* LEX_EOF := 4 */
    {
        {  BEGIN_STATE, Consume, TOKEN_EOF},      /* BEGIN_STATE   */
        {    END_STATE, Consume, TOKEN_EOF},      /* LEX_CHAR      */
        {    END_STATE, Consume, TOKEN_EOF},      /* LEX_COMMA     */
        {    END_STATE, Consume, TOKEN_EOF},      /* LEX_DBL_QUOTE */
        {    END_STATE, Consume, TOKEN_EOF},      /* LEX_EOF       */
        {    END_STATE, Consume, TOKEN_EOF},      /* LEX_NEWLINE   */
    }
};

#define GetCharacter(pLexerState) getc(pLexerState->fp)
#define PushBackCharacter(pLexerState, ch) ((EOF == ungetc(ch, pLexerState->fp) ? errno : 0))

static
void
FreeLexerState(
    PLEXER_STATE pLexerState
    )
{
    if (pLexerState->fp != NULL) {
        fclose(pLexerState->fp);
    }

    EVTFreeMemory(pLexerState);
}

static
DWORD
InitLexer(
    PSTR pszFilePath,
    PLEXER_STATE* ppLexerState
    )
{
    DWORD dwError = 0;
    PLEXER_STATE pLexerState = NULL;

    dwError = EVTAllocateMemory(sizeof(LEXER_STATE), (PVOID*)&pLexerState);
    BAIL_ON_EVT_ERROR(dwError);

    pLexerState->fp = fopen(pszFilePath, "r");
    if (!pLexerState->fp) {
        dwError = errno;
        BAIL_ON_EVT_ERROR(dwError);
    }

    *ppLexerState = pLexerState;

cleanup:

    return dwError;

error:

    *ppLexerState = NULL;

    if (pLexerState) {
        FreeLexerState(pLexerState);
    }

    goto cleanup;
}

static
DWORD
GetCharacterId(
    DWORD ch
    )
{
    if (ch == EOF) {
        return LEX_EOF;
    }

    if (ch == '\n') {
        return LEX_NEWLINE;
    }

    if (ch == ',') {
        return LEX_COMMA;
    }

    if (ch == '"') {
        return LEX_DBL_QUOTE;
    }

    return LEX_CHAR;
}

static
DWORD
GetState(
    DWORD currentState,
    DWORD chId
    )
{
    return(StateTable[currentState][chId].dwNextState);
}

static
DWORD
GetAction(
    DWORD currentState,
    DWORD chId
    )
{
    return(StateTable[currentState][chId].dwAction);
}

static
DWORD
GetTokenId(
    DWORD currentState,
    DWORD chId
    )
{
    return (StateTable[currentState][chId].tokenId);
}

/*
 * Fills the given token as parsed from the file
 * The caller is responsible for allocating memory
 */
static
DWORD
GetToken(
    PLEXER_STATE pLexerState,
    PEVENT_TOKEN pToken
    )
{
    DWORD dwError = 0;
    char szToken[MAXTOKENLEN+1];
    DWORD dwIndex = 0;
    DWORD dwToken;
    DWORD dwCurrentState = BEGIN_STATE;

    dwError = (pToken == NULL ? EINVAL : 0);
    BAIL_ON_EVT_ERROR(dwError);

    memset(pToken, 0, sizeof(EVENT_TOKEN));
    szToken[0] = '\0';

    while (dwCurrentState != END_STATE) {
        DWORD ch = GetCharacter(pLexerState);
        DWORD dwChId = GetCharacterId(ch);
        DWORD dwState;
        DWORD dwAction;

        if (dwChId != LEX_EOF) {
            pLexerState->dwCol++;
        }

        dwState = GetState(dwCurrentState, dwChId);

        if (ch == (DWORD)'\n') {
            pLexerState->dwLine++;
            pLexerState->dwCol = 0;
        }

        dwToken = GetTokenId(dwCurrentState, dwChId);

        dwAction = GetAction(dwCurrentState, dwChId);
        switch(dwAction)
        {
        case Skip:
        {
            ;
        }
        break;
        case Consume:
        {
            szToken[dwIndex++] = (BYTE)ch;
        }
        break;
        case Pushback:
        {
            pLexerState->dwCol--;
            dwError = PushBackCharacter(pLexerState, (BYTE)ch);
            BAIL_ON_EVT_ERROR(dwError);
        }
        break;
        }

        dwCurrentState = dwState;
    }
    szToken[dwIndex] = '\0';

    pToken->dwToken = dwToken;
    pToken->dwLength = dwIndex;
    if (pToken->dwLength) {
        strncpy(pToken->szToken, szToken, pToken->dwLength);
    }

error:

    return dwError;
}

static
DWORD
GetNextToken(
    PLEXER_STATE pLexerState,
    PEVENT_TOKEN pEventToken
    )
{
    DWORD dwError = 0;

    if (pLexerState->bLastToken) {
        memcpy(pEventToken, &pLexerState->lastToken, sizeof(EVENT_TOKEN));
        pLexerState->bLastToken = FALSE;
    } else {
        dwError = GetToken(pLexerState, pEventToken);
    }

    return dwError;
}

static
void
PushbackToken(
    PLEXER_STATE pLexerState,
    PEVENT_TOKEN pEventToken
    )
{
    pLexerState->bLastToken = TRUE;
    memcpy(&pLexerState->lastToken, pEventToken, sizeof(EVENT_TOKEN));
}

#if 0
static
unsigned int
GetHash(
    PSTR pszValue
    )
{
    /* 31 bit hash function */
    const signed char *p = pszValue;
    unsigned int h = *p;

    if (h)
    for (p += 1; *p != '\0'; p++)
        h = (h << 5) - h + *p;

    return h;
}
#endif


static
BOOLEAN
IsDate(
    PSTR pszToken
    )
{
    struct tm timebuf;
    memset(&timebuf, 0, sizeof(timebuf));
    return !IsNullOrEmptyString(pszToken) && (NULL != strptime(pszToken, "%D", &timebuf));
}


static
DWORD
ExportEventRecord (
    PEVENT_LOG_RECORD pRecord,
    FILE* fpExport
    )
{

    DWORD dwError = 0;

    char eventDate[256];
    char eventTime[256];

    if (pRecord == NULL) return -1;
    if (fpExport == NULL) return -1;

    //CSV fields: Type,Date,Time,Source,Category,SourceID,User,Computer,Description

    time_t eventTimeStruct = (time_t) pRecord->dwEventDateTime;

    strftime(eventDate, 255, "%F", localtime(&eventTimeStruct));
    strftime(eventTime, 255, "%r", localtime(&eventTimeStruct));

    fprintf(fpExport, "%s,%s,%s,%s,%s,%s,%d,%s,%s,\"%s\", \"%s\"\n",
        IsNullOrEmptyString(pRecord->pszEventTableCategoryId) ? "<null>" : (PSTR)pRecord->pszEventTableCategoryId,
        eventDate, //PSTR
        eventTime, //PSTR
        IsNullOrEmptyString(pRecord->pszEventSource) ? "<null>" : (PSTR)pRecord->pszEventSource,
        IsNullOrEmptyString(pRecord->pszEventSource) ? "<null>" : (PSTR)pRecord->pszEventType,
        IsNullOrEmptyString(pRecord->pszEventSource) ? "<null>" : (PSTR)pRecord->pszEventCategory,
        pRecord->dwEventSourceId, //DWORD
        IsNullOrEmptyString(pRecord->pszUser) ? "<null>" : (PSTR)pRecord->pszUser,
        IsNullOrEmptyString(pRecord->pszComputer) ? "<null>" : (PSTR)pRecord->pszComputer,
        IsNullOrEmptyString(pRecord->pszDescription) ? "<null>" : (PSTR)pRecord->pszDescription,
        IsNullOrEmptyString(pRecord->pszData) ? "<null>" : (PSTR)pRecord->pszData);

    return dwError;
}



static
DWORD
PrintEventRecordTableRow (
    PEVENT_LOG_RECORD pRecord,
    FILE* fp
    )
{

    DWORD dwError = 0;
    DWORD i = 0;   

    char eventDate[256];
    char eventTime[256];


    char buf[256];

    if (pRecord == NULL) return -1;
    if (fp == NULL) return -1;

    //TableRow fields: RecordID,Type,Date,Time,Source,Category

    time_t eventTimeStruct = (time_t) pRecord->dwEventDateTime;

    strftime(eventDate, 255, "%F", localtime(&eventTimeStruct));
    strftime(eventTime, 255, "%r", localtime(&eventTimeStruct));

    memset(buf, ' ', 255);

    sprintf(buf,                  "%d", pRecord->dwEventRecordId);

    sprintf(buf+TABLOC_TYPE,     "%s%s", TABLOC_BORDER,
        IsNullOrEmptyString(pRecord->pszEventType) ? "<null>" : (PSTR)pRecord->pszEventType);

    sprintf(buf+TABLOC_DATE,     "%s%s", TABLOC_BORDER,
        eventDate);

    sprintf(buf+TABLOC_TIME,     "%s%s", TABLOC_BORDER,
        eventTime);

    sprintf(buf+TABLOC_SOURCE,   "%s%s", TABLOC_BORDER,
        IsNullOrEmptyString(pRecord->pszEventSource) ? "<null>" : (PSTR)pRecord->pszEventSource);

    sprintf(buf+TABLOC_CATEGORY, "%s%s", TABLOC_BORDER,
        IsNullOrEmptyString(pRecord->pszEventCategory) ? "<null>" : (PSTR)pRecord->pszEventCategory);
    
    sprintf(buf+TABLOC_DATA, "%s%s", TABLOC_BORDER,
        IsNullOrEmptyString(pRecord->pszData) ? "<null>" : (PSTR)pRecord->pszData);
            

    for (i = 0; i <= TABLOC_DATA; i++) {
        if (buf[i] == (char)0)
        {
            buf[i] = ' ';
        }
    }


    fprintf(fp, "%s\n", buf);

    return dwError;
}



DWORD
ParseAndAddEvents(
    PEVENT_LOG_HANDLE pEventLogHandle,
    PSTR   pszFilePath,
    PSTR   pszEventTableCategoryId,
    BOOLEAN bEventTableCategoryIdInCSV,
    PFNEventRecordProcessor eventRecordProcessor
    )
{
    DWORD dwError = 0;
    BOOLEAN bFileExists = FALSE;
    PLEXER_STATE pLexerState = NULL;
    EVENT_TOKEN eventToken;
    PEVENT_LOG_RECORD pEventRecord = NULL;
    struct tm tmpTime;
    char dateBuf[128];
    char dateTimeBuf[256];
    int iToken = 0;
    int i = 0;
    int rowCounter = 0;

    memset(&eventToken, 0, sizeof(EVENT_TOKEN));

    dwError = EVTCheckFileExists(pszFilePath, &bFileExists);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = InitLexer(pszFilePath, &pLexerState);
    BAIL_ON_EVT_ERROR(dwError);

    do
    {
        dwError = GetNextToken(pLexerState, &eventToken);
        BAIL_ON_EVT_ERROR(dwError);

        if (!bEventTableCategoryIdInCSV &&
            iToken == EVENT_TABLE_CATEGORY_ID)
        {
            iToken++;
        }
        switch(iToken++)
        {
            case EVENT_TABLE_CATEGORY_ID:
            {
                char* token = eventToken.szToken;

                for (i = 0; i < strlen(token); i++) {
                    if (token[i] < '0' || token[i] > '9') {
                        printf("No TableCategory specified, and first entry in row %d is not an integer!\n", rowCounter);
                        dwError = (DWORD) -1;
                        BAIL_ON_EVT_ERROR(dwError);
                    }
                }

                if (pEventRecord) {
                    dwError = eventRecordProcessor( pEventLogHandle,
                                                    *pEventRecord);
                    BAIL_ON_EVT_ERROR(dwError);
                    LWIFreeEventRecord(pEventRecord);
                    pEventRecord = NULL;
                }

                dwError = EVTAllocateMemory(sizeof(EVENT_LOG_RECORD), (PVOID*)&pEventRecord);
                BAIL_ON_EVT_ERROR(dwError);

                EVT_LOG_VERBOSE("TableCategoryId: %s", IsNullOrEmptyString(eventToken.szToken) ? NULL_STR : eventToken.szToken);
                if (!IsNullOrEmptyString(eventToken.szToken)) {
                    dwError = EVTAllocateString(eventToken.szToken, (PSTR*)&pEventRecord->pszEventTableCategoryId);
                    BAIL_ON_EVT_ERROR(dwError);
                    pEventRecord->dwEventDateTime = 0;
                }
                break;
            }
            case EVENT_DATE:
            {
                if (!bEventTableCategoryIdInCSV) {
                    if (pEventRecord) {
                        dwError = eventRecordProcessor( pEventLogHandle,
                                                        *pEventRecord);
                        BAIL_ON_EVT_ERROR(dwError);
                        LWIFreeEventRecord(pEventRecord);
                        pEventRecord = NULL;
                    }

                    dwError = EVTAllocateMemory(sizeof(EVENT_LOG_RECORD), (PVOID*)&pEventRecord);
                    BAIL_ON_EVT_ERROR(dwError);

			if (!IsNullOrEmptyString(pszEventTableCategoryId)) {
			dwError = EVTAllocateString(pszEventTableCategoryId, (PSTR*)&pEventRecord->pszEventTableCategoryId);
			BAIL_ON_EVT_ERROR(dwError);
					}
                    pEventRecord->dwEventDateTime = 0;
                }

                EVT_LOG_VERBOSE("    Date: %s", IsNullOrEmptyString(eventToken.szToken) ? NULL_STR : eventToken.szToken);

                EVTStripWhitespace(eventToken.szToken, TRUE, TRUE);
                sprintf(dateBuf, "%s", eventToken.szToken);

                break;
            }
            case EVENT_TIME:
            {
                EVT_LOG_VERBOSE("    Time: %s", IsNullOrEmptyString(eventToken.szToken) ? NULL_STR : eventToken.szToken);

                EVTStripWhitespace(eventToken.szToken, TRUE, TRUE);
                sprintf(dateTimeBuf, "%s %s", dateBuf, eventToken.szToken);

                tmpTime.tm_sec = 0;
                tmpTime.tm_min = 0;
                tmpTime.tm_hour = 0;
                tmpTime.tm_mday = 0;
                tmpTime.tm_mon = 0;
                tmpTime.tm_year = 0;
                tmpTime.tm_wday = 0;
                tmpTime.tm_yday = 0;
                tmpTime.tm_isdst = 0;

                sscanf(dateTimeBuf, "%d/%d/%d %d:%d:%d %s",
                    &(tmpTime.tm_mon),
                    &(tmpTime.tm_mday),
                    &(tmpTime.tm_year),
                    &(tmpTime.tm_hour),
                    &(tmpTime.tm_min),
                    &(tmpTime.tm_sec),
                    dateBuf);

                tmpTime.tm_mon--;
                tmpTime.tm_year -= 1900;

                if (strcmp(dateBuf, "PM") == 0) tmpTime.tm_hour += 12;

                pEventRecord->dwEventDateTime = mktime(&tmpTime);

                break;
            }
            case EVENT_SOURCE:
            {
                EVT_LOG_VERBOSE("  Source: %s", IsNullOrEmptyString(eventToken.szToken) ? NULL_STR : eventToken.szToken);
                if (!IsNullOrEmptyString(eventToken.szToken)) {
                    dwError = EVTAllocateString(eventToken.szToken, (PSTR*)&pEventRecord->pszEventSource);
                    BAIL_ON_EVT_ERROR(dwError);
                }
                break;
            }
            case EVENT_TYPE:
            {
                EVT_LOG_VERBOSE("Type: %s", IsNullOrEmptyString(eventToken.szToken) ? NULL_STR : eventToken.szToken);
                if (!IsNullOrEmptyString(eventToken.szToken)) {
                    dwError = EVTAllocateString(eventToken.szToken, (PSTR*)&pEventRecord->pszEventType);
                    BAIL_ON_EVT_ERROR(dwError);
                }
                break;
            }
            case EVENT_CATEGORY:
            {
                EVT_LOG_VERBOSE("Category: %s", IsNullOrEmptyString(eventToken.szToken) ? NULL_STR : eventToken.szToken);
                if (!IsNullOrEmptyString(eventToken.szToken)) {
                    dwError = EVTAllocateString(eventToken.szToken, (PSTR*)&pEventRecord->pszEventCategory);
                    BAIL_ON_EVT_ERROR(dwError);
                }
                break;
            }
            case EVENT_SOURCE_ID:
            {
                EVT_LOG_VERBOSE("      Id: %s", IsNullOrEmptyString(eventToken.szToken) ? NULL_STR : eventToken.szToken);
                pEventRecord->dwEventSourceId = atoi(eventToken.szToken);
                break;
            }
            case EVENT_USER:
            {
                EVT_LOG_VERBOSE("    User: %s", IsNullOrEmptyString(eventToken.szToken) ? NULL_STR : eventToken.szToken);
                if (!IsNullOrEmptyString(eventToken.szToken)) {
                    dwError = EVTAllocateString(eventToken.szToken, (PSTR*)&pEventRecord->pszUser);
                    BAIL_ON_EVT_ERROR(dwError);
                }
                break;
            }
            case EVENT_COMPUTER:
            {
                EVT_LOG_VERBOSE("Computer: %s", IsNullOrEmptyString(eventToken.szToken) ? NULL_STR : eventToken.szToken);
                if (!IsNullOrEmptyString(eventToken.szToken)) {
                    dwError = EVTAllocateString(eventToken.szToken, (PSTR*)&pEventRecord->pszComputer);
                    BAIL_ON_EVT_ERROR(dwError);
                }
                break;
            }
            case EVENT_DESCRIPTION:
            {
                if (!IsNullOrEmptyString(eventToken.szToken)) {
                    dwError = EVTAllocateString( eventToken.szToken,
                                                 (PSTR*)&pEventRecord->pszDescription);
                    BAIL_ON_EVT_ERROR(dwError);
                }

                /* Sometimes the description field has newlines
                * Sometimes, the value is quoted.
                * If it was quoted, we would receive it as a quoted
                * string - otherwise, we have to concatenate any
                * remaining tokens until we get to a date.
                */
                while (eventToken.dwToken != TOKEN_EOF) {
                    dwError = GetNextToken(pLexerState, &eventToken);
                    BAIL_ON_EVT_ERROR(dwError);

                    if (!bEventTableCategoryIdInCSV &&
                        IsDate(eventToken.szToken)) {
                        PushbackToken(pLexerState, &eventToken);
                        break;
                    }
                    if (bEventTableCategoryIdInCSV &&
                        strlen(eventToken.szToken) == 1 &&
                        isdigit((int)eventToken.szToken[0]) &&
                        atoi(eventToken.szToken) >= 0 ) {
                        PushbackToken(pLexerState, &eventToken);
                        break;
                    }

                    if (IsNullOrEmptyString(eventToken.szToken)) {
                        continue;
                    }

                    if (IsNullOrEmptyString(pEventRecord->pszDescription)) {
                        dwError = EVTAllocateString(eventToken.szToken, (PSTR*)&pEventRecord->pszDescription);
                        BAIL_ON_EVT_ERROR(dwError);
                    }
                    else {
                        dwError = EVTReallocMemory(pEventRecord->pszDescription,
                                    (PVOID*)&pEventRecord->pszDescription,
                                    strlen(pEventRecord->pszDescription) + strlen(eventToken.szToken)+1);
                        BAIL_ON_EVT_ERROR(dwError);

                        strcat(pEventRecord->pszDescription, eventToken.szToken);
                    }
                }
                EVT_LOG_VERBOSE("Event Description: %s",
                        IsNullOrEmptyString((PSTR)pEventRecord->pszDescription) ?
                        NULL_STR : (PSTR)pEventRecord->pszDescription);

                iToken = 0;

                printf("importevents: finished row %d...\n", rowCounter);

                rowCounter++;

                break;
            }  //end case EventDescription


        } //end switch

        if (iToken >= EVENT_FIELD_TYPE_SENTINEL) {
            EVT_LOG_ERROR("Parse error in in event import");
            dwError = EVT_ERROR_INTERNAL;
            BAIL_ON_EVT_ERROR(dwError);
        }

    } while (eventToken.dwToken != TOKEN_EOF);

    if (pEventRecord) {
        /* Add the record */
        dwError = eventRecordProcessor(pEventLogHandle,
                            *pEventRecord);

        BAIL_ON_EVT_ERROR(dwError);
    }

 cleanup:

    if (pEventRecord) {
        LWIFreeEventRecord(pEventRecord);
    }

    if (pLexerState) {
        FreeLexerState(pLexerState);
    }

    return dwError;

error:
    goto cleanup;

}


DWORD
PrintEventRecords(
    FILE* output,
    EVENT_LOG_RECORD* eventRecords,
    DWORD nRecords,
    PDWORD totalRecords
    )
{
    char eventDate[256];
    char eventTime[256];

    DWORD dwError = 0;
    DWORD totalRecordsLocal = *totalRecords;
    int iRecord = 0;

    for (iRecord = 0; iRecord < nRecords; iRecord++)
    {
    EVENT_LOG_RECORD* pRecord = &(eventRecords[iRecord]);

    time_t eventTimeStruct = (time_t) pRecord->dwEventDateTime;

    strftime(eventDate, 255, "%F", localtime(&eventTimeStruct));
    strftime(eventTime, 255, "%r", localtime(&eventTimeStruct));

    printf("Event Record: (%d/%d) (%d total)\n", iRecord+1, nRecords, ++totalRecordsLocal);
    printf("========================================\n");
    printf("Event Record ID......... %d\n", pRecord->dwEventRecordId);
    printf("Event Table Category.............. %s\n",
            IsNullOrEmptyString(pRecord->pszEventSource) ? "<null>" : (char*) (pRecord->pszEventTableCategoryId));
    printf("Event Type.............. %s\n",
            IsNullOrEmptyString(pRecord->pszEventSource) ? "<null>" : (char*) (pRecord->pszEventType));
    printf("Event Date.............. %s\n", eventDate);
    printf("Event Time.............. %s\n", eventTime);
    printf("Event Source............ %s\n",
            IsNullOrEmptyString(pRecord->pszEventSource) ? "<null>" : (char*) (pRecord->pszEventSource));
    printf("Event Category.......... %s\n",
            IsNullOrEmptyString(pRecord->pszEventSource) ? "<null>" : (char*) (pRecord->pszEventCategory));
    printf("Event Source ID......... %d\n", pRecord->dwEventSourceId);
    printf("Event User.............. %s\n",
            IsNullOrEmptyString(pRecord->pszUser) ? "<null>" : (char*) (pRecord->pszUser));
    printf("Event Computer.......... %s\n",
            IsNullOrEmptyString(pRecord->pszComputer) ? "<null>" : (char*) (pRecord->pszComputer));
    printf("Event Description....... %s\n",
            IsNullOrEmptyString(pRecord->pszDescription) ? "<null>" : (char*) (pRecord->pszDescription));
    printf("========================================\n");

    }

    *totalRecords = totalRecordsLocal;

    return dwError;
}




DWORD
PrintEventRecordsTable(
    FILE* output,
    EVENT_LOG_RECORD* eventRecords,
    DWORD nRecords,
    PDWORD totalRecords
    )
{
    DWORD i = 0;
    DWORD dwError = 0;
    DWORD totalRecordsLocal = *totalRecords;

    char buf[256];
    memset(buf, ' ', 255);

    sprintf(buf, "Id:   ");
    sprintf(buf+TABLOC_TYPE,     "%sType", TABLOC_BORDER);
    sprintf(buf+TABLOC_DATE,     "%sDate", TABLOC_BORDER);
    sprintf(buf+TABLOC_TIME,     "%sTime", TABLOC_BORDER);
    sprintf(buf+TABLOC_SOURCE,   "%sSource", TABLOC_BORDER);
    sprintf(buf+TABLOC_CATEGORY, "%sCategory", TABLOC_BORDER);
    sprintf(buf+TABLOC_DATA,     "%sData", TABLOC_BORDER);

    for (i = 0; i <= TABLOC_DATA; i++) {
    if (buf[i] == (char)0) {
        buf[i] = ' ';
    }
    }


    fprintf(output, "%s\n", buf);

    for (i = 0; i < nRecords; i++) {
    dwError = PrintEventRecordTableRow(&(eventRecords[i]), output);
    BAIL_ON_EVT_ERROR(dwError);
    totalRecordsLocal++;
    }


 error:

    *totalRecords = totalRecordsLocal;

    return dwError;
}





DWORD
ReadAndExportEvents(
    PEVENT_LOG_HANDLE pEventLogHandle,
    FILE* fpExport
    )
{
    DWORD dwError = 0;
    DWORD i = 0;

    const DWORD pageSize = 2000;
    DWORD currentEntry = 0;
    DWORD entriesRead = 0;
    EVENT_LOG_RECORD* records = NULL;

    if (fpExport == NULL) return -1;
    if (pEventLogHandle == NULL) return -1;

    const char* sqlFilterChar = "eventRecordId > 0";

    wchar16_t* sqlFilter = NULL;
    dwError = EVTAllocateMemory(sizeof(wchar16_t)*(1+strlen(sqlFilterChar)), (PVOID*)(&sqlFilter));
    BAIL_ON_EVT_ERROR(dwError);

    sw16printf(sqlFilter, "%s", sqlFilterChar);

    fprintf(fpExport, "EventTableCategoryId,");
    fprintf(fpExport, "EventRecordId,");
    fprintf(fpExport, "EventType,");
    fprintf(fpExport, "EventTime,");
    fprintf(fpExport, "EventSource,");
    fprintf(fpExport, "EventCategory,");
    fprintf(fpExport, "EventSourceId,");
    fprintf(fpExport, "User,");
    fprintf(fpExport, "Computer,");
    fprintf(fpExport, "Description\n");


    do
    {

    dwError = LWIReadEventLog((HANDLE)pEventLogHandle, currentEntry, pageSize, sqlFilter, &entriesRead, &records);
    BAIL_ON_EVT_ERROR(dwError);

    for (i = 0; i < entriesRead; i++) {
        dwError = ExportEventRecord(&(records[i]), fpExport);
        BAIL_ON_EVT_ERROR(dwError);
    }

    fflush(fpExport);

    currentEntry += entriesRead;

    } while (entriesRead == pageSize && entriesRead > 0);

 cleanup:

    RPCFreeMemory(records);

    EVTFreeMemory(records);

    return dwError;

error:

    goto cleanup;

}

