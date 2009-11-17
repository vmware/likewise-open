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
 *       cmdparse .c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry Shell command-line parser functionalty
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */


/*
 * regshell (Registry Shell) command grammar
 *
 * list_keys ["[KeyName]"]
 * list_values ["[KeyName]"]
 * add_key "[KeyName]"
 * cd "[KeyName]"
 * delete_key "[KeyName]"
 * delete_tree "[KeyName]"
 * set_value ["[KeyName]"] "ValueName" type "Value" ["Value2"] ["Value3"] [...]
 * add_value ["[KeyName]"] "ValueName" type "Value" ["Value2"] ["Value3"] [...]
 *
 * Note: "KeyName" format is [HKEY_THIS_MACHINE/Subkey1/SubKey2]. ["[KeyName]"]
 * means the key parameter is optional.
 *
 * Token sequence from reglex for above comands:
 * list_keys:   REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 * list_values: REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 * add_key:     REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 * add_value:   REGLEX_PLAIN_TEXT [REGLEX_REG_KEY]
 *                  REGLEX_REG_SZ|REGLEX_PLAIN_TEXT REGLEX_PLAIN_TEXT
 *                  REGLEX_REG_SZ ...
 * cd:          REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 * delete_key:  REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 * delete_tree: REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 * set_value:   REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 *                  REGLEX_REG_SZ|REGLEX_PLAIN_TEXT
 *                  REGLEX_PLAIN_TEXT REGLEX_REG_SZ ...
 *
 * Supported types:
 *   REG_SZ
 *   REG_DWORD
 *   REG_MULTISZ
 *   REG_BINARY
 *
 *
 *     BNF Grammar for Registry Shell
 *
 * <regshell_command_line> ::=
 *     <regshell_command_verb> | <regshell_command_verb> <regshell_keyname> |
 *     <regshell_command_verb> <regshell_value_name>
 *     <regshell_value_type> <regshell_values>
 *
 * <regshell_command_verb> ::= "list_keys" | "list_values" | "add_key" |
 *                             "add_value" | "cd" | "delete_key" |
 *                             "delete_tree" | "set_value"
 *
 * <regshell_key_name> ::= "[" <key_path> "]"
 *   <key_path> ::= <name_chars> <end_of_line>
 *
 * <regshell_value_name> ::= <name_chars>
 *
 * <name_chars> ::= <name_char> <name_chars>
 * <name_char> ::= [a-zA-Z0-9\\/_-{}\*\| \$+=#<>~&®]
 * <end_of_line> ::= "\r\n" | "\r"
 *
 * <regshell_value_type> ::= REG_SZ | REG_DWORD | REG_MULTISZ | REG_BINARY
 *
 * <regshell_values> ::= <value_data> <regshell_values>
 *   <value_data> ::= "'" <value_chars> "'" <end_of_line>
 *   <value_chars> ::= <value_char> <value_chars>
 *   <value_char> ::= UCS2-CHAR | UTF-8-CHAR
 */


#if 0
#define _LW_DEBUG
#endif
#include "regshell.h"

typedef enum _REGSHELL_HEXSTRING_STATE_E
{
    REGSHELL_HEX_START,
    REGSHELL_HEX_LEADING_SPACE,
    REGSHELL_HEX_HEXDIGIT,
    REGSHELL_HEX_SPACE_SEPARATOR,
    REGSHELL_HEX_COMMA_SEPARATOR,
    REGSHELL_HEX_END,
    REGSHELL_HEX_ERROR,
    REGSHELL_HEX_STOP,
} REGSHELL_HEXSTRING_STATE_E;




static REGSHELL_CMD_ID shellCmds[] = {
    { "", REGSHELL_CMD_NONE                   },
    { "list_keys", REGSHELL_CMD_LIST_KEYS     },
    { "dir", REGSHELL_CMD_DIRECTORY           },
    { "ls", REGSHELL_CMD_LIST                 },
    { "list_values", REGSHELL_CMD_LIST_VALUES },
    { "add_key", REGSHELL_CMD_ADD_KEY         },
    { "add_value", REGSHELL_CMD_ADD_VALUE     },
    { "cd", REGSHELL_CMD_CHDIR                },
    { "delete_key", REGSHELL_CMD_DELETE_KEY   },
    { "delete_value", REGSHELL_CMD_DELETE_VALUE },
    { "delete_tree", REGSHELL_CMD_DELETE_TREE },
    { "set_value", REGSHELL_CMD_SET_VALUE     },
    { "set_hive", REGSHELL_CMD_SET_HIVE       },
    { "pwd", REGSHELL_CMD_PWD                 },
    { "help", REGSHELL_CMD_HELP               },
    { "exit", REGSHELL_CMD_QUIT               },
    { "quit", REGSHELL_CMD_QUIT               },
    { "import", REGSHELL_CMD_IMPORT           },
    { "export", REGSHELL_CMD_EXPORT           },
    { "upgrade", REGSHELL_CMD_UPGRADE         },
};


DWORD
RegShellCmdEnumToString(
    REGSHELL_CMD_E cmdEnum,
    PCHAR cmdString)
{
    DWORD dwError = 0;

    if (cmdEnum<sizeof(shellCmds)/sizeof(shellCmds[0]))
    {
        strcpy(cmdString, shellCmds[cmdEnum].pszCommand);
    }
    else
    {
        sprintf(cmdString, "Invalid Command: %d", cmdEnum);
        dwError = LWREG_ERROR_INVALID_CONTEXT;
    }
    return dwError;
}


