/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

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

#include "djapi.h"

#include "domainjoin.h"

#include <stdio.h>

void
DJQuery(
    char **computer, 
    char **domain,
    DJOptions* options,
    LWException** exc
    )
{
    PDOMAINJOININFO info = NULL;

    LW_CLEANUP_CTERR(exc,
		     QueryInformation(&info));
    
    if (info->pszName)
    {
	LW_CLEANUP_CTERR(exc,
			 CTAllocateString(info->pszName, computer));
    }
    else
    {
	*computer = NULL;
    }

    if (info->pszDomainName)
    {
	LW_CLEANUP_CTERR(exc,
			 CTAllocateString(info->pszDomainName, domain));
    }
    else
    {
	*domain = NULL;
    }
    
cleanup:
    
    if (info)
    {
	FreeDomainJoinInfo(info);
    }
}

void
DJRenameComputer(
    const char* computer,
    const char* domain,
    DJOptions* options,
    LWException** exc
    )
{
    LW_CLEANUP_CTERR(exc, DJSetComputerName((char*) computer, (char*) domain));

cleanup:

    return;
}

void
DJLeaveDomain(
    DJOptions* options,
    LWException** exc
    )
{
    LW_CLEANUP_CTERR(exc, JoinWorkgroup("WORKGROUP", "empty", ""));

cleanup:
    
    return;
}
