/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        rtlmemory.h
 *
 * Abstract:
 *
 *        Likewise RTL Memory API
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __RTL_MEMORY_H__
#define __RTL_MEMORY_H__

#include <lw/types.h>
#include <lw/attrs.h>
#include <lw/ntstatus.h>

VOID
LwRtlMemoryZero(
    LW_IN LW_OUT LW_PVOID pMemory,
    LW_IN size_t Size
    );

PVOID
LwRtlMemoryAllocate(
    LW_IN size_t Size
    );

VOID
LwRtlMemoryFree(
    LW_IN LW_OUT LW_PVOID pMemory
    );

#define LW_RTL_ALLOCATE(ppMemory, Type, Size) \
    ( (*(ppMemory)) = (Type*) LwRtlMemoryAllocate(Size), (*(ppMemory)) ? LW_NT_STATUS_SUCCESS : LW_NT_STATUS_INSUFFICIENT_RESOURCES )

#define LW_RTL_FREE(ppMemory) \
    do { \
        if (*(ppMemory)) \
        { \
            LwRtlMemoryFree(*(ppMemory)); \
            (*(ppMemory)) = NULL; \
        } \
    } while (0)

#ifndef LW_STRICT_NAMESPACE

#define RtlMemoryZero     LwRtlMemoryZero
#define RtlMemoryAllocate LwRtlMemoryAllocate
#define RtlMemoryFree     LwRtlMemoryFree

#define RTL_ALLOCATE(ppMemory, Type, Size) \
    LW_RTL_ALLOCATE(ppMemory, Type, Size)

#define RTL_FREE(ppMemory) \
    LW_RTL_FREE(ppMemory)

#endif /* LW_STRICT_NAMESPACE */

#endif /* __RTL_MEMORY_H__ */
