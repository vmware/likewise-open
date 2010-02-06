/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        utils.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Server share utilities
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
VOID
SrvShareFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    );

NTSTATUS
SrvGetShareName(
    IN  PCSTR  pszHostname,
    IN  PCSTR  pszDomain,
    IN  PWSTR  pwszPath,
    OUT PWSTR* ppwszSharename
    )
{
    NTSTATUS ntStatus = 0;
    PWSTR    pwszSharename = NULL;
    PSTR     pszPath = NULL;
    PSTR     pszShareName = NULL;
    PSTR     pszCursor = NULL;

    ntStatus = SrvWc16sToMbs(pwszPath, &pszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    pszCursor = pszPath;

    /* Skip a leading pair of backslashes */

    if ((strlen(pszCursor) > 2) &&
        (*pszCursor == '\\')    &&
        (*(pszCursor+1) == '\\'))
    {
        pszCursor += 2;
    }

    pszShareName = strchr(pszCursor, '\\');
    if (pszShareName == NULL)
    {
        pszShareName = pszCursor;
    }
    else
    {
        pszShareName++;
    }

    if (*pszShareName == '\0')
    {
        ntStatus = STATUS_BAD_NETWORK_PATH;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvMbsToWc16s(pszShareName, &pwszSharename);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppwszSharename = pwszSharename;

cleanup:

    if (pszPath)
    {
        SrvFreeMemory(pszPath);
    }

    return ntStatus;

error:

    *ppwszSharename = NULL;

    goto cleanup;
}

NTSTATUS
SrvGetMaximalShareAccessMask(
    PSRV_SHARE_INFO pShareInfo,
    ACCESS_MASK*   pMask
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    ACCESS_MASK mask = 0;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareInfo->mutex);

    switch (pShareInfo->service)
    {
        case SHARE_SERVICE_NAMED_PIPE:

            mask = (FILE_READ_DATA |
                    FILE_WRITE_DATA |
                    FILE_APPEND_DATA |
                    FILE_READ_EA |
                    FILE_WRITE_EA |
                    FILE_EXECUTE |
                    FILE_DELETE_CHILD |
                    FILE_READ_ATTRIBUTES |
                    FILE_WRITE_ATTRIBUTES);

            break;

        case SHARE_SERVICE_DISK_SHARE:

            mask = 0x1FF;

            break;

        case SHARE_SERVICE_PRINTER:
        case SHARE_SERVICE_COMM_DEVICE:
        case SHARE_SERVICE_ANY:

            mask = GENERIC_READ;

            break;

        default:

            mask = 0;

            break;
    }

    *pMask = mask;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    return ntStatus;
}

NTSTATUS
SrvGetGuestShareAccessMask(
    PSRV_SHARE_INFO pShareInfo,
    ACCESS_MASK*   pMask
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    ACCESS_MASK mask = 0;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareInfo->mutex);

    switch (pShareInfo->service)
    {
        case SHARE_SERVICE_NAMED_PIPE:

            mask = (FILE_READ_DATA |
                    FILE_WRITE_DATA |
                    FILE_APPEND_DATA |
                    FILE_READ_EA |
                    FILE_WRITE_EA |
                    FILE_EXECUTE |
                    FILE_DELETE_CHILD |
                    FILE_READ_ATTRIBUTES |
                    FILE_WRITE_ATTRIBUTES);

            break;

        case SHARE_SERVICE_DISK_SHARE:

            mask = 0x1FF;

            break;

        case SHARE_SERVICE_PRINTER:
        case SHARE_SERVICE_COMM_DEVICE:
        case SHARE_SERVICE_ANY:

            mask = 0;

            break;

        default:

            mask = 0;

            break;
    }

    *pMask = mask;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    return ntStatus;
}

VOID
SrvShareFreeSecurity(
    IN PSRV_SHARE_INFO pShareInfo
    )
{
    if (pShareInfo->pSecDesc)
    {
        SrvFreeMemory(pShareInfo->pSecDesc);
        pShareInfo->pSecDesc = NULL;
        pShareInfo->ulSecDescLen = 0;
    }

    if (pShareInfo->pAbsSecDesc)
    {
        SrvShareFreeAbsoluteSecurityDescriptor(&pShareInfo->pAbsSecDesc);
        pShareInfo->pAbsSecDesc = NULL;
    }
}


