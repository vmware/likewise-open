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
 *        provider-main_p.h
 *
 * Abstract:
 *
 *        Likewise Password Storage (LWPS)
 * 
 *        TDB Storage Provider
 * 
 *        Main
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __PROVIDER_MAIN_H__
#define __PROVIDER_MAIN_H__

#include <stddef.h>
#include <sys/types.h>
#include <signal.h>
#include <tdb.h>

typedef struct __TDB_PROVIDER_CONTEXT 
{
	DWORD magic;
	TDB_CONTEXT *pTdb;	
} TDB_PROVIDER_CONTEXT, *PTDB_PROVIDER_CONTEXT;

typedef struct _MACHINE_ACCT_INFO {
    PSTR   pszDomainSID;
    PSTR   pszDomainName;
    PSTR   pszDomainDnsName;
    PSTR   pszHostName;
    PSTR   pszMachineAccountName;
    PSTR   pszMachineAccountPassword;
    time_t tPwdCreationTimestamp;
    time_t tPwdClientModifyTimestamp;
    UINT32 dwSchannelType;
} MACHINE_ACCT_INFO, *PMACHINE_ACCT_INFO;

VOID
FreeMachineAccountInfo(
	PMACHINE_ACCT_INFO pAcctInfo
	);

VOID
FreePasswordInfoStruct(
	PLWPS_PASSWORD_INFO pInfo
	);

#endif /* __PROVIDER_MAIN_H__ */
