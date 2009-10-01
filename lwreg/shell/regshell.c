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
#include "histedit.h"

#define REGSHELL_ESC_CHAR '|'

typedef struct _EDITLINE_CLIENT_DATA
{
    int continuation;
    PREGSHELL_PARSE_STATE pParseState;
} EDITLINE_CLIENT_DATA, *PEDITLINE_CLIENT_DATA;



DWORD
RegShellListKeys(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    DWORD dwError = 0;
    DWORD dwSubKeyLen = 0;
    DWORD i = 0;
    PSTR pszSubKey = NULL;
    LW_WCHAR **ppSubKeys = NULL;

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);
    dwError = RegShellUtilGetKeys(
                  pParseState->hReg,
                  pParseState->pszRootKeyName,
                  pParseState->pszDefaultKey,
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
                  pParseState->pszRootKeyName,
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
                  pParseState->pszRootKeyName,
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
                  pParseState->pszRootKeyName,
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
                  pParseState->pszRootKeyName,
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
                  pParseState->pszRootKeyName,
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
        printf("%02x%s", pByteArray[i], (i+1)<dwByteArrayLen ? "," : "");
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
RegShellExportFile(
    HANDLE hReg,
    PREGSHELL_CMD_ITEM rsItem
    )
{
    DWORD dwError = 0;
    char szRegFileName[PATH_MAX + 1];
    FILE* fp = NULL;


    strcpy(szRegFileName, rsItem->args[0]);

    LwStripWhitespace(szRegFileName, TRUE, TRUE);

    fp = fopen(szRegFileName, "w");
    if (fp == NULL) {
        dwError = errno;
        goto error;
    }

    dwError = RegShellUtilExport(hReg,
                                 fp,
                                 NULL,
                                 NULL,
                                 0);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    if (fp)
    {
        fclose(fp);
    }
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
    DWORD dwMultiIndex = 0;
    PREGSHELL_UTIL_VALUE pValues = NULL;
    PSTR pszValueName = NULL;
    PBYTE pData = NULL;
    PSTR *ppszMultiStrArray = NULL;
    DWORD dwValueNameLenMax = 0;
    DWORD dwValueNameLen = 0;
    DWORD dwValue = 0;

    dwError = RegShellUtilGetValues(
                  pParseState->hReg,
                  pParseState->pszRootKeyName,
                  pParseState->pszDefaultKey,
                  rsItem->keyName,
                  &pValues,
                  &dwValuesLen);
    BAIL_ON_REG_ERROR(dwError);

    for (i=0; i<dwValuesLen; i++)
    {
        dwValueNameLen = wc16slen(pValues[i].pValueName);
        if (dwValueNameLen>dwValueNameLenMax)
        {
            dwValueNameLenMax = dwValueNameLen;
        }
    }
    dwValueNameLenMax++;

    for (i=0; i<dwValuesLen; i++)
    {
        if (dwError == 0)
        {
            dwError = LwWc16sToMbs(pValues[i].pValueName, &pszValueName);
            BAIL_ON_REG_ERROR(dwError);

#ifndef _DEBUG
            printf("%s%*s", pszValueName, (int) (strlen(pszValueName)-dwValueNameLenMax), "");
#else
            printf("ListValues: value='%s\n", pszValueName);
            printf("ListValues: dataLen='%d'\n", pValues[i].dwDataLen);
#endif
            switch (pValues[i].type)
            {
                case REG_SZ:
                    printf("REG_SZ:          \"%s\"\n",
                           (PSTR) pValues[i].pData);
                    break;

                case REG_DWORD:
                    printf("REG_DWORD:       ");


                    memcpy(&dwValue, pValues[i].pData, sizeof(DWORD));
                    dwValue = LW_HTOB32(dwValue);
                    RegShellDumpByteArray((PBYTE) &dwValue, 4);
                    printf("\n");
                    break;

                case REG_BINARY:
                    printf("REG_BINARY:      ");
                    RegShellDumpByteArray(pValues[i].pData,
                                          pValues[i].dwDataLen);
                    printf("\n");
                    break;

                case REG_MULTI_SZ:
                    dwError = ConvertByteArrayToMultiStrs(
                                  pValues[i].pData,
                                  pValues[i].dwDataLen,
                                  &ppszMultiStrArray);
                    BAIL_ON_REG_ERROR(dwError);
                    for (dwMultiIndex=0;
                         ppszMultiStrArray[dwMultiIndex];
                         dwMultiIndex++)
                    {
                        printf("%*sREG_MULTI_SZ[%d]: \"%s\"\n",
                               dwMultiIndex == 0 ? 0 :
                                   dwValueNameLenMax,
                                   "",
                               dwMultiIndex,
                               ppszMultiStrArray[dwMultiIndex]);

                    }
                    break;

                default:
                    printf("no handler for datatype %d\n", pValues[i].type);
            }
            LW_SAFE_FREE_STRING(pszValueName);
            pszValueName = NULL;
            LW_SAFE_FREE_MEMORY(pData);
            pData = NULL;
        }
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
    DWORD dwOpenRootKeyError = 0;
    PREGSHELL_CMD_ITEM rsItem = NULL;
    PCSTR pszErrorPrefix = NULL;
    PSTR pszPwd = NULL;
    PSTR pszToken = NULL;
    PSTR pszKeyName = NULL;
    PSTR pszNewKeyName = NULL;
    PSTR pszNewDefaultKey = NULL;
    PSTR pszStrtokState = NULL;
    BOOLEAN bChdirOk = TRUE;
    HKEY hRootKey = NULL;

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
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_LIST:
            case REGSHELL_CMD_DIRECTORY:
                pszErrorPrefix = "list: failed ";
                dwError = RegShellListKeys(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                printf("\n");
                if (pParseState->pszRootKeyName)
                {
                    dwError = RegShellListValues(pParseState, rsItem);
                    BAIL_ON_REG_ERROR(dwError);
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
                if (!pszToken)
                {
                    /*
                     * Handle special case where the only thing provided in
                     * the path is one or more \ characters (e.g [\\\]). In
                     * this case, strtok_r() return NULL when parsing
                     * a non-zero length pszKeyName string. This is essentially
                     * the cd \ case
                     */
                    if (pParseState->pszDefaultKey)
                    {
                        LW_SAFE_FREE_MEMORY(pParseState->pszDefaultKey);
                        return 0;
                    }
                    LW_SAFE_FREE_MEMORY(pParseState->pszRootKeyName);
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
                    dwOpenRootKeyError = RegOpenRootKey(
                                  pParseState->hReg,
                                  pszToken,
                                  &hRootKey);
                    if (dwOpenRootKeyError == 0)
                    {
                        RegCloseKey(pParseState->hReg, hRootKey);
                        LW_SAFE_FREE_MEMORY(pParseState->pszRootKeyName);
                        dwError = LwAllocateString(
                                      pszToken,
                                      &pParseState->pszRootKeyName);
                        BAIL_ON_REG_ERROR(dwError);
                    }
                    else if (strcmp(pszToken, "..") == 0)
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
                                    LW_SAFE_FREE_MEMORY(pszNewDefaultKey);
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
                        dwError = RegShellIsValidKey(
                                      pParseState->hReg,
                                      pParseState->pszRootKeyName,
                                      pszPwd);
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
                        dwError = RegShellIsValidKey(
                                      pParseState->hReg,
                                      pParseState->pszRootKeyName,
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
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_IMPORT:
                dwError = RegShellImportFile(pParseState->hReg, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_EXPORT:
                dwError = RegShellExportFile(pParseState->hReg, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_SET_HIVE:
                if (argc < 3)
                {
                    dwError = LW_ERROR_INVALID_CONTEXT;
                    goto error;
                }

                LW_SAFE_FREE_STRING(pParseState->pszRootKeyName);
                dwError = LwAllocateString(argv[2],
                                           &pParseState->pszRootKeyName);
                BAIL_ON_REG_ERROR(dwError);
                LW_SAFE_FREE_STRING(pParseState->pszDefaultKey);
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
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegLexOpen(&pParseState->lexHandle);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegIOBufferOpen(&pParseState->ioHandle);
    BAIL_ON_REG_ERROR(dwError);

    *ppParseState = pParseState;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pParseState->pszRootKeyName);
    RegCloseServer(pParseState->hReg);
    RegLexClose(pParseState->lexHandle);
    RegIOClose(pParseState->ioHandle);
    LW_SAFE_FREE_MEMORY(pParseState);
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

static char *
pfnRegShellPromptCallback(EditLine *el)
{
    static char promptBuf[1024] = "";
    EDITLINE_CLIENT_DATA *cldata = NULL;

    el_get(el, EL_CLIENTDATA, (void *) &cldata);
    snprintf(promptBuf, sizeof(promptBuf), "%s%s%s%s ",
             cldata->pParseState->pszRootKeyName ?
                 cldata->pParseState->pszRootKeyName : "",
             cldata->pParseState->pszDefaultKey ? "\\" : "",
             cldata->pParseState->pszDefaultKey ?
             cldata->pParseState->pszDefaultKey : "\\",
             cldata->continuation ? ">>>" : ">");
    return promptBuf;
}


DWORD
RegShellExecuteCmdLine(
    PREGSHELL_PARSE_STATE pParseState,
    PSTR pszCmdLine,
    DWORD dwCmdLineLen)
{
    DWORD dwError = 0;
    DWORD dwNewArgc = 0;
    PSTR *pszNewArgv = NULL;

    dwError = RegIOBufferSetData(
                  pParseState->ioHandle,
                  pszCmdLine,
                  dwCmdLineLen);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegShellCmdlineParseToArgv(
                  pParseState,
                  &dwNewArgc,
                  &pszNewArgv);
    if (dwError == 0)
    {
        dwError = RegShellProcessCmd(pParseState,
                                     dwNewArgc,
                                     pszNewArgv);
    }
    if (dwError)
    {
        PrintError("regshell", dwError);
        dwError = 0;
    }
    RegShellCmdlineParseFree(dwNewArgc, pszNewArgv);

cleanup:
    return dwError;

error:
    goto cleanup;

}


DWORD
RegShellProcessInteractiveEditLine(
    FILE *readFP,
    PREGSHELL_PARSE_STATE pParseState,
    PSTR pszProgramName)
{
    History *hist = NULL;
    HistEvent ev;
    EDITLINE_CLIENT_DATA el_cdata = {0};
    int num = 0;
    int ncontinuation = 0;
    int rv = 0;

    const char *buf = NULL;
    EditLine *el = NULL;
    DWORD dwError = 0;
    PSTR pszCmdLine = NULL;
    PSTR pszNewCmdLine = NULL;
    PSTR pszNumEnd = NULL;
    DWORD dwCmdLineLen = 0;
    DWORD dwEventNum = 0;
    BOOLEAN bHistFirst = FALSE;
    const char *hist_str = NULL;

    hist = history_init();
    history(hist, &ev, H_SETSIZE, 100);

    el = el_init(pszProgramName, stdin, stdout, stderr);

    /* Make configurable in regshellrc file */
    el_set(el, EL_EDITOR, "emacs");

    /* Signal handling in editline seems not to function... */
    el_set(el, EL_SIGNAL, 1);

    /* Set escape character from \ to | */
    el_set(el, EL_ESC_CHAR, (int) REGSHELL_ESC_CHAR);

    /* Editline prompt function; display info from pParseState */
    el_set(el, EL_PROMPT, pfnRegShellPromptCallback);

    /* Set regshell context */
    el_cdata.pParseState = pParseState;
    el_set(el, EL_CLIENTDATA, (void *) &el_cdata);

    /* Setup history context, and load previous history file */
    el_set(el, EL_HIST, history, hist);

    /* Retrieve this from user's home directory */
    history(hist, &ev, H_LOAD, ".regshell_history");

    /*
     * Bind j, k in vi command mode to previous and next line, instead
     * of previous and next history.
     */
    el_set(el, EL_BIND, "-a", "k", "ed-prev-line", NULL);
    el_set(el, EL_BIND, "-a", "j", "ed-next-line", NULL);

    /*
     * Source the user's defaults file.
     */
    el_source(el, NULL);

    while ((buf = el_gets(el, &num))!=NULL && num!=0)
    {
        if (num>1 && buf[num-2] == REGSHELL_ESC_CHAR)
        {
            ncontinuation = 1;
        }
        else
        {
            ncontinuation = 0;
        }

        el_cdata.continuation = ncontinuation;
        dwError = LwAllocateMemory(
                      dwCmdLineLen + num + 1,
                      (LW_PVOID) &pszNewCmdLine);
        BAIL_ON_REG_ERROR(dwError);
        if (pszCmdLine)
        {
            strncat(pszNewCmdLine, pszCmdLine, dwCmdLineLen);
            LW_SAFE_FREE_STRING(pszCmdLine);
        }
        if (ncontinuation)
        {
            num -= 2;
        }
        strncat(&pszNewCmdLine[dwCmdLineLen], buf, num);
        dwCmdLineLen += num;
        pszCmdLine = pszNewCmdLine;
        pszNewCmdLine = NULL;
        if (ncontinuation)
        {
            ncontinuation = 0;
            continue;
        }


        /*
         * Process history command recall (!nnn | !command syntax)
         */
        if (pszCmdLine[0] =='!')
        {
            /* !nnn case */
            if (isdigit((int)pszCmdLine[1]))
            {
                dwEventNum = strtol(&pszCmdLine[1], &pszNumEnd, 0);
                rv = history(hist, &ev, H_NEXT_EVENT, dwEventNum);
                if (rv == -1)
                {
                    printf("regshell: %d: event not found\n", dwEventNum);
                }
            }
            else
            {
                /*
                 * Handle !! command recall. !! recalls previous command,
                 * !!stuff appends "stuff" to end of previous command.
                 */
                if (pszCmdLine[1] == '!')
                {
                    rv = history(hist, &ev, H_FIRST);
                    if (rv == -1)
                    {
                        printf("regshell: !!: event not found\n");
                    }
                    else
                    {
                        bHistFirst = TRUE;
                    }
                }
                else
                {
                    /*
                     * !command case. Searches history for last occurrence
                     * of "command" in history list, and expands command line
                     * with that command.
                     */
                    hist_str = &pszCmdLine[1];
                    rv = history(hist, &ev, H_PREV_STR, hist_str);
                    if (rv == -1)
                    {
                        printf("regshell: %s: event not found\n", hist_str);
                    }
                }
            }

            if (rv == 0)
            {
                dwCmdLineLen = 0;

                if (bHistFirst)
                {
                    dwCmdLineLen += strlen(&pszCmdLine[2]);
                }
                else if (pszNumEnd)
                {
                    dwCmdLineLen += strlen(pszNumEnd);
                }
                dwCmdLineLen += strlen(ev.str);
                dwCmdLineLen++;
                dwError = LwAllocateMemory(
                              dwCmdLineLen,
                              (LW_PVOID) &pszNewCmdLine);
                BAIL_ON_REG_ERROR(dwError);

                strcpy(pszNewCmdLine, ev.str);
                dwCmdLineLen = strlen(pszNewCmdLine);
                if (pszNewCmdLine[dwCmdLineLen-1] == '\n')
                {
                    pszNewCmdLine[dwCmdLineLen-1] = '\0';
                }

                if (bHistFirst)
                {
                    strcat(pszNewCmdLine, &pszNewCmdLine[2]);
                }
                else if (pszNumEnd)
                {
                    strcat(pszNewCmdLine, pszNumEnd);
                }
                dwCmdLineLen = strlen(pszNewCmdLine);
                LW_SAFE_FREE_STRING(pszCmdLine);
                pszCmdLine = pszNewCmdLine;
                pszNewCmdLine = NULL;

                /*
                 * Display command from history list
                 */
                printf("%s\n", pszCmdLine);
            }
        }

        if (pszCmdLine && pszCmdLine[0] != '\n')
        {
            rv = history(hist, &ev, H_ENTER, pszCmdLine);
            if (strcmp(pszCmdLine, "history\n") == 0)
            {
                for (rv = history(hist, &ev, H_LAST);
                     rv != -1;
                     rv = history(hist, &ev, H_PREV))
                {
                    fprintf(stdout, "%4d %s", ev.num, ev.str);
                }
            }
            else
            {
                dwError = RegShellExecuteCmdLine(
                              pParseState,
                              pszCmdLine,
                              dwCmdLineLen);
            }
        }
        LW_SAFE_FREE_STRING(pszCmdLine);
        dwCmdLineLen = 0;
    }

    /* Save current regshell history */
    history(hist, &ev, H_SAVE, ".regshell_history");

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellProcessInteractive(
    FILE *readFP,
    PREGSHELL_PARSE_STATE parseState)
{
    CHAR cmdLine[8192] = {0};
    PSTR pszCmdLine = NULL;
    DWORD dwError = 0;
    PSTR pszTmpStr = NULL;
    BOOLEAN bDoPrompt = TRUE;
    BOOLEAN bFoundComment = FALSE;

    /* Interactive shell */
    do
    {
        if (bDoPrompt)
        {
            printf("%s%s%s> ",
                   parseState->pszRootKeyName,
                   parseState->pszDefaultKey ? "\\" : "",
                   parseState->pszDefaultKey ?
                   parseState->pszDefaultKey : "\\");
            fflush(stdout);
        }
        pszCmdLine = fgets(cmdLine, sizeof(cmdLine)-1, readFP);
        if (pszCmdLine)
        {
            if (strlen(cmdLine) == 1)
            {
                if (readFP != stdin)
                {
                    bDoPrompt = FALSE;
                }
                continue;
            }

            /* Ignore leading white space or # comment on lines */
            for (pszTmpStr=cmdLine;
                 (int)*pszTmpStr && isspace((int)*pszTmpStr);
                 pszTmpStr++)
            {
                ;
            }

            if (pszTmpStr && *pszTmpStr == '#')
            {
                printf("%s%s", bFoundComment ? "" : "\n", cmdLine);
                bDoPrompt = FALSE;
                bFoundComment = TRUE;
                continue;
            }

            if (cmdLine[strlen(cmdLine)-1] == '\n')
            {
               cmdLine[strlen(cmdLine)-1] = '\0';
            }
            if (readFP != stdin)
            {
                printf("%s\n", cmdLine);
            }
            bDoPrompt = TRUE;
            bFoundComment = FALSE;

            dwError = RegShellExecuteCmdLine(
                          parseState,
                          cmdLine,
                          strlen(cmdLine));
            BAIL_ON_REG_ERROR(dwError);
        }
    } while (!feof(readFP));
cleanup:
    return dwError;

error:
    if (dwError)
    {
        PrintError("regshell", dwError);
    }
    goto cleanup;
}


int main(int argc, char *argv[])
{
    DWORD dwError = 0;
    PSTR pszInFile = NULL;
    PREGSHELL_PARSE_STATE parseState = NULL;
    FILE *readFP = stdin;
    DWORD indx = 0;

    setlocale(LC_ALL, "");
    dwError = RegShellInitParseState(&parseState);
    BAIL_ON_REG_ERROR(dwError);

    indx = 1;
    while (argc>1 && argv[indx][0] == '-')
    {
        if (strcmp(argv[indx], "--file") == 0 ||
            strcmp(argv[indx], "-f") == 0)
        {
            argc--;
            indx++;
            if (argc>1)
            {
                pszInFile = argv[indx++];
                argc--;
            }
            if (pszInFile)
            {
                readFP = fopen(pszInFile, "r");
                if (!readFP)
                {
                    fprintf(stderr, "Error opening file '%s'\n", pszInFile);
                    return 0;
                }
            }
            dwError = RegShellProcessInteractive(readFP, parseState);
            BAIL_ON_REG_ERROR(dwError);
            if (readFP != stdin)
            {
                fclose(readFP);
            }
            return 0;
        }
        else if (strcmp(argv[indx], "--help") == 0 ||
            strcmp(argv[indx], "-h") == 0)
        {
            RegShellUsage(argv[0]);
            return 0;
        }
        else if (argv[indx][0] == '-')
        {
            printf("ERROR: unknown option %s\n", argv[indx]);
            RegShellUsage(argv[0]);
            return 0;
        }
    }

    if (argc == 1)
    {
        dwError = RegShellProcessInteractiveEditLine(
                      readFP,
                      parseState,
                      argv[0]);
        BAIL_ON_REG_ERROR(dwError);
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
        PrintError("regshell", dwError);
    }
    goto cleanup;
}
