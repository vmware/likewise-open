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

#define BLOCK_SIZE 4096
#define TIME_SEC_CONVERSION_CONSTANT 11644473600LL

typedef struct
{
    SID base;
    BYTE data[SID_MAX_SIZE-SID_MIN_SIZE];
} SIDMAX;

/* Private prototypes */
static
NTSTATUS
LwIoFuseTranslateAbsoluteSecurityDescriptor(
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    PSID pOwnerSid,
    PSID pGroupSid,
    struct stat* pStatbuf
    );

static
NTSTATUS
LwIoWinToUnixTime(
    time_t *pUnixTime,
    LONG64 WinTime
    );

static
NTSTATUS
LwIoGetNtNameForSid(
    PCSTR pszSidStr,
    PSTR* ppszNtName
    );

static
NTSTATUS
LwIoGetUidForNtName(
    PCSTR pszNtName,
    uid_t* pUid
    );

static
NTSTATUS
LwIoGetGidForNtName(
    PCSTR pszNtName,
    gid_t* pGid
    );

static
NTSTATUS
LwIoFuseTranslateSecurityDescriptorPermissions(
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    PACCESS_TOKEN pOwnerToken,
    PACCESS_TOKEN pGroupToken,
    PACCESS_TOKEN pOtherToken,
    struct stat* pStatbuf
    );

static
NTSTATUS
LwIoFuseTranslateAccess(
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    PACCESS_TOKEN pAccessToken,
    mode_t readMode,
    mode_t writeMode,
    mode_t executeMode,
    mode_t* outMode
    );

static
NTSTATUS
LwIoFuseCreateAccessTokenForOwner(
    PSID pOwnerSid,
    PACCESS_TOKEN* ppOwnerToken
    );

static
NTSTATUS
LwIoFuseCreateAccessTokenForGroup(
    PSID pGroupSid,
    PACCESS_TOKEN* ppGroupToken
    );

static
NTSTATUS
LwIoFuseCreateAccessTokenForOther(
    PACCESS_TOKEN* ppOtherToken
    );

/* Definitions */
PIO_FUSE_CONTEXT
LwIoFuseGetContext(
    void
    )
{
    struct fuse_context* pContext = fuse_get_context();

    return (PIO_FUSE_CONTEXT) pContext->private_data;
}

void
LwIoFuseGetCallerIdentity(
    uid_t *pUid,
    gid_t *pGid,
    pid_t *pPid
    )
{ 
    struct fuse_context* pContext = fuse_get_context();
    
    if (pUid)
    {
        *pUid = pContext->uid;
    }

    if (pGid)
    {
        *pGid = pContext->gid;
    }

    if (pPid)
    {
        *pPid = pContext->pid;
    }
}

NTSTATUS
LwIoFuseGetNtFilename(
    PIO_FUSE_CONTEXT pFuseContext,
    PCSTR pszPath,
    PIO_FILE_NAME pFilename
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszFullPath = NULL;

    memset(pFilename, 0, sizeof(*pFilename));

    status = LwRtlCStringAllocatePrintf(
        &pszFullPath,
        "/%s/%s/%s%s",
        pFuseContext->pszDriver,
        pFuseContext->pszServer,
        pFuseContext->pszShare,
        pszPath);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringAllocateFromCString(
        &pFilename->FileName,
        pszFullPath);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RTL_FREE(&pszFullPath);

    return status;

error:

    RTL_FREE(&pFilename->FileName);
 
    memset(pFilename, 0, sizeof(*pFilename));

    goto cleanup;
}

NTSTATUS
LwIoFuseGetDriverRelativePath(
    PIO_FUSE_CONTEXT pFuseContext,
    PCSTR pszPath,
    PWSTR* ppwszRelativePath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszRelativePath = NULL;
    
    status = LwRtlCStringAllocatePrintf(
        &pszRelativePath,
        "/%s/%s%s",
        pFuseContext->pszServer,
        pFuseContext->pszShare,
        pszPath);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringAllocateFromCString(
        ppwszRelativePath,
        pszRelativePath);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RTL_FREE(&pszRelativePath);

    return status;

error:

    *ppwszRelativePath = NULL;

    goto cleanup;
}

