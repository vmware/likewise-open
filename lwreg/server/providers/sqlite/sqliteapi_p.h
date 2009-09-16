/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        sqliteapi_p.h
 *
 * Abstract:
 *
 *        Registry sqlite provider (Private Header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#ifndef SQLITEAPI_P_H_
#define SQLITEAPI_P_H_

DWORD
SqliteGetParentKeyName(
    PCSTR pszInputString,
    CHAR c,
    PSTR *ppszOutputString
    );

DWORD
SqliteCreateKeyHandle(
    IN PREG_ENTRY pRegEntry,
    OUT PHKEY phkResult
    );

DWORD
SqliteCreateKeyInternal(
    IN PSTR pszKeyName,
    IN OPTIONAL PCWSTR pSubKey, //pSubKey is null only when creating HKEY_LIKEWISE
    OUT OPTIONAL PHKEY ppKeyResult
    );

DWORD
SqliteOpenKeyInternal(
    IN PSTR pszKeyName,
    IN OPTIONAL PCWSTR pSubKey,
    OUT PHKEY phkResult
    );

DWORD
SqliteFillInSubKeysInfo(
    PREG_KEY_CONTEXT pKeyResult
    );

DWORD
SqliteFillinKeyValuesInfo(
    PREG_KEY_CONTEXT pKeyResult
    );

DWORD
SqliteDbDeleteKeyInternal(
    IN PSTR pszKeyName
    );

DWORD
SqliteDeleteActiveKey(
    IN PSTR pszKeyName
    );



PREG_KEY_CONTEXT
RegSrvLocateActiveKey(
    IN PCSTR pszKeyName
    );

PREG_KEY_CONTEXT
RegSrvLocateActiveKey_inlock(
    IN PCSTR pszKeyName
    );

DWORD
RegSrvInsertActiveKey(
    IN PREG_KEY_CONTEXT pKeyResult
    );

DWORD
RegSrvInsertActiveKey_inlock(
    IN PREG_KEY_CONTEXT pKeyResult
    );

VOID
RegSrvDeleteActiveKey(
    IN PSTR pszKeyName
    );

VOID
RegSrvDeleteActiveKey_inlock(
    IN PSTR pszKeyName
    );

void
RegSrvResetParentKeySubKeyInfo(
    IN PSTR pszParentKeyName
    );

void
RegSrvResetParentKeySubKeyInfo_inlock(
    IN PSTR pszParentKeyName
    );

void
RegSrvResetKeyValueInfo(
    IN PSTR pszKeyName
    );

void
RegSrvResetKeyValueInfo_inlock(
    IN PSTR pszKeyName
    );

VOID
RegSrvReleaseKey(
    PREG_KEY_CONTEXT pKeyResult
    );

VOID
RegSrvReleaseKey_inlock(
    PREG_KEY_CONTEXT pKeyResult
    );

#endif /* SQLITEAPI_P_H_ */
