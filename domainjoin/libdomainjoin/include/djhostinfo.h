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

#ifndef __DJ_HOSTINFO_H__
#define __DJ_HOSTINFO_H__

#include "djmodule.h"

CENTERROR
DJGetComputerName(
    PSTR* ppszComputerName
    );

CENTERROR
DJGetFQDN(
    PSTR *shortName,
    PSTR *fqdn
    );

CENTERROR
DJSetComputerName(
    PSTR pszComputerName,
    PSTR pszDnsDomainName
    );

CENTERROR
DJIsValidComputerName(
    PSTR pszComputerName,
    PBOOLEAN bIsValid
    );

void DJCheckValidComputerName(
    PSTR pszComputerName,
    LWException **exc
    );

CENTERROR
DJIsDomainNameResolvable(
    PSTR pszDomainName,
    PBOOLEAN bIsResolvable
    );

CENTERROR
DJGetFinalFqdn(
    const JoinProcessOptions *options,
    PSTR *fqdn
    );

extern const JoinModule DJSetHostname;

#endif /* __DJ_HOSTINFO_H__ */