DWORD
RegShellCmdStringToEnum(
    PCHAR cmdString,
    PREGSHELL_CMD_E pCmdEnum)
{


    DWORD i = 0;
    DWORD dwError = LWREG_ERROR_INVALID_CONTEXT;
    REGSHELL_CMD_E cmd = REGSHELL_CMD_NONE;

    BAIL_ON_INVALID_POINTER(pCmdEnum);

    for (i=0; i<sizeof(shellCmds)/sizeof(shellCmds[0]); i++)
    {
        if (strcasecmp(cmdString, shellCmds[i].pszCommand) == 0)
        {
            cmd = shellCmds[i].eCommand;
            dwError = 0;
            break;
        }
    }
    *pCmdEnum = cmd;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellCmdParseCommand(
    REGSHELL_CMD_E cmd,
    DWORD argc,
    PCHAR *argv,
    PREGSHELL_CMD_ITEM *pCmdItem)
{
    DWORD dwError = 0;
    PREGSHELL_CMD_ITEM pNewCmdItem = NULL;

    BAIL_ON_INVALID_POINTER(argv);
    BAIL_ON_INVALID_POINTER(pCmdItem);

    dwError = LW_RTL_ALLOCATE((PVOID*)&pNewCmdItem, REGSHELL_CMD_ITEM, sizeof(*pNewCmdItem));
    BAIL_ON_REG_ERROR(dwError);

    pNewCmdItem->command = cmd;
    *pCmdItem = pNewCmdItem;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellCmdParseFree(
    PREGSHELL_CMD_ITEM pCmdItem)
{
    DWORD dwError = 0;
    BAIL_ON_INVALID_POINTER(pCmdItem);

    LWREG_SAFE_FREE_MEMORY(pCmdItem->keyName);
    pCmdItem->keyName = NULL;

    LWREG_SAFE_FREE_MEMORY(pCmdItem->valueName);
    pCmdItem->valueName = NULL;

    LWREG_SAFE_FREE_MEMORY(pCmdItem->binaryValue);
    pCmdItem->binaryValue = NULL;
    LWREG_SAFE_FREE_MEMORY(pCmdItem);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellCmdParseKeyName(
    PREGSHELL_PARSE_STATE pParseState,
    REGSHELL_CMD_E cmd,
    DWORD argc,
    PCHAR *argv,
    PREGSHELL_CMD_ITEM *pRetCmdItem)
{
    DWORD dwError = 0;
    DWORD dwRootKeyError = LWREG_ERROR_NO_SUCH_KEY;
    DWORD keyNameLen = 0;
    DWORD dwOffset = 0;
    PCHAR pszKeyName = NULL;
    PSTR pszBackslash = NULL;
    PSTR pszTmpKey = NULL;
    PREGSHELL_CMD_ITEM pCmdItem = NULL;
    HKEY hRootKey = NULL;


    BAIL_ON_INVALID_POINTER(argv);
    BAIL_ON_INVALID_POINTER(pRetCmdItem);
    pszKeyName = argv[2];
    if (pszKeyName[0] == '[')
    {
        dwOffset++;
    }
    dwError = LwRtlCStringDuplicate(&pszTmpKey, &pszKeyName[dwOffset]);
    BAIL_ON_REG_ERROR(dwError);
    pszBackslash = strchr(pszTmpKey, '\\');
    if (pszBackslash)
    {
        *pszBackslash = '\0';
        dwRootKeyError = RegOpenKeyExA(
                             pParseState->hReg,
                             NULL,
                             pszTmpKey,
                             0,
                             0,
                             &hRootKey);
        if (dwRootKeyError == 0)
        {
            RegCloseKey(pParseState->hReg, hRootKey);
            pParseState->pszFullRootKeyName = pszTmpKey;
            *pszBackslash = '\0';
            pszKeyName = pszBackslash+1;
        }
    }

    dwError = RegShellCmdParseCommand(
                  cmd,
                  argc,
                  argv,
                  &pCmdItem);
    BAIL_ON_REG_ERROR(dwError);

    keyNameLen = strlen(pszKeyName);
    if (pszKeyName[0] == '[' && pszKeyName[keyNameLen-1] == ']')
    {
        dwError = LwRtlCStringDuplicate(&pCmdItem->keyName, pszKeyName);
        BAIL_ON_REG_ERROR(dwError);
        /*
         * Copy only the stuff between the [ ] delimiters. The copied string is
         * big enough to do this.
         */
        strcpy(pCmdItem->keyName, &pszKeyName[1]);
        pCmdItem->keyName[keyNameLen - 2] = '\0';
    }
    else
    {
        dwError = LW_RTL_ALLOCATE((PVOID*)&pCmdItem->keyName, CHAR, sizeof(CHAR)*(keyNameLen + 2));
        BAIL_ON_REG_ERROR(dwError);

        strcat(pCmdItem->keyName, "\\");
        strcat(pCmdItem->keyName, pszKeyName);
    }

    keyNameLen = strlen(pCmdItem->keyName);
    if (pCmdItem->keyName[keyNameLen-1] == ']')
    {
        pCmdItem->keyName[keyNameLen-1] = '\0';
    }
    if (dwRootKeyError == 0)
    {
        pParseState->pszFullKeyPath = pCmdItem->keyName;
    }
    *pRetCmdItem = pCmdItem;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellParseStringType(
    PCHAR pszType,
    PREG_DATA_TYPE pType,
    PDWORD pBackendType)
{
    DWORD i = 0;
    DWORD dwError = 0;
    DWORD backendType = 0;
    REG_DATA_TYPE type = REG_UNKNOWN;

    static REGSHELL_REG_TYPE_ENTRY typeStrs[] = {
        { "REG_DWORD", REG_DWORD, RRF_RT_REG_DWORD             },
        { "REG_SZ", REG_SZ, RRF_RT_REG_SZ                      },
        { "REG_BINARY", REG_BINARY, RRF_RT_REG_BINARY          },
        { "REG_NONE", REG_NONE, RRF_RT_REG_NONE                },
        { "REG_EXPAND_SZ", REG_EXPAND_SZ, RRF_RT_REG_EXPAND_SZ },
        { "REG_MULTI_SZ", REG_MULTI_SZ, RRF_RT_REG_MULTI_SZ    },
        { "REG_RESOURCE_LIST", REG_RESOURCE_LIST, RRF_RT_ANY   },
        { "REG_FULL_RESOURCE_DESCRIPTOR",
          REG_FULL_RESOURCE_DESCRIPTOR,
          RRF_RT_ANY                                           },
        { "REG_RESOURCE_REQUIREMENTS_LIST",
           REG_RESOURCE_REQUIREMENTS_LIST ,
           RRF_RT_ANY                                          },
        { "REG_QUADWORD", REG_QWORD, RRF_RT_QWORD              },
        { "REG_KEY", REG_KEY, RRF_RT_REG_SZ                    },
        { "REG_KEY_DEFAULT", REG_KEY_DEFAULT, RRF_RT_REG_SZ    },
        { "REG_PLAIN_TEXT", REG_PLAIN_TEXT, RRF_RT_REG_SZ      },
    };

    BAIL_ON_INVALID_POINTER(pType);

    for (i=0; i<sizeof(typeStrs)/sizeof(REGSHELL_REG_TYPE_ENTRY); i++)
    {
        if (strcasecmp(pszType, typeStrs[i].pszType) == 0)
        {
            type = typeStrs[i].type;
            backendType = typeStrs[i].backendType;
            break;
        }
    }
    *pType = type;
    if (pBackendType)
    {
        *pBackendType = backendType;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


/*
 * Return the longest contiguous extent of hex digits before
 * a separator character or the end of string is found
 */
DWORD
RegShellHexStringExtentLen(
    PSTR pszHexString,
    PDWORD pdwHexLen)
{
    DWORD dwIndx = 0;
    DWORD dwCount = 0;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pszHexString);

    for (dwIndx=0; pszHexString[dwIndx]; dwIndx++)
    {
        if (!isxdigit((int) pszHexString[dwIndx]))
        {
            break;
        }
        dwCount++;
    }

    *pdwHexLen = dwCount;

cleanup:
    return dwError;

error:
    goto cleanup;
}




DWORD
RegShellImportBinaryString(
    PCHAR pszHexString,
    PUCHAR *ppBinaryValue,
    PDWORD pBinaryValueLen)
{
    DWORD dwError = 0;
    CHAR pszHexArray[3];
    REGSHELL_HEXSTRING_STATE_E state = REGSHELL_HEX_START;
    PCHAR cursor = NULL;
    DWORD hexDigitCount = 0;
    DWORD hexDigitModCount = 0;
    PUCHAR binaryValue = NULL;
    DWORD binaryValueLen = 0;
    DWORD dwHexStringLen = 0;

    BAIL_ON_INVALID_POINTER(pszHexString);
    BAIL_ON_INVALID_POINTER(ppBinaryValue);
    BAIL_ON_INVALID_POINTER(pBinaryValueLen);

    dwError = LW_RTL_ALLOCATE((PVOID*)&binaryValue, UCHAR, sizeof(UCHAR) * strlen(pszHexString) / 2);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegShellHexStringExtentLen(
                  pszHexString,
                  &dwHexStringLen);
    BAIL_ON_REG_ERROR(dwError);

    if (dwHexStringLen % 2)
    {
        hexDigitModCount = 1;
    }
    cursor = pszHexString;
    do
    {
        if (*cursor == '\0')
        {
            if (hexDigitCount > 0)
            {
                state = REGSHELL_HEX_END;
            }
            else
            {
                break;
            }
        }

        switch (state)
        {
            case REGSHELL_HEX_START:
                if (hexDigitModCount)
                {
                    pszHexArray[0] = '0';
                    pszHexArray[1] = '\0';
                }
                hexDigitCount = hexDigitModCount;
                hexDigitModCount = 0;
                if (*cursor == ' ')
                {
                    state = REGSHELL_HEX_LEADING_SPACE;
                }
                else if (isxdigit((int)*cursor))
                {
                    state = REGSHELL_HEX_HEXDIGIT;
                }
                else
                {
                    state = REGSHELL_HEX_ERROR;
                }
                break;

            case REGSHELL_HEX_LEADING_SPACE:
                if (*cursor == ' ')
                {
                    cursor++;
                }
                else if (isxdigit((int)*cursor))
                {
                    state = REGSHELL_HEX_HEXDIGIT;
                }
                else
                {
                    state = REGSHELL_HEX_ERROR;
                }
                break;

            case REGSHELL_HEX_HEXDIGIT:
                if (isxdigit((int)*cursor))
                {
                    pszHexArray[hexDigitCount++] = *cursor++;
                    pszHexArray[hexDigitCount] = '\0';
                    if (hexDigitCount == 2 && isxdigit((int)*cursor))
                    {
                        state = REGSHELL_HEX_END;
                    }
                }
                else if (*cursor == ' ')
                {
                    state = REGSHELL_HEX_SPACE_SEPARATOR;
                }
                else if (*cursor == ',')
                {
                    state = REGSHELL_HEX_COMMA_SEPARATOR;
                    cursor++;
                }
                else if (*cursor == '\n')
                {
                    dwError = RegShellHexStringExtentLen(
                                  cursor,
                                  &dwHexStringLen);
                    BAIL_ON_REG_ERROR(dwError);
                    if (dwHexStringLen % 2)
                    {
                        hexDigitModCount = 1;
                    }
                    state = REGSHELL_HEX_END;
                }
                else
                {
                    state = REGSHELL_HEX_ERROR;
                }
                break;

            case REGSHELL_HEX_SPACE_SEPARATOR:
                if (*cursor == ' ')
                {
                    cursor++;
                }
                else if (isxdigit((int)*cursor))
                {
                    dwError = RegShellHexStringExtentLen(
                                  cursor,
                                  &dwHexStringLen);
                    BAIL_ON_REG_ERROR(dwError);
                    if (dwHexStringLen % 2)
                    {
                        hexDigitModCount = 1;
                    }
                    state = REGSHELL_HEX_END;
                }
                else
                {
                    state = REGSHELL_HEX_ERROR;
                }
                break;

            case REGSHELL_HEX_COMMA_SEPARATOR:
                if (*cursor == ' ')
                {
                    state = REGSHELL_HEX_SPACE_SEPARATOR;
                }
                else if (isxdigit((int)*cursor))
                {
                    dwError = RegShellHexStringExtentLen(
                                  cursor,
                                  &dwHexStringLen);
                    BAIL_ON_REG_ERROR(dwError);
                    if (dwHexStringLen % 2)
                    {
                        hexDigitModCount = 1;
                    }
                    state = REGSHELL_HEX_END;
                }
                else
                {
                    state = REGSHELL_HEX_ERROR;
                }
                break;

            case REGSHELL_HEX_END:
                if (hexDigitCount < 2)
                {
                    pszHexArray[1] = pszHexArray[0];
                    pszHexArray[0] = ' ';
                }
                binaryValue[binaryValueLen] =
                    (UCHAR) strtoul(pszHexArray, NULL, 16);
                binaryValueLen++;

                hexDigitCount = 0;
                pszHexArray[hexDigitCount] = '\0';

                if (*cursor == '\0' || *cursor == '\n')
                {
                    state = REGSHELL_HEX_STOP;
                }
                else
                {
                    state = REGSHELL_HEX_START;
                }
                break;

            case REGSHELL_HEX_ERROR:
                printf("RegShellImportBinaryString: ERROR: "
                       "invalid character '%c'\n", *cursor);
                state = REGSHELL_HEX_STOP;
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_HEX_STOP:
            default:
               break;
        }
    } while (state != REGSHELL_HEX_STOP);
    if (binaryValueLen > 0)
    {
        *ppBinaryValue = binaryValue;
        *pBinaryValueLen = binaryValueLen;
    }

cleanup:
    return dwError;

error:
    if (binaryValue)
    {
        LwRtlMemoryFree(binaryValue);
    }
    goto cleanup;
}


DWORD
RegShellCmdParseValueName(
    PREGSHELL_PARSE_STATE pParseState,
    REGSHELL_CMD_E cmd,
    DWORD argc,
    PCHAR *argv,
    PREGSHELL_CMD_ITEM *pRetCmdItem)
{
    DWORD dwError = 0;
    DWORD argCount = 0;
    PREGSHELL_CMD_ITEM pCmdItem = NULL;
    PCHAR pszValue = NULL;
    PCHAR pszType = NULL;
    DWORD i = 0;
    PUCHAR binaryValue = NULL;
    DWORD binaryValueLen = 0;
    DWORD dwValue = 0;
    PBYTE multiString = NULL;
    ssize_t multiStringLen = 0;
    PCHAR pszString = NULL;
    DWORD argIndx = 2;
    DWORD dwOffset = 0;

    BAIL_ON_INVALID_POINTER(argv);
    BAIL_ON_INVALID_POINTER(pRetCmdItem);

    if (argv[2][0] == '[')
    {
        dwError = RegShellCmdParseKeyName(
                      pParseState,
                      cmd,
                      argc,
                      argv,
                      &pCmdItem);
        argIndx = 3;
    }
    else
    {
        dwError = RegShellCmdParseCommand(
                      cmd,
                      argc,
                      argv,
                      &pCmdItem);
    }

    pszValue = argv[argIndx++];

    /* format: add_value [[subkeyname]] value_name */
    if (pCmdItem->command == REGSHELL_CMD_DELETE_VALUE)
    {
        dwError = LwRtlCStringDuplicate(&pCmdItem->valueName, pszValue);
        BAIL_ON_REG_ERROR(dwError);
        *pRetCmdItem = pCmdItem;
        return 0;
    }
    pszType = argv[argIndx++];
    argCount = argc - argIndx;
    if (argCount < 0)
    {
        dwError = LWREG_ERROR_INVALID_CONTEXT;
        goto error;
    }
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwRtlCStringDuplicate(&pCmdItem->valueName, pszValue);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegShellParseStringType(
                  pszType,
                  &pCmdItem->type,
                  &pCmdItem->backendType);
    BAIL_ON_REG_ERROR(dwError);

    switch (pCmdItem->type)
    {
        case REG_BINARY:
        case REG_DWORD:
            if (argCount != 1)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            break;

        case REG_SZ:
            if (argCount > 1)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            break;
        default:
            break;
    }

    if (argCount > 0)
    {
        dwError = LW_RTL_ALLOCATE((PVOID*)&pCmdItem->args, PCHAR, sizeof(*pCmdItem->args) * (argCount + 1));
        BAIL_ON_REG_ERROR(dwError);

        for (i=0; i<argCount; i++)
        {
            dwError = LwRtlCStringDuplicate(&pCmdItem->args[i], argv[i+argIndx]);
            BAIL_ON_REG_ERROR(dwError);
        }
        pCmdItem->args[i] = NULL;
    }
    pCmdItem->argsCount = argCount;

    if (pCmdItem->binaryValue)
    {
        LwRtlMemoryFree(pCmdItem->binaryValue);
        pCmdItem->binaryValue = NULL;
    }

    switch (pCmdItem->type)
    {
        case REG_DWORD:

            dwError = RegShellImportBinaryString(
                            pCmdItem->args[0],
                            &binaryValue,
                            &binaryValueLen);
            BAIL_ON_REG_ERROR(dwError);

            if (binaryValueLen > sizeof(DWORD))
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            if (binaryValueLen < sizeof(DWORD))
            {
                dwOffset = sizeof(DWORD) - binaryValueLen;
                memmove(binaryValue+dwOffset, binaryValue, binaryValueLen);
                memset(binaryValue, 0, dwOffset);
            }
            memcpy(&dwValue, binaryValue, sizeof(DWORD));
            dwValue = LW_BTOH32(dwValue);
            memcpy(binaryValue, &dwValue, sizeof(DWORD));

            dwError = LW_RTL_ALLOCATE((PVOID*)&pCmdItem->binaryValue, DWORD, sizeof(DWORD));
            BAIL_ON_REG_ERROR(dwError);

            pCmdItem->binaryValue = binaryValue;
            pCmdItem->binaryValueLen = sizeof(DWORD);
            break;

        case REG_BINARY:
            dwError = RegShellImportBinaryString(
                            pCmdItem->args[0],
                            &binaryValue,
                            &binaryValueLen);
            BAIL_ON_REG_ERROR(dwError);

            pCmdItem->binaryValue = binaryValue;
            pCmdItem->binaryValueLen = binaryValueLen;
            break;

        case REG_SZ:
            dwError = LwRtlCStringDuplicate(&pszString, pCmdItem->args[0]);
            pCmdItem->binaryValue = (PUCHAR) pszString;
            pCmdItem->binaryValueLen = strlen(pszString);
            BAIL_ON_REG_ERROR(dwError);
            break;

        case REG_MULTI_SZ:
            RegMultiStrsToByteArray(pCmdItem->args,
                                    &multiString,
                                    &multiStringLen);
            pCmdItem->binaryValue = (PUCHAR) multiString;
            pCmdItem->binaryValueLen = multiStringLen;
            break;

        default:
            break;
    }

    *pRetCmdItem = pCmdItem;
cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellDumpCmdItem(
    PREGSHELL_CMD_ITEM rsItem)
{
    CHAR tokenName[128];
    PCHAR pszDumpData = NULL;
    DWORD dumpDataLen = 0;
    DWORD i;
    DWORD dwError = 0;
    PCHAR *outMultiSz = NULL;


    BAIL_ON_INVALID_POINTER(rsItem);

    RegShellCmdEnumToString(rsItem->command, tokenName);
    printf("DumpCmd: command=%s\n", tokenName);
    if (rsItem->keyName)
    {
        printf("DumpCmd: keyName=%s\n", rsItem->keyName);
    }
    if (rsItem->valueName)
    {
        printf("DumpCmd: valueName=%s\n", rsItem->valueName);
    }
    if (rsItem->type != REG_UNKNOWN)
    {
        RegExportBinaryTypeToString(
            rsItem->type,
            tokenName,
            FALSE);
        printf("DumpCmd: type=%s\n", tokenName);
    }
    for (i=0; i<rsItem->argsCount; i++)
    {
        printf("DumpCmd: args[%d]='%s'\n", i, rsItem->args[i]);
    }

    switch (rsItem->type)
    {
        case REG_SZ:
            printf("DumpCmd: value = '%s'\n", (CHAR *) rsItem->binaryValue);
            break;

        case REG_DWORD:
            printf("DumpCmd: value = '%08x'\n", *(DWORD*) rsItem->binaryValue);
            break;

        case REG_BINARY:
            dwError = RegExportBinaryData(
                          REG_SZ,
                          "test_value",
                          REG_BINARY,
                          rsItem->binaryValue,
                          rsItem->binaryValueLen,
                          &pszDumpData,
                          &dumpDataLen);
            BAIL_ON_REG_ERROR(dwError);
            printf("RegShellDumpCmdItem: '%s'\n", pszDumpData);
            break;

        case REG_MULTI_SZ:
            RegByteArrayToMultiStrs(
                rsItem->binaryValue,
                rsItem->binaryValueLen,
                &outMultiSz);

            for (i=0; outMultiSz[i]; i++)
            {
                printf("DumpCmd: outMultiSz[%d] = '%s'\n",
                       i, outMultiSz[i]);
            }


        default:
            break;
    }

cleanup:
    return dwError;

error:
    goto cleanup;


}


DWORD
RegShellCmdParse(
    PREGSHELL_PARSE_STATE pParseState,
    DWORD argc,
    PCHAR *argv,
    PREGSHELL_CMD_ITEM *parsedCmd)
{
    REGSHELL_CMD_E cmd = 0;
    DWORD dwError = 0;
    PCHAR pszCommand = NULL;
    PREGSHELL_CMD_ITEM pCmdItem = NULL;

    BAIL_ON_INVALID_POINTER(argv);

    if (argc < 2)
    {
        dwError = LWREG_ERROR_INVALID_CONTEXT;
        goto error;
    }
    pszCommand = argv[1];

    dwError = RegShellCmdStringToEnum(pszCommand, &cmd);
    switch (cmd)
    {
        /*
         * Commands with no arguments
         */
        case REGSHELL_CMD_HELP:
        case REGSHELL_CMD_PWD:
            if (argc > 2)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            else
            {
                dwError = RegShellCmdParseCommand(cmd, argc, argv, &pCmdItem);
            }
            break;
        case REGSHELL_CMD_LIST_KEYS:
        case REGSHELL_CMD_LIST:
        case REGSHELL_CMD_DIRECTORY:
        case REGSHELL_CMD_LIST_VALUES:
        case REGSHELL_CMD_QUIT:
            if (argc > 2)
            {
                dwError = RegShellCmdParseKeyName(
                              pParseState,
                              cmd,
                              argc,
                              argv,
                              &pCmdItem);
            }
            else
            {
                dwError = RegShellCmdParseCommand(cmd, argc, argv, &pCmdItem);
            }
            break;

        case REGSHELL_CMD_IMPORT:
        case REGSHELL_CMD_EXPORT:
        case REGSHELL_CMD_UPGRADE:
        case REGSHELL_CMD_SET_HIVE:
            if (argc != 3)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            dwError = RegShellCmdParseCommand(cmd, argc, argv, &pCmdItem);
            BAIL_ON_REG_ERROR(dwError);

            dwError = LW_RTL_ALLOCATE((PVOID*)&pCmdItem->args, PSTR, sizeof(PSTR) * 2);
            BAIL_ON_REG_ERROR(dwError);

            dwError = LwRtlCStringDuplicate((LW_PVOID) &pCmdItem->args[0], argv[2]);
            BAIL_ON_REG_ERROR(dwError);
            pCmdItem->argsCount = 1;
            break;

        /*
         * Commands that take a KeyName argument
         *   command [HKLM_LIKEWISE/...]
         */
        case REGSHELL_CMD_ADD_KEY:
        case REGSHELL_CMD_CHDIR:
        case REGSHELL_CMD_DELETE_KEY:
        case REGSHELL_CMD_DELETE_TREE:
            if (argc != 3)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            dwError = RegShellCmdParseKeyName(
                          pParseState,
                          cmd,
                          argc,
                          argv,
                          &pCmdItem);
            break;

        case REGSHELL_CMD_DELETE_VALUE:
            if (argc > 4)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            dwError = RegShellCmdParseValueName(
                          pParseState,
                          cmd,
                          argc,
                          argv,
                          &pCmdItem);
            break;
       /*
        * Commands that take ValueName/REG_TYPE/Values_List
        *   add_value "ValueName" type "Value" ["Value2"] ["Value3"] [...]
        */
        case REGSHELL_CMD_ADD_VALUE:
        case REGSHELL_CMD_SET_VALUE:
            if (argc < 5)
            {
                dwError = LWREG_ERROR_INVALID_CONTEXT;
                goto error;
            }
            dwError = RegShellCmdParseValueName(
                          pParseState,
                          cmd,
                          argc,
                          argv,
                          &pCmdItem);
            break;

        default:
            dwError = LWREG_ERROR_INVALID_CONTEXT;
            break;
    }

    *parsedCmd = pCmdItem;
cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellAllocKey(
    PSTR pszKeyName,
    PSTR *pszNewKey)
{
    DWORD dwError;

    dwError = LW_RTL_ALLOCATE((PVOID*)pszNewKey,PSTR, strlen(pszKeyName) + 3);
    BAIL_ON_REG_ERROR(dwError);

    strcpy(*pszNewKey, "[");
    strcat(*pszNewKey, pszKeyName);
    strcat(*pszNewKey, "]");

cleanup:
    return dwError;

error:
    goto cleanup;
}


/*
 * Token sequence from reglex for above comands:
 **cd:          REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 * add_key:     REGLEX_PLAIN_TEXT REGLEX_REG_KEY
 **list_keys:   REGLEX_PLAIN_TEXT [REGLEX_REG_KEY]
 * list_values: REGLEX_PLAIN_TEXT [REGLEX_REG_KEY]
 * delete_key:  REGLEX_PLAIN_TEXT [REGLEX_REG_KEY]
 * delete_tree: REGLEX_PLAIN_TEXT [REGLEX_REG_KEY]
 *
 * set_value:   REGLEX_PLAIN_TEXT REGLEX_REG_KEY REGLEX_REG_SZ|REGLEX_PLAIN_TEXT
 *                  REGLEX_PLAIN_TEXT REGLEX_REG_SZ ...
 * add_value:   REGLEX_PLAIN_TEXT REGLEX_REG_KEY REGLEX_REG_SZ|REGLEX_PLAIN_TEXT
 *                  REGLEX_PLAIN_TEXT REGLEX_REG_SZ ...
 * help
 */

typedef enum _REGSHELL_CMDLINE_STATE_E
{
    REGSHELL_CMDLINE_STATE_FIRST = 0,
    REGSHELL_CMDLINE_STATE_VERB,
    REGSHELL_CMDLINE_STATE_LIST_KEYS,
    REGSHELL_CMDLINE_STATE_LIST_KEYS_REGKEY,
    REGSHELL_CMDLINE_STATE_LIST_KEYS_STOP,

    REGSHELL_CMDLINE_STATE_CD,
    REGSHELL_CMDLINE_STATE_CD_REGKEY,
    REGSHELL_CMDLINE_STATE_CD_STOP,

    REGSHELL_CMDLINE_STATE_ADDKEY,
    REGSHELL_CMDLINE_STATE_ADDKEY_REGKEY,
    REGSHELL_CMDLINE_STATE_ADDKEY_STOP,

    REGSHELL_CMDLINE_STATE_ADDVALUE,
    REGSHELL_CMDLINE_STATE_ADDVALUE_KEYNAME,
    REGSHELL_CMDLINE_STATE_ADDVALUE_VALUENAME,
    REGSHELL_CMDLINE_STATE_ADDVALUE_TYPE,
    REGSHELL_CMDLINE_STATE_ADDVALUE_VALUE,
    REGSHELL_CMDLINE_STATE_ADDVALUE_STOP,

    REGSHELL_CMDLINE_STATE_IMPORT,
    REGSHELL_CMDLINE_STATE_IMPORT_FILE,
    REGSHELL_CMDLINE_STATE_IMPORT_STOP,
} REGSHELL_CMDLINE_STATE_E, *PREGSHELL_CMDLINE_STATE_E;


DWORD
RegShellCmdlineParseToArgv(
    PREGSHELL_PARSE_STATE pParseState,
    PDWORD pdwNewArgc,
    PSTR **pszNewArgv)
{
    DWORD dwError = 0;
    BOOLEAN eof = FALSE;
    BOOLEAN stop = FALSE;
    REGLEX_TOKEN token = 0;
    REGSHELL_CMDLINE_STATE_E state = 0;
    PSTR pszAttr = NULL;
    DWORD attrSize = 0;
    REGSHELL_CMD_E cmdEnum = 0;
    DWORD dwArgc = 1;
    DWORD dwAllocSize = 1;
    PSTR *pszArgv = NULL;
    PSTR *pszArgvRealloc = NULL;
    REG_DATA_TYPE valueType = REG_UNKNOWN;
    PSTR pszBinaryData = NULL;
    DWORD dwBinaryDataOffset = 0;
    DWORD dwLen = 0;

    BAIL_ON_INVALID_HANDLE(pParseState->ioHandle);
    BAIL_ON_INVALID_HANDLE(pParseState->lexHandle);
    BAIL_ON_INVALID_POINTER(pdwNewArgc);
    BAIL_ON_INVALID_POINTER(pszNewArgv);

    dwError = RegLexGetToken(pParseState->ioHandle,
                             pParseState->lexHandle,
                             &token,
                             &eof);
    BAIL_ON_REG_ERROR(dwError);
    stop = eof;
    do
    {
        switch (state)
        {
            case REGSHELL_CMDLINE_STATE_FIRST:
                if (token == REGLEX_PLAIN_TEXT)
                {
                    state = REGSHELL_CMDLINE_STATE_VERB;
                }
                else
                {
                    /* Syntax error */
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                }
                break;

            case REGSHELL_CMDLINE_STATE_VERB:
                RegLexGetAttribute(pParseState->lexHandle, &attrSize, &pszAttr);
                if (attrSize > 0)
                {
                    dwError = RegShellCmdStringToEnum(pszAttr, &cmdEnum);
                    if (dwError)
                    {
                        /* Syntax error */
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                        break;
                    }
                }

                if (cmdEnum == REGSHELL_CMD_LIST_KEYS ||
                    cmdEnum == REGSHELL_CMD_LIST ||
                    cmdEnum == REGSHELL_CMD_DIRECTORY)
                {

                    d_printf(("RegShellCmdlineParseToArgv: list_keys found\n"));
                    state = REGSHELL_CMDLINE_STATE_LIST_KEYS;
                    dwArgc++;
                    dwAllocSize = dwArgc+1;
                }
                else if (cmdEnum == REGSHELL_CMD_LIST ||
                         cmdEnum == REGSHELL_CMD_DIRECTORY)
                {
                    d_printf(("RegShellCmdlineParseToArgv: list found\n"));
                    state = REGSHELL_CMDLINE_STATE_LIST_KEYS;
                    dwArgc++;
                    dwAllocSize = dwArgc+1;
                }
                else if (cmdEnum == REGSHELL_CMD_LIST_VALUES)
                {
                    d_printf(("RegShellCmdlineParseToArgv: list_values found\n"));
                    state = REGSHELL_CMDLINE_STATE_LIST_KEYS;
                    dwArgc++;
                    dwAllocSize = dwArgc+1;
                }
                else if (cmdEnum == REGSHELL_CMD_CHDIR)
                {
                    d_printf(("RegShellCmdlineParseToArgv: cd found\n"));
                    state = REGSHELL_CMDLINE_STATE_CD;
                    dwArgc += 1;
                    dwAllocSize = dwArgc + 1;
                }
                else if (cmdEnum == REGSHELL_CMD_DELETE_KEY)
                {
                    d_printf(("RegShellCmdlineParseToArgv: delete_key found\n"));
                    state = REGSHELL_CMDLINE_STATE_CD;
                    dwArgc += 1;
                    dwAllocSize = dwArgc + 1;
                }
                else if (cmdEnum == REGSHELL_CMD_DELETE_TREE)
                {
                    d_printf(("RegShellCmdlineParseToArgv: delete_tree found\n"));
                    state = REGSHELL_CMDLINE_STATE_CD;
                    dwArgc += 1;
                    dwAllocSize = dwArgc + 1;
                }
                else if (cmdEnum == REGSHELL_CMD_HELP)
                {
                    d_printf(("RegShellCmdlineParseToArgv: help found\n"));
                    state = REGSHELL_CMDLINE_STATE_LIST_KEYS_STOP;
                    dwArgc += 1;
                    dwAllocSize = dwArgc;
                }
                else if (cmdEnum == REGSHELL_CMD_PWD)
                {
                    d_printf(("RegShellCmdlineParseToArgv: pwd found\n"));
                    state = REGSHELL_CMDLINE_STATE_LIST_KEYS_STOP;
                    dwArgc += 1;
                    dwAllocSize = dwArgc;
                }
                else if (cmdEnum == REGSHELL_CMD_QUIT)
                {
                    d_printf(("RegShellCmdlineParseToArgv: exit found\n"));
                    state = REGSHELL_CMDLINE_STATE_LIST_KEYS_STOP;
                    dwArgc += 1;
                    dwAllocSize = dwArgc;
                }
                else if (cmdEnum == REGSHELL_CMD_ADD_KEY)
                {
                    d_printf(("RegShellCmdlineParseToArgv: add_key found\n"));
                    state = REGSHELL_CMDLINE_STATE_ADDKEY;
                    dwArgc += 2;
                    dwAllocSize = dwArgc;
                }
                else if (cmdEnum == REGSHELL_CMD_ADD_VALUE)
                {
                    d_printf(("RegShellCmdlineParseToArgv: add_value found\n"));
                    state = REGSHELL_CMDLINE_STATE_ADDVALUE;
                    dwArgc += 1;
                    /*
                     * Realloc pszArgv as needed for additional args,
                     * as there is no way to know how many arguments
                     * will follow for REG_MULTI_SZ.
                     */
                    dwAllocSize = dwArgc + 64;
                }
                else if (cmdEnum == REGSHELL_CMD_DELETE_VALUE)
                {
                    d_printf(("RegShellCmdlineParseToArgv: delete_value found\n"));
                    state = REGSHELL_CMDLINE_STATE_ADDVALUE;
                    dwArgc += 1;
                    dwAllocSize = dwArgc + 3;
                }
                else if (cmdEnum == REGSHELL_CMD_SET_VALUE)
                {
                    d_printf(("RegShellCmdlineParseToArgv: set_value found\n"));
                    dwArgc += 1;
                    /*
                     * Realloc pszArgv as needed for additional args,
                     * as there is no way to know how many arguments
                     * will follow for REG_MULTI_SZ.
                     */
                    state = REGSHELL_CMDLINE_STATE_ADDVALUE;
                    dwAllocSize = dwArgc + 64;
                }
                else if (cmdEnum == REGSHELL_CMD_SET_HIVE)
                {
                    dwAllocSize = 4;
                    dwArgc = 2;
                    state = REGSHELL_CMDLINE_STATE_IMPORT;
                }
                else if (cmdEnum == REGSHELL_CMD_IMPORT)
                {
                    dwAllocSize = 4;
                    dwArgc = 2;
                    state = REGSHELL_CMDLINE_STATE_IMPORT;
                }
                else if (cmdEnum == REGSHELL_CMD_EXPORT)
                {
                    dwAllocSize = 4;
                    dwArgc = 2;
                    state = REGSHELL_CMDLINE_STATE_IMPORT;
                }
                else if (cmdEnum == REGSHELL_CMD_UPGRADE)
                {
                    dwAllocSize = 4;
                    dwArgc = 2;
                    state = REGSHELL_CMDLINE_STATE_IMPORT;
                }
                else
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                    break;
                }

                dwError = LW_RTL_ALLOCATE((PVOID*)&pszArgv, PSTR, sizeof(*pszArgv) * dwAllocSize);
                BAIL_ON_REG_ERROR(dwError);

                dwError = LwRtlCStringDuplicate(&pszArgv[0], "regshell");
                BAIL_ON_REG_ERROR(dwError);
                dwError = LwRtlCStringDuplicate(&pszArgv[1], pszAttr);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMDLINE_STATE_LIST_KEYS:
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                BAIL_ON_REG_ERROR(dwError);
                /*
                 * This parameter is optional, but there must have been a
                 * default key specified with a previous 'cd' command.
                 */
                if (!eof)
                {
                    if (token == REGLEX_REG_KEY ||
                        token == REGLEX_PLAIN_TEXT)
                    {
                        dwArgc++;
                        state = REGSHELL_CMDLINE_STATE_LIST_KEYS_REGKEY;
                    }
                    else
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                }
                else
                {
                    state = REGSHELL_CMDLINE_STATE_LIST_KEYS_STOP;
                    stop = eof;
                }
                break;

            case REGSHELL_CMDLINE_STATE_LIST_KEYS_REGKEY:
                RegLexGetAttribute(pParseState->lexHandle,
                                   &attrSize,
                                   &pszAttr);
                RegShellAllocKey(pszAttr, &pszArgv[dwArgc-1]);
                BAIL_ON_REG_ERROR(dwError);
                state = REGSHELL_CMDLINE_STATE_LIST_KEYS_STOP;
                break;

            case REGSHELL_CMDLINE_STATE_LIST_KEYS_STOP:
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                BAIL_ON_REG_ERROR(dwError);
                stop = eof;
                if (!eof)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                }
                break;

            case REGSHELL_CMDLINE_STATE_CD:
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                BAIL_ON_REG_ERROR(dwError);
                if (eof)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                    stop = eof;
                }
                else
                {
                    if (token == REGLEX_REG_KEY ||
                        token == REGLEX_PLAIN_TEXT)
                    {
                        state = REGSHELL_CMDLINE_STATE_CD_REGKEY;
                    }
                    else
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                }
                break;


            case REGSHELL_CMDLINE_STATE_CD_REGKEY:
                dwError = RegShellAllocKey(pszAttr, &pszArgv[2]);
                BAIL_ON_REG_ERROR(dwError);
                dwArgc += 1;

                state = REGSHELL_CMDLINE_STATE_CD_STOP;
                break;

            case REGSHELL_CMDLINE_STATE_CD_STOP:
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                BAIL_ON_REG_ERROR(dwError);
                stop = eof;
                if (!eof)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                }
                break;

            case REGSHELL_CMDLINE_STATE_ADDKEY:
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                BAIL_ON_REG_ERROR(dwError);
                if (eof)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                    stop = eof;
                }
                else
                {
                    if (token == REGLEX_REG_KEY ||
                        token == REGLEX_PLAIN_TEXT)
                    {
                        state = REGSHELL_CMDLINE_STATE_ADDKEY_REGKEY;
                    }
                    else
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                }
                break;

            case REGSHELL_CMDLINE_STATE_ADDKEY_REGKEY:
                RegShellAllocKey(pszAttr, &pszArgv[2]);
                BAIL_ON_REG_ERROR(dwError);
                state = REGSHELL_CMDLINE_STATE_ADDKEY_STOP;
                break;

            case REGSHELL_CMDLINE_STATE_ADDKEY_STOP:
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                BAIL_ON_REG_ERROR(dwError);
                stop = eof;
                if (!eof)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                }
                break;

            case REGSHELL_CMDLINE_STATE_ADDVALUE:
                dwError = RegLexGetToken(pParseState->ioHandle,
                                         pParseState->lexHandle,
                                         &token,
                                         &eof);
                BAIL_ON_REG_ERROR(dwError);
                if (eof)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                }
                else if (token == REGLEX_REG_KEY)
                {
                    state = REGSHELL_CMDLINE_STATE_ADDVALUE_KEYNAME;
                }
                else if (token == REGLEX_REG_SZ ||
                         token == REGLEX_PLAIN_TEXT ||
                         token == REGLEX_KEY_NAME_DEFAULT)
                {
                    state = REGSHELL_CMDLINE_STATE_ADDVALUE_VALUENAME;
                }
                else
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                }
                break;

                case REGSHELL_CMDLINE_STATE_ADDVALUE_KEYNAME:
                    RegLexGetAttribute(pParseState->lexHandle,
                                       &attrSize,
                                       &pszAttr);

                    dwError = LW_RTL_ALLOCATE((PVOID*)&pszArgv[dwArgc], CHAR, sizeof(*pszArgv[dwArgc]) * (strlen(pszAttr)+3));
                    BAIL_ON_REG_ERROR(dwError);

                    strcpy(pszArgv[dwArgc], "[");
                    strcat(pszArgv[dwArgc], pszAttr);
                    strcat(pszArgv[dwArgc], "]");
                    dwArgc++;
                    dwError = RegLexGetToken(pParseState->ioHandle,
                                             pParseState->lexHandle,
                                             &token,
                                             &eof);
                    BAIL_ON_REG_ERROR(dwError);
                    if (eof)
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                    else if (token == REGLEX_REG_SZ ||
                             token == REGLEX_PLAIN_TEXT ||
                         token == REGLEX_KEY_NAME_DEFAULT)
                    {
                        state = REGSHELL_CMDLINE_STATE_ADDVALUE_VALUENAME;
                    }
                    else
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }

                    break;

                case REGSHELL_CMDLINE_STATE_ADDVALUE_VALUENAME:
                    RegLexGetAttribute(pParseState->lexHandle,
                                       &attrSize,
                                       &pszAttr);
                    dwError = LwRtlCStringDuplicate(&pszArgv[dwArgc++], pszAttr);
                    BAIL_ON_REG_ERROR(dwError);

                    dwError = RegLexGetToken(pParseState->ioHandle,
                                             pParseState->lexHandle,
                                             &token,
                                             &eof);
                    BAIL_ON_REG_ERROR(dwError);

                    /* add_value type is next */
                    if (cmdEnum == REGSHELL_CMD_DELETE_VALUE)
                    {
                        stop = TRUE;
                    }
                    else if (eof)
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                    else if (token == REGLEX_REG_SZ ||
                             token == REGLEX_PLAIN_TEXT ||
                             token == REGLEX_REG_MULTI_SZ ||
                             token == REGLEX_REG_BINARY ||
                             token == REGLEX_REG_DWORD)
                    {
                        state = REGSHELL_CMDLINE_STATE_ADDVALUE_TYPE;
                    }
                    else
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                    break;

                case REGSHELL_CMDLINE_STATE_ADDVALUE_TYPE:
                    RegLexGetAttribute(pParseState->lexHandle,
                                       &attrSize,
                                       &pszAttr);
                    dwError = RegShellParseStringType(
                                  pszAttr,
                                  &valueType,
                                  NULL);
                    if (valueType == REG_UNKNOWN)
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                        BAIL_ON_REG_ERROR(dwError);
                    }
                    dwError = LwRtlCStringDuplicate(&pszArgv[dwArgc++], pszAttr);
                    BAIL_ON_REG_ERROR(dwError);

                    state = REGSHELL_CMDLINE_STATE_ADDVALUE_VALUE;
                    break;

                case REGSHELL_CMDLINE_STATE_ADDVALUE_VALUE:
                    /* Get current buffer before reading the next token
                     * to determine if EOF has been found. Do this before
                     * the next call to RegLexGetToken(), as it consumes
                     * the first hex pair if found.
                     */
                    if (valueType == REG_BINARY || REG_DWORD)
                    {
                        dwError = RegIOBufferGetData(
                                      pParseState->ioHandle,
                                      &pszBinaryData,
                                      NULL,
                                      &dwBinaryDataOffset);
                        BAIL_ON_REG_ERROR(dwError);
                    }
                    dwError = RegLexGetToken(pParseState->ioHandle,
                                             pParseState->lexHandle,
                                             &token,
                                             &eof);
                    BAIL_ON_REG_ERROR(dwError);
                    if (eof)
                    {
                        if (dwArgc <= 4)
                        {
                            dwError = LWREG_ERROR_INVALID_CONTEXT;
                        }
                        else
                        {
                            state = REGSHELL_CMDLINE_STATE_ADDVALUE_STOP;
                        }
                    }
                    else if (token == REGLEX_REG_SZ ||
                             token == REGLEX_PLAIN_TEXT)
                    {
                        RegLexGetAttribute(pParseState->lexHandle,
                                           &attrSize,
                                           &pszAttr);
                        dwError = LwRtlCStringDuplicate(&pszArgv[dwArgc++], pszAttr);
                        BAIL_ON_REG_ERROR(dwError);
                        if (dwArgc >= dwAllocSize)
                        {
                            dwAllocSize *= 2;
                            dwError = RegReallocMemory(
                                          pszArgv,
                                          (LW_PVOID) &pszArgvRealloc,
                                          dwAllocSize * sizeof(PSTR *));
                            BAIL_ON_REG_ERROR(dwError);
                            pszArgv = pszArgvRealloc;
                        }
                    }
                    else if (valueType == REG_BINARY || valueType == REG_DWORD)
                    {
                        dwError = LwRtlCStringDuplicate(
                                      &pszArgv[dwArgc],
                                      &pszBinaryData[dwBinaryDataOffset]);
                        BAIL_ON_REG_ERROR(dwError);
                        dwLen = strlen(pszArgv[dwArgc]);
                        if (dwLen>1 && pszArgv[dwArgc][dwLen-1] == '\n')
                        {
                            pszArgv[dwArgc][dwLen-1] = '\0';
                        }
                        dwArgc++;
                        /*
                         * Force a stop now, as the parser will return
                         * subsequent hex pairs, which is not what we want.
                         * Call RegLexResetToken() to flush the remainder
                         * of the command line from the parser.
                         */
                        stop = TRUE;
                        RegLexResetToken(pParseState->lexHandle);
                    }
                    else
                    {
                            dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                    break;

                case REGSHELL_CMDLINE_STATE_ADDVALUE_STOP:
                    dwError = RegLexGetToken(pParseState->ioHandle,
                                             pParseState->lexHandle,
                                             &token,
                                             &eof);
                    BAIL_ON_REG_ERROR(dwError);
                    if (!eof)
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                    else
                    {
                        stop = TRUE;
                    }
                    break;

                case REGSHELL_CMDLINE_STATE_IMPORT:
                    dwError = RegLexGetToken(pParseState->ioHandle,
                                             pParseState->lexHandle,
                                             &token,
                                             &eof);
                    BAIL_ON_REG_ERROR(dwError);
                    if (eof)
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                    else
                    {
                        state = REGSHELL_CMDLINE_STATE_IMPORT_FILE;
                    }
                    break;

                case REGSHELL_CMDLINE_STATE_IMPORT_FILE:
                    RegLexGetAttribute(pParseState->lexHandle,
                                       &attrSize,
                                       &pszAttr);
                    dwError = LwRtlCStringDuplicate((LW_PVOID) &pszArgv[dwArgc++],
				                        pszAttr);
                    BAIL_ON_REG_ERROR(dwError);

                    state = REGSHELL_CMDLINE_STATE_IMPORT_STOP;
                    break;

                case REGSHELL_CMDLINE_STATE_IMPORT_STOP:
                    dwError = RegLexGetToken(pParseState->ioHandle,
                                             pParseState->lexHandle,
                                             &token,
                                             &eof);
                    BAIL_ON_REG_ERROR(dwError);
                    if (!eof)
                    {
                        dwError = LWREG_ERROR_INVALID_CONTEXT;
                    }
                    else
                    {
                        stop = eof;
                    }
                    break;

            default:
                stop = TRUE;
                break;
        }
    }
    while (!stop && dwError == LWREG_ERROR_SUCCESS);

    *pdwNewArgc = dwArgc;
    *pszNewArgv = pszArgv;
