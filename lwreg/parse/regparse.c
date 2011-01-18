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
 *        regparse.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */

/*
 * Parser implementation:
 *
 *                REG_KEY
 *                   |
 *          +--------+---------+
 *          |                  |
 *        Empty              KeyValue
 *
 *
 *               KeyValue
 *                  +
 *                  |
 *       +----------+--------+-------+
 *       |          |        |       |
 *  NameDefault  ValueName  "="  TypeValue
 *     ("@")      (REG_SZ)
 *
 *
 *                          TypeValue
 *                              |
 *      +----------+------------+------------+-------------+-----...
 *      |          |            |            |             |
 *  ValueNone  ValueDword  ValueBinary  ValueString ValueMultiString
 *  (REG_NONE) (REG_DWORD) (REG_BINARY) (REG_SZ)    (REG_MULTI_SZ)
 *    [hex(0)]                [hex]                   [hex(7)]
 *
 * ...---+-----------------+-----------------------+----------...
 *       |                 |                       |
 *   ValueExpandSz    ValueQuaword   ValueResourceRequirementsList
 *   (REG_EXPAND_SZ)  (REG_QUADWORD) (REG_RESOURCE_REQUIREMENTS_LIST)
 *      [hex(2)]         [hex(b)]         [hex(a)]
 *
 *
 * ...-------+-----------------------+-------------------------+
 *           |                       |                         |
 *   ValueResourceList    FullResourceDescriptor       ValueStringArray
 *   (REG_RESOURCE_LIST)  (REG_FULL_RESOURCE_LIST)    (REG_MULTI_SZ)
 *         [hex(8)]             [hex(9)]                    [sza]
 *
 * Note: All "hex()" types are TypeBinary, a sequence of binary
 *       hex values.
 *
 *
 *            TypeBinary
 *               |
 *        +-->(HexPair)---->(HexPairEnd)
 *        |      |
 *        |      |
 *        +------+
 */

#include "includes.h"

void RegParseExternDataType(
    REGLEX_TOKEN valueType,
    PREG_DATA_TYPE externValueType)
{
    if (!externValueType)
    {
        return;
    }
    switch (valueType)
    {
        case REGLEX_REG_DWORD:
            *externValueType = REG_DWORD;
            break;
        case REGLEX_REG_SZ:
            *externValueType = REG_SZ;
            break;
        case REGLEX_REG_BINARY:
            *externValueType = REG_BINARY;
            break;
        case REGLEX_REG_NONE:
            *externValueType = REG_NONE;
            break;
        case REGLEX_REG_EXPAND_SZ:
            *externValueType = REG_EXPAND_SZ;
            break;
        case REGLEX_REG_MULTI_SZ:
            *externValueType = REG_MULTI_SZ;
            break;
        case REGLEX_REG_RESOURCE_LIST:
            *externValueType = REG_RESOURCE_LIST;
            break;
        case REGLEX_REG_FULL_RESOURCE_DESCRIPTOR:
            *externValueType = REG_FULL_RESOURCE_DESCRIPTOR;
            break;
        case REGLEX_REG_RESOURCE_REQUIREMENTS_LIST:
            *externValueType = REG_RESOURCE_REQUIREMENTS_LIST;
            break;
        case REGLEX_REG_QUADWORD:
            *externValueType = REG_QWORD;
            break;
        case REGLEX_REG_KEY:
            *externValueType = REG_KEY;
            break;
        case REGLEX_KEY_NAME_DEFAULT:
            *externValueType = REG_KEY_DEFAULT;
            break;
        case REGLEX_PLAIN_TEXT:
        default:
            *externValueType = REG_PLAIN_TEXT;
            break;
    }
}


DWORD
RegParseTypeNone(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD dwError = 0;

    parseHandle->dataType = REGLEX_REG_NONE;
    RegParseBinaryData(parseHandle);

    return dwError;
}


DWORD
RegParseReAllocateData(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD dwError = 0;
    DWORD newValueSize = 0;
    PVOID pNewMemory = NULL;

    BAIL_ON_INVALID_POINTER(parseHandle);

    if (parseHandle->binaryDataLen >= parseHandle->binaryDataAllocLen)
    {
        newValueSize = parseHandle->binaryDataAllocLen * 2;
        dwError = RegReallocMemory(
                      parseHandle->binaryData,
                      &pNewMemory,
                      newValueSize);
        BAIL_ON_REG_ERROR(dwError);

        parseHandle->binaryData = pNewMemory;
        parseHandle->binaryDataAllocLen = newValueSize;
    }

cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(pNewMemory);
    goto cleanup;
}


