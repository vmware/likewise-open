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
