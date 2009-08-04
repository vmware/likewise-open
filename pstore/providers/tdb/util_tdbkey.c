/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *  Copyright (C) Likewise Software. All rights reserved.
 *
 *  Module Name:
 *
 *     provider-main.c
 *
 *  Abstract:
 *
 *        Likewise Password Storage (LWPS)
 *
 *        API to support TDB Password Storage
 *
 *  Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */



#include "util_tdbkey.h"
#include "util_sid.h"

/*************************************************************
 ************************************************************/

static DWORD
KeyCreate(
    PCSTR pszPrefix,
    PCSTR pszData,
    PSTR *pszKey
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;
    DWORD dwLen = 0;
    PVOID pBuffer = NULL;

    dwLen = strlen(pszPrefix) + strlen(pszData) + 1;

    dwError = LwpsAllocateMemory(dwLen, &pBuffer);
    BAIL_ON_LWPS_ERROR(dwError);

    snprintf((PSTR)pBuffer, dwLen,
             "%s%s",
             pszPrefix,
             pszData);
    *pszKey = (PSTR)pBuffer;

    dwError = LWPS_ERROR_SUCCESS;

error:
    return dwError;
}

/*************************************************************
 ************************************************************/

DWORD
KeyMachinePassword(
    PCSTR pszDomain,
    PSTR *pszKey
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;
    PCSTR pszKeyPrefix = "SECRETS/MACHINE_PASSWORD/";

    BAIL_ON_INVALID_POINTER(pszDomain);

    dwError = KeyCreate(pszKeyPrefix, pszDomain, pszKey);
    BAIL_ON_LWPS_ERROR(dwError);

    StrUpper(*pszKey);

error:
    return dwError;
}

/*************************************************************
 ************************************************************/

DWORD
KeyDomainSid(
    PCSTR pszDomain,
    PSTR *pszKey
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;
    PCSTR pszKeyPrefix = "SECRETS/SID/";

    BAIL_ON_INVALID_POINTER(pszDomain);

    dwError = KeyCreate(pszKeyPrefix, pszDomain, pszKey);
    BAIL_ON_LWPS_ERROR(dwError);

    StrUpper(*pszKey);

error:
    return dwError;
}

/*************************************************************
 ************************************************************/

DWORD
KeyPasswordLastChangeTime(
    PCSTR pszDomain,
    PSTR *pszKey
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;
    PCSTR pszKeyPrefix = "SECRETS/MACHINE_LAST_CHANGE_TIME/";

    BAIL_ON_INVALID_POINTER(pszDomain);

    dwError = KeyCreate(pszKeyPrefix, pszDomain, pszKey);
    BAIL_ON_LWPS_ERROR(dwError);

    StrUpper(*pszKey);

error:
    return dwError;
}

/*************************************************************
 ************************************************************/

DWORD
KeySecureChannelType(
    PCSTR pszDomain,
    PSTR *pszKey
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;
    PCSTR pszKeyPrefix = "SECRETS/MACHINE_SEC_CHANNEL_TYPE/";

    BAIL_ON_INVALID_POINTER(pszDomain);

    dwError = KeyCreate(pszKeyPrefix, pszDomain, pszKey);
    BAIL_ON_LWPS_ERROR(dwError);

    StrUpper(*pszKey);

error:
    return dwError;
}

/*************************************************************
 ************************************************************/

TDB_DATA TdbDataBlob(
    PBYTE Data,
    size_t Len
    )
{
    TDB_DATA Blob;

    Blob.dptr = Data;
    Blob.dsize = Len;

    return Blob;
}

/*************************************************************
 ************************************************************/