DWORD
RegParseAppendData(
    PREGPARSE_HANDLE parseHandle,
    PSTR pszHexValue)
{
    DWORD dwError = 0;
    DWORD attrSize = 0;
    PSTR pszAttr = 0;
    DWORD binaryValue = 0;

    BAIL_ON_INVALID_POINTER(parseHandle);

    RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
    dwError = RegParseReAllocateData(parseHandle);
    BAIL_ON_REG_ERROR(dwError);

    switch(parseHandle->dataType)
    {
        case REGLEX_REG_DWORD:
            binaryValue = strtoul(pszHexValue,
                                  NULL,
                                  16);
            memcpy(&parseHandle->binaryData[parseHandle->binaryDataLen],
                   &binaryValue,
                   sizeof(binaryValue));
            parseHandle->binaryDataLen += sizeof(binaryValue);
            break;

        case REGLEX_REG_MULTI_SZ:
        case REGLEX_REG_BINARY:
        case REGLEX_REG_EXPAND_SZ:
        case REGLEX_REG_RESOURCE_REQUIREMENTS_LIST:
        case REGLEX_REG_RESOURCE_LIST:
        case REGLEX_REG_FULL_RESOURCE_DESCRIPTOR:
        case REGLEX_REG_QUADWORD:
        case REGLEX_REG_NONE:
            /* Need to enforce only 2 bytes are being converted */
            binaryValue = strtoul(pszHexValue,
                                  NULL,
                                  16);
            parseHandle->binaryData[parseHandle->binaryDataLen] = binaryValue;
            parseHandle->binaryDataLen++;
            break;

        default:
            break;
    }
cleanup:
    return dwError;

error:
    goto cleanup;
}


void RegParsePrintBinaryData(
    PUCHAR binaryData,
    DWORD binaryDataLen)
{
    int i;
    for (i=0; i<binaryDataLen; i++)
    {
        printf("%02X ", binaryData[i]);
    }
    printf("\n");
}


DWORD
RegParseBinaryData(
    PREGPARSE_HANDLE parseHandle)
{
    PSTR dataName = NULL;
    BOOLEAN eof = FALSE;
    REGLEX_TOKEN token = 0;
    DWORD attrSize = 0;
    PSTR pszAttr = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    CHAR tokenName[256];

    BAIL_ON_INVALID_POINTER(parseHandle);

    parseHandle->binaryDataLen = 0;
    RegParseExternDataType(parseHandle->dataType,
                           &parseHandle->registryEntry.type);

    switch (parseHandle->dataType)
    {
        case REGLEX_REG_MULTI_SZ:
            dataName = "MultiStringValue";
            break;

        case REGLEX_REG_BINARY:
            dataName = "Binary";
            break;

        case REGLEX_REG_EXPAND_SZ:
            dataName = "ExpandString";
            break;

        case REGLEX_REG_RESOURCE_REQUIREMENTS_LIST:
            dataName = "ResourceRequirementsList";
            break;

        case REGLEX_REG_RESOURCE_LIST:
            dataName = "ResourceList";
            break;

        case REGLEX_REG_FULL_RESOURCE_DESCRIPTOR:
            dataName = "FullResourceDescriptor";
            break;

        case REGLEX_REG_QUADWORD:
            dataName = "Quadword";
            break;

        case REGLEX_REG_NONE:
            dataName = "REG_NONE";
            break;

        default:
            break;
    }

    do {
        dwError = RegLexGetToken(parseHandle->ioHandle,
                                 parseHandle->lexHandle,
                                 &token,
                                 &eof);
        if (eof)
        {
            return dwError;
        }

        RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
        RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);
        RegLexTokenToString(token, tokenName);
        if (token == REGLEX_HEXPAIR)
        {
            RegParseAppendData(parseHandle,
                               parseHandle->lexHandle->curToken.pszValue);
        }
        else if (token == REGLEX_HEXPAIR_END)
        {
            if (parseHandle->lexHandle->curToken.valueCursor == 0)
            {
                return dwError;
            }

            RegParseAppendData(parseHandle,
                               parseHandle->lexHandle->curToken.pszValue);
            parseHandle->registryEntry.valueLen = parseHandle->binaryDataLen;
            parseHandle->registryEntry.value = parseHandle->binaryData;

            return dwError;
        }
        else
        {
            printf("RegParseType%s: ERROR (syntax error) type '%s' "
                   "unknown line=%d\n\n",
                   dataName, tokenName, lineNum);
            return dwError;
        }
    } while (!eof);


