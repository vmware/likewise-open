/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *       FileClient Test Tool
 *
 * Abstract:
 *
 *        Likewise File Browser Client
 *
 *        Public Client API
 *
 * Authors:
 */

#include "lwfileclientsys.h"
#include "fileclient.h"
#include <lwmem.h>
#include <wc16str.h>
#include <lwstr.h>


#define BAIL_ON_WIN_ERROR(err)    \
    if ((err) != ERROR_SUCCESS) { \
        goto error;               \
    }

enum
{
    FILE_OP_UNKNOWN,
    FILE_OP_COPY,
    FILE_OP_MOVE,
    FILE_OP_LIST,
    FILE_OP_DELETE,
    FILE_OP_EXIT
};

enum
{
    PARSE_FILECLIENT_UNKNOWN,
    PARSE_FILECLIENT_OPEN,
    PARSE_FILECLIENT_COPY,
    PARSE_FILECLIENT_MOVE,
    PARSE_FILECLIENT_LIST,
    PARSE_FILECLIENT_DELETE,
    PARSE_FILECLIENT_EXIT
};

static
DWORD
GetWord(
    PSTR *ppszField,
    PSTR *ppszValue,
    PDWORD pdwLen
    );

DWORD
ParseArgs(
    PSTR pszCommand,
    PDWORD pdwCommandType,
    PWSTR *ppszSourceFile,
    PWSTR *ppszRemoteFilePath,
    PBOOL pbOverWrite
    );

static
DWORD
ListFiles(
    PWSTR pszSourceFiles
    );

static
void
PrintData(
    LIKEWISE_FIND_DATA data
    );

static
void
ShowUsage();

static const PSTR pszLikewiseStr = "File command # ";


int main(
    int argc,
    char **argv)
{
    DWORD dwError = 0;
    PWSTR pszSourceFilePath = NULL;
    PWSTR pszRemoteFilePath = NULL;
    char pszCommand[256] = {0};
    DWORD dwOpType = 0;
    BOOL bOverWrite = FALSE;

    do
    {
        printf("\n%s", pszLikewiseStr);
        if(fgets(pszCommand, 256, stdin) != NULL)
        {
            dwError = ParseArgs(pszCommand,
                                &dwOpType,
                                &pszSourceFilePath,
                                &pszRemoteFilePath,
                                &bOverWrite);
            BAIL_ON_WIN_ERROR(dwError);

            switch(dwOpType)
            {
                case FILE_OP_COPY:
                    if(CopyFile(pszSourceFilePath,
                                pszRemoteFilePath,
                                bOverWrite) == FALSE)
                    {
                        dwError = GetLastError();
                        BAIL_ON_WIN_ERROR(dwError);
                    }
                    break;
                case FILE_OP_MOVE:
                    if(MoveFile(pszSourceFilePath,
                                pszRemoteFilePath,
                                bOverWrite) == FALSE)
                    {
                        dwError = GetLastError();
                        BAIL_ON_WIN_ERROR(dwError);
                    }
                    break;

                case FILE_OP_DELETE:
                    if(DeleteFile(pszSourceFilePath) == FALSE)
                    {
                        dwError = GetLastError();
                        BAIL_ON_WIN_ERROR(dwError);
                    }
                    break;

                case FILE_OP_LIST:
                    dwError = ListFiles(pszSourceFilePath);
                    BAIL_ON_WIN_ERROR(dwError);
                    break;

                case FILE_OP_EXIT:
                    goto cleanup;

                case FILE_OP_UNKNOWN:
                    break;

                default:
                    break;
            }
        }
    } while(dwOpType != FILE_OP_EXIT);

cleanup:

    return dwError;

error:

    goto cleanup;
}


DWORD
ParseArgs(
    PSTR pszCommand,
    PDWORD pdwCommandType,
    PWSTR *ppszSourceFilePath,
    PWSTR *ppszRemoteFilePath,
    PBOOL pbOverWrite)
{
    PSTR pszField = NULL;
    DWORD dwLen = 0;
    DWORD dwParserState = PARSE_FILECLIENT_OPEN;
    PWSTR pszSourceFilePath = NULL;
    PWSTR pszRemoteFilePath = NULL;
    DWORD dwCommandType = FILE_OP_UNKNOWN;
    DWORD dwError = 0;
    size_t i = 0;
    BOOL bRemote = FALSE;
    BOOL bOverWrite = FALSE;

    for(i = 0; pszCommand[i] != '\0';)
    {
        dwError = GetWord(&pszCommand, &pszField, &dwLen);

        i += dwLen;

        switch (dwParserState)
        {
            case PARSE_FILECLIENT_UNKNOWN:
                goto error;

            case PARSE_FILECLIENT_OPEN:
                if (!strcmp(pszField, "cp"))
                {
                    dwParserState = PARSE_FILECLIENT_COPY;
                    dwCommandType = FILE_OP_COPY;
                    break;
                }
                else if (!strcmp(pszField, "mv"))
                {
                    dwParserState = PARSE_FILECLIENT_MOVE;
                    dwCommandType = FILE_OP_MOVE;
                    break;
                }
                else if (!strcmp(pszField, "rm"))
                {
                    dwParserState = PARSE_FILECLIENT_DELETE;
                    dwCommandType = FILE_OP_DELETE;
                    break;
                }
                else if (!strcmp(pszField, "ls"))
                {
                    dwParserState = PARSE_FILECLIENT_LIST;
                    dwCommandType = FILE_OP_LIST;
                    break;
                }
                else if (!strcmp(pszField, "exit"))
                {
                    dwParserState = PARSE_FILECLIENT_EXIT;
                    dwCommandType = FILE_OP_EXIT;
                    goto cleanup;
                }
                else
                {
                    printf("%s Invalid command", pszLikewiseStr);
                    ShowUsage();
                    dwParserState = PARSE_FILECLIENT_UNKNOWN;
                    dwCommandType = FILE_OP_UNKNOWN;
                    goto error;
                }
                break;

            case PARSE_FILECLIENT_DELETE:
            case PARSE_FILECLIENT_LIST:
                LwMbsToWc16s(pszField, &pszSourceFilePath);
                dwParserState = PARSE_FILECLIENT_EXIT;
                break;

            case PARSE_FILECLIENT_COPY:
            case PARSE_FILECLIENT_MOVE:
                if (!strcmp(pszField, "-f"))
                {
                    bOverWrite = TRUE;
                    break;
                }
                if (!bRemote)
                {
                    LwMbsToWc16s(pszField, &pszSourceFilePath);
                    bRemote = 1;
                }
                else
                {
                    dwParserState = PARSE_FILECLIENT_EXIT;
                    LwMbsToWc16s(pszField, &pszRemoteFilePath);
                }
                break;
            default:
                printf("%s Invalid command", pszLikewiseStr);
                ShowUsage();
                goto error;
        }
    }

    if (dwParserState != PARSE_FILECLIENT_EXIT)
    {
        printf("%s Invalid command", pszLikewiseStr);
        ShowUsage();
        goto error;
    }

    *ppszSourceFilePath = pszSourceFilePath;
    *ppszRemoteFilePath = pszRemoteFilePath;
    *pdwCommandType = dwCommandType;
    *pbOverWrite = bOverWrite;

cleanup:

    return dwError;

error:

    *ppszSourceFilePath = NULL;
    *ppszRemoteFilePath = NULL;
    *pdwCommandType = dwCommandType;
    *pbOverWrite = FALSE;

    if (pszSourceFilePath)
    {
        free(pszSourceFilePath);
    }

    if (pszRemoteFilePath)
    {
        free(pszRemoteFilePath);
    }

    goto cleanup;
}

