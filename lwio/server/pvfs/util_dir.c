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
 *        util_dir.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Directory utility functions
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"


/* Forward declarations */



/* File Globals */



/* Code */

/***********************************************************
 **********************************************************/

VOID
PvfsFreeDirectoryContext(
    PPVFS_DIRECTORY_CONTEXT pDirCtx
    )
{
    int i = 0;

    if (!pDirCtx) {
        return;
    }

    for (i=0; i<pDirCtx->dwNumEntries; i++)
    {
        RtlCStringFree(&pDirCtx->pDirEntries[i].pszFilename);
    }


    PVFS_FREE(&pDirCtx->pDirEntries);
    PVFS_FREE(&pDirCtx);

    return;
}

/***********************************************************
 **********************************************************/

static NTSTATUS
PvfsDirContextAddEntry(
    PPVFS_DIRECTORY_CONTEXT pDirCtx,
    PCSTR pszPathname
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    DWORD dwCurrent = 0;
    PPVFS_DIRECTORY_ENTRY pList = NULL;

    if ((pDirCtx->ulAllocated == 0) ||
        ((pDirCtx->dwNumEntries+1) == pDirCtx->ulAllocated))
    {
        pDirCtx->ulAllocated += 256;
        ntError = PvfsReallocateMemory((PVOID*)&pDirCtx->pDirEntries,
                                       sizeof(PVFS_DIRECTORY_ENTRY)*pDirCtx->ulAllocated);
        BAIL_ON_NT_STATUS(ntError);
    }

    dwCurrent = pDirCtx->dwNumEntries;
    pList = pDirCtx->pDirEntries;

    pList[dwCurrent].bValidStat = FALSE;
    ntError = RtlCStringDuplicate(&pList[dwCurrent].pszFilename, pszPathname);
    BAIL_ON_NT_STATUS(ntError);


    pDirCtx->dwNumEntries++;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/********************************************************
 *******************************************************/

static NTSTATUS
AllocateCStringFileSpec(
    PSTR *ppszPattern,
    PIO_MATCH_FILE_SPEC pFileSpec
    )
{
    NTSTATUS ntError = STATUS_INSUFFICIENT_RESOURCES;


    if (pFileSpec == NULL)
    {
        ntError = RtlCStringDuplicate(ppszPattern, "*");
        BAIL_ON_NT_STATUS(ntError);

        goto cleanup;
    }

    ntError = RtlCStringAllocateFromUnicodeString(ppszPattern,
                                                  &pFileSpec->Pattern);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/***********************************************************
 **********************************************************/

NTSTATUS
PvfsEnumerateDirectory(
    PPVFS_CCB pCcb,
    PIO_MATCH_FILE_SPEC pFileSpec,
    LONG Count,
    BOOLEAN bRescan
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    DIR *pDir = NULL;
    struct dirent *pDirEntry = NULL;
    struct dirent dirEntry = { 0 };
    PSTR pszPattern = NULL;
    BOOLEAN bCaseSensitive = FALSE;
    PSTR pszDiskFilename = NULL;
    PVFS_STAT Stat = { 0 };

    if (Count == 0)
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = AllocateCStringFileSpec(&pszPattern, pFileSpec);
    BAIL_ON_NT_STATUS(ntError);

    if (bRescan)
    {
        if (pCcb->pDirContext->pDir)
        {
            PvfsSysCloseDir(pCcb->pDirContext->pDir);

        }
        PvfsFreeDirectoryContext(pCcb->pDirContext);
        pCcb->pDirContext = NULL;

        ntError = PvfsAllocateMemory(
                      (PVOID)&pCcb->pDirContext,
                      sizeof(PVFS_DIRECTORY_CONTEXT),
                      TRUE);
        BAIL_ON_NT_STATUS(ntError);

        pCcb->pDirContext->bScanned = FALSE;
    }


    if (!pCcb->pDirContext->bScanned)
    {
        if (!strchr(pszPattern, '?') && !strchr(pszPattern, '*'))
        {
            // A single file name match is the equivalent of a Windows Stat()
            ntError = PvfsLookupFile(
                          &pszDiskFilename,
                          &Stat,
                          pCcb->pszFilename,
                          pszPattern,
                          FALSE);
            BAIL_ON_NT_STATUS(ntError);

            RtlCStringFree(&pszDiskFilename);

            if (pCcb->EcpFlags & PVFS_ECP_ENABLE_ABE)
            {
                ntError = PvfsAccessCheckFileEnumerate(pCcb, pszPattern);
                BAIL_ON_NT_STATUS(ntError);
            }

            ntError = PvfsDirContextAddEntry(pCcb->pDirContext, pszPattern);
            BAIL_ON_NT_STATUS(ntError);

            pCcb->pDirContext->bScanned = TRUE;

            // Success
            goto cleanup;
        }
        else
        {
            // Prepare to enumerate the entire directory
            ntError = PvfsSysOpenDir(
                          pCcb->pszFilename,
                          &pCcb->pDirContext->pDir);
            BAIL_ON_NT_STATUS(ntError);

            pCcb->pDirContext->bScanned = TRUE;
        }
    }

    /* Loop to read entries */

    pDir = pCcb->pDirContext->pDir;

    /* -1 means to fill in whatever you can including current and
       parent directory entries */

    if (Count == -1)
    {
        /* Always add '.' and '..' first if thet match */

        if (PvfsWildcardMatch(".", pszPattern, FALSE))
        {
            ntError = PvfsDirContextAddEntry(pCcb->pDirContext, ".");
            BAIL_ON_NT_STATUS(ntError);
        }
        if (PvfsWildcardMatch("..", pszPattern, FALSE))
        {
            ntError = PvfsDirContextAddEntry(pCcb->pDirContext, "..");
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    /* Loop through directory entries */

    for(ntError = PvfsSysReadDir(pDir, &dirEntry, &pDirEntry);
        pDirEntry && ((Count == -1) || (pCcb->pDirContext->dwNumEntries < Count));
        ntError = PvfsSysReadDir(pDir, &dirEntry, &pDirEntry))
    {
        /* First check the error return */
        BAIL_ON_NT_STATUS(ntError);

        /* We've already added the "." and ".." directories */

        if (RtlCStringIsEqual(pDirEntry->d_name, ".", FALSE) ||
            RtlCStringIsEqual(pDirEntry->d_name, "..", FALSE))
        {
            continue;
        }

        if (PvfsWildcardMatch(pDirEntry->d_name, pszPattern, bCaseSensitive))
        {
            if (pCcb->EcpFlags & PVFS_ECP_ENABLE_ABE)
            {
                ntError = PvfsAccessCheckFileEnumerate(
                              pCcb,
                              pDirEntry->d_name);
                if (ntError != STATUS_SUCCESS)
                {
                    continue;
                }
            }

            ntError = PvfsDirContextAddEntry(pCcb->pDirContext, pDirEntry->d_name);
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    /* Bail if there were no matches */

    if (pCcb->pDirContext->dwNumEntries == 0) {
        ntError = STATUS_NO_SUCH_FILE;
        BAIL_ON_NT_STATUS(ntError);
    }

cleanup:
    RtlCStringFree(&pszPattern);

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

