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
 *        macro.h
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *       Common macros for error checking, etc....
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#ifndef _PVFS_MACROS_H
#define _PVFS_MACROS_H

/* Error checking macros */

#define PVFS_BAIL_ON_NULL_PTR(x, err)   \
    do {                                \
        if ((x) == NULL) {              \
            err = STATUS_NO_MEMORY;     \
            goto error;                 \
        }                               \
    } while(0);                         \

#define PVFS_BAIL_ON_NTSTATUS_ERROR(err)                 \
    do {                                               \
        if ((err) != STATUS_SUCCESS) {                 \
            goto error;                                \
        }                                              \
    } while (0);

#define PVFS_BAIL_ON_NULL_PTR_PARAM(ptr, err)          \
    do {                                               \
        if ((ptr) == NULL) {                           \
            erro = STATUS_INVALID_PARAMETER;           \
            goto error;                                \
        }                                              \
    } while (0);


/* Memory Macros */

#define PVFS_SAFE_FREE_MEMORY(p) \
    do {                         \
    if ((p) != NULL) {           \
            RtlMemoryFree(p);    \
            p = NULL;            \
        }                        \
    } while(0);


#endif    /* _PVFS_MACROS_H */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
