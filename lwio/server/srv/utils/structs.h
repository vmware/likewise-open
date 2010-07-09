/*
 * Copyright Likewise Software    2004-2009
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
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        Utilities
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

typedef struct _SRV_LOG_FILTER_OP
{
    LONG           refCount;

    ULONG          ulOpcode;

    LWIO_LOG_LEVEL logLevel;

    struct _SRV_LOG_FILTER_OP* pNext;

} SRV_LOG_FILTER_OP, *PSRV_LOG_FILTER_OP;

typedef struct _SRV_LOG_FILTER
{
    struct sockaddr    clientAddress;
    SOCKLEN_T          ulClientAddressLength;

    LWIO_LOG_LEVEL     defaultLogLevel_smb1;
    PSRV_LOG_FILTER_OP pFilterList_smb1;

    LWIO_LOG_LEVEL     defaultLogLevel_smb2;
    PSRV_LOG_FILTER_OP pFilterList_smb2;

    struct _SRV_LOG_FILTER* pNext;

} SRV_LOG_FILTER, *PSRV_LOG_FILTER;

typedef struct _SRV_LOG_SPEC
{
    LONG            refCount;

    PSRV_LOG_FILTER pClientSpecList;

    PSRV_LOG_FILTER pDefaultSpec;

    DWORD           dwMaxRequestLogLength;

} SRV_LOG_SPEC, *PSRV_LOG_SPEC;

typedef struct _SRV_LOG_CONTEXT
{
    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    PSRV_LOG_SPEC   pLogSpec;

    PSRV_LOG_FILTER pCurFilter;

} SRV_LOG_CONTEXT;

typedef struct _SRV_LOG_FILTER_TOKEN
{
    SRV_LOG_FILTER_TOKEN_TYPE tokenType;
    PSTR                      pData;
    ULONG                     ulLength;
} SRV_LOG_FILTER_TOKEN, *PSRV_LOG_FILTER_TOKEN;

typedef struct _SRV_LOG_FILTER_LEX_STATE
{
    PSTR pszData;
    PSTR pszCursor;
} SRV_LOG_FILTER_LEX_STATE, *PSRV_LOG_FILTER_LEX_STATE;

typedef struct _SRV_UTILS_GLOBALS
{
    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    PSRV_LOG_SPEC     pLogSpec;

} SRV_UTILS_GLOBALS, *PSRV_UTILS_GLOBALS;
