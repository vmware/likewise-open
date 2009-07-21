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

#include "includes.h"


WINERR
DsrGetDcName(
    IN  handle_t hNetrBinding,
    IN  PCWSTR pwszServerName,
    IN  PCWSTR pwszDomainName,
    IN  const Guid *pDomainGuid,
    IN  const Guid *pSiteGuid,
    IN  UINT32 GetDcFlags,
    OUT DsrDcNameInfo **ppInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    PWSTR pwszServer = NULL;
    PWSTR pwszDomain = NULL;
    Guid *pDomainGuidCopy = NULL;
    Guid *pSiteGuidCopy = NULL;
    DsrDcNameInfo *pDcInfo = NULL;
    DsrDcNameInfo *pDcRetInfo = NULL;

    BAIL_ON_INVALID_PTR(hNetrBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServerName, ntStatus);
    BAIL_ON_INVALID_PTR(pwszDomainName, ntStatus);
    BAIL_ON_INVALID_PTR(ppInfo, ntStatus);

    pwszServer = wc16sdup(pwszServerName);
    BAIL_ON_NULL_PTR(pwszServer, ntStatus);

    pwszDomain = wc16sdup(pwszDomainName);
    BAIL_ON_NULL_PTR(pwszDomain, ntStatus);

    if (pDomainGuid) {
        ntStatus = NetrAllocateMemory((void**)&pDomainGuidCopy,
                                      sizeof(*pDomainGuidCopy),
                                      NULL);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pDomainGuidCopy,
               pDomainGuid,
               sizeof(*pDomainGuidCopy));
    }

    if (pSiteGuid) {
        ntStatus = NetrAllocateMemory((void**)&pSiteGuidCopy,
                                      sizeof(*pSiteGuidCopy),
                                      NULL);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pSiteGuidCopy,
               pSiteGuid,
               sizeof(*pSiteGuid));
    }

    DCERPC_CALL(err, _DsrGetDcName(hNetrBinding,
                                   pwszServer,
                                   pwszDomain,
                                   pDomainGuidCopy,
                                   pSiteGuidCopy,
                                   GetDcFlags,
                                   &pDcInfo));
    if (err) goto error;

    ntStatus = NetrAllocateDcNameInfo(&pDcRetInfo,
                                      pDcInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppInfo = pDcRetInfo;

cleanup:
    SAFE_FREE(pwszServer);
    SAFE_FREE(pwszDomain);

    if (pDomainGuidCopy) {
        NetrFreeMemory(pDomainGuidCopy);
    }

    if (pSiteGuidCopy) {
        NetrFreeMemory(pSiteGuidCopy);
    }

    if (pDcInfo) {
        NetrFreeStubDcNameInfo(pDcInfo);
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(ntStatus);
    }

    return err;

error:
    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(ntStatus);
    }

    if (pDcRetInfo) {
        NetrFreeMemory(pDcRetInfo);
    }

    *ppInfo = NULL;

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
