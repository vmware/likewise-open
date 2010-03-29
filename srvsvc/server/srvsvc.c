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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * SrvSvc Server
 *
 */

#include "includes.h"

static
NET_API_STATUS
InitializeUnixRootSid(
    PSID pSid
    )
{
    DWORD dwError = 0;
    SID_IDENTIFIER_AUTHORITY identifierAuthority = { SECURITY_UNMAPPED_UNIX_AUTHORITY };

    dwError = LwNtStatusToWin32Error(
        RtlInitializeSid(
            pSid,
            &identifierAuthority,
            SECURITY_UNMAPPED_UNIX_RID_COUNT));
    BAIL_ON_SRVSVC_ERROR(dwError);

    pSid->SubAuthority[0] = SECURITY_UNMAPPED_UNIX_UID_RID;
    pSid->SubAuthority[1] = 0;

error:

    return dwError;
}

NET_API_STATUS
SrvSvcInitSecurity(
    void
    )
{
    DWORD dwError = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsolute = NULL;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } unixRootSid;
    PSID pBuiltinAdminsSid = NULL;
    PSID pAuthedUsersSid = NULL;
    DWORD dwSidSize = 0;
    DWORD dwDaclSize = 0;
    PACL pDacl = NULL;

    dwError = LwCreateWellKnownSid(
            WinBuiltinAdministratorsSid,
            NULL,
            &pBuiltinAdminsSid,
            &dwSidSize);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwCreateWellKnownSid(
            WinAuthenticatedUserSid,
            NULL,
            &pAuthedUsersSid,
            &dwSidSize);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = InitializeUnixRootSid(&unixRootSid.sid);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwDaclSize = ACL_HEADER_SIZE +
        sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(pBuiltinAdminsSid) - sizeof(ULONG) +
        sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(pAuthedUsersSid) - sizeof(ULONG) +
        sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(&unixRootSid.sid) - sizeof(ULONG) +
        RtlLengthSid(pBuiltinAdminsSid);

    dwError = LwAllocateMemory(
        dwDaclSize,
        OUT_PPVOID(&pDacl));
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlCreateAcl(pDacl, dwDaclSize, ACL_REVISION));
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlAddAccessAllowedAceEx(
            pDacl,
            ACL_REVISION,
            0,
            FILE_GENERIC_READ | FILE_GENERIC_WRITE | DELETE,
            &unixRootSid.sid));
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlAddAccessAllowedAceEx(
            pDacl,
            ACL_REVISION,
            0,
            FILE_GENERIC_READ | FILE_GENERIC_WRITE | DELETE,
            pBuiltinAdminsSid));
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlAddAccessAllowedAceEx(
            pDacl,
            ACL_REVISION,
            0,
            FILE_GENERIC_READ,
            pAuthedUsersSid));
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwAllocateMemory(
        SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE,
        OUT_PPVOID(&pAbsolute));
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlCreateSecurityDescriptorAbsolute(
            pAbsolute,
            SECURITY_DESCRIPTOR_REVISION));
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlSetOwnerSecurityDescriptor(
            pAbsolute,
            pBuiltinAdminsSid,
            FALSE));
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlSetGroupSecurityDescriptor(
            pAbsolute,
            pBuiltinAdminsSid,
            FALSE));
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(
        RtlSetDaclSecurityDescriptor(
            pAbsolute,
            TRUE,
            pDacl,
            FALSE));
    BAIL_ON_SRVSVC_ERROR(dwError);

    gServerInfo.pServerSecDesc = pAbsolute;

cleanup:
    LW_SAFE_FREE_MEMORY(pAuthedUsersSid);

    return dwError;

error:

    LW_SAFE_FREE_MEMORY(pAbsolute);
    LW_SAFE_FREE_MEMORY(pDacl);
    LW_SAFE_FREE_MEMORY(pBuiltinAdminsSid);

    goto cleanup;
}

