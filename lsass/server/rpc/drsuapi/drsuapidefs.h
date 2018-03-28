/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        Drsuapidefs.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Drsuapi rpc server definitions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *          Adam Bernstein (abernstein@vmware.com)
 */

#ifndef _DRSUAPISRVDEFS_H_
#define _DRSUAPISRVDEFS_H_

#define LSA_RPC_DIR                              CACHEDIR "/rpc"

#define DRSUAPI_RPC_CFG_DEFAULT_LPC_SOCKET_PATH     LSA_RPC_DIR "/lsass"
#define DRSUAPI_RPC_CFG_DEFAULT_LOGIN_SHELL         "/bin/sh"
#define DRSUAPI_RPC_CFG_DEFAULT_HOMEDIR_PREFIX      "/home"
#define DRSUAPI_RPC_CFG_DEFAULT_HOMEDIR_TEMPLATE    "%H/%U"

#define LSASS_KRB5_CACHE_PATH                    "FILE:" CACHEDIR "/krb5cc_lsass"

typedef enum  {
  DS_UNKNOWN_NAME             = 0,
  DS_FQDN_1779_NAME           = 1,
  DS_NT4_ACCOUNT_NAME         = 2,
  DS_DISPLAY_NAME             = 3,
  DS_UNIQUE_ID_NAME           = 6,
  DS_CANONICAL_NAME           = 7,
  DS_USER_PRINCIPAL_NAME      = 8,
  DS_CANONICAL_NAME_EX        = 9,
  DS_SERVICE_PRINCIPAL_NAME   = 10,
  DS_SID_OR_SID_HISTORY_NAME  = 11,
  DS_DNS_DOMAIN_NAME          = 12
} DS_NAME_FORMAT;


#define BAIL_ON_NTSTATUS_ERROR(status)                   \
    do {                                                 \
        if ((status) != STATUS_SUCCESS) {                \
            LSA_LOG_DEBUG("Error at %s:%d code: %s "     \
                          "(0x%08x)",                    \
                          __FILE__, __LINE__,            \
                          LwNtStatusToName((status)),    \
                          (status));                     \
            goto error;                                  \
        }                                                \
    } while (0)



#define BAIL_ON_NO_MEMORY(ptr)                           \
    do {                                                 \
        if ((ptr) == NULL) {                             \
            ntStatus = STATUS_NO_MEMORY;                 \
            LSA_LOG_ERROR("Error: out of memory");       \
            goto error;                                  \
        }                                                \
    } while (0)


#define BAIL_ON_INVALID_PTR(ptr)                         \
    do {                                                 \
        if (ptr == NULL) {                               \
            ntStatus = STATUS_INVALID_PARAMETER;         \
            LSA_LOG_ERROR("Error: invalid pointer");     \
            goto error;                                  \
        }                                                \
    } while (0)


#define BAIL_ON_INVALID_PARAMETER(cond)                  \
    do {                                                 \
        if (!(cond)) {                                   \
            ntStatus = STATUS_INVALID_PARAMETER;         \
            LSA_LOG_ERROR("Error: invalid parameter");   \
            goto error;                                  \
        }                                                \
    } while (0)


#define GLOBAL_DATA_LOCK(locked)                         \
    do {                                                 \
        int ret = 0;                                     \
        ret = pthread_mutex_lock(&gDrsuapiSrvDataMutex);    \
        if (ret) {                                       \
            dwError = LwErrnoToWin32Error(ret);          \
            BAIL_ON_LSA_ERROR(dwError);                  \
        } else {                                         \
            (locked) = 1;                                \
        }                                                \
    } while (0)


#define GLOBAL_DATA_UNLOCK(locked)                       \
    do {                                                 \
        int ret = 0;                                     \
        if (!locked) break;                              \
        ret = pthread_mutex_unlock(&gDrsuapiSrvDataMutex);  \
        if (ret && dwError == STATUS_SUCCESS) {          \
            dwError = LwErrnoToWin32Error(ret);          \
            BAIL_ON_LSA_ERROR(dwError);                  \
        } else {                                         \
            (locked) = 0;                                \
        }                                                \
    } while (0)



#endif /* _DRSUAPISRVDEFS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