cleanup:
    return dwError;

error:

    goto cleanup;
}


DWORD
RegParseTypeBinary(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD dwError = 0;

    RegParseBinaryData(parseHandle);
    return dwError;
}


DWORD
RegParseTypeExpandString(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD dwError = 0;

    RegParseBinaryData(parseHandle);
    return dwError;
}


DWORD
RegParseTypeQuadWord(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD dwError = 0;

    RegParseBinaryData(parseHandle);
    return dwError;
}


DWORD
RegParseTypeFullResDescriptor(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD dwError = 0;

    RegParseBinaryData(parseHandle);
    return dwError;
}


DWORD
RegParseTypeResourceReqList(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD dwError = 0;

    RegParseBinaryData(parseHandle);
    return dwError;
}


DWORD
RegParseTypeResourceList(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD dwError = 0;

    RegParseBinaryData(parseHandle);
    return dwError;
}



DWORD
RegParseTypeDword(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD attrSize = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    PSTR pszAttr = 0;
    CHAR tokenName[256];

    if (parseHandle->dataType == REGLEX_REG_DWORD)
    {
        RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
        RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);

        parseHandle->binaryDataLen = 0;
        RegParseAppendData(parseHandle, pszAttr);
    }
    else
    {
        printf("RegParseTypeDword: ERROR (syntax error) type '%s' "
               "unknown line=%d\n\n",
               tokenName, lineNum);
    }
    return dwError;
}


DWORD
RegParseTypeMultiStringValue(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD attrSize = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    PSTR pszAttr = 0;

    RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
    RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);

    RegParseBinaryData(parseHandle);
    return dwError;
}


DWORD
RegParseTypeStringArrayValue(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD attrSize = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    DWORD dwStrLen = 0;
    PSTR pszAttr = 0;
    REGLEX_TOKEN token = 0;
    BOOLEAN eof = FALSE;
    NTSTATUS ntStatus = 0;
    PWSTR pwszString = NULL;

    RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
    RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);

    dwError = RegLexGetToken(parseHandle->ioHandle,
                             parseHandle->lexHandle,
                             &token,
                             &eof);
    if (eof)
    {
        return eof;
    }
    while (token == REGLEX_REG_SZ ||
           (token == REGLEX_PLAIN_TEXT && strcmp(pszAttr, "\\")==0))
    {
        RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
        RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);
        if (token == REGLEX_REG_SZ)
        {
            ntStatus = LwRtlWC16StringAllocateFromCString(&pwszString, pszAttr);
            if (ntStatus)
            {
                goto error;
            }

            dwStrLen = wc16slen(pwszString) * 2 + 2;
            while (parseHandle->binaryDataAllocLen < dwStrLen)
            {
                dwError = RegParseReAllocateData(parseHandle);
                BAIL_ON_REG_ERROR(dwError);
            }
            memcpy(&parseHandle->binaryData[parseHandle->binaryDataLen],
                   pwszString,
                   dwStrLen);
            parseHandle->binaryDataLen += dwStrLen;
        }

        dwError = RegLexGetToken(parseHandle->ioHandle,
                                 parseHandle->lexHandle,
                                 &token,
                                 &eof);
        RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
    }
    parseHandle->binaryData[parseHandle->binaryDataLen++] = '\0';
    parseHandle->binaryData[parseHandle->binaryDataLen++] = '\0';
    RegLexUnGetToken(parseHandle->lexHandle);

    parseHandle->dataType = REGLEX_REG_MULTI_SZ;
    parseHandle->lexHandle->isToken = TRUE;
    RegParseExternDataType(parseHandle->dataType,
                           &parseHandle->registryEntry.type);


cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszString);
    return dwError;

error:
    goto cleanup;
}