static DWORD
TdbStore(
    PTDB_PROVIDER_CONTEXT pCtx,
    PSTR pszKey,
    TDB_DATA Data
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;
    int ret;
    TDB_DATA TdbKey;

    TdbKey = TdbDataBlob((PBYTE)pszKey, strlen(pszKey));

    if ((ret = tdb_transaction_start(pCtx->pTdb)) != 0) {
        dwError = LWPS_ERROR_UNEXPECTED_DB_RESULT;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    if ((ret = tdb_store(pCtx->pTdb, TdbKey, Data, TDB_REPLACE)) != 0) {
        tdb_transaction_cancel(pCtx->pTdb);
        dwError = LWPS_ERROR_DATA_ERROR;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    if ((ret = tdb_transaction_commit(pCtx->pTdb)) != 0) {
        dwError = LWPS_ERROR_UNEXPECTED_DB_RESULT;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    dwError = LWPS_ERROR_SUCCESS;

error:
    return dwError;
}

/*************************************************************
 ************************************************************/

static DWORD
TdbFetch(
    PTDB_PROVIDER_CONTEXT pCtx,
    PSTR pszKey,
    TDB_DATA *pData
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;
    TDB_DATA TdbRec;
    TDB_DATA TdbKey;

    TdbKey = TdbDataBlob((PBYTE)pszKey, strlen(pszKey));
    TdbRec = tdb_fetch(pCtx->pTdb, TdbKey);

    if (TdbRec.dsize == 0) {
        dwError = LWPS_ERROR_DB_RECORD_NOT_FOUND;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    pData->dptr = TdbRec.dptr;
    pData->dsize = TdbRec.dsize;

    dwError = LWPS_ERROR_SUCCESS;

error:
    return dwError;
}

/*************************************************************
 ************************************************************/

#if 0
static DWORD
TdbDelete(
    PTDB_PROVIDER_CONTEXT pCtx,
    PSTR pszKey
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;
    TDB_DATA TdbKey;
    int ret;

    TdbKey = TdbDataBlob((PBYTE)pszKey, strlen(pszKey));

    if ((ret = tdb_transaction_start(pCtx->pTdb)) != 0) {
        dwError = LWPS_ERROR_UNEXPECTED_DB_RESULT;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    if ((ret = tdb_delete(pCtx->pTdb, TdbKey)) != 0) {
        tdb_transaction_cancel(pCtx->pTdb);
        dwError = LWPS_ERROR_DATA_ERROR;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    if ((ret = tdb_transaction_commit(pCtx->pTdb)) != 0) {
        dwError = LWPS_ERROR_UNEXPECTED_DB_RESULT;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    dwError = LWPS_ERROR_SUCCESS;

error:
    return dwError;
}

#endif

/*************************************************************
 ************************************************************/

DWORD
TdbStoreMachineAccountInfo(
    PTDB_PROVIDER_CONTEXT pCtx,
    PCSTR pszDomain,
    PMACHINE_ACCT_INFO pAcctInfo
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;
    TDB_DATA data;
    PSTR pszKeyMachPw = NULL;
    PSTR pszKeySid = NULL;
    PSTR pszKeyLCT = NULL;
    PSTR pszKeySchannel = NULL;
    DOMAIN_SID Sid = {0};
    DWORD dwSchannelType = 0;
    DWORD dwLCT = 0;

    /* Machine Password */

    dwError = KeyMachinePassword(pszDomain, &pszKeyMachPw);
    BAIL_ON_LWPS_ERROR(dwError);

    data = TdbDataBlob((PBYTE)pAcctInfo->pszMachineAccountPassword,
                       (size_t)strlen(pAcctInfo->pszMachineAccountPassword)+1);
    dwError = TdbStore(pCtx, pszKeyMachPw, data);
    BAIL_ON_LWPS_ERROR(dwError);

    /* Domain SID */

    dwError = StringToSid(pAcctInfo->pszDomainSID, &Sid);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = KeyDomainSid(pszDomain, &pszKeySid);
    BAIL_ON_LWPS_ERROR(dwError);

    data = TdbDataBlob((PBYTE)&Sid, sizeof(Sid));
    dwError = TdbStore(pCtx, pszKeySid, data);
    BAIL_ON_LWPS_ERROR(dwError);

    /* Schannel Type */

    dwError = KeySecureChannelType(pszDomain, &pszKeySchannel);
    BAIL_ON_LWPS_ERROR(dwError);

    dwSchannelType = LW_HTOL32(pAcctInfo->dwSchannelType);

    data = TdbDataBlob((PBYTE)&dwSchannelType, sizeof(DWORD));
    dwError = TdbStore(pCtx, pszKeySchannel, data);
    BAIL_ON_LWPS_ERROR(dwError);

    /* Last Change Time */

    dwError = KeyPasswordLastChangeTime(pszDomain, &pszKeyLCT);
    BAIL_ON_LWPS_ERROR(dwError);

    dwLCT = LW_HTOL32(pAcctInfo->tPwdClientModifyTimestamp);

    data = TdbDataBlob((PBYTE)&dwLCT, sizeof(DWORD));
    dwError = TdbStore(pCtx, pszKeyLCT, data);
    BAIL_ON_LWPS_ERROR(dwError);

error:

    LWPS_SAFE_FREE_STRING(pszKeyMachPw);
    LWPS_SAFE_FREE_STRING(pszKeyLCT);
    LWPS_SAFE_FREE_STRING(pszKeySchannel);
    LWPS_SAFE_FREE_STRING(pszKeySid);

    return dwError;
}

/*************************************************************
 ************************************************************/

DWORD
TdbFetchMachineAccountInfo(
    PTDB_PROVIDER_CONTEXT pCtx,
    PCSTR pszDomain,
    PMACHINE_ACCT_INFO *ppAcctInfo
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;
    TDB_DATA data = { NULL, 0 };
    PSTR pszKeyMachPw = NULL;
    PSTR pszKeySid = NULL;
    PSTR pszKeySchannel = NULL;
    PSTR pszKeyLCT = NULL;

    dwError = LwpsAllocateMemory(sizeof(MACHINE_ACCT_INFO), (PVOID*)*ppAcctInfo);
    BAIL_ON_LWPS_ERROR(dwError);

    /* Machine Password
       Fetch the record early so we can go ahead and bail
       if it is not there */

    dwError = KeyMachinePassword(pszDomain, &pszKeyMachPw);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = TdbFetch(pCtx, pszKeyMachPw, &data);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsAllocateString((PCSTR)data.dptr,
                                 &(*ppAcctInfo)->pszMachineAccountPassword);
    BAIL_ON_LWPS_ERROR(dwError);

    if (data.dptr) {
        free(data.dptr);
        memset(&data, 0x0, sizeof(data));
    }

    /* Copy short domain name */

    dwError = LwpsAllocateString(pszDomain,
                                 &(*ppAcctInfo)->pszDomainName);
    BAIL_ON_LWPS_ERROR(dwError);

    /* Domain Sid */

    dwError = KeyDomainSid(pszDomain, &pszKeySid);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = TdbFetch(pCtx, pszKeySid, &data);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = SidToString((PDOMAIN_SID)data.dptr,
                          &(*ppAcctInfo)->pszDomainSID);
    BAIL_ON_LWPS_ERROR(dwError);

    if (data.dptr) {
        free(data.dptr);
        memset(&data, 0x0, sizeof(data));
    }

    /* LCT */

    dwError = KeyPasswordLastChangeTime(pszDomain, &pszKeyLCT);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = TdbFetch(pCtx, pszKeyLCT, &data);
    BAIL_ON_LWPS_ERROR(dwError);

    (*ppAcctInfo)->tPwdClientModifyTimestamp  = LW_HTOL32(*data.dptr);

    if (data.dptr) {
        free(data.dptr);
        memset(&data, 0x0, sizeof(data));
    }

    /* Schannel Type */

    dwError = KeySecureChannelType(pszDomain, &pszKeySchannel);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = TdbFetch(pCtx, pszKeySchannel, &data);
    BAIL_ON_LWPS_ERROR(dwError);

    (*ppAcctInfo)->dwSchannelType = LW_HTOL32(*data.dptr);

    if (data.dptr) {
        free(data.dptr);
        memset(&data, 0x0, sizeof(data));
    }

cleanup:
    LWPS_SAFE_FREE_STRING(pszKeyMachPw);
    LWPS_SAFE_FREE_STRING(pszKeyLCT);
    LWPS_SAFE_FREE_STRING(pszKeySchannel);
    LWPS_SAFE_FREE_STRING(pszKeySid);

    if (data.dptr) {
        free(data.dptr);
    }

    return dwError;

error:
    if (*ppAcctInfo) {
        TDB_FreeMachineAccountInfo(*ppAcctInfo);
    }

    goto cleanup;
}

/*************************************************************
 ************************************************************/

DWORD
TdbDeleteMachineAccountInfo(
    PTDB_PROVIDER_CONTEXT pCtx,
    PCSTR pszDomain
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;

    return dwError;
}

/*
  local variables:
  mode: c
  c-basic-offset: 4
  indent-tabs-mode: nil
  tab-width: 4
  end:
*/
