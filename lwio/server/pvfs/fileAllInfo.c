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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        fileAllInfo.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FileAllInformation Handler
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static NTSTATUS
PvfsQueryFileAllInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );


/* File Globals */



/* Code */


NTSTATUS
PvfsFileAllInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    switch(Type)
    {
    case PVFS_SET:
        ntError = STATUS_NOT_SUPPORTED;
        break;

    case PVFS_QUERY:
        ntError = PvfsQueryFileAllInfo(pIrpContext);
        break;

    default:
        ntError = STATUS_INVALID_PARAMETER;
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

static NTSTATUS
PvfsQueryFileAllInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PFILE_ALL_INFORMATION pFileInfo = NULL;
    IRP_ARGS_QUERY_SET_INFORMATION Args = pIrpContext->pIrp->Args.QuerySetInformation;
    PVFS_STAT Stat = {0};
    PWSTR pwszFilename  = NULL;
    ULONG W16FilenameLen = 0;
    ULONG W16FilenameLenBytes = 0;
    ULONG Needed = 0;
    off_t CurrentOffset = 0;
    PSTR pszWinFileName = NULL;
    PSTR pszCursor = NULL;
    BOOLEAN bDeletePending = FALSE;

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(Args.FileInformation, ntError);

    /* No access checked needed for this call */

    if (Args.Length < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pFileInfo = (PFILE_ALL_INFORMATION)Args.FileInformation;

    /* Real work starts here */

    ntError = PvfsCcbQueryFileBasicInformation(
                  pCcb,
                  &pFileInfo->BasicInformation);
    BAIL_ON_NT_STATUS(ntError);

    /* Standard */

    ntError = PvfsSysFstat(pCcb->fd, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    bDeletePending = PvfsFcbIsPendingDelete(pCcb->pFcb);

    if (PVFS_IS_DIR(pCcb))
    {
        /* NTFS reports the allocation and end-of-file on
           directories as 0.  smbtorture cares about this even
           if no apps that I know of do. */

        pFileInfo->StandardInformation.AllocationSize = 0;
        pFileInfo->StandardInformation.EndOfFile      = 0;

        pFileInfo->StandardInformation.NumberOfLinks  = bDeletePending ? 0 : 1;
    }
    else
    {
        pFileInfo->StandardInformation.EndOfFile      = Stat.s_size;
        pFileInfo->StandardInformation.AllocationSize = Stat.s_alloc > Stat.s_size ?
                                                        Stat.s_alloc : Stat.s_size;
        pFileInfo->StandardInformation.NumberOfLinks  = bDeletePending ?
                                                        Stat.s_nlink - 1:
                                                        Stat.s_nlink;
    }

    pFileInfo->StandardInformation.DeletePending  = bDeletePending;
    pFileInfo->StandardInformation.Directory      = S_ISDIR(Stat.s_mode) ? TRUE : FALSE;

    /* Internal */
    pFileInfo->InternalInformation.IndexNumber = Stat.s_ino;

    /* EA */
    pFileInfo->EaInformation.EaSize = 0;

    /* Access */
    pFileInfo->AccessInformation.AccessFlags = pCcb->AccessGranted;

    /* Position */
    ntError = PvfsSysLseek(pCcb->fd, 0, SEEK_CUR, &CurrentOffset);
    BAIL_ON_NT_STATUS(ntError);

    pFileInfo->PositionInformation.CurrentByteOffset = (LONG64)CurrentOffset;

    /* Mode */
    pFileInfo->ModeInformation.Mode = pCcb->CreateOptions;

    /* Alignment */

    pFileInfo->AlignmentInformation.AlignmentRequirement = 0L;

    /* Name */

    ntError = LwRtlCStringDuplicate(&pszWinFileName, pCcb->pFcb->pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Convert to a Windows pathname equivalent */
    for (pszCursor=pszWinFileName; pszCursor && *pszCursor; pszCursor++)
    {
        if (*pszCursor == '/') {
            *pszCursor = '\\';
        }
    }

    ntError = LwRtlWC16StringAllocateFromCString(&pwszFilename, pszWinFileName);
    BAIL_ON_NT_STATUS(ntError);

    W16FilenameLen = RtlWC16StringNumChars(pwszFilename);
    W16FilenameLenBytes = W16FilenameLen * sizeof(WCHAR);
    Needed = sizeof(*pFileInfo) + W16FilenameLenBytes;

    if (Needed > (Args.Length - sizeof(*pFileInfo))) {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pFileInfo->NameInformation.FileNameLength = W16FilenameLenBytes;
    memcpy(pFileInfo->NameInformation.FileName, pwszFilename, W16FilenameLenBytes);

    pIrp->IoStatusBlock.BytesTransferred = sizeof(*pFileInfo);
    ntError = STATUS_SUCCESS;

cleanup:
    LwRtlCStringFree(&pszWinFileName);
    LwRtlWC16StringFree(&pwszFilename);

    if (pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
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