static
DWORD
GetWord(
    PSTR *ppszField,
    PSTR *ppszValue,
    PDWORD pdwLen
    )
{

    int i = 0;
    PSTR pszField = *ppszField;
    DWORD dwStartIndex = 0;
    DWORD dwEndIndex = 0;
    PSTR pszValue = NULL;
    DWORD dwLen = 0;


    LwStripLeadingWhitespace(pszField);
    for(i = 0; pszField[i] != '\0'; i++)
    {
        if (pszField[i] ==  ' ')
        {
            if (dwEndIndex != 0)
            {
                /*Initial spaces...Skip it*/
                break;
            }
        }
        else
        {
            if(dwEndIndex == 0)
            {
                dwStartIndex = i;
            }
            dwEndIndex++;
        }
        dwLen++;
    }
    pszValue = calloc(dwLen + 1, 1);
    if (pszValue == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    memcpy(pszValue, pszField + dwStartIndex, dwLen);
    LwStripTrailingWhitespace(pszValue);
    pszField = pszField + i;
    pszValue[dwLen] = '\0';
    *pdwLen = dwLen;
    *ppszValue = pszValue;
    *ppszField = pszField;
    return 0;
}


static
DWORD
ListFiles(
    PWSTR pszSourceFilePath
    )
{
    LIKEWISE_FIND_DATA Data = {0};
    HANDLE hFindHandle = (HANDLE) -1;

    hFindHandle = FindFirstFile(pszSourceFilePath, &Data);
    if(hFindHandle == (HANDLE)-1)
    {
        if (GetLastError() == ERROR_GEN_FAILURE)
        {
            printf("\nError reading file path. Check to make sure lwio pvfs or rdr driver is running.\n");
        }
        else
        {
            printf("\nError Reading File. Error: %d\n", GetLastError());
        }
        return GetLastError();
    }

    do
    {
        PrintData(Data);
    } while(FindNextFile(hFindHandle, &Data));

    printf("\n");

    return 0;
}


static
void
ShowUsage()
{

    printf("\n %sLikewise File Client Test Suit",pszLikewiseStr);
    printf("\n");
    printf("\n To copy  files \n cp  -f sourcepath remotepath\n");
    printf("\n To move files \n mv -f sourcepath remotepath\n");
    printf("\n To delete files \n rm -f sourcepath remotepath\n");
    printf("\n To list files  \n ls filepath\n");

}


static
VOID
PrintData(
    LIKEWISE_FIND_DATA data
    )
{
    DWORD dwError = 0;
    SYSTEM_TIME timeX;

    if (data.FileName)
    {
        char * filename = awc16stombs(data.FileName);
        printf("\n %s", filename);
        free(filename);
    }
    else
    {
        char * altname = awc16stombs(data.Alternate);
        printf(" (%s)", altname);
        free(altname);
    }

    dwError = FileTimeToSystemTime(&data.ftCreationTime , &timeX);
    printf("\nCreated %d:%d:%d %d/%d/%d",
           timeX.wHour,
           timeX.wMinute,
           timeX.wSecond,
           timeX.wMonth,
           timeX.wDay,
           timeX.wYear);
    dwError = FileTimeToSystemTime(&data.ftLastAccessTime , &timeX);
    printf("\nAccessed %d:%d:%d %d/%d/%d",
           timeX.wHour,
           timeX.wMinute,
           timeX.wSecond,
           timeX.wMonth,
           timeX.wDay,
           timeX.wYear);
    dwError = FileTimeToSystemTime(&data.ftLastWriteTime , &timeX);
    printf("\nModified %d:%d:%d %d/%d/%d",
           timeX.wHour,
           timeX.wMinute,
           timeX.wSecond,
           timeX.wMonth,
           timeX.wDay,
           timeX.wYear);
}


