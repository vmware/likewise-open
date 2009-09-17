/*
 * Copyright Likewise Software    2004-2009
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
 *       regshell.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry Shell application
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */

#include "rsutils.h"
#include "regshell.h"
#include <locale.h>


DWORD
RegShellListKeys(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    DWORD dwError = 0;
    DWORD dwSubKeyLen = 0;
    DWORD i = 0;
    PSTR pszSubKey = NULL;
    LW_WCHAR **ppSubKeys;

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);
    dwError = RegShellUtilGetKeys(
                  pParseState->hReg,
                  pParseState-> pszDefaultKey,
                  rsItem->keyName,
                  &ppSubKeys,
                  &dwSubKeyLen);
    BAIL_ON_REG_ERROR(dwError);

    for (i=0; i<dwSubKeyLen; i++)
    {
        dwError = LwWc16sToMbs(ppSubKeys[i], &pszSubKey);
        BAIL_ON_REG_ERROR(dwError);

#ifndef _DEBUG
        printf("%s\n", pszSubKey);
#else
        printf("SubKey %d name is '%s'\n", i, pszSubKey);
#endif
        LW_SAFE_FREE_STRING(pszSubKey);
    }
cleanup:
    return dwError;

error:
    for (i=0; i<dwSubKeyLen; i++)
    {
        LW_SAFE_FREE_MEMORY(ppSubKeys[i]);
    }
    LW_SAFE_FREE_MEMORY(ppSubKeys);
    goto cleanup;
}


DWORD
RegShellAddKey(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);

    dwError = RegShellUtilAddKey(
                  pParseState->hReg,
                  pParseState->pszDefaultKey,
                  rsItem->keyName);

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
RegShellDeleteKey(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);

    dwError = RegShellUtilDeleteKey(
                  pParseState->hReg,
                  pParseState->pszDefaultKey,
                  rsItem->keyName);

cleanup:
    return dwError;
error:
    goto cleanup;
}


DWORD
RegShellDeleteTree(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);

    dwError = RegShellUtilDeleteTree(
                  pParseState->hReg,
                  pParseState->pszDefaultKey,
                  rsItem->keyName);

cleanup:
    return dwError;
error:
    goto cleanup;
}


DWORD
RegShellDeleteValue(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);
    dwError = RegShellUtilDeleteValue(
                  pParseState->hReg,
                  pParseState->pszDefaultKey,
                  rsItem->keyName,
                  rsItem->valueName);
cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellSetValue(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{

    LW_PVOID data = NULL;
    DWORD dataLen = 0;
    DWORD dwError = 0;

    switch (rsItem->type)
    {
        case REG_MULTI_SZ:
            data = rsItem->args;
            break;
        case REG_SZ:
            data = rsItem->args[0];
            break;
        case REG_DWORD:
        case REG_BINARY:
            data = rsItem->binaryValue;
            dataLen = rsItem->binaryValueLen;
            break;
        default:
            break;
    }
    dwError = RegShellUtilSetValue(
                  pParseState->hReg,
                  pParseState->pszDefaultKey,
                  rsItem->keyName,
                  rsItem->valueName,
                  rsItem->type,
                  data,
                  dataLen);
    return dwError;
}



DWORD
RegShellDumpByteArray(
    PBYTE pByteArray,
    DWORD dwByteArrayLen)
{
    DWORD i = 0;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pByteArray);

    for (i=0; i<dwByteArrayLen; i++)
    {
        printf("%02x", pByteArray[i]);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellImportFile(
    HANDLE hReg,
    PREGSHELL_CMD_ITEM rsItem)
{
    IMPORT_CONTEXT ctx = {0};
    HANDLE parseH = NULL;
    DWORD dwError = 0;


    ctx.hReg = hReg;

    dwError = RegParseOpen(rsItem->args[0], NULL, NULL, &parseH);
    BAIL_ON_REG_ERROR(dwError);

    RegParseInstallCallback(parseH, RegShellUtilImportCallback, &ctx, NULL);

    dwError = RegParseRegistry(parseH);
    BAIL_ON_REG_ERROR(dwError);

    RegParseClose(parseH);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellListValues(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    DWORD dwError = 0;
    DWORD dwValuesLen = 0;
    DWORD i = 0;
    PREGSHELL_UTIL_VALUE pValues = NULL;
    PSTR pszValueName = NULL;
    PBYTE pData = NULL;

    dwError = RegShellUtilGetValues(
                  pParseState->hReg,
                  pParseState->pszDefaultKey,
                  rsItem->keyName,
                  &pValues,
                  &dwValuesLen);
    BAIL_ON_REG_ERROR(dwError);

    for (i=0; i<dwValuesLen; i++)
    {
        if (dwError == 0)
        {
            dwError = LwWc16sToMbs(pValues[i].pValueName, &pszValueName);
            BAIL_ON_REG_ERROR(dwError);

#ifndef _DEBUG
            printf("%s\n  ", pszValueName);
#else
            printf("ListValues: value='%s\n", pszValueName);
            printf("ListValues: dataLen='%d'\n", dwDataLen);
#endif
            switch (pValues[i].type)
            {
                case REG_SZ:
                    printf("REG_SZ:     pData='%s'\n",
                           (PSTR) pValues[i].pData);
                    break;

                case REG_DWORD:
                    printf("REG_DWORD:  pData=<%08x>\n",
                           *((PDWORD) pValues[i].pData));
                    break;

                case REG_BINARY:
                    printf("REG_BINARY: ");
                    RegShellDumpByteArray(pValues[i].pData,
                                          pValues[i].dwDataLen);
                    printf("\n");
                    break;

                default:
                    printf("no handler for datatype %d\n", pValues[i].type);
            }
            LW_SAFE_FREE_STRING(pszValueName);
            pszValueName = NULL;
            LW_SAFE_FREE_MEMORY(pData);
            pData = NULL;
        }
        printf("\n");
    }
cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellProcessCmd(
    PREGSHELL_PARSE_STATE pParseState,
    DWORD argc,
    PSTR *argv)
{

    DWORD dwError = 0;
    PREGSHELL_CMD_ITEM rsItem = NULL;
    PCSTR pszErrorPrefix = NULL;
    PSTR pszPwd = NULL;
    PSTR pszToken = NULL;
    PSTR pszKeyName = NULL;
    PSTR pszNewKeyName = NULL;
    PSTR pszNewDefaultKey = NULL;
    PSTR pszStrtokState = NULL;
    BOOLEAN bChdirOk = TRUE;

    dwError = RegShellCmdParse(argc, argv, &rsItem);
    if (dwError == 0)
    {
#ifdef _DEBUG
        RegShellDumpCmdItem(rsItem);
#endif
        switch (rsItem->command)
        {
            case REGSHELL_CMD_LIST_KEYS:
                pszErrorPrefix = "list_keys: failed ";
                dwError = RegShellListKeys(pParseState, rsItem);
                if (dwError)
                {
                    PrintError(pszErrorPrefix, dwError);
                    dwError = 0;
                }
                break;

            case REGSHELL_CMD_LIST:
            case REGSHELL_CMD_DIRECTORY:
                pszErrorPrefix = "list: failed ";
                dwError = RegShellListKeys(pParseState, rsItem);
                if (dwError)
                {
                    PrintError(pszErrorPrefix, dwError);
                    dwError = 0;
                }
                printf("\n");
                dwError = RegShellListValues(pParseState, rsItem);
                if (dwError)
                {
                    PrintError(pszErrorPrefix, dwError);
                    dwError = 0;
                }
                break;

            case REGSHELL_CMD_ADD_KEY:
                pszErrorPrefix = "add_key: failed";
                dwError = RegShellAddKey(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_DELETE_KEY:
                pszErrorPrefix = "delete_key: failed ";
                dwError = RegShellDeleteKey(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_DELETE_VALUE:
                pszErrorPrefix = "delete_value: failed ";
                dwError = RegShellDeleteValue(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_DELETE_TREE:
                pszErrorPrefix = "delete_tree: failed ";
                dwError = RegShellDeleteTree(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_SET_VALUE:
                pszErrorPrefix = "set_value: failed ";
                dwError = RegShellSetValue(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_ADD_VALUE:
                pszErrorPrefix = "add_value: failed ";
                dwError = RegShellSetValue(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_HELP:
                RegShellUsage(argv[0]);
                break;

            case REGSHELL_CMD_CHDIR:
                dwError = LwAllocateString(&argv[2][1], &pszNewKeyName);
                BAIL_ON_REG_ERROR(dwError);
                pszNewKeyName [strlen(pszNewKeyName) - 1] = '\0';
                pszKeyName = pszNewKeyName;
                pszToken = strtok_r(pszKeyName, "\\/", &pszStrtokState);
                if (!pszToken && strlen(pszKeyName) > 0)
                {
                    /*
                     * Handle special case where the only thing provided in
                     * the path is one or more \ characters (e.g [\\\]). In
                     * this case, strtok_r() return NULL when parsing
                     * a non-zero length pszKeyName string. This is essentially
                     * the cd \ case
                     */
                    LwFreeMemory(pParseState->pszDefaultKey);
                    pParseState->pszDefaultKey = NULL;
                }


                if (pParseState->pszDefaultKey)
                {
                    /*
                     * Another special case when the path begins with a / or \.
                     * Force the current working directory to root.
                     */
                    if (pszNewKeyName[0] != '/' && pszNewKeyName[0] != '\\')
                    {
                        dwError = LwAllocateString(pParseState->pszDefaultKey,
                                                   &pszNewDefaultKey);
                        BAIL_ON_REG_ERROR(dwError);
                    }
                }
                while (pszToken)
                {
                    pszKeyName = NULL;
                    if (strcmp(pszToken, "..") == 0)
                    {
                        if (pszNewDefaultKey)
                        {
                            pszPwd = strrchr(pszNewDefaultKey,
                                             '\\');
                            if (pszPwd)
                            {
                                    pszPwd[0] = '\0';
                            }
                            else
                            {
                                pszPwd = strrchr(pszNewDefaultKey,
                                                 '/');
                                if (pszPwd)
                                {
                                    pszPwd[0] = '\0';
                                }
                                else
                                {
                                    LwFreeMemory(pszNewDefaultKey);
                                    pszNewDefaultKey = NULL;
                                }
                            }
                        }
                    }
                    else if (strcmp(pszToken, ".") == 0)
                    {
                        /* This is a no-op */
                    }
                    else if (strcmp(pszToken, "...") == 0)
                    {
                        /* This is a broken path! */
                        dwError = LW_ERROR_INVALID_CONTEXT;
                        BAIL_ON_REG_ERROR(dwError);

                    }
                    else if (pszNewDefaultKey)
                    {
                        /* Append this token to current relative path */
                        dwError = LwAllocateMemory(
                                      strlen(pszToken) +
                                      strlen(pszNewDefaultKey)+3,
                                      (LW_PVOID) &pszPwd);
                        BAIL_ON_REG_ERROR(dwError);
                        strcpy(pszPwd, pszNewDefaultKey);
                        strcat(pszPwd, "\\");
                        strcat(pszPwd, pszToken);
                        dwError = RegShellIsValidKey(pParseState->hReg, pszPwd);
                        if (dwError)
                        {
                            dwError = 0;
                            printf("cd: key not valid '%s'\n", pszPwd);
                            LW_SAFE_FREE_MEMORY(pszPwd);
                            pszPwd = NULL;
                            bChdirOk = FALSE;
                            break;
                        }
                        else
                        {
                            LW_SAFE_FREE_MEMORY(pszNewDefaultKey);
                            pszNewDefaultKey = pszPwd;
                            pszPwd = NULL;
                        }
                    }
                    else
                    {
                        dwError = RegShellIsValidKey(pParseState->hReg,
                                                     pszToken);
                        if (dwError)
                        {
                            dwError = 0;
                            printf("cd: key not valid '%s'\n", pszToken);
                            bChdirOk = FALSE;
                            break;
                        }
                        else
                        {
                            dwError = LwAllocateString(
                                          pszToken,
                                          &pszNewDefaultKey);
                            BAIL_ON_REG_ERROR(dwError);
                        }
                    }
                    pszToken = strtok_r(pszKeyName, "\\/", &pszStrtokState);
                }
                if (bChdirOk)
                {
                    if (pParseState->pszDefaultKey)
                    {
                        LW_SAFE_FREE_MEMORY(pParseState->pszDefaultKey);
                    }
                    pParseState->pszDefaultKey = pszNewDefaultKey;
                }
                else
                {
                    LW_SAFE_FREE_MEMORY(pszNewDefaultKey);
                }
                break;

            case REGSHELL_CMD_PWD:
                if (pParseState->pszDefaultKey)
                {
                    printf("'%s'\n\n",
                            pParseState->pszDefaultKey);
                }
                else
                {
                    printf("'%s'\n\n", "\\");
                }
                break;

            case REGSHELL_CMD_QUIT:
                exit(0);
                break;

            case REGSHELL_CMD_LIST_VALUES:
                pszErrorPrefix = "list_values: failed ";
                dwError = RegShellListValues(pParseState, rsItem);
                if (dwError)
                {
                    PrintError(pszErrorPrefix, dwError);
                    dwError = 0;
                }
                break;

            case REGSHELL_CMD_IMPORT:
                dwError = RegShellImportFile(pParseState->hReg, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            default:
                break;
        }
    }
    else
    {
        printf("%s: error parsing command, invalid syntax\n", argv[0]);
        RegShellUsage(argv[0]);
    }

cleanup:
    RegShellCmdParseFree(rsItem);

    LwFreeMemory(pszNewKeyName);
    return dwError;

error:
    PrintError("regshell", dwError);
    goto cleanup;
}


DWORD
RegShellInitParseState(
    PREGSHELL_PARSE_STATE *ppParseState)
{
    DWORD dwError = 0;
    PREGSHELL_PARSE_STATE pParseState = NULL;

    BAIL_ON_INVALID_POINTER(ppParseState);


    dwError = LwAllocateMemory(
                  sizeof(REGSHELL_PARSE_STATE),
                  (LW_PVOID) &pParseState);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenServer(&pParseState->hReg);

    dwError = RegLexOpen(&pParseState->lexHandle);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegIOBufferOpen(&pParseState->ioHandle);
    BAIL_ON_REG_ERROR(dwError);

    *ppParseState = pParseState;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellCloseParseState(
    PREGSHELL_PARSE_STATE pParseState)
{
    DWORD dwError = 0;
    BAIL_ON_INVALID_POINTER(pParseState);


    RegIOClose(pParseState->ioHandle);
    RegLexClose(pParseState->lexHandle);
    RegCloseServer(pParseState->hReg);
    LwFreeMemory(pParseState);

cleanup:
    return dwError;

error:
    goto cleanup;
}


int main(int argc, char *argv[])
{
    DWORD dwError = 0;
    PCSTR pszErrorPrefix = NULL;
    DWORD dwNewArgc = 0;
    PSTR *pszNewArgv = NULL;
    CHAR cmdLine[8192] = {0};
    PSTR pszCmdLine = NULL;
    PREGSHELL_PARSE_STATE parseState = NULL;

    setlocale(LC_ALL, "");
    dwError = RegShellInitParseState(&parseState);
    BAIL_ON_REG_ERROR(dwError);

    /* Interactive shell */
    if (argc == 1)
    {
        do
        {
            printf("%s> ",
                   parseState->pszDefaultKey ?
                   parseState->pszDefaultKey : "\\");
            fflush(stdout);
            pszCmdLine = fgets(cmdLine, sizeof(cmdLine)-1, stdin);
            if (cmdLine[strlen(cmdLine)-1] == '\n')
            {
               cmdLine[strlen(cmdLine)-1] = '\0';
            }
            if (strlen(cmdLine) > 0)
            {
                dwError = RegIOBufferSetData(
                              parseState->ioHandle,
                              cmdLine,
                              strlen(cmdLine));
                BAIL_ON_REG_ERROR(dwError);

                dwError = RegShellCmdlineParseToArgv(
                              parseState,
                              &dwNewArgc,
                              &pszNewArgv);

                if (dwError == 0 && dwNewArgc > 0 && pszNewArgv)
                {
                    dwError = RegShellProcessCmd(parseState,
                                                 dwNewArgc,
                                                 pszNewArgv);
                    if (dwError == LW_ERROR_INVALID_CONTEXT)
                    {
                        PrintError(pszErrorPrefix, dwError);
                        dwError = 0;
                    }
                    RegShellCmdlineParseFree(dwNewArgc, pszNewArgv);
                }
                else
                {
                    printf("regshell: unable to parse command '%s'\n\n",
                           cmdLine);
                }
            }
        } while (!feof(stdin));

    }
    else
    {
        dwError = RegShellProcessCmd(parseState, argc, argv);
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    RegShellCloseParseState(parseState);
    return dwError;

error:
    if (dwError)
    {
        PrintError(pszErrorPrefix, dwError);
    }
    goto cleanup;
}
