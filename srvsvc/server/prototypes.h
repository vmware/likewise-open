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
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Likewise Server Service (LWSRVSVC)
 *
 * Server Main
 *
 * Function Prototypes
 *
 */

// accesstoken.c

DWORD
SrvSvcSrvInitAuthInfo(
    IN  handle_t            hBinding,
    OUT PSRVSVC_SRV_CONTEXT pSrvCtx
    );


VOID
SrvSvcSrvFreeAuthInfoContents(
    IN  PSRVSVC_SRV_CONTEXT pSrvCtx
    );

// config.c

DWORD
SrvSvcReadConfigSettings(
    VOID
    );

DWORD
SrvSvcConfigGetLsaLpcSocketPath(
    PSTR* ppszPath
    );

// main.c

VOID
SrvSvcSetProcessShouldExit(
    BOOLEAN val
    );

// netfile.c

NET_API_STATUS
SrvSvcNetFileEnum(
    handle_t           IDL_handle,           /* [in]      */
    PWSTR              pwszServername,       /* [in]      */
    PWSTR              pwszBasepath,         /* [in]      */
    PWSTR              pwszUsername,         /* [in]      */
    DWORD              dwInfoLevel,          /* [in, out] */
    srvsvc_NetFileCtr* pInfo,                /* [in, out] */
    DWORD              dwPreferredMaxLength, /* [in]      */
    PDWORD             pdwEntriesRead,       /* [out]     */
    PDWORD             pdwTotalEntries,      /* [out]     */
    PDWORD             pdwResumeHandle       /* [in, out] */
    );

NET_API_STATUS
SrvSvcNetFileGetInfo(
    handle_t            IDL_handle,          /* [in]  */
    PWSTR               pwszServername,      /* [in]  */
    DWORD               dwFileId,            /* [in]  */
    DWORD               dwInfoLevel,         /* [in]  */
    srvsvc_NetFileInfo* pInfo                /* [out] */
    );

NET_API_STATUS
SrvSvcNetFileClose(
    handle_t IDL_handle,                     /* [in] */
    PWSTR    pwszServername,                 /* [in] */
    DWORD    dwFileId                        /* [in] */
    );

// netsession.c

NET_API_STATUS
SrvSvcNetSessionEnum(
    handle_t           IDL_handle,           /* [in]      */
    PWSTR              pwszServername,       /* [in]      */
    PWSTR              pwszUncClientname,    /* [in]      */
    PWSTR              pwszUsername,         /* [in]      */
    DWORD              dwInfoLevel,          /* [in, out] */
    srvsvc_NetSessCtr* pInfo,                /* [in, out] */
    DWORD              dwPreferredMaxLength, /* [in]      */
    PDWORD             pdwEntriesRead,       /* [out]     */
    PDWORD             pdwTotalEntries,      /* [out]     */
    PDWORD             pdwResumeHandle       /* [in, out] */
    );

NET_API_STATUS
SrvSvcNetSessionDel(
    handle_t IDL_handle,                     /* [in] */
    PWSTR    pwszServername,                 /* [in] */
    PWSTR    pwszUncClientname,              /* [in] */
    PWSTR    pwszUsername                    /* [in] */
    );

// security.c

DWORD
SrvSvcSrvInitServerSecurityDescriptor(
    PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    );

DWORD
SrvSvcSrvDestroyServerSecurityDescriptor(
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc
    );

// signalhandler.c

VOID
SrvSvcBlockSelectedSignals(
    VOID
    );

DWORD
SrvSvcHandleSignals(
    VOID
    );

// srvsvc.c

NET_API_STATUS
SrvSvcInitSecurity(
    void
    );

NET_API_STATUS
SrvSvcNetShareAdd(
    handle_t IDL_handle,
    wchar16_t *server_name,
    UINT32 level,
    srvsvc_NetShareInfo info,
    UINT32 *parm_error
    );


NET_API_STATUS
SrvSvcNetShareEnum(
    handle_t IDL_handle,
    wchar16_t *server_name,
    UINT32 *level,
    srvsvc_NetShareCtr *ctr,
    UINT32 preferred_maximum_length,
    UINT32 *total_entries,
    UINT32 *resume_handle
    );