DWORD
RegParseInstallCallback(
    PREGPARSE_HANDLE parseHandle,
    PFN_REG_CALLBACK parseCallback,
    HANDLE userContext,
    PDWORD indexCallback)
{
    DWORD dwError = 0;
    DWORD i = 0;

    BAIL_ON_INVALID_POINTER(parseHandle);
    BAIL_ON_INVALID_POINTER(parseCallback);

    for (i=0;
         i < sizeof(parseHandle->parseCallback.callbacks) /
             sizeof(REGPARSE_CALLBACK_ENTRY);
         i++)
    {
        if (!parseHandle->parseCallback.callbacks[i].used)
        {
            parseHandle->parseCallback.callbacks[i].pfnCallback = parseCallback;
            parseHandle->parseCallback.callbacks[i].userContext = userContext;
            parseHandle->parseCallback.callbacks[i].used = TRUE;
            parseHandle->parseCallback.entries++;
            if (indexCallback)
            {
                *indexCallback = i;
            }
            break;
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegParseRemoveCallback(
    PREGPARSE_HANDLE parseHandle,
    DWORD indexCallback)
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(parseHandle);

    if (indexCallback >= 0 &&
        (indexCallback < sizeof(parseHandle->parseCallback.callbacks) /
             sizeof(REGPARSE_CALLBACK_ENTRY)) &&
         parseHandle->parseCallback.callbacks[indexCallback].used)
    {
        parseHandle->parseCallback.callbacks[indexCallback].used = FALSE;
        parseHandle->parseCallback.callbacks[indexCallback].pfnCallback = NULL;
        parseHandle->parseCallback.callbacks[indexCallback].userContext = NULL;
        parseHandle->parseCallback.entries--;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegParseRunCallbacks(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD dwError = 0;
    DWORD i = 0;
    BAIL_ON_INVALID_POINTER(parseHandle);
    DWORD calledCallback = 0;

    if (parseHandle->parseCallback.entries == 0)
    {
        return dwError;
    }

    for (i=0; calledCallback < parseHandle->parseCallback.entries; i++)
    {
        if (parseHandle->parseCallback.callbacks[i].used)
        {
            parseHandle->parseCallback.callbacks[i].pfnCallback(
                &parseHandle->registryEntry,
                parseHandle->parseCallback.callbacks[i].userContext);
            calledCallback++;
        }
    }


cleanup:
    return dwError;

error:

    goto cleanup;
}


DWORD
RegParseTypeStringValue(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD attrSize = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    PSTR pszAttr = 0;

    RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
    RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);

    if (parseHandle->parseCallback.entries > 0)
    {
        parseHandle->registryEntry.value = pszAttr;
        parseHandle->registryEntry.valueLen = attrSize;
    }
    return dwError;
}

void
RegParsePrintASCII(
    UCHAR *buf,
    DWORD buflen)
{
    int i;
    char c;
    printf("PrintASCII: '");
    for (i=0; i<buflen; i++)
    {
        c = (char) buf[i];
        if (isprint((int)c))
        {
            putchar(c);
        }
    }
    printf("'\n");
}


DWORD
RegParseTypeValue(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD attrSize = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    BOOLEAN eof = FALSE;
    PSTR pszAttr = 0;
    CHAR tokenName[256];
    REGLEX_TOKEN token = 0;

    dwError = RegLexGetToken(parseHandle->ioHandle,
                             parseHandle->lexHandle,
                             &token,
                             &eof);
    if (eof)
    {
        return dwError;
    }
    RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
    RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);
    parseHandle->registryEntry.lineNumber = lineNum;

    parseHandle->dataType = REGLEX_FIRST;
    parseHandle->binaryDataLen = 0;
    switch(token)
    {
        case REGLEX_KEY_NAME_DEFAULT:
            parseHandle->valueType = REGLEX_KEY_NAME_DEFAULT;
            parseHandle->dataType = REGLEX_REG_SZ;
            RegParseTypeStringValue(parseHandle);
            break;

        case REGLEX_REG_SZ:
            parseHandle->dataType = REGLEX_REG_SZ;
            RegParseTypeStringValue(parseHandle);
            break;

        case REGLEX_REG_MULTI_SZ:
            parseHandle->dataType = REGLEX_REG_MULTI_SZ;
            RegParseTypeMultiStringValue(parseHandle);
            break;

        case REGLEX_REG_STRING_ARRAY:
            parseHandle->dataType = REGLEX_REG_STRING_ARRAY;
            RegParseTypeStringArrayValue(parseHandle);
            break;

        case REGLEX_REG_DWORD:
            parseHandle->dataType = REGLEX_REG_DWORD;
            RegParseTypeDword(parseHandle);
            break;

        case REGLEX_REG_BINARY:
            parseHandle->dataType = REGLEX_REG_BINARY;
            RegParseTypeBinary(parseHandle);
            break;

        case REGLEX_REG_EXPAND_SZ:
            parseHandle->dataType = REGLEX_REG_EXPAND_SZ;
            RegParseTypeExpandString(parseHandle);
            break;

        case REGLEX_REG_QUADWORD:
            parseHandle->dataType = REGLEX_REG_QUADWORD;
            RegParseTypeQuadWord(parseHandle);
            break;

        case REGLEX_REG_RESOURCE_REQUIREMENTS_LIST:
            parseHandle->dataType = REGLEX_REG_RESOURCE_REQUIREMENTS_LIST;
            RegParseTypeResourceReqList(parseHandle);
            break;

        case REGLEX_REG_RESOURCE_LIST:
            parseHandle->dataType = REGLEX_REG_RESOURCE_LIST;
            RegParseTypeResourceList(parseHandle);
            break;

        case REGLEX_REG_FULL_RESOURCE_DESCRIPTOR:
            parseHandle->dataType = REGLEX_REG_FULL_RESOURCE_DESCRIPTOR;
            RegParseTypeFullResDescriptor(parseHandle);
            break;

        case REGLEX_REG_NONE:
            parseHandle->dataType = REGLEX_REG_NONE;
            RegParseTypeNone(parseHandle);
            break;

        default:
            printf("RegParseTypeValue: ERROR (syntax error) type '%s' "
                   "unknown line=%d\n\n",
                   tokenName, lineNum);
            break;
    }
    RegLexResetToken(parseHandle->lexHandle);
    return 0;
}


DWORD
RegParseKeyValue(
    PREGPARSE_HANDLE parseHandle)
{
    DWORD attrSize = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    BOOLEAN eof = FALSE;
    PSTR pszAttr = 0;
    REGLEX_TOKEN token = 0;

    dwError = RegLexGetToken(
                  parseHandle->ioHandle,
                  parseHandle->lexHandle,
                  &token,
                  &eof);
    if (eof)
    {
        return dwError;
    }
    RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
    RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);
    parseHandle->registryEntry.lineNumber = lineNum;

    if (token == REGLEX_EQUALS)
    {
        if (pszAttr)
        {
            if (parseHandle->registryEntry.valueName)
            {
                RegMemoryFree(parseHandle->registryEntry.valueName);
                parseHandle->registryEntry.valueName = NULL;
            }
            dwError = RegCStringDuplicate(&parseHandle->registryEntry.valueName, pszAttr);
            BAIL_ON_INVALID_POINTER(parseHandle->registryEntry.valueName);
        }
    }
    else
    {
        printf("RegParseKeyValue: ERROR (syntax error) line=%d\n\n", lineNum);
        return dwError;
    }

    dwError = RegParseTypeValue(parseHandle);
    if (dwError == 0)
    {
        RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);
        RegParseExternDataType(parseHandle->dataType,
                               &parseHandle->registryEntry.type);
        RegParseExternDataType(parseHandle->valueType,
                               &parseHandle->registryEntry.valueType);
        if (parseHandle->dataType != REGLEX_REG_SZ)
        {
            parseHandle->registryEntry.valueLen = parseHandle->binaryDataLen;
            parseHandle->registryEntry.value = parseHandle->binaryData;
        }
    }
    RegParseRunCallbacks(parseHandle);

cleanup:
    return dwError;

error:

    goto cleanup;
}

