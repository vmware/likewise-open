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

#ifndef __CTPREFDC_H__
#define __CTPREFDC_H__


/* This one uses lwiinfo -D | grep "^Name" | awk '{print $3} */
CENTERROR
CTGetShortDomainName(
    PSTR pszDomainName,
    PSTR * ppszShortDomainName
    );

/* This one uses lwiinfo -D | grep "^Alt_name" | awk '{print $3} */
CENTERROR
CTGetFullDomainName(
    PCSTR pszShortDomainName,
    PSTR* ppszDomainFQDN
    );

/* This one uses cat /var/lib/lwidentity/smb_krb5/krb5.conf.%s | grep default_realm | awk '{print $3}'*/
CENTERROR
CTGetDomainFQDN(
		PCSTR pszShortDomainName,
		PSTR* ppszDomainFQDN
		);

/* This one use cat %s | grep \"kdc\" | awk '{print $3}' */
CENTERROR
CTGetPreferredDCAddress(
    PSTR pszShortDomainName,
    PSTR * ppszDCAddress
    );

#endif /* __CTPREFDC_H__ */
