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

static
NTSTATUS
SrvShareCreateAbsoluteSecDescFromRel(
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppAbsSecDesc,
    IN   PSECURITY_DESCRIPTOR_RELATIVE pRelSecDesc
    );

NTSTATUS
SrvGetShareName(
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
    IN  PSECURITY_DESCRIPTOR_RELATIVE pIncRelSecDesc,
    IN  ULONG ulIncRelSecDescLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pIncAbsSecDesc = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pFinalAbsSecDesc = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pFinalRelSecDesc = NULL;
    ULONG ulFinalRelSecDescLen = 0;
    SECURITY_INFORMATION secInfo = 0;
    PSID pOwner = NULL;
    PSID pGroup = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    BOOLEAN bDefaulted = FALSE;
    BOOLEAN bPresent = FALSE;
    GENERIC_MAPPING GenericMap = {
        .GenericRead    = FILE_GENERIC_READ,
        .GenericWrite   = FILE_GENERIC_WRITE,
        .GenericExecute = FILE_GENERIC_EXECUTE,
        .GenericAll     = FILE_ALL_ACCESS };

    /* Sanity checks */

    if ((pIncRelSecDesc == NULL) || (ulIncRelSecDescLen == 0))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pShareInfo->ulSecDescLen == 0)
    {
        ntStatus = SrvShareSetDefaultSecurity(pShareInfo);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Make the Absolute version of thr incoming SD and
       get the SecurityInformation */

    ntStatus = SrvShareCreateAbsoluteSecDescFromRel(
                    &pIncAbsSecDesc,
                    pIncRelSecDesc) ;
    BAIL_ON_NT_STATUS(ntStatus);

    /* Don't bail on these.  We'll be checking the pointer */

    ntStatus = RtlGetOwnerSecurityDescriptor(
                   pIncAbsSecDesc,
                   &pOwner,
                   &bDefaulted);
    secInfo |= pOwner ? OWNER_SECURITY_INFORMATION : 0;

    ntStatus = RtlGetGroupSecurityDescriptor(
                   pIncAbsSecDesc,
                   &pGroup,
                   &bDefaulted);
    secInfo |= pGroup ? GROUP_SECURITY_INFORMATION : 0;

    ntStatus = RtlGetDaclSecurityDescriptor(
                   pIncAbsSecDesc,
                   &bPresent,
                   &pDacl,
                   &bDefaulted);
    secInfo |= pDacl ? DACL_SECURITY_INFORMATION : 0;

    ntStatus = RtlGetSaclSecurityDescriptor(
                   pIncAbsSecDesc,
                   &bPresent,
                   &pSacl,
                   &bDefaulted);
    secInfo |= pSacl ? SACL_SECURITY_INFORMATION  : 0;

    /* Assume the new length is not longer than the combined length
       of both the current and incoming relative SecDesc buffers */

    ulFinalRelSecDescLen = ulIncRelSecDescLen + pShareInfo->ulSecDescLen;

    ntStatus = SrvAllocateMemory(
                   ulFinalRelSecDescLen,
                   (PVOID*)&pFinalRelSecDesc);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlSetSecurityDescriptorInfo(
                   secInfo,
                   pIncRelSecDesc,
                   pShareInfo->pSecDesc,
                   pFinalRelSecDesc,
                   &ulFinalRelSecDescLen,
                   &GenericMap);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareCreateAbsoluteSecDescFromRel(
                    &pFinalAbsSecDesc,
                    pFinalRelSecDesc) ;
    BAIL_ON_NT_STATUS(ntStatus);

    /* Free the old SecDesc and save the new one */

    SrvShareFreeSecurity(pShareInfo);

    pShareInfo->pSecDesc = pFinalRelSecDesc;
    pShareInfo->ulSecDescLen = ulFinalRelSecDescLen;
    pShareInfo->pAbsSecDesc = pFinalAbsSecDesc;

    ntStatus = STATUS_SUCCESS;

cleanup:
    if (pIncAbsSecDesc)
    {
        SrvShareFreeAbsoluteSecurityDescriptor(&pIncAbsSecDesc);
    }

    return ntStatus;

error:
    if (pFinalRelSecDesc)
    {
        SrvFreeMemory(pFinalRelSecDesc);
    }

    if (pFinalAbsSecDesc)
    {
        SrvShareFreeAbsoluteSecurityDescriptor(&pFinalAbsSecDesc);
    }

    goto cleanup;
}

