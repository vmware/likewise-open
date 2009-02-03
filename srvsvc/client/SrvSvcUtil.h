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

#ifndef _SRVSVC_UTIL_H_
#define _SRVSVC_UTIL_H_

//#include <lwrpc/ntstatus.h>
//#include <lwrpc/winerror.h>
#include <compat/rpcstatus.h>
#include <lw/ntstatus.h>
#include <types.h>

#define SAFE_FREE(ptr)  do { if (ptr) free(ptr); (ptr) = NULL; } while (0)

#define NTSTATUS_CODE(status) ((NTSTATUS)(0xc0000000 | (status)))

#define ERROR_SUCCESS 0
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_NOT_ENOUGH_MEMORY 8
#define ERROR_BAD_NET_RESP 58
#define ERROR_INVALID_PARAMETER 87
#define ERROR_INVALID_LEVEL 124
#define ERROR_INTERNAL_ERROR 1359
#define ERROR_BAD_DESCRIPTOR_FORMAT 1361
#define ERROR_INVALID_SUB_AUTHORITY 1335
#define ERROR_INVALID_ACL 1336
#define ERROR_INVALID_SID 1337
#define ERROR_INVALID_SECURITY_DESCR 1338

#define NERR_BASE 2100
#define NERR_DuplicateShare (NERR_BASE+18)
#define NERR_BufTooSmall (NERR_BASE+23)
#define NERR_NetNameNotFound (NERR_BASE+210)
#define NERR_FileIdNotFound (NERR_BASE+214)

#define goto_if_ntstatus_not_success(s, lbl) \
    if ((s) != STATUS_SUCCESS) {             \
        status = (s);                        \
        goto lbl;                            \
    }

#define goto_if_err_not_success(e, lbl)      \
    if ((e) != ERROR_SUCCESS) {              \
        status= (e);                         \
        goto lbl;                            \
    }

#define goto_if_rpcstatus_not_success(s, lbl) \
    if ((s) != RPC_S_OK) {                    \
        rpcstatus = (s);                      \
        goto lbl;                             \
    }

#define goto_if_no_memory_ntstatus(p, lbl)   \
    if ((p) == NULL) {                       \
        status = STATUS_NO_MEMORY;           \
        goto lbl;                            \
    }

#define goto_if_invalid_param_ntstatus(p, lbl) \
    if ((p) == NULL) {                       \
        status = STATUS_INVALID_PARAMETER;   \
        goto lbl;                            \
    }

#define goto_if_invalid_param_err(p, lbl) \
    if ((p) == NULL) {                       \
        status = ERROR_INVALID_PARAMETER;   \
        goto lbl;                            \
    }


#define DCERPC_CALL(fn_call)                     \
    do {                                         \
        dcethread_exc *dceexc = NULL;            \
                                                 \
        DCETHREAD_TRY                            \
        {                                        \
            status = fn_call;                    \
        }                                        \
        DCETHREAD_CATCH_ALL(dceexc)              \
        {                                        \
            /* TODO:                             \
               Implement DCE/RPC exc -> NTSTATUS \
               mapping logic as soon as we have  \
               more information about it */      \
            status = dceexc->match.value;        \
        }                                        \
        DCETHREAD_ENDTRY;                        \
    } while (0);


#endif /* _SRVSVC_UTIL_H_ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
