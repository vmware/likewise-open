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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        filename.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        PVFS filename type
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"


////////////////////////////////////////////////////////////////////////

VOID
PvfsDestroyFileName(
    PPVFS_FILE_NAME pFileName
    )
{
    if (pFileName->FileName)
    {
        LwRtlCStringFree(&pFileName->FileName);
    }

    if (pFileName->StreamName)
    {
        LwRtlCStringFree(&pFileName->StreamName);
    }

    pFileName->Type = PVFS_STREAM_TYPE_UNKNOWN;
}


////////////////////////////////////////////////////////////////////////

VOID
PvfsFreeFileName(
    PPVFS_FILE_NAME pFileName
    )
{
    if (pFileName)
    {
        PvfsDestroyFileName(pFileName);
        PvfsFreeMemory((PVOID*)&pFileName);
    }

    return;
}

////////////////////////////////////////////////////////////////////////

static
NTSTATUS
PvfsParseStreamType(
    OUT PPVFS_STREAM_TYPE pType,
    IN PCSTR StreamTypeString
    );

NTSTATUS
PvfsBuildFileNameFromCString(
    IN OUT PPVFS_FILE_NAME pFileName,
    IN PCSTR FullFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR currentPosition = NULL;
    PSTR cursor = NULL;
    PSTR fileName = NULL;
    PSTR streamName = NULL;
    PSTR streamTypeString = NULL;
    PVFS_STREAM_TYPE streamTypeValue = PVFS_STREAM_TYPE_UNKNOWN;

    BAIL_ON_INVALID_PTR(pFileName, ntError);

    pFileName->FileName = NULL;
    pFileName->StreamName = NULL;
    pFileName->Type = PVFS_STREAM_TYPE_UNKNOWN;

    ntError = LwRtlCStringDuplicate(&fileName, FullFileName);
    BAIL_ON_NT_STATUS(ntError);

    currentPosition = fileName;

    // Get the File Object name
    // Foo.txt:Summary:$DATA
    // ^

    cursor = strchr(currentPosition, PVFS_STREAM_DELIMINATOR_C);
    if (cursor == NULL)
    {
        // entire currentPosition is the file object name

        ntError = LwRtlCStringDuplicate(&pFileName->FileName, fileName);
        BAIL_ON_NT_STATUS(ntError);

        pFileName->StreamName = NULL;
        pFileName->Type = PVFS_STREAM_TYPE_DATA;

        goto cleanup;
    }

    *cursor = '\0';
    cursor++;

    if (*cursor == '\0')
    {
        // Cannot end in a trailing ':'
        ntError = STATUS_OBJECT_NAME_INVALID;
        BAIL_ON_NT_STATUS(ntError);
    }

    currentPosition = cursor;
    streamName = currentPosition;

    // Get the stream name
    // Foo.txt:Summary:$DATA
    //         ^

    if (*cursor != PVFS_STREAM_DELIMINATOR_C)
    {
        // This is not a default stream name ("::")
        cursor = strchr(currentPosition, PVFS_STREAM_DELIMINATOR_C);
        if (cursor == NULL)
        {
            // Missing StreamType
            ntError = STATUS_OBJECT_NAME_INVALID;
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    *cursor = '\0';
    cursor++;

    currentPosition = cursor;
    streamTypeString = currentPosition;

    if (*cursor != '\0')
    {
        // Get the stream type
        // Foo.txt:Summary:$DATA
        //                 ^
        ntError = PvfsParseStreamType(&streamTypeValue, streamTypeString);
        BAIL_ON_NT_STATUS(ntError);
    }
    else
    {
        streamTypeValue = PVFS_STREAM_TYPE_DATA;
    }

    ntError = LwRtlCStringDuplicate(&pFileName->FileName, fileName);
    BAIL_ON_NT_STATUS(ntError);

    if (*streamName != '\0')
    {
        // only allocate a stream name if we have a non-empty string
        ntError = LwRtlCStringDuplicate(&pFileName->StreamName, streamName);
        BAIL_ON_NT_STATUS(ntError);
    }

    pFileName->Type = streamTypeValue;

cleanup:
    if (!NT_SUCCESS(ntError))
    {
        PvfsDestroyFileName(pFileName);
    }

    if (fileName)
    {
        LwRtlCStringFree(&fileName);
    }


    return ntError;

error:
    goto cleanup;
}

////////////////////////////////////////////////////////////////////////

static
NTSTATUS
PvfsParseStreamType(
    OUT PPVFS_STREAM_TYPE pStreamType,
    IN PCSTR StreamTypeString
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PVFS_STREAM_TYPE streamType = PVFS_STREAM_TYPE_UNKNOWN;

    BAIL_ON_INVALID_PTR(pStreamType, ntError);

    if (!StreamTypeString ||
        (*StreamTypeString == '\0') ||
        (*StreamTypeString != '$'))
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (LwRtlCStringIsEqual(StreamTypeString, "$DATA", FALSE))
    {
        streamType = PVFS_STREAM_TYPE_DATA;
    }
    else
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    *pStreamType = streamType;

cleanup:
    return ntError;

error:
    goto cleanup;

}


////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsAllocateFileNameFromCString(
    OUT PPVFS_FILE_NAME *ppFileName,
    IN PCSTR SourceFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_FILE_NAME pFileName = NULL;

    ntError = PvfsAllocateMemory((PVOID*)&pFileName, sizeof(*pFileName), TRUE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsBuildFileNameFromCString(pFileName, SourceFileName);
    BAIL_ON_NT_STATUS(ntError);

    *ppFileName = pFileName;
    pFileName = NULL;

error:
    if (pFileName)
    {
        PvfsFreeFileName(pFileName);
    }

    return ntError;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsBuildFileNameFromScb(
    IN OUT PPVFS_FILE_NAME pFileName,
    IN PPVFS_SCB pScb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN scbRwLock = FALSE;
    BOOLEAN fcbRwLock = FALSE;

    BAIL_ON_INVALID_PTR(pScb, ntError);

    LWIO_LOCK_RWMUTEX_SHARED(scbRwLock, &pScb->BaseControlBlock.RwLock);
    LWIO_LOCK_RWMUTEX_SHARED(fcbRwLock, &pScb->pOwnerFcb->BaseControlBlock.RwLock);

    ntError = LwRtlCStringDuplicate(&pFileName->FileName, pScb->pOwnerFcb->pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    if (!((pScb->StreamType == PVFS_STREAM_TYPE_DATA) &&
          ((pScb->pszStreamname == NULL) || (*pScb->pszStreamname == '\0'))))
    {
        LwRtlCStringDuplicate(&pFileName->StreamName, pScb->pszStreamname);
        BAIL_ON_NT_STATUS(ntError);
    }
    else
    {
        pFileName->StreamName = NULL;
    }

    pFileName->Type = pScb->StreamType;

cleanup:
    LWIO_UNLOCK_RWMUTEX(fcbRwLock, &pScb->pOwnerFcb->BaseControlBlock.RwLock);
    LWIO_UNLOCK_RWMUTEX(scbRwLock, &pScb->BaseControlBlock.RwLock);

    return ntError;

error:
    goto cleanup;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsAllocateFileNameFromScb(
    OUT PPVFS_FILE_NAME *ppFileName,
    IN PPVFS_SCB pScb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_FILE_NAME pFileName = NULL;

    ntError = PvfsAllocateMemory((PVOID*)&pFileName, sizeof(*pFileName), TRUE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsBuildFileNameFromScb(pFileName, pScb);
    BAIL_ON_NT_STATUS(ntError);

    *ppFileName = pFileName;
    pFileName = NULL;

error:
    if (pFileName)
    {
        PvfsFreeFileName(pFileName);
    }

    return ntError;
}


////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsAllocateCStringFromFileName(
    OUT PSTR *ppFullFileName,
    IN PPVFS_FILE_NAME pFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR outputFileName = NULL;

    switch (pFileName->Type)
    {
        case PVFS_STREAM_TYPE_DATA:
            ntError = LwRtlCStringAllocatePrintf(
                          &outputFileName,
                          "%s%s%s%s%s",
                          pFileName->FileName,
                          pFileName->StreamName ? PVFS_STREAM_DELIMINATOR_S : "",
                          pFileName->StreamName ? pFileName->StreamName : "",
                          pFileName->StreamName ? PVFS_STREAM_DELIMINATOR_S : "",
                          pFileName->StreamName ? PVFS_STREAM_DEFAULT_TYPE_S : "");
            break;

        default:
            ntError = STATUS_OBJECT_NAME_INVALID;
            break;
    }
    BAIL_ON_NT_STATUS(ntError);

    *ppFullFileName = outputFileName;
    outputFileName = NULL;

error:
    if (!NT_SUCCESS(ntError))
    {
        if (outputFileName)
        {
            LwRtlCStringFree(&outputFileName);
        }
    }

    return ntError;
}


////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsFileNameCopy(
    IN OUT PPVFS_FILE_NAME pDstFileName,
    IN PPVFS_FILE_NAME pSrcFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pDstFileName, ntError);

    ntError = LwRtlCStringDuplicate(
                  &pDstFileName->FileName,
                  pSrcFileName->FileName);
    BAIL_ON_NT_STATUS(ntError);

    if (pSrcFileName->StreamName)
    {
        ntError = LwRtlCStringDuplicate(
                      &pDstFileName->StreamName,
                      pSrcFileName->StreamName);
        BAIL_ON_NT_STATUS(ntError);
    }
    else
    {
        pDstFileName->StreamName = NULL;
    }

    pDstFileName->Type = pSrcFileName->Type;

error:
    if (!NT_SUCCESS(ntError))
    {
        PvfsDestroyFileName(pDstFileName);
    }

    return ntError;
}


////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsFileNameDuplicate(
    OUT PPVFS_FILE_NAME *ppDstFileName,
    IN PPVFS_FILE_NAME pSrcFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_FILE_NAME outFileName = NULL;

    *ppDstFileName = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&outFileName,
                  sizeof(*outFileName),
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsFileNameCopy(outFileName, pSrcFileName);
    BAIL_ON_NT_STATUS(ntError);

    *ppDstFileName = outFileName;

error:
    if (!NT_SUCCESS(ntError))
    {
        if (outFileName)
        {
            PvfsFreeFileName(outFileName);

        }
    }

    return ntError;
}

////////////////////////////////////////////////////////////////////////

LONG
PvfsFileNameCompare(
    IN PPVFS_FILE_NAME FileName1,
    IN PPVFS_FILE_NAME FileName2,
    IN BOOLEAN CaseSensitive
    )
{
    LONG cmpResult = 0;

    cmpResult = LwRtlCStringCompare(
                    FileName1->FileName,
                    FileName2->FileName,
                    TRUE);

    if (cmpResult == 0)
    {
        // Base FielName is the same

        if ((FileName1->StreamName == NULL) && (FileName2->StreamName == NULL))
        {
            // Both default streams, so compare the Stream Type value

            if (FileName1->Type == FileName2->Type)
            {
                cmpResult = 0;
            }
            else if (FileName1->Type < FileName2->Type)
            {
                cmpResult = -1;
            }
            else
            {
                cmpResult = 1;
            }
        }
        else if (FileName1->StreamName == NULL)
        {
            cmpResult = 1;
        }
        else if (FileName2->StreamName == NULL)
        {
            cmpResult = -1;
        }
        else
        {
            cmpResult = LwRtlCStringCompare(
                            FileName1->StreamName,
                            FileName2->StreamName,
                            TRUE);

        }
    }

    return cmpResult;

}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsSplitFileNamePath(
    OUT PPVFS_FILE_NAME *ppDirectoryName,
    OUT PPVFS_FILE_NAME *ppRelativeFileName,
    IN PPVFS_FILE_NAME SourceFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_FILE_NAME fileNameDirectory= NULL;
    PPVFS_FILE_NAME fileNameRelative = NULL;
    PSTR directoryName = NULL;
    PSTR relativeFileName = NULL;
    PSTR streamName = NULL;

    ntError = PvfsFileSplitPath(
                  &directoryName,
                  &relativeFileName,
                  SourceFileName->FileName);
    BAIL_ON_NT_STATUS(ntError);

    if (SourceFileName->StreamName)
    {
        ntError = LwRtlCStringDuplicate(&streamName, SourceFileName->StreamName);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAllocateMemory(
                  (PVOID*)&fileNameDirectory,
                  sizeof(*fileNameDirectory),
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateMemory(
                  (PVOID*)&fileNameRelative,
                  sizeof(*fileNameRelative),
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    fileNameDirectory->FileName = directoryName;
    fileNameDirectory->StreamName = NULL;
    fileNameDirectory->Type = PVFS_STREAM_TYPE_DATA;
    directoryName = NULL;

    fileNameRelative->FileName = relativeFileName;
    fileNameRelative->StreamName = streamName;
    fileNameRelative->Type = SourceFileName->Type;
    relativeFileName = NULL;
    streamName = NULL;

    *ppDirectoryName = fileNameDirectory;
    *ppRelativeFileName = fileNameRelative;

error:
    if (!NT_SUCCESS(ntError))
    {
        if (fileNameDirectory)
        {
            PvfsFreeFileName(fileNameDirectory);
        }
        if (fileNameRelative)
        {
            PvfsFreeFileName(fileNameRelative);
        }
    }

    if (directoryName)
    {
        LwRtlCStringFree(&directoryName);
    }
    if (relativeFileName)
    {
        LwRtlCStringFree(&relativeFileName);
    }
    if (streamName)
    {
        LwRtlCStringFree(&streamName);
    }

    return ntError;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsAppendFileName(
    OUT PPVFS_FILE_NAME *ppDstFileName,
    IN PPVFS_FILE_NAME BaseFileName,
    IN PPVFS_FILE_NAME RelativeFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_FILE_NAME destFileName = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&destFileName,
                  sizeof(*destFileName),
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = LwRtlCStringAllocatePrintf(
                  &destFileName->FileName,
                  "%s/%s",
                  BaseFileName->FileName,
                  RelativeFileName->FileName);
    BAIL_ON_NT_STATUS(ntError);

    if (RelativeFileName->StreamName)
    {
        ntError = LwRtlCStringDuplicate(
                      &destFileName->StreamName,
                      RelativeFileName->StreamName);
        BAIL_ON_NT_STATUS(ntError);
    }

    destFileName->Type = RelativeFileName->Type;

    *ppDstFileName = destFileName;

error:
    if (!NT_SUCCESS(ntError))
    {
        if (destFileName)
        {
            PvfsFreeFileName(destFileName);
        }
    }

    return ntError;
}

////////////////////////////////////////////////////////////////////////

PCSTR
PvfsGetCStringBaseFileName(
    IN PPVFS_FILE_NAME FileName
    )
{
    return FileName->FileName;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsRenameBaseFileName(
    IN OUT PPVFS_FILE_NAME FileName,
    PCSTR NewBaseFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR newBaseFileName = NULL;

    BAIL_ON_INVALID_PTR(NewBaseFileName, ntError);

    ntError = LwRtlCStringDuplicate(&newBaseFileName, NewBaseFileName);
    BAIL_ON_NT_STATUS(ntError);

    if (FileName->FileName)
    {
        LwRtlCStringFree(&FileName->FileName);
    }

    FileName->FileName = newBaseFileName;


error:
    if (!NT_SUCCESS(ntError))
    {
        if (newBaseFileName)
        {
            LwRtlCStringFree(&newBaseFileName);
        }
    }

    return ntError;
}
