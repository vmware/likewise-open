/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Abstract: Netlogon memory (de)allocation routines (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


NTSTATUS
NetrInitMemory(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;

    LIBRPC_LOCK_MUTEX(bLocked, &gNetrDataMutex);

    if (!bInitialised) {
        ntStatus = MemPtrListInit((PtrList**)&gNetrMemoryList);
        BAIL_ON_NT_STATUS(ntStatus);

        bInitialised = TRUE;
    }

cleanup:
    LIBRPC_UNLOCK_MUTEX(bLocked, &gNetrDataMutex);

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
NetrDestroyMemory(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;

    LIBRPC_LOCK_MUTEX(bLocked, &gNetrDataMutex);

    if (bInitialised && gNetrMemoryList) {
        ntStatus = MemPtrListDestroy((PtrList**)&gNetrMemoryList);
        BAIL_ON_NT_STATUS(ntStatus);

        bInitialised = FALSE;
    }

cleanup:
    LIBRPC_UNLOCK_MUTEX(bLocked, &gNetrDataMutex);

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
NetrAllocateMemory(
    OUT PVOID *pOut,
    IN  size_t Size,
    IN  PVOID  pDep
    )
{
    return MemPtrAllocate((PtrList*)gNetrMemoryList,
                          pOut,
                          Size,
                          pDep);
}


NTSTATUS
NetrFreeMemory(
    IN  PVOID pBuffer
    )
{
    return MemPtrFree((PtrList*)gNetrMemoryList,
                      pBuffer);
}


NTSTATUS
NetrAddDepMemory(
    IN  PVOID pPtr,
    IN  PVOID pDep
    )
{
    return MemPtrAddDependant((PtrList*)gNetrMemoryList,
                              pPtr,
                              pDep);
}


/*
 * Type specific functions
 */

NTSTATUS
NetrAllocateUniString(
    OUT  PWSTR  *ppwszOut,
    IN   PCWSTR  pwszIn,
    IN   PVOID   pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    size_t Len = 0;
    PWSTR pwszOut = NULL;

    BAIL_ON_INVALID_PTR(ppwszOut, ntStatus);
    BAIL_ON_INVALID_PTR(pwszIn, ntStatus);

    Len = wc16slen(pwszIn);

    ntStatus = NetrAllocateMemory((void**)&pwszOut, sizeof(WCHAR) * (Len + 1),
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    wc16sncpy(pwszOut, pwszIn, Len);

    *ppwszOut = pwszOut;

cleanup:
    return ntStatus;

error:
    if (pwszOut) {
        NetrFreeMemory((void*)pwszOut);
    }

    *ppwszOut = NULL;

    goto cleanup;
}


NTSTATUS
NetrAllocateDomainTrusts(
    OUT NetrDomainTrust     **ppOut,
    IN  NetrDomainTrustList  *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NetrDomainTrust *pOut = NULL;
    UINT32 i = 0;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = NetrAllocateMemory((void**)&pOut,
                                  sizeof(NetrDomainTrust) * pIn->count,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < pIn->count; i++) {
        NetrDomainTrust *pTrustOut = &pOut[i];
        NetrDomainTrust *pTrustIn  = &pIn->array[i];

        pTrustOut->trust_flags  = pTrustIn->trust_flags;
        pTrustOut->parent_index = pTrustIn->parent_index;
        pTrustOut->trust_type   = pTrustIn->trust_type;
        pTrustOut->trust_attrs  = pTrustIn->trust_attrs;

        if (pTrustIn->netbios_name) {
            pTrustOut->netbios_name = wc16sdup(pTrustIn->netbios_name);
            BAIL_ON_NULL_PTR(pTrustOut->netbios_name, ntStatus);

            ntStatus = NetrAddDepMemory((void*)pTrustOut->netbios_name,
                                        (void*)pOut);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (pTrustIn->dns_name) {
            pTrustOut->dns_name = wc16sdup(pTrustIn->dns_name);
            BAIL_ON_NULL_PTR(pTrustOut->dns_name, ntStatus);

            ntStatus = NetrAddDepMemory((void*)pTrustOut->dns_name,
                                        (void*)pOut);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (pTrustIn->sid)
        {
            MsRpcDuplicateSid(&pTrustOut->sid, pTrustIn->sid);
            BAIL_ON_NULL_PTR(pTrustOut->sid, ntStatus);

            ntStatus = NetrAddDepMemory((void*)pTrustOut->sid,
                                        (void*)pOut);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pTrustOut->guid = pTrustIn->guid;
    }

cleanup:
    *ppOut = pOut;

    return ntStatus;

error:
    if (pOut) {
        NetrFreeMemory((void*)pOut);
    }

    *ppOut = NULL;

    goto cleanup;
}


NTSTATUS
NetrInitIdentityInfo(
    OUT NetrIdentityInfo *pIdentity,
    IN  PVOID pDep,
    IN  PCWSTR pwszDomain,
    IN  PCWSTR pwszWorkstation,
    IN  PCWSTR pwszAccount,
    IN  UINT32 ParamControl,
    IN  UINT32 LogonIdLow,
    IN  UINT32 LogonIdHigh
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszNbtWorkstation = NULL;
    size_t NbtWorkstationLen = 0;

    BAIL_ON_INVALID_PTR(pIdentity, ntStatus);
    BAIL_ON_INVALID_PTR(pwszDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pwszAccount, ntStatus);
    BAIL_ON_INVALID_PTR(pwszWorkstation, ntStatus);

    NbtWorkstationLen = wc16slen(pwszWorkstation);
    ntStatus = NetrAllocateMemory((void**)&pwszNbtWorkstation,
                                  (NbtWorkstationLen  + 3) * sizeof(WCHAR),
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    if (sw16printfw(
            pwszNbtWorkstation,
            NbtWorkstationLen + 3,
            L"\\\\%ws",
            pwszWorkstation) < 0)
    {
        ntStatus = ErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = InitUnicodeString(&pIdentity->domain_name,
                                 pwszDomain);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIdentity->domain_name.string) {
        ntStatus = NetrAddDepMemory((void*)pIdentity->domain_name.string,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = InitUnicodeString(&pIdentity->account_name,
                                 pwszAccount);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIdentity->account_name.string) {
        ntStatus = NetrAddDepMemory((void*)pIdentity->account_name.string,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = InitUnicodeString(&pIdentity->workstation,
                                 pwszNbtWorkstation);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIdentity->workstation.string) {
        ntStatus = NetrAddDepMemory((void*)pIdentity->workstation.string,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pIdentity->param_control = ParamControl;
    pIdentity->logon_id_low  = LogonIdLow;
    pIdentity->logon_id_high = LogonIdHigh;

cleanup:
    if (pwszNbtWorkstation) {
        NetrFreeMemory((void*)pwszNbtWorkstation);
    }

    return ntStatus;

error:
    FreeUnicodeString(&pIdentity->domain_name);
    FreeUnicodeString(&pIdentity->account_name);
    FreeUnicodeString(&pIdentity->workstation);

    goto cleanup;
}

/*
 * Compatibility wrapper
 */
NTSTATUS
NetrAllocateLogonInfo(
    OUT NetrLogonInfo **ppOut,
    IN  UINT16          Level,
    IN  PCWSTR          pwszDomain,
    IN  PCWSTR          pwszWorkstation,
    IN  PCWSTR          pwszAccount,
    IN  PCWSTR          pwszPassword
    )
{
    return NetrAllocateLogonInfoHash(ppOut,
                                     Level,
                                     pwszDomain,
                                     pwszWorkstation,
                                     pwszAccount,
                                     pwszPassword);
}


NTSTATUS
NetrAllocateLogonInfoHash(
    OUT NetrLogonInfo **ppOut,
    IN  UINT16          Level,
    IN  PCWSTR          pwszDomain,
    IN  PCWSTR          pwszWorkstation,
    IN  PCWSTR          pwszAccount,
    IN  PCWSTR          pwszPassword
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NetrLogonInfo *pLogonInfo = NULL;
    NetrPasswordInfo *pPassword = NULL;
    BYTE LmHash[16] = {0};
    BYTE NtHash[16] = {0};

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);
    BAIL_ON_INVALID_PTR(pwszDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pwszAccount, ntStatus);
    BAIL_ON_INVALID_PTR(pwszWorkstation, ntStatus);
    BAIL_ON_INVALID_PTR(pwszPassword, ntStatus);

    ntStatus = NetrAllocateMemory((void**)&pLogonInfo,
                                  sizeof(NetrLogonInfo),
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);


    switch (Level)
    {
    case 1:
    case 3:
    case 5:
        /* Create password hashes (NT and LM) */
        deshash(LmHash, pwszPassword);
        md4hash(NtHash, pwszPassword);

        ntStatus = NetrAllocateMemory((void**)&pPassword,
                                      sizeof(*pPassword),
                                      (void*)pLogonInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NetrInitIdentityInfo(&pPassword->identity,
                                        (void*)pPassword,
                                        pwszDomain,
                                        pwszWorkstation,
                                        pwszAccount,
                                        0,
                                        0,
                                        0);
        BAIL_ON_NT_STATUS(ntStatus);

        /* Copy the password hashes */
        memcpy((void*)pPassword->lmpassword.data,
               (void*)LmHash,
               sizeof(pPassword->lmpassword.data));
        memcpy((void*)pPassword->ntpassword.data,
               (void*)NtHash,
               sizeof(pPassword->ntpassword.data));
        break;

    default:
        ntStatus = STATUS_INVALID_LEVEL;
        goto error;
    }

    switch (Level) {
    case 1:
        pLogonInfo->password1 = pPassword;
        break;
    case 3:
        pLogonInfo->password3 = pPassword;
        break;
    case 5:
        pLogonInfo->password5 = pPassword;
        break;
    }

cleanup:
    *ppOut = pLogonInfo;

    return ntStatus;

error:
    if (pLogonInfo) {
        NetrFreeMemory((void*)pLogonInfo);
    }

    pLogonInfo = NULL;

    goto cleanup;
}

NTSTATUS
NetrAllocateLogonInfoNet(
    OUT NetrLogonInfo **ppOut,
    IN  UINT16          Level,
    IN  PCWSTR          pwszDomain,
    IN  PCWSTR          pwszWorkstation,
    IN  PCWSTR          pwszAccount,
    IN  PBYTE           pChallenge,
    IN  PBYTE           pLmResp,
    IN  UINT32          LmRespLen,
    IN  PBYTE           pNtResp,
    IN  UINT32          NtRespLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NetrLogonInfo *pLogonInfo = NULL;
    NetrNetworkInfo *pNetworkInfo = NULL;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);
    BAIL_ON_INVALID_PTR(pwszDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pwszAccount, ntStatus);
    BAIL_ON_INVALID_PTR(pwszWorkstation, ntStatus);
    BAIL_ON_INVALID_PTR(pChallenge, ntStatus);
    /* LanMan Response can be NULL */
    BAIL_ON_INVALID_PTR(pNtResp, ntStatus);

    ntStatus = NetrAllocateMemory((void**)&pLogonInfo,
                                  sizeof(NetrLogonInfo),
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (Level)
    {
    case 2:
    case 6:
        ntStatus = NetrAllocateMemory((void**)&pNetworkInfo,
                                      sizeof(NetrNetworkInfo),
                                      (void*)pLogonInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NetrInitIdentityInfo(&pNetworkInfo->identity,
                                        (void*)pLogonInfo,
                                        pwszDomain,
                                        pwszWorkstation,
                                        pwszAccount,
                                        0,
                                        0,
                                        0);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pNetworkInfo->challenge,
               pChallenge,
               sizeof(pNetworkInfo->challenge));

        /* Allocate challenge structures */
        if ((pLmResp != NULL) && (LmRespLen != 0))
        {
            ntStatus = NetrAllocateMemory((void**)&pNetworkInfo->lm.data,
                                          LmRespLen,
                                          (void*)pLogonInfo);
            BAIL_ON_NT_STATUS(ntStatus);

            pNetworkInfo->lm.length = LmRespLen;
            pNetworkInfo->lm.size   = LmRespLen;
            memcpy(pNetworkInfo->lm.data, pLmResp, pNetworkInfo->lm.size);
        }

        /* Always have NT Response */

        ntStatus = NetrAllocateMemory((void**)&pNetworkInfo->nt.data,
                                      NtRespLen,
                                      (void*)pLogonInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pNetworkInfo->nt.length = NtRespLen;
        pNetworkInfo->nt.size   = NtRespLen;
        memcpy(pNetworkInfo->nt.data, pNtResp, pNetworkInfo->nt.size);

        break;

    default:
        ntStatus = STATUS_INVALID_LEVEL;
        goto error;
    }

    switch (Level) {
    case 2:
        pLogonInfo->network2  = pNetworkInfo;
        break;

    case 6:
        pLogonInfo->network6  = pNetworkInfo;
        break;
    }

    *ppOut = pLogonInfo;

cleanup:
    return ntStatus;

error:
    if (pLogonInfo) {
        NetrFreeMemory((void*)pLogonInfo);
    }

    *ppOut = NULL;

    goto cleanup;
}


static
NTSTATUS
NetrInitSamBaseInfo(
    OUT NetrSamBaseInfo *pOut,
    IN  NetrSamBaseInfo *pIn,
    IN  PVOID            pDep)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    int i = 0;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->last_logon            = pIn->last_logon;
    pOut->last_logoff           = pIn->last_logoff;
    pOut->acct_expiry           = pIn->acct_expiry;
    pOut->last_password_change  = pIn->last_password_change;
    pOut->allow_password_change = pIn->allow_password_change;
    pOut->force_password_change = pIn->force_password_change;

    ntStatus = CopyUnicodeStringEx(&pOut->account_name,
                                   &pIn->account_name);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->account_name.string) {
        ntStatus = NetrAddDepMemory((void*)pOut->account_name.string,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = CopyUnicodeStringEx(&pOut->full_name,
                                   &pIn->full_name);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->full_name.string) {
        ntStatus = NetrAddDepMemory((void*)pOut->full_name.string,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = CopyUnicodeStringEx(&pOut->logon_script,
                                   &pIn->logon_script);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->logon_script.string) {
        ntStatus = NetrAddDepMemory((void*)pOut->logon_script.string,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = CopyUnicodeStringEx(&pOut->profile_path,
                                   &pIn->profile_path);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->profile_path.string) {
        ntStatus = NetrAddDepMemory((void*)pOut->profile_path.string,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = CopyUnicodeStringEx(&pOut->home_directory,
                                   &pIn->home_directory);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->home_directory.string) {
        ntStatus = NetrAddDepMemory((void*)pOut->home_directory.string,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = CopyUnicodeStringEx(&pOut->home_drive,
                                   &pIn->home_drive);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->home_drive.string) {
        ntStatus = NetrAddDepMemory((void*)pOut->home_drive.string,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pOut->logon_count        = pIn->logon_count;
    pOut->bad_password_count = pIn->bad_password_count;
    pOut->rid                = pIn->rid;
    pOut->primary_gid        = pIn->primary_gid;

    pOut->groups.count       = pIn->groups.count;
    ntStatus = NetrAllocateMemory((void*)&pOut->groups.rids,
                                sizeof(RidWithAttribute) * pOut->groups.count,
                                (void*)pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < pOut->groups.count; i++) {
        RidWithAttribute *pRidAttrOut = &(pOut->groups.rids[i]);
        RidWithAttribute *pRidAttrIn  = &(pIn->groups.rids[i]);

        pRidAttrOut->rid        = pRidAttrIn->rid;
        pRidAttrOut->attributes = pRidAttrIn->attributes;
    }

    pOut->user_flags = pIn->user_flags;

    memcpy((void*)pOut->key.key,
           (void*)pIn->key.key,
           sizeof(pOut->key.key));

    ntStatus = CopyUnicodeStringEx(&pOut->logon_server,
                                   &pIn->logon_server);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->logon_server.string) {
        ntStatus = NetrAddDepMemory((void*)pOut->logon_server.string,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = CopyUnicodeStringEx(&pOut->domain,
                                   &pIn->domain);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->domain.string) {
        ntStatus = NetrAddDepMemory((void*)pOut->domain.string,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pIn->domain_sid) {
        MsRpcDuplicateSid(&pOut->domain_sid, pIn->domain_sid);
        BAIL_ON_NULL_PTR(pOut->domain_sid, ntStatus);

        ntStatus = NetrAddDepMemory((void*)pOut->domain_sid,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);

    } else {
        pOut->domain_sid = NULL;
    }

    memcpy((void*)pOut->lmkey.key, (void*)pIn->lmkey.key,
           sizeof(pOut->lmkey.key));

    pOut->acct_flags = pIn->acct_flags;

cleanup:
    return ntStatus;

error:
    FreeUnicodeStringEx(&pOut->account_name);
    FreeUnicodeStringEx(&pOut->full_name);
    FreeUnicodeStringEx(&pOut->logon_script);
    FreeUnicodeStringEx(&pOut->profile_path);
    FreeUnicodeStringEx(&pOut->home_directory);
    FreeUnicodeStringEx(&pOut->home_drive);
    FreeUnicodeStringEx(&pOut->logon_server);
    FreeUnicodeStringEx(&pOut->domain);

    if (pOut->groups.rids) {
        NetrFreeMemory((void*)pOut->groups.rids);
    }

    if (pOut->domain_sid) {
        MsRpcFreeSid(pOut->domain_sid);
    }

    goto cleanup;
}


static
NTSTATUS
NetrAllocateSamInfo2(
    OUT NetrSamInfo2 **ppOut,
    IN  NetrSamInfo2  *pIn,
    IN  PVOID          pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NetrSamInfo2 *pInfo = NULL;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);

    ntStatus = NetrAllocateMemory((void*)&pInfo,
                                  sizeof(NetrSamInfo2),
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn) {
        ntStatus = NetrInitSamBaseInfo(&pInfo->base,
                                       &pIn->base,
                                       (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppOut = pInfo;

cleanup:
    return ntStatus;

error:
    if (pInfo) {
        NetrFreeMemory((void*)pInfo);
    }

    *ppOut = NULL;

    goto cleanup;
}


static
NTSTATUS
NetrAllocateSamInfo3(
    OUT NetrSamInfo3 **ppOut,
    IN  NetrSamInfo3  *pIn,
    IN  PVOID          pDep)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NetrSamInfo3 *pInfo = NULL;
    UINT32 i = 0;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);

    ntStatus = NetrAllocateMemory((void*)&pInfo,
                                  sizeof(NetrSamInfo3),
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn) {
        ntStatus = NetrInitSamBaseInfo(&pInfo->base,
                                       &pIn->base,
                                       (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);

        pInfo->sidcount = pIn->sidcount;

        ntStatus = NetrAllocateMemory((void*)&pInfo->sids,
                                      sizeof(NetrSidAttr) * pInfo->sidcount,
                                      (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);

        for (i = 0; i < pInfo->sidcount; i++) {
            NetrSidAttr *pSidAttrOut = &(pInfo->sids[i]);
            NetrSidAttr *pSidAttrIn  = &(pIn->sids[i]);

            if (pSidAttrIn->sid) {
                MsRpcDuplicateSid(&pSidAttrOut->sid, pSidAttrIn->sid);
                BAIL_ON_NULL_PTR(pSidAttrOut->sid, ntStatus);

                ntStatus = NetrAddDepMemory((void*)pSidAttrOut->sid,
                                            (void*)pDep);
                BAIL_ON_NT_STATUS(ntStatus);

            } else {
                pSidAttrOut->sid = NULL;
            }

            pSidAttrOut->attribute = pSidAttrIn->attribute;
        }
    }

    *ppOut = pInfo;

cleanup:
    return ntStatus;

error:
    if (pInfo) {
        NetrFreeMemory((void*)pInfo);
    }

    *ppOut = NULL;

    goto cleanup;
}


static
NTSTATUS
NetrAllocatePacInfo(
    OUT NetrPacInfo **ppOut,
    IN  NetrPacInfo  *pIn,
    IN  PVOID         pDep
    )
{
    return STATUS_NOT_IMPLEMENTED;
}


static
NTSTATUS
NetrAllocateSamInfo6(
    OUT NetrSamInfo6 **ppOut,
    IN  NetrSamInfo6  *pIn,
    IN  PVOID          pDep
    )
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NetrAllocateValidationInfo(
    OUT NetrValidationInfo **ppOut,
    IN  NetrValidationInfo  *pIn,
    IN  UINT16               Level
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NetrValidationInfo *pInfo = NULL;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);

    ntStatus = NetrAllocateMemory((void**)&pInfo,
                                  sizeof(NetrValidationInfo),
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (Level) {
    case 2:
        ntStatus = NetrAllocateSamInfo2(&pInfo->sam2,
                                        pIn->sam2,
                                        (void*)pInfo);
        break;

    case 3:
        ntStatus = NetrAllocateSamInfo3(&pInfo->sam3,
                                        pIn->sam3,
                                        (void*)pInfo);
        break;

    case 4:
        ntStatus = NetrAllocatePacInfo(&pInfo->pac4,
                                       pIn->pac4,
                                       (void*)pInfo);
        break;

    case 5:
        ntStatus = NetrAllocatePacInfo(&pInfo->pac5,
                                       pIn->pac5,
                                       (void*)pInfo);
        break;

    case 6:
        ntStatus = NetrAllocateSamInfo6(&pInfo->sam6,
                                        pIn->sam6,
                                        (void*)pInfo);
        break;

    default:
        ntStatus = STATUS_INVALID_LEVEL;
    }

    BAIL_ON_NT_STATUS(ntStatus);

    *ppOut = pInfo;

cleanup:
    return ntStatus;

error:
    if (pInfo) {
        NetrFreeMemory((void*)pInfo);
    }

    *ppOut = NULL;

    goto cleanup;
}


static
NTSTATUS
NetrCopyDomainTrustInfo(
    OUT NetrDomainTrustInfo *pOut,
    IN  NetrDomainTrustInfo *pIn,
    IN  PVOID                pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = CopyUnicodeString(&pOut->domain_name,
                                 &pIn->domain_name);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->domain_name.string) {
        ntStatus = NetrAddDepMemory((void*)pOut->domain_name.string,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = CopyUnicodeString(&pOut->full_domain_name,
                                 &pIn->full_domain_name);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->full_domain_name.string) {
        ntStatus = NetrAddDepMemory((void*)pOut->full_domain_name.string,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = CopyUnicodeString(&pOut->forest,
                                 &pIn->forest);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->forest.string) {
        ntStatus = NetrAddDepMemory((void*)pOut->forest.string,
                                    (void*)pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(&pOut->guid, &pIn->guid, sizeof(pOut->guid));

    if (pIn->sid) {
        MsRpcDuplicateSid(&pOut->sid, pIn->sid);
        BAIL_ON_NULL_PTR(pOut->sid, ntStatus);

        ntStatus = NetrAddDepMemory((void*)pOut->sid, pDep);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    return ntStatus;

error:
    memset(pOut, 0, sizeof(*pOut));
    goto cleanup;
}


static
NTSTATUS
NetrAllocateDomainInfo1(
    OUT NetrDomainInfo1 **ppOut,
    IN  NetrDomainInfo1  *pIn,
    IN  PVOID             pDep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NetrDomainInfo1 *pInfo = NULL;
    UINT32 i = 0;

    ntStatus = NetrAllocateMemory((void**)&pInfo,
                                  sizeof(NetrDomainInfo1),
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn == NULL) goto cleanup;

    ntStatus = NetrCopyDomainTrustInfo(&pInfo->domain_info,
                                       &pInfo->domain_info,
                                       pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    pInfo->num_trusts = pIn->num_trusts;

    ntStatus = NetrAllocateMemory((void**)&pInfo->trusts,
                                  sizeof(NetrDomainTrustInfo) * pInfo->num_trusts,
                                  pDep);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < pInfo->num_trusts; i++) {
        ntStatus = NetrCopyDomainTrustInfo(&pInfo->trusts[i],
                                           &pIn->trusts[i],
                                           pInfo->trusts);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppOut = pInfo;

cleanup:
    return ntStatus;

error:
    if (pInfo) {
        NetrFreeMemory(pInfo);
    }

    *ppOut = NULL;

    goto cleanup;
}


NTSTATUS
NetrAllocateDomainInfo(
    OUT NetrDomainInfo **ppOut,
    IN  NetrDomainInfo  *pIn,
    IN  UINT32           Level
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NetrDomainInfo *pInfo = NULL;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);

    ntStatus = NetrAllocateMemory((void**)&pInfo,
                                  sizeof(NetrDomainInfo),
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn == NULL) goto cleanup;

    switch (Level) {
    case 1:
        ntStatus = NetrAllocateDomainInfo1(&pInfo->info1,
                                           pIn->info1,
                                           (void*)pInfo);
        break;

    case 2:
        ntStatus = NetrAllocateDomainInfo1(&pInfo->info2,
                                           pIn->info2,
                                           (void*)pInfo);
        break;

    default:
        ntStatus = STATUS_INVALID_LEVEL;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppOut = pInfo;

cleanup:
    return ntStatus;

error:
    if (pInfo) {
        NetrFreeMemory((void*)pInfo);
    }

    *ppOut = NULL;

    goto cleanup;
}


NTSTATUS
NetrAllocateDcNameInfo(
    OUT DsrDcNameInfo **ppOut,
    IN  DsrDcNameInfo  *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DsrDcNameInfo *pInfo = NULL;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);

    ntStatus = NetrAllocateMemory((void**)&pInfo,
                                  sizeof(DsrDcNameInfo),
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn == NULL) goto cleanup;

    if (pIn->dc_name) {
        pInfo->dc_name = wc16sdup(pIn->dc_name);
        BAIL_ON_NULL_PTR(pInfo->dc_name, ntStatus);

        ntStatus = NetrAddDepMemory(pInfo->dc_name, pInfo);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pIn->dc_address) {
        pInfo->dc_address = wc16sdup(pIn->dc_address);
        BAIL_ON_NULL_PTR(pInfo->dc_address, ntStatus);

        ntStatus = NetrAddDepMemory(pInfo->dc_address, pInfo);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pInfo->address_type = pIn->address_type;
    pInfo->flags        = pIn->flags;

    memcpy(&pInfo->domain_guid, &pIn->domain_guid, sizeof(pInfo->domain_guid));

    if (pIn->domain_name) {
        pInfo->domain_name = wc16sdup(pIn->domain_name);
        BAIL_ON_NULL_PTR(pInfo->domain_name, ntStatus);

        ntStatus = NetrAddDepMemory(pInfo->domain_name,
                                    pInfo);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pIn->forest_name) {
        pInfo->forest_name = wc16sdup(pIn->forest_name);
        BAIL_ON_NULL_PTR(pInfo->forest_name, ntStatus);

        ntStatus = NetrAddDepMemory(pInfo->forest_name,
                                    pInfo);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pIn->dc_site_name) {
        pInfo->dc_site_name = wc16sdup(pIn->dc_site_name);
        BAIL_ON_NULL_PTR(pInfo->dc_site_name, ntStatus);

        ntStatus = NetrAddDepMemory(pInfo->dc_site_name,
                                    pInfo);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pIn->cli_site_name) {
        pInfo->cli_site_name = wc16sdup(pIn->cli_site_name);
        BAIL_ON_NULL_PTR(pInfo->cli_site_name, ntStatus);

        ntStatus = NetrAddDepMemory(pInfo->cli_site_name,
                                    pInfo);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppOut = pInfo;

cleanup:
    return ntStatus;

error:
    if (pInfo) {
        NetrFreeMemory((void*)pInfo);
    }

    *ppOut = NULL;

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
