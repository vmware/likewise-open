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
 *        defs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Definitions
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#define LWIO_DEFAULT_GLOBAL_CREDIT_LIMIT (250000)
#define LWIO_DEFAULT_CLIENT_CREDIT_LIMIT (5)

#define SRV_ELEMENTS_CONFIG_TABLE_INITIALIZER                   \
{                                                               \
    {                                                           \
        .pszName        = "GlobalCreditLimit",                  \
        .bUsePolicy     = FALSE,                                \
        .Type           = LwIoTypeDword,                        \
        .dwMin          = 1,                                    \
        .dwMax          = 3200000,                              \
        .pValue         = &pConfig->ulGlobalCreditLimit         \
    },                                                          \
    {                                                           \
        .pszName        = "ClientCreditLimit",                  \
        .bUsePolicy     = FALSE,                                \
        .Type           = LwIoTypeDword,                        \
        .dwMin          = 1,                                    \
        .dwMax          = 32,                                   \
        .pValue         = &pConfig->usClientCreditLimit         \
    },                                                          \
};

#define SRV_ELEMENTS_INCREMENT_STAT(stat, maxstat) \
{ \
    BOOLEAN bStatsInLock = FALSE; \
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bStatsInLock, &gSrvElements.statsLock); \
    if (++stat > maxstat) \
    { \
        maxstat = stat; \
    } \
    LWIO_UNLOCK_RWMUTEX(bStatsInLock, &gSrvElements.statsLock); \
}

#define SRV_ELEMENTS_DECREMENT_STAT(stat) \
{ \
    BOOLEAN bStatsInLock = FALSE; \
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bStatsInLock, &gSrvElements.statsLock); \
    stat--; \
    LWIO_UNLOCK_RWMUTEX(bStatsInLock, &gSrvElements.statsLock); \
}

#define SRV_ELEMENTS_INCREMENT_CONNECTIONS \
        SRV_ELEMENTS_INCREMENT_STAT(gSrvElements.stats.llNumConnections, \
                                    gSrvElements.stats.llMaxNumConnections)

#define SRV_ELEMENTS_DECREMENT_CONNECTIONS \
        SRV_ELEMENTS_DECREMENT_STAT(gSrvElements.stats.llNumConnections)

#define SRV_ELEMENTS_INCREMENT_SESSIONS \
        SRV_ELEMENTS_INCREMENT_STAT(gSrvElements.stats.llNumSessions, \
                                    gSrvElements.stats.llMaxNumSessions)

#define SRV_ELEMENTS_DECREMENT_SESSIONS \
        SRV_ELEMENTS_DECREMENT_STAT(gSrvElements.stats.llNumSessions)

#define SRV_ELEMENTS_INCREMENT_TREE_CONNECTS \
        SRV_ELEMENTS_INCREMENT_STAT(gSrvElements.stats.llNumTreeConnects, \
                                    gSrvElements.stats.llMaxNumTreeConnects)

#define SRV_ELEMENTS_DECREMENT_TREE_CONNECTS \
        SRV_ELEMENTS_DECREMENT_STAT(gSrvElements.stats.llNumTreeConnects)

#define SRV_ELEMENTS_INCREMENT_OPEN_FILES \
        SRV_ELEMENTS_INCREMENT_STAT(gSrvElements.stats.llNumOpenFiles, \
                                    gSrvElements.stats.llMaxNumOpenFiles)

#define SRV_ELEMENTS_DECREMENT_OPEN_FILES \
        SRV_ELEMENTS_DECREMENT_STAT(gSrvElements.stats.llNumOpenFiles)
