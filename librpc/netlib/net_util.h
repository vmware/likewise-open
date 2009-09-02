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

#ifndef _NET_UTIL_H_
#define _NET_UTIL_H_


#define BAIL_ON_NTSTATUS_ERROR(s)            \
    if ((s) != STATUS_SUCCESS) {             \
        status = (s);                        \
        err = NtStatusToWin32Error((s));     \
        goto error;                          \
    }

#define BAIL_ON_WINERR_ERROR(e)              \
    if ((e) != ERROR_SUCCESS) {              \
        err = (e);                           \
        goto error;                          \
    }

#define BAIL_ON_LDERR_ERROR(e)               \
    if ((e) != LDAP_SUCCESS) {               \
        lderr = (e);                         \
        goto error;                          \
    }

#define BAIL_ON_NO_MEMORY(p)                 \
    if ((p) == NULL) {                       \
        err = ERROR_OUTOFMEMORY;             \
        goto error;                          \
    }

#define goto_if_no_memory_lderr(p, lbl) \
    if ((p) == NULL) {                  \
        lderr = LDAP_NO_MEMORY;         \
        err = ERROR_OUTOFMEMORY;        \
        goto lbl;                       \
    }

#define BAIL_ON_INVALID_PTR(p)                 \
    if ((p) == NULL) {                         \
        status = STATUS_INVALID_PARAMETER;     \
        err = ERROR_INVALID_PARAMETER;         \
        goto error;                            \
    }


#if !defined(MACHPASS_LEN)
#define MACHPASS_LEN  (16)
#endif


#endif /* _NET_UTIL_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
