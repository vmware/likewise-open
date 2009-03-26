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
 *  Copyright (C) Likewise Software. All rights reserved.
 *  
 *  Module Name:
 *
 *     provider-main.c
 *
 *  Abstract:
 *
 *        Likewise Password Storage (LWPS)
 *
 *        API to support TDB Password Storage
 *
 *  Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */

#include <config.h>

#include "util_sid.h"
#include "lwps-def.h"
#include "lwps-validate.h"
#include "lwps-logger.h"
#include "lwps-mem.h"
#include <string.h>
#include <stdio.h>

#define MAX_SID_STRING_LEN 1024

DWORD 
SidToString(
	PDOMAIN_SID pSid,
	PSTR *pszSidString
	)
{
	DWORD dwError = LWPS_ERROR_INTERNAL;
	CHAR pszSidStr[MAX_SID_STRING_LEN] = "";
	uint32_t dwAuthId = 0;
	int i = 0;

	BAIL_ON_INVALID_POINTER(pSid);
	BAIL_ON_INVALID_POINTER(pszSidString);

	dwAuthId = pSid->id_auth[5] +
		(pSid->id_auth[4] << 8) +
		(pSid->id_auth[3] << 16) +
		(pSid->id_auth[2] << 24);

	snprintf(pszSidStr, 
		 sizeof(pszSidStr)-strlen(pszSidStr),
		 "S-%d-%d", 
		 pSid->sid_rev_num,
		 dwAuthId);

	for (i=0; i<pSid->num_auths; i++) {
		CHAR pszAuth[12];

		snprintf(pszAuth, sizeof(pszAuth), "-%u", pSid->sub_auths[i]);		
		strncat(pszSidStr, pszAuth, sizeof(pszSidStr)-strlen(pszSidStr));
	}

	dwError = LwpsAllocateString(pszSidStr, pszSidString);
	BAIL_ON_LWPS_ERROR(dwError);	
		
	dwError = LWPS_ERROR_SUCCESS;

error:
	return dwError;	
}


DWORD
StringToSid(
	PCSTR pszSidString,
	PDOMAIN_SID pSid
	)
{
	DWORD dwError = LWPS_ERROR_INTERNAL;
	CHAR *pszStrToken = NULL;
	CHAR *pszStrNextToken = NULL;
	DWORD dwX;

	BAIL_ON_INVALID_POINTER(pSid);
	BAIL_ON_INVALID_POINTER(pszSidString);

	/* Some additional sanity checks on the SID string format */

	if ((strlen((const char*) pszSidString) < 2) 
	    || (pszSidString[0] != 's' && pszSidString[0] != 'S')
	    || (pszSidString[1] != '-'))
	{
		dwError = LWPS_ERROR_INVALID_SID;
		BAIL_ON_LWPS_ERROR(dwError);		
	}

	/* Revision */

	pszStrToken = (PSTR)pszSidString+2;
	dwX = (DWORD)strtol(pszStrToken, &pszStrNextToken, 10);
	if ((dwX == 0) || !pszStrNextToken || (pszStrNextToken[0] != '-')) {
		dwError = LWPS_ERROR_INVALID_SID;
		BAIL_ON_LWPS_ERROR(dwError);
	}
	pSid->sid_rev_num = (uint8_t)dwX;

	/* Id Auth */

	pszStrToken = pszStrNextToken + 1;
	dwX = (DWORD)strtol(pszStrToken, &pszStrNextToken, 10);
	if ((dwX == 0) || !pszStrNextToken || (pszStrNextToken[0] != '-')) {
		dwError = LWPS_ERROR_INVALID_SID;
		BAIL_ON_LWPS_ERROR(dwError);
	}

	pSid->id_auth[5] = (dwX & 0x000000FF);
	pSid->id_auth[4] = (dwX & 0x0000FF00) >> 8;
	pSid->id_auth[3] = (dwX & 0x00FF0000) >> 16;
	pSid->id_auth[2] = (dwX & 0xFF000000) >> 24;
	pSid->id_auth[1] = 0;
	pSid->id_auth[0] = 0;

	/* Subauths */

	pSid->num_auths = 0;
	do {
		pszStrToken = pszStrNextToken + 1;

		dwX = (DWORD)strtol(pszStrToken, &pszStrNextToken, 10);
		if (dwX == 0) {			
			break;
		}
		
		pSid->sub_auths[pSid->num_auths++] = dwX;

		if (!pszStrNextToken || (pszStrNextToken[0] != '-')) {
			break;
		}

	} while (pSid->num_auths < MAXSUBAUTHS);

	/* Check for a premature end to the above loop */

	if (pszStrNextToken && (pszStrNextToken[0] != '\0')) {
		dwError = LWPS_ERROR_INVALID_SID;
		BAIL_ON_LWPS_ERROR(dwError);
	}

	dwError = LWPS_ERROR_SUCCESS;

error:

	return dwError;	
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

