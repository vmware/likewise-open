/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        regutil.h
 *
 * Abstract:
 *
 *        Registry Helper Utilities
 *
 *        Public Client API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *          Adam Bernstein (abernstein@likewisesoftware.com)
 */
#ifndef REGUTIL_H
#define REGUTIL_H

#include <reg/reg.h>


typedef struct _REGSHELL_UTIL_VALUE
{
    REG_DATA_TYPE type;
    PWSTR pValueName;
    PVOID pData;
    DWORD dwDataLen;
} REGSHELL_UTIL_VALUE, *PREGSHELL_UTIL_VALUE;


DWORD
RegUtilIsValidKey(
    IN HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszKey);

DWORD
RegUtilAddKey(
    IN HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszDefaultKey,
    IN PSTR pszKeyName);

DWORD
RegUtilDeleteKey(
    IN HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszDefaultKey,
    IN PSTR keyName);

DWORD
RegUtilDeleteTree(
    IN HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszDefaultKey,
    IN PSTR keyName);

DWORD
RegUtilGetKeys(
    IN HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszDefaultKey,
    IN PSTR keyName,
    OUT WCHAR ***pppRetSubKeys,
    OUT PDWORD pdwRetSubKeyCount);

DWORD
RegUtilSetValue(
    IN HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszDefaultKey,
    IN PSTR keyName,
    IN PSTR valueName,
    IN REG_DATA_TYPE type,
    IN PVOID data,
    IN DWORD dataLen);

DWORD
RegUtilGetValues(
    IN HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszDefaultKey,
    IN PSTR keyName,
    OUT PREGSHELL_UTIL_VALUE *valueArray,
    OUT PDWORD pdwValueArrayLen);

DWORD
RegUtilDeleteValue(
    IN HANDLE hReg,
    IN PSTR pszRootKeyName,
    IN PSTR pszDefaultKey,
    IN PSTR keyName,
    IN PSTR valueName);

#endif
