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
 *        regdump.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG file export routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */

#define REGDUMP_LINE_WIDTH 80
#define REGDUMP_MAX_LINE_ELEMENTS 25

#include "includes.h"


#if 1 /* public export functions */
DWORD
RegExportEntry(
    PSTR keyName,
    REG_DATA_TYPE valueType,
    PSTR valueName,
    REG_DATA_TYPE type,
    LW_PVOID value,
    DWORD valueLen,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    DWORD dwError = 0;
    switch (type)
    {
        case REG_BINARY:
        case REG_NONE:
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
        case REG_RESOURCE_LIST:
        case REG_FULL_RESOURCE_DESCRIPTOR:
        case REG_RESOURCE_REQUIREMENTS_LIST:
        case REG_QUADWORD:
            dwError = RegExportBinaryData(valueType,
                                          valueName,
                                          type,
                                          value,
                                          valueLen,
                                          dumpString,
                                          dumpStringLen);
            break;
        case REG_DWORD:
            dwError = RegExportDword(valueType,
                                     valueName,
                                     *((PDWORD) value),
                                     dumpString,
                                     dumpStringLen);
            break;

        case REG_KEY:
            dwError = RegExportRegKey(keyName,
                                      dumpString,
                                      dumpStringLen);
            break;

        case REG_SZ:
            dwError = RegExportString(valueType,
                                      valueName,
                                      (PCHAR) value,
                                      dumpString,
                                      dumpStringLen);
            break;
        case REG_PLAIN_TEXT:
        default:
            dwError = RegExportPlainText((PCHAR) value,
                                        dumpString,
                                        dumpStringLen);
    }
    return dwError;
}
#endif /* public export functions */


DWORD
RegExportDword(
    REG_DATA_TYPE valueType,
    PSTR valueName,
    DWORD value,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    DWORD bufLen = 0;
    PSTR dumpBuf = NULL;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(valueName);
    BAIL_ON_INVALID_POINTER(dumpString);
    BAIL_ON_INVALID_POINTER(dumpStringLen);

    /*
     *  "name"=1234ABCD\r\n\0
     *  14: *  ""=1234ABCD\r\n\0
     */
    bufLen = strlen(valueName) + 20;
    dwError = LwAllocateMemory(bufLen, (LW_PVOID) &dumpBuf);
    BAIL_ON_REG_ERROR(dwError);

    if (valueType == REG_KEY_DEFAULT)
    {
        *dumpStringLen = sprintf(dumpBuf, "@=dword:%08x",
                                 value);
    }
    else
    {
        *dumpStringLen = sprintf(dumpBuf, "\"%s\"=dword:%08x",
                                 valueName,
                                 value);
    }

    *dumpString = dumpBuf;
cleanup:
    return dwError;

error:
    goto cleanup;

}