DWORD
RegParseKey(
    PREGPARSE_HANDLE parseHandle,
    REGLEX_TOKEN token)
{
    DWORD attrSize = 0;
    DWORD lineNum = 0;
    DWORD dwError = 0;
    BOOLEAN eof = FALSE;
    PSTR pszAttr = 0;

    do {
        RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
        RegLexGetLineNumber(parseHandle->lexHandle, &lineNum);

        /*
         * Test for empty registry subkey case first
         */
        if (token == REGLEX_REG_KEY)
        {
            /* Empty subkey, with no value, free any previous value */
            if (parseHandle->registryEntry.valueName)
            {
                RegMemoryFree(parseHandle->registryEntry.valueName);
                parseHandle->registryEntry.valueName = NULL;
            }
            parseHandle->dataType = REGLEX_REG_KEY;
            parseHandle->valueType = REGLEX_REG_NONE;
            if (pszAttr)
            {
                if (parseHandle->registryEntry.keyName)
                {
                    RegMemoryFree(parseHandle->registryEntry.keyName);
                }
                dwError = RegCStringDuplicate(
				&parseHandle->registryEntry.keyName, pszAttr);
                BAIL_ON_INVALID_POINTER(parseHandle->registryEntry.keyName);
            }
            if (parseHandle->parseCallback.entries > 0)
            {
                parseHandle->registryEntry.lineNumber = lineNum;
                RegParseExternDataType(parseHandle->dataType,
                                       &parseHandle->registryEntry.type);
                RegParseExternDataType(parseHandle->valueType,
                                       &parseHandle->registryEntry.valueType);
                parseHandle->registryEntry.valueLen = 0;
                parseHandle->registryEntry.value = NULL;
                RegParseRunCallbacks(parseHandle);
            }
            return dwError;
        }
        else if (token == REGLEX_REG_SZ || token == REGLEX_KEY_NAME_DEFAULT)
        {
            parseHandle->valueType = token;
            RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
            RegParseKeyValue(parseHandle);
        }
        else if (token == REGLEX_PLAIN_TEXT)
        {
            parseHandle->dataType = REGLEX_PLAIN_TEXT;
            parseHandle->valueType = REGLEX_REG_SZ;
            if (parseHandle->parseCallback.entries > 0)
            {
                parseHandle->registryEntry.lineNumber = lineNum;
                RegParseExternDataType(parseHandle->dataType,
                                       &parseHandle->registryEntry.type);
                RegParseExternDataType(parseHandle->valueType,
                                       &parseHandle->registryEntry.valueType);
                parseHandle->registryEntry.value = pszAttr;
                parseHandle->registryEntry.valueLen = attrSize;
                RegParseRunCallbacks(parseHandle);
            }
        }
        else
        {
            /*
             * Maybe treat as a syntax error here. May be in infinite loop
             * for poorly formed entries.
             */
            parseHandle->valueType = token;
            RegLexGetAttribute(parseHandle->lexHandle, &attrSize, &pszAttr);
            RegParseKeyValue(parseHandle);
printf("Unhandled token '%s'!\n", pszAttr);
            return dwError;
        }
        dwError = RegLexGetToken(parseHandle->ioHandle,
                                 parseHandle->lexHandle,
                                 &token,
                                 &eof);
        if (eof)
        {
            return dwError;
        }
    } while (!eof);

cleanup:
    return dwError;

error:

    goto cleanup;
}


