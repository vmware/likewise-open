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
 *        sharemode.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Support for Windows File Share Modes
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


/* Code */

/***********************************************************
 **********************************************************/

NTSTATUS
PvfsCheckShareMode(
    IN PSTR pszFilename,
    IN FILE_SHARE_FLAGS SharedAccess,
    IN ACCESS_MASK DesiredAccess,
    OUT PPVFS_FCB *ppFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_FCB pFcb = NULL;

    ntError = PvfsFindFCB(&pFcb, pszFilename);
    if (ntError == STATUS_SUCCESS) {

        ntError = PvfsEnforceShareMode(
                      pFcb,
                      SharedAccess,
                      DesiredAccess);

        /* If we have success, then we are good.  If we have a sharing
           violation, give the caller a chance to break the oplock and
           we'll try again when the create is resumed. */

        if (ntError == STATUS_SUCCESS ||
            ntError == STATUS_SHARING_VIOLATION)
        {
            *ppFcb = PvfsReferenceFCB(pFcb);
        }
        goto cleanup;
    }

    if (ntError != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        BAIL_ON_NT_STATUS(ntError);
    }

    /* If not found, then add one */

    ntError = PvfsCreateFCB(
                  &pFcb,
                  pszFilename,
                  SharedAccess,
                  DesiredAccess);
    BAIL_ON_NT_STATUS(ntError);

    *ppFcb = PvfsReferenceFCB(pFcb);
    ntError = STATUS_SUCCESS;

cleanup:
    if (pFcb) {
        PvfsReleaseFCB(pFcb);
    }

    return ntError;

error:
    goto cleanup;
}


/***********************************************************
 **********************************************************/

static NTSTATUS
_PvfsEnforceShareMode(
    IN PPVFS_FCB pFcb,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN ACCESS_MASK DesiredAccess
    );

NTSTATUS
PvfsEnforceShareMode(
    IN PPVFS_FCB pFcb,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    int i = 0;
    ULONG RetryMax = 3;
    struct timespec SleepTime = {0};
    struct timespec RemainingTime = {0};

    for (i=0; i<RetryMax && (ntError != STATUS_SUCCESS); i++)
    {
        ntError = _PvfsEnforceShareMode(pFcb, ShareAccess, DesiredAccess);
        if ((ntError == STATUS_SHARING_VIOLATION) && (i<(RetryMax-1)))
        {
            NTSTATUS ntErrorSleep = STATUS_SUCCESS;

            /* 2 milliseconds */
            RemainingTime.tv_sec = 0;
            RemainingTime.tv_nsec = 2 * 1000000;

            do {
                SleepTime.tv_sec = RemainingTime.tv_sec;
                SleepTime.tv_nsec = RemainingTime.tv_nsec;

                ntErrorSleep = PvfsSysNanoSleep(&SleepTime, &RemainingTime);
            } while (ntErrorSleep == STATUS_MORE_PROCESSING_REQUIRED);

            continue;
        }
        BAIL_ON_NT_STATUS(ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}


/***********************************************************
 **********************************************************/

struct _SHARE_MODE_ACCESS_COMPATIBILITY
{
    FILE_SHARE_FLAGS ShareFlag;
    ACCESS_MASK Access;
} ShareModeTable[] = {
    { FILE_SHARE_READ,   (FILE_READ_DATA|FILE_EXECUTE) },
    { FILE_SHARE_WRITE,  (FILE_WRITE_DATA|FILE_APPEND_DATA) },
    { FILE_SHARE_DELETE, (DELETE) }
};

NTSTATUS
_PvfsEnforceShareMode(
    IN PPVFS_FCB pFcb,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PLW_LIST_LINKS pCursor = NULL;
    PPVFS_CCB pCcb = NULL;
    BOOLEAN bLocked = FALSE;
    ACCESS_MASK AllRights = (FILE_READ_DATA|
                             FILE_WRITE_DATA|
                             FILE_APPEND_DATA|
                             FILE_EXECUTE|
                             DELETE);
    DWORD TableSize = sizeof(ShareModeTable) /
                      sizeof(struct _SHARE_MODE_ACCESS_COMPATIBILITY);
    int i = 0;

    RtlMapGenericMask(&DesiredAccess, &gPvfsFileGenericMapping);

    LWIO_LOCK_RWMUTEX_SHARED(bLocked, &pFcb->rwCcbLock);

    while ((pCursor = PvfsListTraverse(pFcb->pCcbList, pCursor)) != NULL)
    {
        pCcb = LW_STRUCT_FROM_FIELD(
                   pCursor,
                   PVFS_CCB,
                   FcbList);

        /* Ignore handles that are in the midst of being closed */

        if (pCcb->bCloseInProgress)
        {
            continue;
        }

        /* Fast Path - If we are not asking for read/write/execute/delete
           access, then we cannot conflict */

        if (((DesiredAccess & AllRights) == 0) ||
            ((pCcb->AccessGranted & AllRights) == 0))
        {
            continue;
        }

        for (i=0; i<TableSize; i++)
        {
            /* Check for a conflict between the request access and
               an existing share mode */

            if ((DesiredAccess & ShareModeTable[i].Access) &&
                (!(pCcb->ShareFlags & ShareModeTable[i].ShareFlag)))
            {
                ntError = STATUS_SHARING_VIOLATION;
                BAIL_ON_NT_STATUS(ntError);
            }

            /* Check for a conflict between the request share mode
               and an existing granted access mask */

            if ((pCcb->AccessGranted & ShareModeTable[i].Access) &&
                (!(ShareAccess & ShareModeTable[i].ShareFlag)))
            {
                ntError = STATUS_SHARING_VIOLATION;
                BAIL_ON_NT_STATUS(ntError);
            }
        }

        pCcb = NULL;
    }

    /* Cone - No conflicts*/

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bLocked, &pFcb->rwCcbLock);

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