NTSTATUS
LwIoFuseTranslateBasicInformation(
    PFILE_BASIC_INFORMATION pInfo,
    struct stat* pStatbuf
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    /* Set basic file type */
    if (pInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        pStatbuf->st_mode |= S_IFDIR;
    }
    else
    {
        pStatbuf->st_mode |= S_IFREG;
    }

    status = LwIoWinToUnixTime(
        &pStatbuf->st_atime,
        pInfo->LastAccessTime);
    BAIL_ON_NT_STATUS(status);

    status = LwIoWinToUnixTime(
        &pStatbuf->st_mtime,
        pInfo->LastWriteTime);
    BAIL_ON_NT_STATUS(status);

    status = LwIoWinToUnixTime(
        &pStatbuf->st_ctime,
        pInfo->ChangeTime);
    BAIL_ON_NT_STATUS(status);

error:

    return status;
}

NTSTATUS
LwIoFuseTranslateStandardInformation(
    PFILE_STANDARD_INFORMATION pInfo,
    struct stat* pStatbuf
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    pStatbuf->st_size = (off_t) pInfo->EndOfFile;
    pStatbuf->st_nlink = (nlink_t) pInfo->NumberOfLinks;
    pStatbuf->st_blksize = (blksize_t) BLOCK_SIZE;
    pStatbuf->st_blocks = (blkcnt_t) pInfo->AllocationSize / BLOCK_SIZE;

    return status;
}

NTSTATUS
LwIoFuseTranslateSecurityDescriptor(
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    struct stat* pStatbuf
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsolute = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    ULONG ulOwnerSidSize = 0;
    ULONG ulGroupSidSize = 0;
    ULONG ulDaclSize = 0;
    ULONG ulSaclSize = 0;
    ULONG ulAbsSize = 0;

    /* Get required sizes */
    RtlSelfRelativeToAbsoluteSD(
        pSecurityDescriptor,
        pAbsolute,
        &ulAbsSize,
        pDacl,
        &ulDaclSize,
        pSacl,
        &ulSaclSize,
        pOwnerSid,
        &ulOwnerSidSize,
        pGroupSid,
        &ulGroupSidSize);
    
    /* Allocate space */
    status = RTL_ALLOCATE(&pAbsolute, struct _SECURITY_DESCRIPTOR_ABSOLUTE, ulAbsSize);
    BAIL_ON_NT_STATUS(status);

    if (ulDaclSize)
    {
        status = RTL_ALLOCATE(&pDacl, struct _ACL, ulDaclSize);
        BAIL_ON_NT_STATUS(status);
    }

    if (ulSaclSize)
    {
        status = RTL_ALLOCATE(&pSacl, struct _ACL, ulSaclSize);
        BAIL_ON_NT_STATUS(status);
    }

    if (ulOwnerSidSize)
    {
        status = RTL_ALLOCATE(&pOwnerSid, SID, ulOwnerSidSize);
        BAIL_ON_NT_STATUS(status);
    }

    if (ulGroupSidSize)
    {
        status = RTL_ALLOCATE(&pGroupSid, SID, ulGroupSidSize);
        BAIL_ON_NT_STATUS(status);
    }

    /* Unpack descriptor */
    status = RtlSelfRelativeToAbsoluteSD(
        pSecurityDescriptor,
        pAbsolute,
        &ulAbsSize,
        pDacl,
        &ulDaclSize,
        pSacl,
        &ulSaclSize,
        pOwnerSid,
        &ulOwnerSidSize,
        pGroupSid,
        &ulGroupSidSize);
    BAIL_ON_NT_STATUS(status);

    status = LwIoFuseTranslateAbsoluteSecurityDescriptor(
        pAbsolute,
        pOwnerSid,
        pGroupSid,
        pStatbuf);
    BAIL_ON_NT_STATUS(status);

error:
    
    RTL_FREE(&pAbsolute);
    RTL_FREE(&pDacl);
    RTL_FREE(&pSacl);
    RTL_FREE(&pOwnerSid);
    RTL_FREE(&pGroupSid);

    return status;
}

