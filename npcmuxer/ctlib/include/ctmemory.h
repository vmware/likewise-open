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

#ifndef __CT_MEMORY_H__
#define __CT_MEMORY_H__

#include <ctstatus.h>
#include <sys/types.h>

#define CT_PTR_ADD(Pointer, Offset) \
    ((char*)(Pointer) + Offset)

#define CT_FIELD_OFFSET(Type, Field) \
    ((size_t)(&(((Type*)(0))->Field)))

#define CT_FIELD_RECORD(Pointer, Type, Field) \
    ((Type*)CT_PTR_ADD(Pointer, -((ssize_t)CT_FIELD_OFFSET(Type, Field))))

#define CT_ARRAY_SIZE(StaticArray) \
    (sizeof(StaticArray)/sizeof((StaticArray)[0]))

#define CT_SAFE_FREE(Pointer) \
    do { \
        if (Pointer) \
        { \
            CtFreeMemory(Pointer); \
            (Pointer) = NULL; \
        } \
    } while (0)

#define CT_SAFE_CLOSE_FD(Fd) \
    do { \
        if ((Fd) != -1) \
        { \
            close(Fd); \
            (Fd) = -1; \
        } \
    } while (0)

#define CT_SAFE_CLOSE_FD_WITH_STATUS(Fd, Status) \
    do { \
        (Status) = CT_STATUS_SUCCESS; \
        if ((Fd) != -1) \
        { \
            if (close(Fd) < 0) \
            { \
                (Status) = CtErrnoToStatus(errno); \
            } \
            else \
            { \
                (Fd) = -1; \
            } \
        } \
    } while (0)

CT_STATUS
CtAllocateMemory(
    OUT void** Pointer,
    IN size_t Size
    );

CT_STATUS
CtReallocMemory(
    IN OUT void** Pointer,
    IN size_t NewSize
    );

void
CtFreeMemory(
    IN OUT void* Pointer
    );

#endif /* __CT_MEMORY_H__ */
