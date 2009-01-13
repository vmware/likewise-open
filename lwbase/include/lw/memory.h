/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        memory.h
 *
 * Abstract:
 *
 *        Likewise Memory Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
            Sriram Nambakam (snambakam@likewise.com)
 */
#ifndef __LW_MEMORY_H__
#define __LW_MEMORY_H__

#ifndef LW_SAFE_FREE_MEMORY \

#define LW_SAFE_FREE_MEMORY(pMemory) \
    if (pMemory) { \
        LWFreeMemory(pMemory); \
        (pMemory) = NULL; \
    }

#endif /* LW_SAFE_FREE_MEMORY */


NTSTATUS
LWAllocateMemory(
    size_t size,
    PVOID* ppMemory
    );

NTSTATUS
LWReallocMemory(
    PVOID  pMemory,
    PVOID* ppNewMemory,
    size_t size
    );

VOID
LWFreeMemory(
    PVOID pMemory
    );

#endif /* __LW_MEMORY_H__ */

