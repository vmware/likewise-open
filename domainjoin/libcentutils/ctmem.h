/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007.  
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTMEM_H__
#define __CTMEM_H__

CENTERROR CTAllocateMemory(DWORD dwSize, PVOID * ppMemory);

CENTERROR CTReallocMemory(PVOID pMemory, PVOID * ppNewMemory, DWORD dwSize);

#define CT_SAFE_FREE_MEMORY(mem) \
    do { if (mem) { CTFreeMemory(mem); (mem) = NULL; } } while (0)

void CTFreeMemory(PVOID pMemory);

#endif				/* __CTMEM_H__ */