static
NTSTATUS
SrvShareCreateAbsoluteSecDescFromRel(
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppAbsSecDesc,
    IN   PSECURITY_DESCRIPTOR_RELATIVE pRelSecDesc
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
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

    ntStatus = RTL_ALLOCATE(&pAbsSecDesc, VOID, ulAbsSecDescLen);
    BAIL_ON_NT_STATUS(ntStatus);

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

    *ppAbsSecDesc = pAbsSecDesc;

cleanup:
    return ntStatus;


error:
    LW_RTL_FREE(&pAbsSecDesc);
    LW_RTL_FREE(&pOwner);
    LW_RTL_FREE(&pGroup);
    LW_RTL_FREE(&pSacl);
    LW_RTL_FREE(&pDacl);

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
    NTSTATUS ntStatus = STATUS_SUCCESS;
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

    ntStatus = RtlGetOwnerSecurityDescriptor(pSecDesc, &pOwner, &bDefaulted);
    ntStatus = RtlGetGroupSecurityDescriptor(pSecDesc, &pGroup, &bDefaulted);

    ntStatus = RtlGetDaclSecurityDescriptor(pSecDesc, &bPresent, &pDacl, &bDefaulted);
    ntStatus = RtlGetSaclSecurityDescriptor(pSecDesc, &bPresent, &pSacl, &bDefaulted);

    RTL_FREE(&pSecDesc);
    RTL_FREE(&pOwner);
    RTL_FREE(&pGroup);
    RTL_FREE(&pDacl);
    RTL_FREE(&pSacl);

    *ppSecDesc = NULL;

    return;
}


NTSTATUS
SrvShareAccessCheck(
    PSRV_SHARE_INFO pShareInfo,
    PACCESS_TOKEN pToken,
    ACCESS_MASK DesiredAccess,
    PGENERIC_MAPPING pGenericMap,
    PACCESS_MASK pGrantedAccess
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bShareInLock = FALSE;
    BOOLEAN bAccessResult = FALSE;

    if (!pGrantedAccess || !pToken)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_SHARED(bShareInLock, &pShareInfo->mutex);

    if (!pShareInfo->pAbsSecDesc)
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    bAccessResult = RtlAccessCheck(
                        pShareInfo->pAbsSecDesc,
                        pToken,
                        DesiredAccess,
                        0,
                        pGenericMap,
                        pGrantedAccess,
                        &ntStatus);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!bAccessResult)
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    LWIO_UNLOCK_RWMUTEX(bShareInLock, &pShareInfo->mutex);

    return ntStatus;

error:

    goto cleanup;
}