NTSTATUS
LwIoFuseTranslatePosixOpenFlags(
    int flags,
    ACCESS_MASK* pAccessMask,
    FILE_CREATE_DISPOSITION* pDisposition,
    FILE_CREATE_OPTIONS* pOptions
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ACCESS_MASK accessMask = 0;
    FILE_CREATE_DISPOSITION disposition = 0;
    FILE_CREATE_OPTIONS options = 0;

    if (flags & O_WRONLY)
    {
        accessMask |= GENERIC_WRITE;
    }
    else if (flags & O_RDWR)
    {
        accessMask |= (GENERIC_READ | GENERIC_WRITE);
    }
    else
    {
        accessMask |= GENERIC_READ;
    }

    if (flags & O_APPEND)
    {
        accessMask |= FILE_APPEND_DATA;
    }

    if (flags & O_CREAT && flags & O_EXCL)
    {
        disposition = FILE_CREATE;
    }
    else if (flags & O_CREAT && flags & O_TRUNC)
    {
        disposition = FILE_OVERWRITE_IF;
    }
    else if (flags & O_TRUNC)
    {
        disposition = FILE_OVERWRITE;
    }
    else if (flags & O_CREAT)
    {
        disposition = FILE_OPEN_IF;
    }
    else
    {
        disposition = FILE_OPEN;
    }

    if (flags & O_DIRECTORY)
    {
        options |= FILE_DIRECTORY_FILE;
    }

    *pAccessMask = accessMask;
    *pDisposition = disposition;
    *pOptions = options;

    return status;
}

NTSTATUS
LwIoFuseTranslateFsInfo(
    PFILE_FS_SIZE_INFORMATION pSizeInfo,
    struct statvfs* pStatbuf
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG64 ullFsSize = 0;
    ULONG64 ullFsFree = 0;
    unsigned long blockSize = 0;

    blockSize = (unsigned long) (pSizeInfo->SectorsPerAllocationUnit * pSizeInfo->BytesPerSector);

    ullFsSize = 
        (ULONG64) pSizeInfo->TotalAllocationUnits *
        (ULONG64) pSizeInfo->SectorsPerAllocationUnit *
        (ULONG64) pSizeInfo->BytesPerSector;
    
    ullFsFree =
        (ULONG64) pSizeInfo->AvailableAllocationUnits *
        (ULONG64) pSizeInfo->SectorsPerAllocationUnit *
        (ULONG64) pSizeInfo->BytesPerSector;

    pStatbuf->f_bsize = blockSize;
    pStatbuf->f_blocks = (fsblkcnt_t) (ullFsSize / blockSize);
    pStatbuf->f_bfree = (fsblkcnt_t) (ullFsFree / blockSize);
    pStatbuf->f_bavail = pStatbuf->f_bfree;
    pStatbuf->f_files = 0;
    pStatbuf->f_ffree = 0;
    pStatbuf->f_namemax = 255;

    return status;
}

int
LwIoFuseMapNtStatus(
    NTSTATUS status
    )
{
    int err = LwNtStatusToUnixErrno(status);

    if (err < 0)
    {
        return -EIO;
    }
    else
    {
        return -err;
    }
}