static
NET_API_STATUS
SrvSvcAccessCheck(
    handle_t handle,
    ACCESS_MASK access
    )
{
    DWORD dwError = 0;
    unsigned32 rpcStatus = 0;
    PACCESS_TOKEN pToken = NULL;
    ACCESS_MASK granted = 0;
    NTSTATUS status = STATUS_SUCCESS;

    rpc_binding_inq_access_token_caller(handle, &pToken, &rpcStatus);

    dwError = LwNtStatusToWin32Error(LwRpcStatusToNtStatus(rpcStatus));
    BAIL_ON_SRVSVC_ERROR(dwError);

    if (!RtlAccessCheck(
            gServerInfo.pServerSecDesc,
            pToken,
            access,
            0,
            &gServerInfo.genericMapping,
            &granted,
            &status))
    {
        dwError = LwNtStatusToWin32Error(status);
        BAIL_ON_SRVSVC_ERROR(dwError);
    }
    else if (granted != access)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

cleanup:

    RtlReleaseAccessToken(&pToken);

    return dwError;

error:

    goto cleanup;
}


void _srvsvc_Function0(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function1(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function2(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function3(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function4(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function5(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function6(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function7(
    /* [in] */ handle_t IDL_handle
    )
{
}

NET_API_STATUS _NetrConnectionEnum(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *qualifier,
    /* [in, out] */ UINT32 *level,
    /* [in, out] */ srvsvc_NetConnCtr *ctr,
    /* [in] */ UINT32 prefered_maximum_length,
    /* [out] */ UINT32 *total_entries,
    /* [in, out] */ UINT32 *resume_handle
    )
{
    DWORD dwError = ERROR_NOT_SUPPORTED;

    return dwError;
}

NET_API_STATUS
_NetrFileEnum(
    handle_t           IDL_handle,           /* [in]      */
    PWSTR              pwszServername,       /* [in]      */
    PWSTR              pwszBasepath,         /* [in]      */
    PWSTR              pwszUsername,         /* [in]      */
    PDWORD             pdwInfoLevel,         /* [in, out] */
    srvsvc_NetFileCtr* pInfo,                /* [in, out] */
    DWORD              dwPreferredMaxLength, /* [in]      */
    PDWORD             pdwTotalEntries,      /* [out]     */
    PDWORD             pdwResumeHandle       /* [in, out] */
    )
{
    DWORD dwError        = 0;
    DWORD dwInfoLevel    = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;

    BAIL_ON_INVALID_PTR(pwszBasepath,    dwError);
    BAIL_ON_INVALID_PTR(pwszUsername,    dwError);
    BAIL_ON_INVALID_PTR(pdwInfoLevel,    dwError);
    BAIL_ON_INVALID_PTR(pInfo,           dwError);
    BAIL_ON_INVALID_PTR(pdwTotalEntries, dwError);
    BAIL_ON_INVALID_PTR(pdwResumeHandle, dwError);

    dwInfoLevel    = *pdwInfoLevel;
    if (pdwResumeHandle)
    {
        dwResumeHandle = *pdwResumeHandle;
    }

    dwError = SrvSvcAccessCheck(
                    IDL_handle,
                    FILE_GENERIC_READ | FILE_GENERIC_WRITE);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcNetFileEnum(
                    IDL_handle,
                    pwszServername,
                    pwszBasepath,
                    pwszUsername,
                    dwInfoLevel,
                    pInfo,
                    dwPreferredMaxLength,
                    &dwTotalEntries,
                    pdwResumeHandle ? &dwResumeHandle : NULL);
    BAIL_ON_SRVSVC_ERROR(dwError);

    *pdwTotalEntries = dwTotalEntries;
    if (pdwResumeHandle)
    {
        *pdwResumeHandle = dwResumeHandle;
    }

cleanup:

    return dwError;

error:

    if (pdwTotalEntries)
    {
        *pdwTotalEntries = dwTotalEntries;
    }

    goto cleanup;
}

NET_API_STATUS
_NetrFileGetInfo(
    handle_t            IDL_handle,     /* [in] */
    PWSTR               pwszServername, /* [in] */
    DWORD               dwFileId,       /* [in] */
    DWORD               dwInfoLevel,    /* [in] */
    srvsvc_NetFileInfo* pInfo           /* [out] */
    )
{
    DWORD dwError        = 0;

    BAIL_ON_INVALID_PTR(pInfo, dwError);

    dwError = SrvSvcAccessCheck(
                    IDL_handle,
                    FILE_GENERIC_READ | FILE_GENERIC_WRITE);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcNetFileGetInfo(
                    IDL_handle,
                    pwszServername,
                    dwFileId,
                    dwInfoLevel,
                    pInfo);
    BAIL_ON_SRVSVC_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

NET_API_STATUS
_NetrFileClose(
    handle_t IDL_handle,     /* [in] */
    PWSTR    pwszServername, /* [in] */
    DWORD    dwFileId        /* [in] */
    )
{
    DWORD dwError = 0;

    dwError = SrvSvcAccessCheck(
                    IDL_handle,
                    FILE_GENERIC_READ | FILE_GENERIC_WRITE);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcNetFileClose(
                    IDL_handle,
                    pwszServername,
                    dwFileId);
    BAIL_ON_SRVSVC_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

NET_API_STATUS
_NetrSessionEnum(
    handle_t           IDL_handle,           /* [in]      */
    PWSTR              pwszServername,       /* [in]      */
    PWSTR              pwszUncClientname,    /* [in]      */
    PWSTR              pwszUsername,         /* [in]      */
    PDWORD             pdwInfoLevel,         /* [in, out] */
    srvsvc_NetSessCtr* pInfo,                /* [in, out] */
    DWORD              dwPreferredMaxLength, /* [in]      */
    PDWORD             pdwTotalEntries,      /* [out]     */
    PDWORD             pdwResumeHandle       /* [in, out] */
    )
{
    DWORD dwError        = 0;
    DWORD dwInfoLevel    = 0;
    DWORD dwEntriesRead  = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;

    BAIL_ON_INVALID_PTR(pdwInfoLevel,      dwError);
    BAIL_ON_INVALID_PTR(pInfo,             dwError);
    BAIL_ON_INVALID_PTR(pdwTotalEntries,   dwError);

    dwInfoLevel    = *pdwInfoLevel;
    dwResumeHandle = pdwResumeHandle ? *pdwResumeHandle : 0;

    dwError = SrvSvcAccessCheck(
                    IDL_handle,
                    FILE_GENERIC_READ | FILE_GENERIC_WRITE);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcNetSessionEnum(
                    IDL_handle,
                    pwszServername,
                    pwszUncClientname,
                    pwszUsername,
                    dwInfoLevel,
                    pInfo,
                    dwPreferredMaxLength,
                    &dwEntriesRead,
                    &dwTotalEntries,
                    pdwResumeHandle ? &dwResumeHandle : NULL);
    BAIL_ON_SRVSVC_ERROR(dwError);

    *pdwTotalEntries = dwTotalEntries;
    *pdwResumeHandle = dwResumeHandle;

cleanup:

    return dwError;

error:

    if (pdwTotalEntries)
    {
        *pdwTotalEntries = dwTotalEntries;
    }

    goto cleanup;
}

NET_API_STATUS
_NetrSessionDel(
    handle_t IDL_handle,        /* [in] */
    PWSTR    pwszServername,    /* [in] */
    PWSTR    pwszUncClientname, /* [in] */
    PWSTR    pwszUsername       /* [in] */
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_PTR(pwszUncClientname, dwError);
    BAIL_ON_INVALID_PTR(pwszUsername,      dwError);

    dwError = SrvSvcAccessCheck(
                    IDL_handle,
                    FILE_GENERIC_READ | FILE_GENERIC_WRITE);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcNetSessionDel(
                    IDL_handle,
                    pwszServername,
                    pwszUncClientname,
                    pwszUsername);
    BAIL_ON_SRVSVC_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

NET_API_STATUS _NetrShareAdd(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ UINT32 level,
    /* [in] */ srvsvc_NetShareInfo info,
    /* [in, out] */ UINT32 *parm_error
    )
{
    DWORD dwError = 0;

    dwError = SrvSvcAccessCheck(IDL_handle, FILE_GENERIC_READ | FILE_GENERIC_WRITE);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcNetShareAdd(
                    IDL_handle,
                    server_name,
                    level,
                    info,
                    parm_error
                    );

cleanup:
    return dwError;

error:
    *parm_error = 0;

    goto cleanup;
}

NET_API_STATUS _NetrShareEnum(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in, out] */ UINT32 *level,
    /* [in, out] */ srvsvc_NetShareCtr *ctr,
    /* [in] */ UINT32 prefered_maximum_length,
    /* [out] */ UINT32 *total_entries,
    /* [in, out] */ UINT32 *resume_handle
    )
{
    DWORD dwError = 0;

    dwError = SrvSvcAccessCheck(IDL_handle, FILE_GENERIC_READ);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcNetShareEnum(
                    IDL_handle,
                    server_name,
                    level,
                    ctr,
                    prefered_maximum_length,
                    total_entries,
                    resume_handle
                    );
    BAIL_ON_SRVSVC_ERROR(dwError);

cleanup:
    return dwError;

error:
    memset(ctr, 0, sizeof(*ctr));

    *total_entries = 0;
    *resume_handle = 0;

    goto cleanup;
}

NET_API_STATUS _NetrShareGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
    /* [in] */ UINT32 level,
    /* [out] */ srvsvc_NetShareInfo *info
    )
{
    DWORD dwError = 0;

    dwError = SrvSvcAccessCheck(IDL_handle, FILE_GENERIC_READ);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcNetShareGetInfo(
                    IDL_handle,
                    server_name,
                    netname,
                    level,
                    info
                    );
cleanup:
    return dwError;

error:
    memset(info, 0, sizeof(*info));

    goto cleanup;
}


NET_API_STATUS _NetrShareSetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
    /* [in] */ UINT32 level,
    /* [in] */ srvsvc_NetShareInfo info,
    /* [in, out] */ UINT32 *parm_error
    )
{
    DWORD dwError = 0;

    dwError = SrvSvcAccessCheck(IDL_handle, FILE_GENERIC_READ | FILE_GENERIC_WRITE);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcNetShareSetInfo(
                    IDL_handle,
                    server_name,
                    netname,
                    level,
                    info,
                    parm_error
                    );
cleanup:
    return dwError;

error:
    *parm_error = 0;

    goto cleanup;
}


NET_API_STATUS _NetrShareDel(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
    /* [in] */ UINT32 reserved
    )
{
    DWORD dwError = 0;

    dwError = SrvSvcAccessCheck(IDL_handle, DELETE);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcNetShareDel(
                    IDL_handle,
                    server_name,
                    netname,
                    reserved
                    );

error:

    return dwError;
}

void _srvsvc_Function13(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function14(
    /* [in] */ handle_t IDL_handle
    )
{
}

NET_API_STATUS _NetrServerGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ UINT32 level,
    /* [out] */ srvsvc_NetSrvInfo *info
    )
{
    DWORD dwError = 0;

    dwError = SrvSvcAccessCheck(IDL_handle, FILE_GENERIC_READ);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcNetrServerGetInfo(
                    IDL_handle,
                    server_name,
                    level,
                    info
                    );
    BAIL_ON_SRVSVC_ERROR(dwError);

cleanup:
    return dwError;

error:
    memset(info, 0, sizeof(*info));

    goto cleanup;
}

NET_API_STATUS _NetrServerSetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ UINT32 level,
    /* [in] */ srvsvc_NetSrvInfo info,
    /* [in, out] */ UINT32 *parm_error
    )
{
    DWORD dwError = ERROR_NOT_SUPPORTED;

    return dwError;

}

void _srvsvc_Function17(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function18(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function19(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function1a(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function1b(
    /* [in] */ handle_t IDL_handle
    )
{
}

NET_API_STATUS _NetrRemoteTOD(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [out] */ TIME_OF_DAY_INFO **info
    )
{
    DWORD dwError = ERROR_NOT_SUPPORTED;

    return dwError;
}

void _srvsvc_Function1d(
    /* [in] */ handle_t IDL_handle
)
{
}

void _srvsvc_Function1e(
    /* [in] */ handle_t IDL_handle
)
{
}

void _srvsvc_Function1f(
    /* [in] */ handle_t IDL_handle
)
{
}

void _srvsvc_Function20(
    /* [in] */ handle_t IDL_handle
)
{
}

NET_API_STATUS _NetrNameValidate(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *name,
    /* [in] */ UINT32 type,
    /* [in] */ UINT32 flags
)
{
    DWORD dwError = 0;

    dwError = SrvSvcAccessCheck(IDL_handle, FILE_GENERIC_READ);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcNetNameValidate(
                       IDL_handle,
                       server_name,
                       name,
                       type,
                       flags
                       );

error:

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