NTSTATUS
SrvShareSetDefaultSecurity(
    PSRV_SHARE_INFO pShareInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_RELATIVE pRelSecDesc = NULL;
    ULONG ulRelSecDescLen = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsSecDesc = NULL;
    DWORD dwAceCount = 0;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;
    DWORD dwSizeDacl = 0;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } administratorsSid;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } powerUsersSid;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } everyoneSid;
    ULONG ulAdministratorsSidSize = sizeof(administratorsSid.buffer);
    ULONG ulPowerUsersSidSize = sizeof(powerUsersSid.buffer);
    ULONG ulEveryoneSidSize = sizeof(everyoneSid.buffer);
    ACCESS_MASK worldAccessMask = 0;

    /* Clear out any existing SecDesc's.  This is not a normal
       use case, but be paranoid */

    if (pShareInfo->ulSecDescLen)
    {
        SrvShareFreeSecurity(pShareInfo);
    }

    /* Build the new Absolute Security Descriptor */

    ntStatus = RTL_ALLOCATE(
                   &pAbsSecDesc,
                   VOID,
                   SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlCreateSecurityDescriptorAbsolute(
                  pAbsSecDesc,
                  SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Create some SIDs */

    ntStatus = RtlCreateWellKnownSid(
                   WinBuiltinAdministratorsSid,
                   NULL,
                   (PSID)administratorsSid.buffer,
                   &ulAdministratorsSidSize);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlCreateWellKnownSid(
                   WinBuiltinPowerUsersSid,
                   NULL,
                   (PSID)powerUsersSid.buffer,
                   &ulPowerUsersSidSize);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlCreateWellKnownSid(
                   WinWorldSid,
                   NULL,
                   (PSID)everyoneSid.buffer,
                   &ulEveryoneSidSize);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Owner: Administrators */

    ntStatus = RtlDuplicateSid(&pOwnerSid, &administratorsSid.sid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlSetOwnerSecurityDescriptor(
                   pAbsSecDesc,
                   pOwnerSid,
                   FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Group: Power Users */

    ntStatus = RtlDuplicateSid(&pGroupSid, &powerUsersSid.sid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlSetGroupSecurityDescriptor(
                   pAbsSecDesc,
                   pGroupSid,
                   FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /* DACL:
       Administrators - (Full Control)
       Everyone - (Read) for disk shares, (Read/Write) to IPC shares */

    dwAceCount = 2;

    dwSizeDacl = ACL_HEADER_SIZE +
        dwAceCount * sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(&administratorsSid.sid) +
        RtlLengthSid(&everyoneSid.sid) -
        dwAceCount * sizeof(ULONG);

    ntStatus= RTL_ALLOCATE(&pDacl, VOID, dwSizeDacl);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlCreateAcl(pDacl, dwSizeDacl, ACL_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlAddAccessAllowedAceEx(
                  pDacl,
                  ACL_REVISION,
                  0,
                  FILE_ALL_ACCESS,
                  &administratorsSid.sid);
    BAIL_ON_NT_STATUS(ntStatus);

    worldAccessMask = FILE_GENERIC_READ | FILE_GENERIC_EXECUTE;
    if (pShareInfo->service == SHARE_SERVICE_NAMED_PIPE)
    {
        worldAccessMask |= FILE_GENERIC_WRITE;
    }

    ntStatus = RtlAddAccessAllowedAceEx(
                  pDacl,
                  ACL_REVISION,
                  0,
                  worldAccessMask,
                  &everyoneSid.sid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlSetDaclSecurityDescriptor(
                  pAbsSecDesc,
                  TRUE,
                  pDacl,
                  FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Create the SelfRelative SD and assign them to the Share */

    ntStatus = RtlAbsoluteToSelfRelativeSD(
                   pAbsSecDesc,
                   NULL,
                   &ulRelSecDescLen);
    if (ntStatus == STATUS_BUFFER_TOO_SMALL)
    {
        ntStatus = SrvAllocateMemory(ulRelSecDescLen, (PVOID*)&pRelSecDesc);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RtlAbsoluteToSelfRelativeSD(
                       pAbsSecDesc,
                       pRelSecDesc,
                       &ulRelSecDescLen);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pShareInfo->pSecDesc = pRelSecDesc;
    pShareInfo->ulSecDescLen = ulRelSecDescLen;
    pShareInfo->pAbsSecDesc = pAbsSecDesc;

    ntStatus = STATUS_SUCCESS;


cleanup:

    return ntStatus;

error:
    RTL_FREE(&pAbsSecDesc);
    RTL_FREE(&pOwnerSid);
    RTL_FREE(&pGroupSid);
    RTL_FREE(&pDacl);

    if (pRelSecDesc)
    {
        SrvFreeMemory(pRelSecDesc);
    }

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
