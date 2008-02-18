/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

/* ex: set tabstop=4 expandtab shiftwidth=4: */
#include "ctbase.h"

CENTERROR
CTAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    )
{

    CENTERROR ceError = CENTERROR_SUCCESS;
    PVOID pMemory = NULL;

    /* malloc is supposed to return a non-NULL pointer when it is asked to
     * allocate 0 bytes of memory. Linux systems usually follow this rule.
     *
     * AIX does not.
     */
    if (dwSize == 0)
        dwSize = 1;

    pMemory = malloc(dwSize);
    if (!pMemory){
        ceError = CENTERROR_OUT_OF_MEMORY;
        *ppMemory = NULL;
    }else {
        memset(pMemory,0, dwSize);
        *ppMemory = pMemory;
    }
    return (ceError);
}

CENTERROR
CTReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PVOID pNewMemory = NULL;

    if (pMemory == NULL) {
        pNewMemory = malloc(dwSize);
        memset(pNewMemory, 0, dwSize);
    }else {
        pNewMemory = realloc(pMemory, dwSize);
    }
    if (!pNewMemory){
        ceError = CENTERROR_OUT_OF_MEMORY;
        *ppNewMemory = NULL;
    }else {
        *ppNewMemory = pNewMemory;
    }

    return(ceError);
}


void
CTFreeMemory(
    PVOID pMemory
    )
{
    free(pMemory);
    return;
}

