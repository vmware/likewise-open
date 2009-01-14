/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        memory.h
 *
 * Abstract:
 *
 *        Memory utilities
 *
 */

#ifndef __LW_MEMORY_H__
#define __LW_MEMORY_H__

#include <stddef.h>
#include <lw/types.h>

#define LW_SAFE_FREE_MEMORY(pMemory)            \
    do                                          \
    {                                           \
        if ((pMemory))                          \
        {                                       \
            LwRtlFreeMemory((pMemory));         \
            ((pMemory)) = NULL;                 \
        }                                       \
    } while (0)

LW_NTSTATUS
LwRtlAllocateMemory(
    size_t size,
    LW_PVOID* ppMemory
    );

LW_NTSTATUS
LwRtlReallocMemory(
    LW_PVOID  pMemory,
    LW_PVOID* ppNewMemory,
    size_t size
    );

LW_VOID
LwRtlFreeMemory(
    LW_PVOID pMemory
    );

#ifndef LW_STRICT_NAMESPACE

#define RtlAllocateMemory     LwRtlAllocateMemory
#define RtlReallocMemory      LwRtlReallocMemory
#define RtlFreeMemory         LwRtlFreeMemory

/* FIXME: ensure this is safe, then uncomment it */
/*
#define SAFE_FREE_MEMORY      LW_SAFE_FREE_MEMORY
*/

#endif /* LW_STRICT_NAMESPACE */

#endif /* __LW_MEMORY_H__ */