NET_API_STATUS
SrvSvcNetShareGetInfo(
    handle_t IDL_handle,
    wchar16_t *server_name,
    wchar16_t *netname,
    UINT32 level,
    srvsvc_NetShareInfo *info
    );


NET_API_STATUS
SrvSvcNetShareSetInfo(
    handle_t IDL_handle,
    wchar16_t *server_name,
    wchar16_t *netname,
    UINT32 level,
    srvsvc_NetShareInfo info,
    UINT32 *parm_error
    );


NET_API_STATUS
SrvSvcNetShareDel(
    handle_t IDL_handle,
    wchar16_t *server_name,
    wchar16_t *netname,
    UINT32 reserved
    );


NET_API_STATUS
SrvSvcNetrServerGetInfo(
    handle_t b,
    wchar16_t *server_name,
    UINT32 level,
    srvsvc_NetSrvInfo *info
    );


NET_API_STATUS
SrvSvcNetNameValidate(
    handle_t IDL_handle,
    wchar16_t *server_name,
    wchar16_t *name,
    UINT32 type,
    UINT32 flags
    );


NET_API_STATUS
SrvSvcNetrConnectionEnum(
    handle_t IDL_handle,
    wchar16_t *pwszServerName,
    wchar16_t *pwszQualifier,
    PDWORD pdwLevel,
    srvsvc_NetConnCtr *pCtr,
    DWORD dwPrefMaxLength,
    PDWORD pdwTotalEntries,
    PDWORD pdwResume
    );

// utils.c

DWORD
SrvSvcSrvAllocateWC16StringFromUnicodeString(
    OUT PWSTR           *ppwszOut,
    IN  PUNICODE_STRING pIn
    );

DWORD
SrvSvcSrvAllocateWC16String(
    OUT PWSTR  *ppwszOut,
    IN  PCWSTR  pwszIn
    );

DWORD
SrvSvcSrvAllocateWC16StringFromCString(
    OUT PWSTR  *ppwszOut,
    IN  PCSTR   pszIn
    );

DWORD
SrvSvcSrvCopyShareInfo0(
    IN OUT PSHARE_INFO_0 pOutShareInfo,
    IN     PSHARE_INFO_0 pInShareInfo
    );

DWORD
SrvSvcSrvCopyShareInfo1(
    IN OUT PSHARE_INFO_1 pOutShareInfo,
    IN     PSHARE_INFO_1 pInShareInfo
    );

DWORD
SrvSvcSrvCopyShareInfo2(
    IN OUT PSHARE_INFO_2 pOutShareInfo,
    IN     PSHARE_INFO_2 pInShareInfo
    );

DWORD
SrvSvcSrvCopyShareInfo501(
    IN OUT PSHARE_INFO_501 pOutShareInfo,
    IN     PSHARE_INFO_501 pInShareInfo
    );

DWORD
SrvSvcSrvCopyShareInfo502(
    IN OUT PSHARE_INFO_502 pOutShareInfo,
    IN     PSHARE_INFO_502 pInShareInfo
    );

DWORD
SrvSvcSrvCopyShareInfo1005(
    IN OUT PSHARE_INFO_1005 pOutShareInfo,
    IN     PSHARE_INFO_1005 pInShareInfo
    );

VOID
SrvSvcSrvFreeServerInfo101(
    PSERVER_INFO_101 pServerInfo101
    );

VOID
SrvSvcSrvFreeServerInfo102(
    PSERVER_INFO_102 pServerInfo101
    );

DWORD
SrvSvcSrvCopyConnectionInfo0(
    IN OUT PCONNECTION_INFO_0 pOutConnectionInfo,
    IN     PCONNECTION_INFO_0 pInConnectionInfo
    );

DWORD
SrvSvcSrvCopyConnectionInfo1(
    IN OUT PCONNECTION_INFO_1 pOutConnectionInfo,
    IN     PCONNECTION_INFO_1 pInConnectionInfo
    );

VOID
SrvSvcSrvFreeConnectionInfo0(
    IN PCONNECTION_INFO_0 pConnectionInfo0
    );

VOID
SrvSvcSrvFreeConnectionInfo1(
    IN PCONNECTION_INFO_1 pConnectionInfo1
    );

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
