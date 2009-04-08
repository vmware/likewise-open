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
 *        attrib.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Supporting DOS Attribute routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


/* File Globals */



/* Code */

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsGetFileAttributes(
    IN PPVFS_CCB pCcb,
    OUT PFILE_ATTRIBUTES pAttributes
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    *pAttributes = 0;

#ifdef HAVE_EA_SUPPORT
    ntError = PvfsGetFileAttributesXattr(pCcb, pAttributes);
#endif

    /* Fallback to generating a default attribute set */

    if (!NT_SUCCESS(ntError))
    {
        PSTR pszRelativeFilename = NULL;

        ntError = PvfsFileBasename(&pszRelativeFilename, pCcb->pszFilename);
        BAIL_ON_NT_STATUS(ntError);

        *pAttributes |= FILE_ATTRIBUTE_ARCHIVE;

        /* Hide 'dot' files (except "." and "..") */

        if (!RtlCStringIsEqual(pszRelativeFilename, ".", FALSE) &&
            !RtlCStringIsEqual(pszRelativeFilename, "..", FALSE))
        {
            if (*pszRelativeFilename == '.') {
                *pAttributes |= FILE_ATTRIBUTE_HIDDEN;
            }
        }

        RTL_FREE(&pszRelativeFilename);
    }

    /* Some attributes are properties on the file and not
       not settable.  Make sure to clear and then reset if
       necessary */

    *pAttributes &= ~(FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_DIRECTORY);
    if (pCcb->CreateOptions & FILE_DIRECTORY_FILE) {
        *pAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }

    /* FILE_ATTRIBUTE_NORMAL is only valid when the file
       has no other attributes set (according to SetFileAttribues()
       in the MS SDK) */

    if (*pAttributes == 0) {
        *pAttributes = FILE_ATTRIBUTE_NORMAL;
    }

    /* This will always succeed somehow */

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsGetFilenameAttributes(
    IN PSTR pszPath,
    OUT PFILE_ATTRIBUTES pAttributes
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PVFS_STAT Stat = {0};

    BAIL_ON_INVALID_PTR(pszPath, ntError);

    *pAttributes = 0;

#ifdef HAVE_EA_SUPPORT
    ntError = PvfsGetFilenameAttributesXattr(pszPath, pAttributes);
#endif

    /* Fallback to generating a default attribute set */

    if (!NT_SUCCESS(ntError))
    {
        PSTR pszRelativeFilename = NULL;

        ntError = PvfsFileBasename(&pszRelativeFilename, pszPath);
        BAIL_ON_NT_STATUS(ntError);

        *pAttributes |= FILE_ATTRIBUTE_ARCHIVE;

        /* Hide 'dot' files (except "." and "..") */

        if (!RtlCStringIsEqual(pszRelativeFilename, ".", FALSE) &&
            !RtlCStringIsEqual(pszRelativeFilename, "..", FALSE))
        {
            if (*pszRelativeFilename == '.') {
                *pAttributes |= FILE_ATTRIBUTE_HIDDEN;
            }
        }

        RTL_FREE(&pszRelativeFilename);
    }

    ntError = PvfsSysStat(pszPath, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    /* Some attributes are properties on the file and not
       not settable.  Make sure to clear and then reset if
       necessary */

    *pAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;
    if (S_ISDIR(Stat.s_mode)) {
        *pAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }
    /* FILE_ATTRIBUTE_NORMAL is only valid when the file
       has no other attributes set (according to SetFileAttribues()
       in the MS SDK) */

    if (*pAttributes == 0) {
        *pAttributes = FILE_ATTRIBUTE_NORMAL;
    }

    /* This will always succeed somehow */

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsSetFileAttributes(
    IN PPVFS_CCB pCcb,
    IN FILE_ATTRIBUTES Attributes
    )
{
    NTSTATUS ntError = STATUS_ACCESS_DENIED;
    FILE_ATTRIBUTES AttribNotSettable = FILE_ATTRIBUTE_ENCRYPTED |
                                        FILE_ATTRIBUTE_DEVICE |
                                        FILE_ATTRIBUTE_COMPRESSED |
                                        FILE_ATTRIBUTE_REPARSE_POINT |
                                        FILE_ATTRIBUTE_SPARSE_FILE;

    /* Use PvfsSetFileAttributesEx() for IoControl */

    if (Attributes & AttribNotSettable) {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Clear some bits that should not be stored */

    Attributes &= ~(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_NORMAL);

#ifdef HAVE_EA_SUPPORT
    ntError = PvfsSetFileAttributesXattr(pCcb, Attributes);
#else
    ntError = STATUS_SUCCESS;
#endif

    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 Used by the DeviceIoControl for setting attributes
 ***************************************************************/

NTSTATUS
PvfsSetFileAttributesEx(
    IN PPVFS_CCB pCcb,
    IN FILE_ATTRIBUTES Attributes
    )
{
    NTSTATUS ntError = STATUS_ACCESS_DENIED;
    FILE_ATTRIBUTES AttribNotSettable = FILE_ATTRIBUTE_ENCRYPTED |
                                        FILE_ATTRIBUTE_DEVICE;
    FILE_ATTRIBUTES AttribNotSupported = FILE_ATTRIBUTE_COMPRESSED |
                                        FILE_ATTRIBUTE_REPARSE_POINT |
                                        FILE_ATTRIBUTE_SPARSE_FILE;

    /* Not all attributes are settable */

    if (Attributes & AttribNotSettable) {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Not all attributes are supported */

    if (Attributes & AttribNotSupported) {
        ntError = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Clear some bits that should not be stored */

    Attributes &= ~(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_NORMAL);

#ifdef HAVE_EA_SUPPORT
    ntError = PvfsSetFileAttributesXattr(pCcb, Attributes);
#endif

    BAIL_ON_NT_STATUS(ntError);

cleanup:
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