#ifdef _LW_DEBUG
    if (pszArgv)
    {
        int i;
        for (i=0; i<dwArgc; i++)
        {
            printf("RegShellCmdlineParseToArgv: argv[%d] = '%s'\n",
                   i, pszArgv[i]);
        }
    }
#endif

cleanup:
    if (dwError)
    {
        RegLexResetToken(pParseState->lexHandle);
    }
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellCmdlineParseFree(
    DWORD dwArgc,
    PSTR *pszArgv)
{
    DWORD dwError = 0;
    DWORD i = 0;

    BAIL_ON_INVALID_POINTER(pszArgv);

    for (i=0; i<dwArgc; i++)
    {
        LwRtlMemoryFree(pszArgv[i]);
    }
    LwRtlMemoryFree(pszArgv);

cleanup:
    return dwError;

error:
    goto cleanup;
}


LW_VOID
RegShellUsage(
    PSTR progname)
{
    printf("usage: %s [--file | -f] command_file.txt\n"
        "       add_key [[KeyName]]\n"
        "       list_keys [[keyName]]\n"
        "       delete_key [KeyName]\n"
        "       delete_tree [KeyName]\n"
        "       cd [KeyName]\n"
        "       pwd\n"
        "       add_value [[KeyName]] \"ValueName\" Type \"Value\" [\"Value2\"] [...]\n"
        "       set_value [[KeyName]] \"ValueName\" Type \"Value\" [\"Value2\"] [...]\n"
        "       list_values [[keyName]]\n"
        "       delete_value [[KeyName]] \"ValueName\"\n"
        "       set_hive HIVE_NAME\n"
        "       import file.reg\n"
        "       export file.reg\n"
        "       upgrade file.reg\n"
        "       exit | quit | ^D\n"
        "\n"
        "         Type: REG_SZ | REG_DWORD | REG_BINARY | REG_MULTI_SZ\n"
        "               REG_DWORD and REG_BINARY values are hexadecimal\n"
        "         Note: cd and pwd only function in interactive mode\n"
        "         Note: HKEY_THIS_MACHINE is the only supported hive\n"
        ,progname);
}