static
NTSTATUS
LwIoFuseTranslateAbsoluteSecurityDescriptor(
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    PSID pOwnerSid,
    PSID pGroupSid,
    struct stat* pStatbuf
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszOwnerSidStr = NULL;
    PSTR pszGroupSidStr = NULL;
    PSTR pszUserName = NULL;
    PSTR pszGroupName = NULL;
    PACCESS_TOKEN pOwnerToken = NULL;
    PACCESS_TOKEN pGroupToken = NULL;
    PACCESS_TOKEN pOtherToken = NULL;
    
    /* Translate owner/group sids to uid/gid */
    status = RtlAllocateCStringFromSid(
        &pszOwnerSidStr,
        pOwnerSid);
    BAIL_ON_NT_STATUS(status);

    status = RtlAllocateCStringFromSid(
        &pszGroupSidStr,
        pGroupSid);
    BAIL_ON_NT_STATUS(status);

    status = LwIoGetNtNameForSid(
        pszOwnerSidStr,
        &pszUserName);
    if (status == STATUS_GENERIC_NOT_MAPPED)
    {
        status = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(status);

    status = LwIoGetNtNameForSid(
        pszGroupSidStr,
        &pszGroupName);
    if (status == STATUS_GENERIC_NOT_MAPPED)
    {
        status = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(status);

    if (pszUserName)
    {
        status = LwIoGetUidForNtName(
            pszUserName,
            &pStatbuf->st_uid);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        pStatbuf->st_uid = (uid_t) 65534;
    }

    if (pszGroupName)
    {
        status = LwIoGetGidForNtName(
            pszGroupName,
            &pStatbuf->st_gid);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        pStatbuf->st_gid = (gid_t) 65534;
    }
    
    /* Create access tokens for user/group/other */
    status = LwIoFuseCreateAccessTokenForOwner(
        pOwnerSid,
        &pOwnerToken);
    BAIL_ON_NT_STATUS(status);

    status = LwIoFuseCreateAccessTokenForGroup(
        pGroupSid,
        &pGroupToken);
    BAIL_ON_NT_STATUS(status);

    status = LwIoFuseCreateAccessTokenForOther(
        &pOtherToken);
    BAIL_ON_NT_STATUS(status);

    /* Generate unix permissions */
    status = LwIoFuseTranslateSecurityDescriptorPermissions(
        pSecurityDescriptor,
        pOwnerToken,
        pGroupToken,
        pOtherToken,
        pStatbuf);
    BAIL_ON_NT_STATUS(status);

error:
    
    RTL_FREE(&pszUserName);
    RTL_FREE(&pszGroupName);
    RTL_FREE(&pszOwnerSidStr);
    RTL_FREE(&pszGroupSidStr);

    if (pOwnerToken)
    {
        RtlReleaseAccessToken(&pOwnerToken);
    }

    if (pGroupToken)
    {
        RtlReleaseAccessToken(&pGroupToken);
    }

    if (pOtherToken)
    {
        RtlReleaseAccessToken(&pOtherToken);
    }

    return status;
}

static
NTSTATUS
LwIoWinToUnixTime(
    time_t *pUnixTime,
    LONG64 WinTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    *pUnixTime = (WinTime /  10000000LL) - TIME_SEC_CONVERSION_CONSTANT;

    return status;
}

static
NTSTATUS
LwIoGetNtNameForSid(
    PCSTR pszSidStr,
    PSTR* ppszNtName
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hLsaConnection = NULL;
    PLSA_SID_INFO pSidInfo = NULL;
    PSTR pszNtName = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    if (dwError)
    {
        status = STATUS_GENERIC_NOT_MAPPED;
        BAIL_ON_NT_STATUS(status);
    }

    dwError = LsaGetNamesBySidList(
        hLsaConnection,
        1,
        (PSTR*) &pszSidStr,
        &pSidInfo,
        NULL);
    if (dwError)
    {
        status = STATUS_GENERIC_NOT_MAPPED;
        BAIL_ON_NT_STATUS(status);
    }

    
    if (pSidInfo->accountType != AccountType_NotFound)
    {
        status = LwRtlCStringAllocatePrintf(
            &pszNtName,
            "%s\\%s",
            pSidInfo->pszDomainName,
            pSidInfo->pszSamAccountName);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        status = STATUS_GENERIC_NOT_MAPPED;
        BAIL_ON_NT_STATUS(status);
    }

    *ppszNtName = pszNtName;

cleanup:

    if (pSidInfo)
    {
        LsaFreeSIDInfoList(pSidInfo, 1);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    return status;

error:

    *ppszNtName = NULL;

    RTL_FREE(&pszNtName);

    goto cleanup;
}

static
NTSTATUS
LwIoGetUidForNtName(
    PCSTR pszNtName,
    uid_t* pUid
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hLsaConnection = NULL;
    PLSA_USER_INFO_0 pUserInfo = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    if (dwError)
    {
        status = STATUS_GENERIC_NOT_MAPPED;
        BAIL_ON_NT_STATUS(status);
    }

    dwError = LsaFindUserByName(
        hLsaConnection,
        pszNtName,
        0,
        OUT_PPVOID(&pUserInfo));
    if (dwError)
    {
        status = STATUS_GENERIC_NOT_MAPPED;
        BAIL_ON_NT_STATUS(status);
    }

    *pUid = pUserInfo->uid;

cleanup:

    if (pUserInfo)
    {
        LsaFreeUserInfo(0, pUserInfo);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    return status;

error:

    *pUid = -1;

    goto cleanup;
}

static
NTSTATUS
LwIoGetGidForNtName(
    PCSTR pszNtName,
    gid_t* pGid
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hLsaConnection = NULL;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;

    dwError = LsaOpenServer(&hLsaConnection);
    if (dwError)
    {
        status = STATUS_GENERIC_NOT_MAPPED;
        BAIL_ON_NT_STATUS(status);
    }

    dwError = LsaFindGroupByName(
        hLsaConnection,
        pszNtName,
        0,
        0,
        OUT_PPVOID(&pGroupInfo));
    if (dwError)
    {
        status = STATUS_GENERIC_NOT_MAPPED;
        BAIL_ON_NT_STATUS(status);
    }

    *pGid = pGroupInfo->gid;

cleanup:

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(0, pGroupInfo);
    }

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    return status;

error:

    *pGid = -1;

    goto cleanup;
}

static
NTSTATUS
LwIoFuseTranslateSecurityDescriptorPermissions(
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    PACCESS_TOKEN pOwnerToken,
    PACCESS_TOKEN pGroupToken,
    PACCESS_TOKEN pOtherToken,
    struct stat* pStatbuf
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bIsDirectory = S_ISDIR(pStatbuf->st_mode);

    status = LwIoFuseTranslateAccess(
        pSecurityDescriptor,
        pOwnerToken,
        !bIsDirectory ? S_IRUSR : S_IRUSR | S_IXUSR,
        S_IWUSR,
        S_IXUSR,
        &pStatbuf->st_mode);
    BAIL_ON_NT_STATUS(status);

    status = LwIoFuseTranslateAccess(
        pSecurityDescriptor,
        pGroupToken,
        !bIsDirectory ? S_IRGRP : S_IRGRP | S_IXGRP,
        S_IWGRP,
        S_IXGRP,
        &pStatbuf->st_mode);
    BAIL_ON_NT_STATUS(status);

    status = LwIoFuseTranslateAccess(
        pSecurityDescriptor,
        pOtherToken,
        !bIsDirectory ? S_IROTH : S_IROTH | S_IXOTH,
        S_IWOTH,
        S_IXOTH,
        &pStatbuf->st_mode);
    BAIL_ON_NT_STATUS(status);

error:

    return status;
}

static
NTSTATUS
LwIoFuseTranslateAccess(
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    PACCESS_TOKEN pAccessToken,
    mode_t readMode,
    mode_t writeMode,
    mode_t executeMode,
    mode_t* outMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    static GENERIC_MAPPING mapping =
        {
            .GenericRead = FILE_GENERIC_READ,
            .GenericWrite = FILE_GENERIC_WRITE,
            .GenericExecute = FILE_GENERIC_EXECUTE,
            .GenericAll = FILE_ALL_ACCESS
        };
    ACCESS_MASK grantedAccess = 0;
    
    if (RtlAccessCheck(
            pSecurityDescriptor,
            pAccessToken,
            MAXIMUM_ALLOWED,
            0,
            &mapping,
            &grantedAccess,
            &status))
    {
        if (grantedAccess & FILE_READ_DATA)
        {
            *outMode |= readMode;
        }
        if (grantedAccess & FILE_WRITE_DATA)
        {
            *outMode |= writeMode;
        }
        if (grantedAccess & FILE_EXECUTE)
        {
            *outMode |= executeMode;
        }
    }
    else
    {
        if (status == STATUS_ACCESS_DENIED)
        {
            status = STATUS_SUCCESS;
        }
    }

    return status;
}

static
NTSTATUS
LwIoFuseCreateAccessTokenForOwner(
    PSID pOwnerSid,
    PACCESS_TOKEN* ppOwnerToken
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN pOwnerToken = NULL;
    TOKEN_USER user = {{0}};
    TOKEN_OWNER owner = {0};
    TOKEN_GROUPS groups = {0};
    TOKEN_PRIMARY_GROUP primaryGroup = {0};
    TOKEN_DEFAULT_DACL dacl = {0};

    user.User.Sid = pOwnerSid;
    
    status = RtlCreateAccessToken(
        &pOwnerToken,
        &user,
        &groups,
        &owner,
        &primaryGroup,
        &dacl,
        NULL);
    BAIL_ON_NT_STATUS(status);

cleanup:

    *ppOwnerToken = pOwnerToken;

    return status;

error:

    *ppOwnerToken = NULL;

    if (pOwnerToken)
    {
        RtlReleaseAccessToken(&pOwnerToken);
    }

    goto cleanup;
}

static
NTSTATUS
LwIoFuseCreateAccessTokenForGroup(
    PSID pGroupSid,
    PACCESS_TOKEN* ppGroupToken
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN pGroupToken = NULL;
    TOKEN_USER user = {{0}};
    TOKEN_OWNER owner = {0};
    TOKEN_GROUPS groups = {0};
    TOKEN_PRIMARY_GROUP primaryGroup = {0};
    TOKEN_DEFAULT_DACL dacl = {0};

    user.User.Sid = pGroupSid;
    
    status = RtlCreateAccessToken(
        &pGroupToken,
        &user,
        &groups,
        &owner,
        &primaryGroup,
        &dacl,
        NULL);
    BAIL_ON_NT_STATUS(status);

    *ppGroupToken = pGroupToken;

cleanup:

    return status;

error:

    *ppGroupToken = NULL;

    if (pGroupToken)
    {
        RtlReleaseAccessToken(&pGroupToken);
    }

    goto cleanup;
}

static
NTSTATUS
LwIoFuseCreateAccessTokenForOther(
    PACCESS_TOKEN* ppOtherToken
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN pOtherToken = NULL;
    TOKEN_USER user = {{0}};
    TOKEN_OWNER owner = {0};
    TOKEN_GROUPS groups = {0};
    TOKEN_PRIMARY_GROUP primaryGroup = {0};
    TOKEN_DEFAULT_DACL dacl = {0};

    status = RtlAllocateSidFromCString(
        &user.User.Sid,
        "S-1-1-0");
    BAIL_ON_NT_STATUS(status);
    
    status = RtlCreateAccessToken(
        &pOtherToken,
        &user,
        &groups,
        &owner,
        &primaryGroup,
        &dacl,
        NULL);
    BAIL_ON_NT_STATUS(status);

    *ppOtherToken = pOtherToken;

cleanup:

    RTL_FREE(&user.User.Sid);

    return status;

error:

    *ppOtherToken = NULL;

    if (pOtherToken)
    {
        RtlReleaseAccessToken(&pOtherToken);
    }

    goto cleanup;
}
