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
 *        regparse.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry .REG parser header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewise.com)
 */
#ifndef REGDUMP_H
#define REGDUMP_H

/*
 * Call this function to export a registry entry.
 * It supports all of the registry data types supported
 * regparse.
 */

DWORD
RegExportEntry(
    PSTR keyName,
    REG_DATA_TYPE valueType,
    PSTR valueName,
    REG_DATA_TYPE type,
    LW_PVOID value,
    DWORD valueLen,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegExportDword(
    REG_DATA_TYPE valueType,
    PSTR valueName,
    DWORD value,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegExportRegKey(
    PSTR keyName,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegExportString(
    REG_DATA_TYPE valueType,
    PSTR valueName,
    PCHAR value,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegExportBinaryData(
    REG_DATA_TYPE valueType,
    PSTR valueName,
    REG_DATA_TYPE type,
    UCHAR *value,
    DWORD valueLen,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegDumpDword(
    PREG_PARSE_ITEM regEntry,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegDumpRegKey(
    PREG_PARSE_ITEM regEntry,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegDumpRegKey(
    PREG_PARSE_ITEM regEntry,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD RegDumpString(
    PREG_PARSE_ITEM regEntry,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegDumpBinaryData(
    PREG_PARSE_ITEM regEntry,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegExportPlainText(
    PCHAR value,
    PSTR *dumpString,
    PDWORD dumpStringLen);
#endif