NTSTATUS
SrvShareSetSecurity(
    IN  PSRV_SHARE_INFO pShareInfo,
    IN  PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    IN  ULONG ulSecDescLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_RELATIVE pRelSecDesc = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsSecDesc = NULL;
    ULONG ulAbsSecDescLen = 0;
    PACL pDacl = NULL;
    ULONG ulDaclLen = 0;
    PACL pSacl = NULL;
    ULONG ulSaclLen = 0;
    PSID pOwner = NULL;
    ULONG ulOwnerLen = 0;
    PSID pGroup = NULL;
    ULONG ulGroupLen = 0;

    if ((pSecDesc == NULL) || (ulSecDescLen == 0))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Allocate space for the Relative SD */

    ntStatus = SrvAllocateMemory(ulSecDescLen, (PVOID*)&pRelSecDesc);
    BAIL_ON_NT_STATUS(ntStatus);

    LwRtlCopyMemory(pRelSecDesc, pSecDesc, ulSecDescLen);

    /* Get sizes for the Absolute SD */

    ntStatus = RtlSelfRelativeToAbsoluteSD(
                   pRelSecDesc,
                   pAbsSecDesc,
                   &ulAbsSecDescLen,
                   pDacl,
                   &ulDaclLen,
                   pSacl,
                   &ulSaclLen,
                   pOwner,
                   &ulOwnerLen,
                   pGroup,
                   &ulGroupLen);
    if (ntStatus == STATUS_BUFFER_TOO_SMALL)
    {
        ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    /* Allocate -- Always use RTL routines for Absolute SDs */

    ntStatus = RTL_ALLOCATE(&pAbsSecDesc, VOID, ulSecDescLen);
    BAIL_ON_NT_STATUS(ntStatus);

    if (ulOwnerLen)
    {
        ntStatus = RTL_ALLOCATE(&pOwner, SID, ulOwnerLen);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ulGroupLen)
    {
        ntStatus = RTL_ALLOCATE(&pGroup, SID, ulGroupLen);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ulDaclLen)
    {
        ntStatus = RTL_ALLOCATE(&pDacl, VOID, ulDaclLen);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ulSaclLen)
    {
        ntStatus = RTL_ALLOCATE(&pSacl, VOID, ulSaclLen);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Translate the SD */

    ntStatus = RtlSelfRelativeToAbsoluteSD(
                   pRelSecDesc,
                   pAbsSecDesc,
                   &ulAbsSecDescLen,
                   pDacl,
                   &ulDaclLen,
                   pSacl,
                   &ulSaclLen,
                   pOwner,
                   &ulOwnerLen,
                   pGroup,
                   &ulGroupLen);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Free the old SD and save the poiter to the new one */

    SrvShareFreeSecurity(pShareInfo);

    pShareInfo->pSecDesc = pRelSecDesc;
    pShareInfo->ulSecDescLen = ulSecDescLen;
    pShareInfo->pAbsSecDesc = pAbsSecDesc;

    ntStatus = STATUS_SUCCESS;

cleanup:

    return ntStatus;

error:
    if (pRelSecDesc)
    {
        SrvFreeMemory(pRelSecDesc);
    }

    if (pAbsSecDesc)
    {
        SrvShareFreeAbsoluteSecurityDescriptor(&pAbsSecDesc);
    }

    goto cleanup;
}


/**
 * ATTN: Always use RTL routines when allocating memory for
 * PSECURITY_DESCRIPTOR_ABSOLUTE.  Else the Free() here will not
 * be symmetic.
 **/

static
VOID
SrvShareFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSID pOwner = NULL;
    PSID pGroup = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    BOOLEAN bDefaulted = FALSE;
    BOOLEAN bPresent = FALSE;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;

    if ((ppSecDesc == NULL) || (*ppSecDesc == NULL))
    {
        return;
    }

    pSecDesc = *ppSecDesc;

    ntError = RtlGetOwnerSecurityDescriptor(pSecDesc, &pOwner, &bDefaulted);
    ntError = RtlGetGroupSecurityDescriptor(pSecDesc, &pGroup, &bDefaulted);

    ntError = RtlGetDaclSecurityDescriptor(pSecDesc, &bPresent, &pDacl, &bDefaulted);
    ntError = RtlGetSaclSecurityDescriptor(pSecDesc, &bPresent, &pSacl, &bDefaulted);

    RTL_FREE(&pSecDesc);
    RTL_FREE(&pOwner);
    RTL_FREE(&pGroup);
    RTL_FREE(&pDacl);
    RTL_FREE(&pSacl);

    *ppSecDesc = NULL;

    return;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
