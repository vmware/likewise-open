/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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


#include "includes.h"

NTSTATUS
SrvIoCreateFile(
    PSRV_SHARE_INFO               pShareInfo,              /* IN              */
    PIO_FILE_HANDLE               pFileHandle,             /*    OUT          */
    PIO_ASYNC_CONTROL_BLOCK       pAsyncControlBlock,      /* IN OUT OPTIONAL */
    PIO_STATUS_BLOCK              pIoStatusBlock,          /*    OUT          */
    PIO_CREATE_SECURITY_CONTEXT   pSecurityContext,        /* IN              */
    PIO_FILE_NAME                 pFileName,               /* IN              */
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,     /* IN     OPTIONAL */
    PVOID                         pSecurityQOS,            /* IN     OPTIONAL */
    ACCESS_MASK                   DesiredAccess,           /* IN              */
    LONG64                        AllocationSize,          /* IN     OPTIONAL */
    FILE_ATTRIBUTES               FileAttributes,          /* IN              */
    FILE_SHARE_FLAGS              ShareAccess,             /* IN              */
    FILE_CREATE_DISPOSITION       CreateDisposition,       /* IN              */
    FILE_CREATE_OPTIONS           CreateOptions,           /* IN              */
    PVOID                         pEaBuffer,               /* IN     OPTIONAL */
    ULONG                         EaLength,                /* IN              */
    PIO_ECP_LIST                  pEcpList                 /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PACCESS_TOKEN pAccessToken = NULL;
    ACCESS_MASK GrantedAccess = 0;
    ACCESS_MASK MappedDesiredAccess = 0;
    GENERIC_MAPPING GenericMap = {
        .GenericRead    = FILE_GENERIC_READ,
        .GenericWrite   = FILE_GENERIC_WRITE,
        .GenericExecute = FILE_GENERIC_EXECUTE,
        .GenericAll     = FILE_ALL_ACCESS };
    wchar16_t    wszBackslash[] = {'\\', 0};
    IO_FILE_NAME fileName =
    {
          .RootFileHandle = pFileName ? pFileName->RootFileHandle : NULL,
          .FileName       = pFileName ? pFileName->FileName       : NULL,
          .IoNameOptions  = pFileName ? pFileName->IoNameOptions  : 0
    };

    pAccessToken = IoSecurityGetAccessToken(pSecurityContext);
    if (pAccessToken == NULL)
    {
        ntStatus = STATUS_NO_TOKEN;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (    LwIsSetFlag(CreateOptions, FILE_RESERVE_OPFILTER) ||
            LwIsSetFlag(CreateOptions, FILE_OPEN_BY_FILE_ID) ||
            LwIsSetFlag(CreateOptions, FILE_CREATE_TREE_CONNECTION))
    {
        ntStatus = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Check against the Share Security Descriptor */

    ntStatus = SrvShareAccessCheck(
                   pShareInfo,
                   pAccessToken,
                   MAXIMUM_ALLOWED,
                   &GenericMap,
                   &GrantedAccess);
    BAIL_ON_NT_STATUS(ntStatus);

    if (GrantedAccess == 0)
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Special access requirements for specific CreateDispositions */

    switch (CreateDisposition)
    {
    case FILE_SUPERSEDE:
        if ((GrantedAccess & (FILE_WRITE_DATA|DELETE)) != (FILE_WRITE_DATA|DELETE))
        {
            ntStatus = STATUS_ACCESS_DENIED;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        break;

    case FILE_CREATE:
    case FILE_OVERWRITE:
    case FILE_OVERWRITE_IF:
    case FILE_OPEN_IF:
        if (!(GrantedAccess & FILE_WRITE_DATA))
        {
            ntStatus = STATUS_ACCESS_DENIED;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        break;
    case FILE_OPEN:
        break;
    }

    /* Is the caller asking for more bits than we allow?  */

    MappedDesiredAccess = DesiredAccess;
    RtlMapGenericMask(&MappedDesiredAccess, &GenericMap);

    if ((GrantedAccess & MappedDesiredAccess) != MappedDesiredAccess)
    {
        /* Escape clause for MAXIMUM_ALLOWED */

        if ((MappedDesiredAccess & MAXIMUM_ALLOWED) &&
            (GrantedAccess == FILE_ALL_ACCESS))
        {
            ntStatus = STATUS_SUCCESS;
        }
        else
        {
            ntStatus = STATUS_ACCESS_DENIED;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    if (fileName.RootFileHandle)
    {
       if (!IsNullOrEmptyString(fileName.FileName) &&
           (fileName.FileName[0] == wszBackslash[0]))
       {
           fileName.FileName++;
       }

       if (IsNullOrEmptyString(fileName.FileName) ||
           !SMBWc16sCmp(fileName.FileName, &wszBackslash[0]))
        {
            fileName.FileName = NULL;
        }
    }

    /* Do the open */

    ntStatus = IoCreateFile(
                   pFileHandle,
                   pAsyncControlBlock,
                   pIoStatusBlock,
                   pSecurityContext,
                   &fileName,
                   pSecurityDescriptor,
                   pSecurityQOS,
                   DesiredAccess,
                   AllocationSize,
                   FileAttributes,
                   ShareAccess,
                   CreateDisposition,
                   CreateOptions,
                   pEaBuffer,
                   EaLength,
                   pEcpList);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}


NTSTATUS
SrvIoPrepareAbeEcpList(
    PIO_ECP_LIST pEcpList    /* IN */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBOOLEAN pEnableAbe = NULL;

    ntStatus = LW_RTL_ALLOCATE(&pEnableAbe, BOOLEAN, sizeof(BOOLEAN));
    BAIL_ON_NT_STATUS(ntStatus);

    *pEnableAbe = TRUE;

    ntStatus = IoRtlEcpListInsert(
                   pEcpList,
                   SRV_ECP_TYPE_ABE,
                   pEnableAbe,
                   sizeof(BOOLEAN),
                   LwRtlMemoryFree);
    if (ntStatus == STATUS_OBJECT_NAME_EXISTS)
    {
        ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    if (pEnableAbe)
    {
        LW_RTL_FREE(&pEnableAbe);
    }

    goto cleanup;
}


NTSTATUS
SrvIoSecCreateSecurityContext(
    OUT PIO_CREATE_SECURITY_CONTEXT* ppSecurityContext,
    OUT PBOOLEAN pbLoggedInAsGuest,
    IN LW_MAP_SECURITY_GSS_CONTEXT hContextHandle,
    IN PCSTR pszUsername
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UNICODE_STRING uniClientPrincipalName = {0};
    BOOLEAN bLoggedInAsGuest = FALSE;
    PACCESS_TOKEN pToken = NULL;
    PIO_CREATE_SECURITY_CONTEXT pIoSecCreateCtx = NULL;
    union {
        TOKEN_OWNER TokenOwnerInfo;
        BYTE Buffer[SID_MAX_SIZE];
    } TokenOwnerBuffer;
    PTOKEN_OWNER pTokenOwnerInformation = (PTOKEN_OWNER)&TokenOwnerBuffer;
    ULONG ulTokenOwnerLength = 0;
    ULONG ulRid = 0;

    // Generate and store the IoSecurityContext.  Fallback to creating
    // one from the username if the GssContext based call fails

    ntStatus = IoSecurityCreateSecurityContextFromGssContext(
                   &pIoSecCreateCtx,
                   hContextHandle);
    if (ntStatus == STATUS_BAD_LOGON_SESSION_STATE)
    {
        ntStatus = LwRtlUnicodeStringAllocateFromCString(
                       &uniClientPrincipalName,
                       pszUsername);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = IoSecurityCreateSecurityContextFromUsername(
                       &pIoSecCreateCtx,
                       &uniClientPrincipalName);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pToken = IoSecurityGetAccessToken(pIoSecCreateCtx);

    if (!pToken)
    {
        ntStatus = STATUS_NO_TOKEN;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = RtlQueryAccessTokenInformation(
                   pToken,
                   TokenOwner,
                   (PVOID)pTokenOwnerInformation,
                   sizeof(TokenOwnerBuffer),
                   &ulTokenOwnerLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlGetRidSid(&ulRid, pTokenOwnerInformation->Owner);
    BAIL_ON_NT_STATUS(ntStatus);

    if (ulRid == DOMAIN_USER_RID_GUEST)
    {
        bLoggedInAsGuest = TRUE;
    }

    *pbLoggedInAsGuest = bLoggedInAsGuest;
    *ppSecurityContext = pIoSecCreateCtx;

cleanup:

    LwRtlUnicodeStringFree(&uniClientPrincipalName);

    return ntStatus;

error:

    if (pIoSecCreateCtx)
    {
        IoSecurityDereferenceSecurityContext(&pIoSecCreateCtx);
        pIoSecCreateCtx = NULL;
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