DWORD
RegExportRegKey(
    PSTR keyName,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    DWORD bufLen = 0;
    PSTR dumpBuf = NULL;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(keyName);
    BAIL_ON_INVALID_POINTER(dumpString);
    BAIL_ON_INVALID_POINTER(dumpStringLen);

    /*
     *  [key_name]\r\n\0
     *  5:  []\r\n\0
     */
    bufLen = strlen(keyName) + 5;
    dwError = LwAllocateMemory(bufLen, (LW_PVOID) &dumpBuf);
    BAIL_ON_REG_ERROR(dwError);
    *dumpStringLen = sprintf(dumpBuf, "[%s]", keyName);
    *dumpString = dumpBuf;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD RegExportString(
    REG_DATA_TYPE valueType,
    PSTR valueName,
    PCHAR value,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    DWORD bufLen = 0;
    PSTR dumpBuf = NULL;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(valueName);
    BAIL_ON_INVALID_POINTER(dumpString);
    BAIL_ON_INVALID_POINTER(dumpStringLen);

    /*
     */
    bufLen = strlen(valueName) + strlen(value) + 8;
    dwError = LwAllocateMemory(bufLen, (LW_PVOID) &dumpBuf);
    BAIL_ON_REG_ERROR(dwError);
    if (valueType == REG_KEY_DEFAULT)
    {
        *dumpStringLen = sprintf(dumpBuf, "@=\"%s\"",
                             (PCHAR) value);
    }
    else
    {
        *dumpStringLen = sprintf(dumpBuf, "\"%s\"=\"%s\"",
                             valueName,
                             (PCHAR) value);
    }
    *dumpString = dumpBuf;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegExportPlainText(
    PCHAR value,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    DWORD bufLen = 0;
    PSTR dumpBuf = NULL;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(dumpString);
    BAIL_ON_INVALID_POINTER(dumpStringLen);

    bufLen = strlen(value) + 8;
    dwError = LwAllocateMemory(bufLen, (LW_PVOID) &dumpBuf);
    BAIL_ON_REG_ERROR(dwError);
    *dumpStringLen = sprintf(dumpBuf, "%s", (PCHAR) value);
    *dumpString = dumpBuf;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegExportBinaryData(
    REG_DATA_TYPE valueType,
    PSTR valueName,
    REG_DATA_TYPE type,
    UCHAR *value,
    DWORD valueLen,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    DWORD bufLen = 0;
    DWORD formatLines = 0;
    DWORD indx = 0;
    DWORD dwError = 0;
    DWORD linePos = 0;
    PSTR dumpBuf = NULL;
    PSTR fmtCursor = NULL;
    UCHAR *pValue = NULL;
    BOOLEAN firstHex = FALSE;

    CHAR typeName[128];

    RegLexBinaryTypeToString(type, typeName, TRUE);
    /* 5 extra for " "= \\n characters on first line */
    bufLen = strlen(valueName) + strlen(typeName) + 6;

    /* 4 extra characters per line: Prefix "  " spaces, suffix \ and \r\n */
    formatLines = valueLen / 25 + 1;
    bufLen += valueLen * 3 + (formatLines * 5) + 1;

    dwError = LwAllocateMemory(bufLen, (LW_PVOID) &dumpBuf);
    BAIL_ON_REG_ERROR(dwError);

    /* Format binary prefix */
    fmtCursor = dumpBuf;
    if (valueType == REG_KEY_DEFAULT)
    {
        fmtCursor += sprintf(fmtCursor, "@=%s", typeName);
    }
    else
    {
        fmtCursor += sprintf(fmtCursor, "\"%s\"=%s",
                             valueName, typeName);
    }

    pValue = (UCHAR *) value;
    linePos = fmtCursor - dumpBuf;
    indx = 0;
    while (indx < valueLen)
    {
        while(((linePos + 3)<REGDUMP_LINE_WIDTH && indx<valueLen) ||
               !firstHex)
        {
            firstHex = TRUE;
            fmtCursor += sprintf(fmtCursor, "%02x,", pValue[indx]);
            linePos += 3;
            indx++;
        }
        if (indx < valueLen)
        {
            fmtCursor += sprintf(fmtCursor, "\\\r\n  ");
            linePos = 2;
        }
        else
        {
            fmtCursor[-1] = '\0';
            linePos = 0;
        }

    }

    *dumpString = dumpBuf;
    *dumpStringLen = fmtCursor - dumpBuf;

cleanup:
    return dwError;

error:
    goto cleanup;
}



DWORD RegDumpDword(
    PREG_PARSE_ITEM regEntry,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    return RegExportDword(regEntry->valueType,
                          regEntry->valueName,
                          *((PDWORD) regEntry->value),
                          dumpString,
                          dumpStringLen);
}


DWORD
RegDumpRegKey(
    PREG_PARSE_ITEM regEntry,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    return RegExportRegKey(regEntry->keyName,
                           dumpString,
                           dumpStringLen);
}


DWORD RegDumpString(
    PREG_PARSE_ITEM regEntry,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    return RegExportString(regEntry->valueType,
                           regEntry->valueName,
                           (PCHAR) regEntry->value,
                           dumpString,
                           dumpStringLen);
}



DWORD
RegDumpBinaryData(
    PREG_PARSE_ITEM regEntry,
    PSTR *dumpString,
    PDWORD dumpStringLen)
{
    return RegExportBinaryData(regEntry->valueType,
                               regEntry->valueName,
                               regEntry->type,
                               regEntry->value,
                               regEntry->valueLen,
                               dumpString,
                               dumpStringLen);
}
