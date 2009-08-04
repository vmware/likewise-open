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

#include "lwps-utils.h"
#include "lwps/lwps.h"
#include "lwps-provider.h"
#include "provider-main_p.h"

#define FREE_PARAM_STRING(x) \
    if (x != NULL) { \
         LwpsFreeMemory(x); \
         x = NULL; \
    }

typedef struct __Tdb_providers_Params {
    BOOLEAN bInitialized;
    pthread_rwlock_t mutex;
    PSTR pszDbPath;
} TDB_PROVIDER_PARAMS, *PTDB_PROVIDER_PARAMS;

static TDB_PROVIDER_PARAMS Globals;

/******************************************************************
 ******************************************************************/

DWORD TdbInitProviderParams(void)
{
    if (Globals.bInitialized == TRUE) {
        return LWPS_ERROR_SUCCESS;
    }

    pthread_rwlock_init(&Globals.mutex, NULL);

    Globals.bInitialized = TRUE;

    return LWPS_ERROR_SUCCESS;
}

/******************************************************************
 ******************************************************************/

DWORD TdbReleaseProviderParams(void)
{
    FREE_PARAM_STRING(Globals.pszDbPath);

    return LWPS_ERROR_SUCCESS;
}

/******************************************************************
 ******************************************************************/

DWORD TdbSetDbPath(PCSTR pszPath)
{
    DWORD dwError = LWPS_ERROR_INTERNAL;

    FREE_PARAM_STRING(Globals.pszDbPath);

    dwError = LwpsAllocateString(pszPath, &Globals.pszDbPath);
    BAIL_ON_LWPS_ERROR(dwError);

error:
    return dwError;
}


/******************************************************************
 ******************************************************************/

DWORD TdbGetDbPath(PSTR *pszPath)
{
    DWORD dwError = LWPS_ERROR_INTERNAL;

    if (Globals.pszDbPath == NULL) {
        dwError = LWPS_ERROR_DATA_ERROR;
        BAIL_ON_LWPS_ERROR(dwError);
    }

    *pszPath = Globals.pszDbPath;
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
