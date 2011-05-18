/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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


/* 1.3.6.1.4.1.311.2.2.10 */
#define GSS_MECH_NTLM       "\x2b\x06\x01\x04\x01\x82\x37\x02\x02\x0a"
#define GSS_MECH_NTLM_LEN   10


static
NTSTATUS
SrvIoGetMappedToGuestFromGssContext(
    IN LW_MAP_SECURITY_GSS_CONTEXT hContextHandle,
    OUT PBOOLEAN pMappedToGuest
    );


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
    WCHAR wszBackslash[] = {'\\', 0};
    UNICODE_STRING backslash = RTL_CONSTANT_STRING(wszBackslash);
    IO_FILE_NAME fileName = { 0 };

    if (pFileName)
    {
        fileName = *pFileName;
    }

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
            ((GrantedAccess & FILE_ALL_ACCESS) == FILE_ALL_ACCESS))
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
        if (LwRtlUnicodeStringIsPrefix(&backslash, &fileName.Name, TRUE))
        {
            fileName.Name.Buffer++;
            fileName.Name.Length -= sizeof(fileName.Name.Buffer[0]);
            fileName.Name.MaximumLength -= sizeof(fileName.Name.Buffer[0]);
        }

        if (LwRtlUnicodeStringIsEqual(&backslash, &fileName.Name, TRUE))
        {
            fileName.Name.Buffer = NULL;
            fileName.Name.Length = 0;
            fileName.Name.MaximumLength = 0;
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
SrvIoSecCreateSecurityContextFromGssContext(
    OUT PIO_CREATE_SECURITY_CONTEXT* ppSecurityContext,
    OUT PBOOLEAN pMappedToGuest,
    IN LW_MAP_SECURITY_GSS_CONTEXT hContextHandle,
    IN PCSTR pszUsername
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UNICODE_STRING uniClientPrincipalName = {0};
    BOOLEAN mappedToGuest = FALSE;
    PIO_CREATE_SECURITY_CONTEXT pIoSecCreateCtx = NULL;

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

    ntStatus = SrvIoGetMappedToGuestFromGssContext(
                    hContextHandle,
                    &mappedToGuest);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LwRtlUnicodeStringFree(&uniClientPrincipalName);

    *pMappedToGuest = mappedToGuest;
    *ppSecurityContext = pIoSecCreateCtx;

    return ntStatus;

error:

    if (pIoSecCreateCtx)
    {
        IoSecurityDereferenceSecurityContext(&pIoSecCreateCtx);
        pIoSecCreateCtx = NULL;
    }

    mappedToGuest = FALSE;

    goto cleanup;
}

NTSTATUS
SrvIoSecCreateSecurityContextFromNtlmLogon(
    OUT PIO_CREATE_SECURITY_CONTEXT* ppSecurityContext,
    OUT PBOOLEAN pbLoggedInAsGuest,
    OUT PSTR* ppszUsername,
    OUT PVOID* ppSessionKey,
    OUT PULONG pulSessionKeyLength,
    IN PLW_MAP_SECURITY_NTLM_LOGON_INFO pNtlmLogonInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLW_MAP_SECURITY_NTLM_LOGON_RESULT pNtlmLogonResult = NULL;
    ULONG ulSessionKeySize = 0;

    PIO_CREATE_SECURITY_CONTEXT pSecurityContext = NULL;
    PSTR pszUsername = NULL;
    PVOID pSessionKey = NULL;

    // Create security context
    ntStatus = IoSecurityCreateSecurityContextFromNtlmLogon(
                &pSecurityContext,
                &pNtlmLogonResult,
                pNtlmLogonInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    // Session Key for raw NTLM is the user session key catenated with
    // the NT response blob

    ulSessionKeySize = NTLM_SESSION_KEY_SIZE + pNtlmLogonInfo->ulNtResponseSize;
    ntStatus = SrvAllocateMemory(ulSessionKeySize, &pSessionKey);
    BAIL_ON_NT_STATUS(ntStatus);

    RtlCopyMemory(
        pSessionKey,
        pNtlmLogonResult->SessionKey,
        NTLM_SESSION_KEY_SIZE);
    RtlCopyMemory(
        pSessionKey + NTLM_SESSION_KEY_SIZE,
        pNtlmLogonInfo->pNtResponse,
        pNtlmLogonInfo->ulNtResponseSize);


    // Username
    ntStatus = SrvAllocateString(pNtlmLogonResult->pszUsername, &pszUsername);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSecurityContext = pSecurityContext;
    *pbLoggedInAsGuest = pNtlmLogonResult->bMappedToGuest;
    *ppszUsername = pszUsername;
    *ppSessionKey = pSessionKey;
    *pulSessionKeyLength = ulSessionKeySize;

cleanup:

    if (pNtlmLogonResult)
    {
        IoSecurityFreeNtlmLogonResult(&pNtlmLogonResult);
    }

    return ntStatus;

error:

    IoSecurityDereferenceSecurityContext(&pSecurityContext);
    SRV_SAFE_FREE_MEMORY(pSessionKey);
    SRV_SAFE_FREE_MEMORY(pszUsername);

    goto cleanup;
}


static
NTSTATUS
SrvIoGetMappedToGuestFromGssContext(
    IN LW_MAP_SECURITY_GSS_CONTEXT hContextHandle,
    OUT PBOOLEAN pMappedToGuest
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    CHAR szMappedToGuestUrn[] = "urn:likewise:mapped-to-guest";
    OM_uint32 minorStatus = 0;
    OM_uint32 majorStatus = 0;
    gss_name_t srcName = GSS_C_NO_NAME;
    gss_OID mechType = { 0 };
    gss_buffer_desc mappedToGuestUrn =
    {
        .length = sizeof(szMappedToGuestUrn) - 1,
        .value = (PBYTE) szMappedToGuestUrn
    };
    gss_buffer_desc mappedToGuestData = { 0 };
    gss_buffer_desc displayData = { 0 };
    int more = -1;
    BOOLEAN mappedToGuest = FALSE;

    majorStatus = gss_inquire_context(
                    &minorStatus,
                    hContextHandle,
                    &srcName,
                    NULL,
                    NULL,
                    &mechType,
                    NULL,
                    NULL,
                    NULL);
    if (majorStatus != GSS_S_COMPLETE)
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // The mapped-to-guest attribute is valid only for NTLM
    if (mechType->length != GSS_MECH_NTLM_LEN ||
        memcmp(mechType->elements, GSS_MECH_NTLM, GSS_MECH_NTLM_LEN))
    {
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }

    majorStatus = gss_get_name_attribute(
                    &minorStatus,
                    srcName,
                    &mappedToGuestUrn,
                    NULL,
                    NULL,
                    &mappedToGuestData,
                    &displayData,
                    &more);
    if (majorStatus != GSS_S_COMPLETE)
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (mappedToGuestData.value == NULL ||
        mappedToGuestData.length != sizeof(BOOLEAN))
    {
        ntStatus = STATUS_INVALID_USER_BUFFER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    mappedToGuest = *((PBOOLEAN)mappedToGuestData.value);

cleanup:

    if (mappedToGuestData.value)
    {
        gss_release_buffer(&minorStatus, &mappedToGuestData);
    }

    if (displayData.value)
    {
        gss_release_buffer(&minorStatus, &displayData);
    }

    if (srcName)
    {
        gss_release_name(&minorStatus, &srcName);
    }

    *pMappedToGuest = mappedToGuest;

    return ntStatus;

error:

    mappedToGuest = FALSE;

    goto cleanup;
}
