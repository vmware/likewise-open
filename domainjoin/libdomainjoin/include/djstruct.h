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

#ifndef __DJSTRUCT_H__
#define __DJSTRUCT_H__

typedef struct __DOMAINJOININFO {

	PSTR pszName;
	PSTR pszDescription;
	PSTR pszDnsDomain;
	PSTR pszDomainName;	/* Null if not joined  */
	PSTR pszDomainShortName;	/* Null if not joined  */
	PSTR pszWorkgroupName;	/* Null if not joined  */
	PSTR pszLogFilePath;	/* Null if not logging */

} DOMAINJOININFO, *PDOMAINJOININFO;

typedef struct __PROCBUFFER {
	BOOLEAN bEndOfFile;
	CHAR szOutBuf[MAX_PROC_BUF_LEN];
	DWORD dwOutBytesRead;
	CHAR szErrBuf[MAX_PROC_BUF_LEN];
	DWORD dwErrBytesRead;
} PROCBUFFER, *PPROCBUFFER;

typedef enum {
	SERVER_LICENSE = 0x1,
	WORKSTATION_LICENSE = 0x2,
	EVALUATION_LICENSE = 0x4,
	SITE_LICENSE = 0x8
} LicenseMagic;

typedef struct __LICENSEINFO {
	uint16_t crc16;
	BYTE licenseVersion;
	BYTE productId;
	uint16_t variable;
	uint32_t licenseExpiration;
	LicenseMagic licenseMagic;

} LICENSEINFO, *PLICENSEINFO;

#endif				/* __DJSTRUCT_H__ */
