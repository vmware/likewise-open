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

#include <ctmemory.h>
#include <stdlib.h>
#include <string.h>


CT_STATUS
CtAllocateMemory(
    OUT void** Pointer,
    IN size_t Size
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    void* memory = NULL;

    /*
     * malloc is supposed to return a non-NULL pointer when it is asked to
     * allocate 0 bytes of memory. Linux systems usually follow this rule.
     *
     * AIX does not.
     */
    memory = malloc(CT_MAX(Size, 1));
    if (!memory)
    {
        status = CT_STATUS_OUT_OF_MEMORY;
    }

    memset(memory, 0, Size);

    *Pointer = memory;

    return status;
}

CT_STATUS
CtReallocMemory(
    IN OUT void** Pointer,
    IN size_t NewSize
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;

    if (!*Pointer)
    {
        status = CtAllocateMemory(Pointer, NewSize);
    }
    else
    {
        /*
         * realloc is supposed to return a non-NULL pointer when it is asked to
         * allocate 0 bytes of memory. Linux systems usually follow this rule.
         *
         * AIX does not.
         */
        void* memory = realloc(Pointer, CT_MAX(NewSize, 1));
        if (!memory)
        {
            status = CT_STATUS_OUT_OF_MEMORY;
        }
        *Pointer = memory;
    }

    return status;
}

void
CtFreeMemory(
    IN OUT void* Pointer
    )
{
    if (Pointer)
    {
        free(Pointer);
    }
}
