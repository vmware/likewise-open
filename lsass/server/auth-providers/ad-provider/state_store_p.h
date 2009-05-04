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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        state_store_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Private functions for AD Caching
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#ifndef __STATE_STORE_P_H__
#define __STATE_STORE_P_H__

typedef struct _ADSTATE_CONNECTION
{
    sqlite3 *pDb;
    pthread_rwlock_t lock;

    sqlite3_stmt *pstGetProviderData;
    sqlite3_stmt *pstGetDomainTrustList;
    sqlite3_stmt *pstGetCellList;
} ADSTATE_CONNECTION, *PADSTATE_CONNECTION;

// This is the maximum number of characters necessary to store a guid in
// string form.
#define UUID_STR_SIZE 37

#define ADSTATE_DB     LSASS_DB_DIR "/lsass-adstate.db"

static
DWORD
ADState_Setup(
    IN sqlite3* pSqlHandle
    );

static
DWORD
ADState_FreePreparedStatements(
    IN OUT PADSTATE_CONNECTION pConn
    );

static
DWORD
ADState_UnpackDomainTrust(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO pResult
    );

static
DWORD
ADState_UnpackLinkedCellInfo(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN OUT PAD_LINKED_CELL_INFO pResult);

static
DWORD
ADState_GetCellListNoLock(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    // Contains type PAD_LINKED_CELL_INFO
    IN OUT PDLINKEDLIST* ppCellList
    );

static
DWORD
ADState_GetCacheCellListCommand(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    // Contains type PAD_LINKED_CELL_INFO
    IN const DLINKEDLIST* pCellList,
    OUT PSTR* ppszCommand
    );

static
VOID
ADState_FreeEnumDomainInfoCallback(
    IN OUT PVOID pData,
    IN PVOID pUserData
    );

static
VOID
ADState_FreeEnumDomainInfo(
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo
    );

#define AD_STATE_TABLE_NAME_LINKED_CELLS     "lwilinkedcells"
#define AD_STATE_TABLE_NAME_PROVIDER_DATA    "lwiproviderdata"
#define AD_STATE_TABLE_NAME_TRUSTS           "lwidomaintrusts"

#define _AD_STATE_SQL_CREATE_TABLE(Table) \
    "CREATE TABLE IF NOT EXISTS " Table " "

#define AD_STATE_CREATE_TABLES \
    "\n" \
    _AD_STATE_SQL_CREATE_TABLE(AD_STATE_TABLE_NAME_LINKED_CELLS) "(\n" \
    "    RowIndex integer PRIMARY KEY,\n" \
    "    CellDN text,\n" \
    "    Domain text,\n" \
    "    IsForestCell integer\n" \
    "    );\n" \
    "\n" \
    _AD_STATE_SQL_CREATE_TABLE(AD_STATE_TABLE_NAME_PROVIDER_DATA) "(\n" \
    "    DirectoryMode integer,\n" \
    "    ADConfigurationMode integer,\n" \
    "    ADMaxPwdAge integer,\n" \
    "    Domain text PRIMARY KEY,\n" \
    "    ShortDomain text,\n" \
    "    ComputerDN text,\n" \
    "    CellDN text\n" \
    "    );\n" \
    "\n" \
    _AD_STATE_SQL_CREATE_TABLE(AD_STATE_TABLE_NAME_TRUSTS) "(\n" \
    "    RowIndex integer PRIMARY KEY,\n" \
    "    DnsDomainName text,\n" \
    "    NetbiosDomainName text,\n" \
    "    Sid text,\n" \
    "    Guid text,\n" \
    "    TrusteeDnsDomainName text,\n" \
    "    TrustFlags integer,\n" \
    "    TrustType integer,\n" \
    "    TrustAttributes integer,\n" \
    "    TrustDirection integer,\n" \
    "    TrustMode integer,\n" \
    "    ForestName text,\n" \
    "    Flags integer\n" \
    "    );\n" \
    ""

#endif /* __STATE_STORE_P_H__ */
