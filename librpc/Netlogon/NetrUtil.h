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
 * Abstract: Netlogon interface utility macros (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _NETR_UTIL_H_
#define _NETR_UTIL_H_

#include <lwrpc/ntstatus.h>
#include <lwrpc/winerror.h>
#include <compat/rpcstatus.h>


#define goto_if_ntstatus_not_success(s, lbl) \
    if ((s) != STATUS_SUCCESS) {             \
        status = s;                          \
        goto lbl;                            \
    }

#define goto_if_winerr_not_success(e, lbl)   \
    if ((e) != ERROR_SUCCESS) {              \
        err = (e);                           \
        goto lbl;                            \
    }

#define goto_if_rpcstatus_not_success(s, lbl) \
    if ((s) != RPC_S_OK) {                    \
        rpcstatus = s;                        \
        goto lbl;                             \
    }

#define goto_if_no_memory_ntstatus(p, lbl)   \
    if ((p) == NULL) {                       \
        status = STATUS_NO_MEMORY;           \
        goto lbl;                            \
    }

#define goto_if_no_memory_winerr(p, lbl)     \
    if ((p) == NULL) {                       \
        err = ERROR_OUTOFMEMORY;             \
        goto lbl;                            \
    }

#define goto_if_invalid_param_ntstatus(p, lbl) \
    if ((p) == NULL) {                       \
        status = STATUS_INVALID_PARAMETER;   \
        goto lbl;                            \
    }

#define goto_if_invalid_param_winerr(p, lbl) \
    if ((p) == NULL) {                       \
        err = ERROR_INVALID_PARAMETER;       \
        goto lbl;                            \
    }

#define goto_if_no_memory_rpcstatus(p, lbl)  \
    if ((p) == NULL) {                       \
        rpcstatus = RPC_S_OUT_OF_MEMORY;     \
        goto lbl;                            \
    }

#define goto_if_invalid_param_rpcstatus(p, lbl) \
    if ((p) == NULL) {                          \
        rpcstatus = RPC_S_INVALID_ARG;          \
        goto lbl;                               \
    }


#define DCERPC_CALL(status, fn_call)             \
    do {                                         \
        dcethread_exc *dceexc;                   \
                                                 \
        DCETHREAD_TRY                            \
        {                                        \
            dceexc = NULL;                       \
            (status) = fn_call;                  \
        }                                        \
        DCETHREAD_CATCH_ALL(dceexc)              \
        {                                        \
            /* TODO:                             \
               Implement DCE/RPC exc -> NTSTATUS \
               mapping logic as soon as we have  \
               more information about it */      \
            (status) = dceexc->match.value;      \
        }                                        \
        DCETHREAD_ENDTRY;                        \
    } while (0);


#endif /* _NETR_UTIL_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