DWORD
RegParseOpen(
    PSTR pszRegFileName,
    PFN_REG_CALLBACK parseCallback,
    HANDLE userContext,
    HANDLE *ppNewHandle)
{
    HANDLE ioHandle = NULL;
    PREGLEX_ITEM lexHandle = NULL;
    PREGPARSE_HANDLE newHandle = NULL;
    DWORD dwError = 0;
    void *binaryData = NULL;

    dwError = RegIOOpen(pszRegFileName, &ioHandle);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegLexOpen(&lexHandle);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegAllocateMemory(sizeof(REGPARSE_HANDLE),
                                (LW_PVOID) &newHandle);
    BAIL_ON_INVALID_POINTER(newHandle);

    dwError = RegAllocateMemory(REGPARSE_BUFSIZ,
                                &binaryData);
    BAIL_ON_INVALID_POINTER(newHandle);

    newHandle->ioHandle = ioHandle;
    newHandle->lexHandle = lexHandle;
    if (parseCallback)
    {
        RegParseInstallCallback(newHandle,
                                parseCallback,
                                userContext,
                                NULL);
    }
    newHandle->binaryData = binaryData;
    newHandle->binaryDataAllocLen = REGPARSE_BUFSIZ;
    newHandle->binaryDataLen = 0;

    *ppNewHandle = (HANDLE) newHandle;

cleanup:
    return dwError;

error:

    goto cleanup;
}


void
RegParseClose(
    HANDLE pHandle)
{
    PREGPARSE_HANDLE pParseHandle = (PREGPARSE_HANDLE) pHandle;

    if (pParseHandle)
    {

        if (pParseHandle->registryEntry.keyName)
        {
            RegMemoryFree(pParseHandle->registryEntry.keyName);
        }

        if (pParseHandle->binaryData)
        {
            RegMemoryFree(pParseHandle->binaryData);
        }
        RegLexClose(pParseHandle->lexHandle);
        RegIOClose(pParseHandle->ioHandle);
        RegMemoryFree(pParseHandle);
    }
}


DWORD
RegParseRegistry(
    HANDLE pHandle)
{
    DWORD dwError = 0;
    REGLEX_TOKEN token = 0;
    BOOLEAN eof = 0;
    PREGPARSE_HANDLE parseHandle = (PREGPARSE_HANDLE) pHandle;

    do {
        dwError = RegLexGetToken(parseHandle->ioHandle,
                                 parseHandle->lexHandle,
                                 &token,
                                 &eof);
        if (!eof)
        {
            dwError = RegParseKey(parseHandle, token);
        }
    } while (!eof);

    return dwError;
}
