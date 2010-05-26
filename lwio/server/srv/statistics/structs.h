/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        globals.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Statistics Logging Module
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */
typedef struct _SRV_STAT_INFO
{
    LONG                      refCount;

    pthread_mutex_t           mutex;
    pthread_mutex_t*          pMutex;

    SRV_STATISTICS_USAGE_FLAG ulFlags;

    HANDLE                    hContext;

} SRV_STAT_INFO;

typedef struct _SRV_STATISTICS_CONFIG
{
    PSTR    pszProviderPath;

    BOOLEAN bEnableLogging;

} SRV_STATISTICS_CONFIG, *PSRV_STATISTICS_CONFIG;

typedef struct _SRV_STATISTICS_GLOBALS
{
    pthread_rwlock_t      mutex;
    pthread_rwlock_t*     pMutex;

    SRV_STATISTICS_CONFIG config;

    PVOID                 hModule;

    PLWIO_SRV_STAT_PROVIDER_FUNCTION_TABLE pStatFnTable;
    PFN_SHUTDOWN_SRV_STAT_PROVIDER         pfnShutdownStatProvider;

} SRV_STATISTICS_GLOBALS, *PSRV_STATISTICS_GLOBALS;
