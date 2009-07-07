/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:
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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


NTSTATUS
NetrSamLogonInteractive(
    IN  handle_t              hNetrBinding,
    IN  NetrCredentials      *pCreds,
    IN  PCWSTR                pwszServer,
    IN  PCWSTR                pwszDomain,
    IN  PCWSTR                pwszComputer,
    IN  PCWSTR                pwszUsername,
    IN  PCWSTR                pwszPassword,
    IN  UINT16                LogonLevel,
    IN  UINT16                ValidationLevel,
    OUT NetrValidationInfo  **ppValidationInfo,
    OUT PBYTE                 pAuthoritative
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    wchar16_t *pwszServerName = NULL;
    wchar16_t *pwszComputerName = NULL;
    NetrAuth *pAuth = NULL;
    NetrAuth *pReturnedAuth = NULL;
    NetrLogonInfo *pLogonInfo = NULL;
    NetrValidationInfo ValidationInfo = {0};
    NetrValidationInfo *pValidationInfo = NULL;
    BYTE Authoritative = 0;

    BAIL_ON_INVALID_PTR(hNetrBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pCreds, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServer, ntStatus);
    BAIL_ON_INVALID_PTR(pwszDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pwszComputer, ntStatus);
    BAIL_ON_INVALID_PTR(pwszUsername, ntStatus);
    BAIL_ON_INVALID_PTR(pwszPassword, ntStatus);
    BAIL_ON_INVALID_PTR(ppValidationInfo, ntStatus);
    BAIL_ON_INVALID_PTR(pAuthoritative, ntStatus);

    if (!(LogonLevel == 1 ||
          LogonLevel == 3 ||
          LogonLevel == 5))
    {
        ntStatus = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = NetrAllocateUniString(&pwszServerName,
                                     pwszServer,
                                     NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateUniString(&pwszComputerName,
                                     pwszComputer,
                                     NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Create authenticator info with credentials chain */
    ntStatus = NetrAllocateMemory((void**)&pAuth,
                                  sizeof(NetrAuth),
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    pCreds->sequence += 2;
    NetrCredentialsCliStep(pCreds);

    pAuth->timestamp = pCreds->sequence;
    memcpy(pAuth->cred.data,
           pCreds->cli_chal.data,
           sizeof(pAuth->cred.data));

    /* Allocate returned authenticator */
    ntStatus = NetrAllocateMemory((void**)&pReturnedAuth,
                                  sizeof(NetrAuth),
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateLogonInfoHash(&pLogonInfo,
                                         LogonLevel,
                                         pwszDomain,
                                         pwszComputer,
                                         pwszUsername,
                                         pwszPassword);
    BAIL_ON_NT_STATUS(ntStatus);

    DCERPC_CALL(ntStatus, _NetrLogonSamLogon(hNetrBinding,
                                             pwszServerName,
                                             pwszComputerName,
                                             pAuth,
                                             pReturnedAuth,
                                             LogonLevel,
                                             pLogonInfo,
                                             ValidationLevel,
                                             &ValidationInfo,
                                             &Authoritative));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateValidationInfo(&pValidationInfo,
                                          &ValidationInfo,
                                          ValidationLevel);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppValidationInfo  = pValidationInfo;
    *pAuthoritative    = Authoritative;

cleanup:
    NetrCleanStubValidationInfo(&ValidationInfo,
                                ValidationLevel);

    if (pwszServerName) {
        NetrFreeMemory((void*)pwszServerName);
    }

    if (pwszComputerName) {
        NetrFreeMemory((void*)pwszComputerName);
    }

    if (pAuth) {
        NetrFreeMemory((void*)pAuth);
    }

    if (pLogonInfo) {
        NetrFreeMemory((void*)pLogonInfo);
    }

    return ntStatus;

error:
    if (pValidationInfo) {
        NetrFreeMemory((void*)pValidationInfo);
    }

    *ppValidationInfo  = NULL;
    *pAuthoritative    = 0;

    goto cleanup;
}


NTSTATUS
NetrSamLogonNetwork(
    IN  handle_t               hNetrBinding,
    IN  NetrCredentials       *pCreds,
    IN  PCWSTR                 pwszServer,
    IN  PCWSTR                 pwszDomain,
    IN  PCWSTR                 pwszComputer,
    IN  PCWSTR                 pwszUsername,
    IN  PBYTE                  pChallenge,
    IN  PBYTE                  pLmResp,
    IN  UINT32                 LmRespLen,
    IN  PBYTE                  pNtResp,
    IN  UINT32                 NtRespLen,
    IN  UINT16                 LogonLevel,
    IN  UINT16                 ValidationLevel,
    OUT NetrValidationInfo   **ppValidationInfo,
    OUT PBYTE                  pAuthoritative
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszServerName = NULL;
    PWSTR pwszComputerName = NULL;
    NetrAuth *pAuth = NULL;
    NetrAuth *pReturnedAuth = NULL;
    NetrLogonInfo *pLogonInfo = NULL;
    NetrValidationInfo ValidationInfo = {0};
    NetrValidationInfo *pValidationInfo = NULL;
    BYTE Authoritative = 0;

    BAIL_ON_INVALID_PTR(hNetrBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pCreds, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServer, ntStatus);
    BAIL_ON_INVALID_PTR(pwszDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pwszComputer, ntStatus);
    BAIL_ON_INVALID_PTR(pwszUsername, ntStatus);
    BAIL_ON_INVALID_PTR(pChallenge, ntStatus);
    /* LanMan Response could be NULL */
    BAIL_ON_INVALID_PTR(pNtResp, ntStatus);
    BAIL_ON_INVALID_PTR(ppValidationInfo, ntStatus);
    BAIL_ON_INVALID_PTR(pAuthoritative, ntStatus);

    if (!(LogonLevel == 2 ||
          LogonLevel == 6))
    {
        ntStatus = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = RtlWC16StringDuplicate(&pwszServerName,
                                      pwszServer);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlWC16StringDuplicate(&pwszComputerName,
                                      pwszComputer);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Create authenticator info with credentials chain */
    ntStatus = RTL_ALLOCATE((void**)&pAuth,
                            NetrAuth,
                            sizeof(NetrAuth));
    BAIL_ON_NT_STATUS(ntStatus);

    pCreds->sequence += 2;
    NetrCredentialsCliStep(pCreds);

    pAuth->timestamp = pCreds->sequence;
    memcpy(pAuth->cred.data,
           pCreds->cli_chal.data,
           sizeof(pAuth->cred.data));

    /* Allocate returned authenticator */
    ntStatus = RTL_ALLOCATE((void**)&pReturnedAuth,
                            NetrAuth,
                            sizeof(NetrAuth));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateLogonInfoNet(&pLogonInfo,
                                        LogonLevel,
                                        pwszDomain,
                                        pwszComputer,
                                        pwszUsername,
                                        pChallenge,
                                        pLmResp,
                                        LmRespLen,
                                        pNtResp,
                                        NtRespLen);
    BAIL_ON_NT_STATUS(ntStatus);

    DCERPC_CALL(ntStatus, _NetrLogonSamLogon(hNetrBinding,
                                             pwszServerName,
                                             pwszComputerName,
                                             pAuth,
                                             pReturnedAuth,
                                             LogonLevel,
                                             pLogonInfo,
                                             ValidationLevel,
                                             &ValidationInfo,
                                             &Authoritative));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateValidationInfo(&pValidationInfo,
                                          &ValidationInfo,
                                          ValidationLevel);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppValidationInfo  = pValidationInfo;
    *pAuthoritative    = Authoritative;

cleanup:
    NetrCleanStubValidationInfo(&ValidationInfo,
                                ValidationLevel);

    RtlWC16StringFree(&pwszServerName);
    RtlWC16StringFree(&pwszComputerName);
    RTL_FREE(&pAuth);
    RTL_FREE(&pReturnedAuth);

    if (pLogonInfo) {
        NetrFreeMemory((void*)pLogonInfo);
    }

    return ntStatus;

error:
    if (pValidationInfo) {
        NetrFreeMemory((void*)pValidationInfo);
    }

    *ppValidationInfo  = NULL;
    *pAuthoritative    = 0;

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
