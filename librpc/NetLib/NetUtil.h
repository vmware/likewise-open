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

#ifndef _NET_UTIL_H_
#define _NET_UTIL_H_

#include <lw/ntstatus.h>
#include <lwrpc/winerror.h>
#include <ldap.h>


#define goto_if_ntstatus_not_success(s, lbl) \
    if (s != STATUS_SUCCESS) {               \
        status = (s);                        \
        err = NtStatusToWin32Error((s));     \
        goto lbl;                            \
    }

#define goto_if_err_not_success(e, lbl) \
    if (e != ERROR_SUCCESS) {           \
        err = e;                        \
        goto lbl;                       \
    }

#define goto_if_lderr_not_success(e, lbl) \
    if (e != LDAP_SUCCESS) {              \
        lderr = e;                        \
        goto lbl;                         \
    }


#define goto_if_no_memory_ntstatus(p, lbl)   \
    if ((p) == NULL) {                       \
        status = STATUS_NO_MEMORY;           \
        goto lbl;                            \
    }

#define goto_if_no_memory_winerr(ptr, lbl) \
    if (ptr == NULL) {                     \
        err = ERROR_OUTOFMEMORY;           \
        goto_if_err_not_success(err, lbl); \
    }

#define goto_if_no_memory_lderr(p, lbl) \
    if ((p) == NULL) {                  \
        lderr = LDAP_NO_MEMORY;         \
        goto lbl;                       \
    }

#define goto_if_invalid_param_winerr(parm, lbl) \
    if ((parm) == NULL) {                       \
        err = ERROR_INVALID_PARAMETER;          \
        goto lbl;                               \
    }

#endif /* _NET_UTIL_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
