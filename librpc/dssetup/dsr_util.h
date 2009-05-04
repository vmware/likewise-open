/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Abstract: Lsa interface utility macros (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _DSR_UTIL_H_
#define _DSR_UTIL_H_

#include <lw/ntstatus.h>
#include <lwrpc/winerror.h>
#include <compat/rpcstatus.h>


#define BAIL_ON_WINERR_ERROR(err)             \
    if ((err) != ERROR_SUCCESS) {             \
        err = (err);                          \
        goto error;                           \
    }

#define BAIL_ON_NTSTATUS_ERROR(status)       \
    if ((status) != STATUS_SUCCESS) {        \
        status = (status);                   \
        goto error;                          \
    }

#define BAIL_ON_RPCSTATUS_ERROR(st)           \
    if ((st) != RPC_S_OK) {                   \
        rpcstatus = (st);                     \
        goto error;                           \
    }

#define BAIL_ON_NO_MEMORY(ptr)                \
    if ((ptr) == NULL) {                      \
        status = STATUS_NO_MEMORY;            \
        goto error;                           \
    }

#define BAIL_ON_NO_MEMORY_RPCSTATUS(ptr)         \
    if ((ptr) == NULL) {                         \
        rpcstatus = RPC_S_OUT_OF_MEMORY;         \
        goto error;                              \
    }

#define BAIL_ON_NULL_PARAM(p)                    \
    if ((p) == NULL) {                           \
        status = STATUS_INVALID_PARAMETER;       \
        goto cleanup;                            \
    }

#define BAIL_ON_NULL_PARAM_RPCSTATUS(p)          \
    if ((p) == NULL) {                           \
        rpcstatus = RPC_S_INVALID_ARG;           \
        goto cleanup;                            \
    }

#define DCERPC_CALL(fn_call)                     \
    do {                                         \
        dcethread_exc *dceexc;                   \
                                                 \
        DCETHREAD_TRY                            \
        {                                        \
            dceexc = NULL;                       \
            err = fn_call;                       \
        }                                        \
        DCETHREAD_CATCH_ALL(dceexc)              \
        {                                        \
            /* TODO:                             \
               Implement DCE/RPC exc -> NTSTATUS \
               mapping logic as soon as we have  \
               more information about it */      \
            err = dceexc->match.value;           \
        }                                        \
        DCETHREAD_ENDTRY;                        \
    } while (0);


#endif /* _DSR_UTIL_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
