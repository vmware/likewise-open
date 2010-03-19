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
 *        Likewise Distributed File System Driver (DFS)
 *
 *        Common macros for error checking, etc....
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#ifndef _DFS_MACROS_H
#define _DFS_MACROS_H

#define DFS_LOG_HEADER          "DFS"

/* Developer cycle testing macros; Disabled for release */

/* #define _DFS_DEVELOPER_DEBUG 1 */

#ifdef _DFS_DEVELOPER_DEBUG
/* Insert macro for developing here */
#endif

/* Error checking macros */


/* Memory Macros */


#define DFS_ZERO_MEMORY(p)                     \
    do {                                        \
        if ((p) != NULL) {                      \
            memset(p, 0x0, sizeof(*p));          \
        }                                       \
    } while (0)

#define DFS_ALIGN_MEMORY(bytes, n)               \
    if ((bytes) % (n)) {                          \
        bytes += (n) - ((bytes) % (n));           \
    }

#define DFS_PTR_DIFF(old,new)  ((size_t)((new)-(old)))

#define DFS_IS_DIR(pCcb)       \
    (((pCcb)->CreateOptions & FILE_DIRECTORY_FILE) == FILE_DIRECTORY_FILE)

#define DFS_CSTRING_NON_NULL(s)    ((s) && (*(s)))

#endif    /* _DFS_MACROS_H */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
