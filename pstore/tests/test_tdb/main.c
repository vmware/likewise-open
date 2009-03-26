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
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Password Storage (LWPS) 
 *        
 *        Test Program for exercising SqlDBSetPwdEntry
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "lwps-utils.h"
#include "lwps/lwps.h"
#include "lwps-provider.h"
#include "lwps-validate.h"
#include "lwps-logger.h"

DWORD
LwpsInitializeProvider(
	PCSTR pszConfigFilePath,
	PSTR* ppszName,
	PLWPS_PROVIDER_FUNC_TABLE* ppFnTable
	);
DWORD
LwpsShutdownProvider(
	PSTR pszName,
	PLWPS_PROVIDER_FUNC_TABLE pFnTable
	);


int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;
    PSTR pszProviderName = NULL;
    HANDLE hTdb = (HANDLE)NULL;
    PLWPS_PROVIDER_FUNC_TABLE pFuncs = NULL;
    PLWPS_PASSWORD_INFO pPassInfo = NULL;
    LWPS_PASSWORD_INFO PwInfo;
    
    if (argc < 2) {
	    printf("Usage: test-tdb <DOMAIN> <Password>\n");
	    return 1;
    }

    lwps_init_logging_to_file(LOG_LEVEL_VERBOSE, TRUE, "");

    memset(&PwInfo, 0x0, sizeof(PwInfo));
    
    dwError = LwpsInitializeProvider("test-tdb.conf", 
				     &pszProviderName, 
				     &pFuncs);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = pFuncs->pFnOpenProvider(&hTdb);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(argv[1], &PwInfo.pwszDomainName);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsMbsToWc16s(argv[2], &PwInfo.pwszMachinePassword);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = pFuncs->pFnWritePassword(hTdb, &PwInfo);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = pFuncs->pFnReadPasswordByDomainName(hTdb, argv[1], &pPassInfo);
    BAIL_ON_LWPS_ERROR(dwError);    

    dwError = pFuncs->pFnCloseProvider(hTdb);
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = LwpsShutdownProvider(pszProviderName, pFuncs);    
    BAIL_ON_LWPS_ERROR(dwError);
    
error:

    if (dwError != LWPS_ERROR_SUCCESS)
	    printf("Error! (0x%x)\n", dwError);
    else
	    printf("Success\n");
    
    return 0;
}


