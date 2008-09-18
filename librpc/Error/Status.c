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

#include <lwrpc/ntstatus.h>
#include <lwrpc/winerror.h>
#include <lwrpc/errconv.h>
#include <ldaperr.h>

#include <stdlib.h>
#include <errno.h>


#define STATUS_CODE(status, werror, errno, desc)             \
    {status, werror, errno, #status, #werror, #errno, desc },
#define __ERROR_XMACRO__

struct table_entry
{
    NTSTATUS status;
    int werror;
    int uerror;
    const char* status_name;
    const char* werror_name;
    const char* uerror_name;
    const char* desc;
} status_table[] =
{
#include <lwrpc/error.inc>
    {-1, 0, 0}
};

#undef STATUS_CODE
#undef __ERROR_XMACRO__

typedef int (*predicate) (struct table_entry* e, void* data);

static int
match_status(struct table_entry* e, void *data)
{
    return e->status == *((NTSTATUS*) data);
}

static int
match_werror(struct table_entry* e, void *data)
{
    return e->werror == *((int*) data);
}

static int
match_uerror(struct table_entry* e, void *data)
{
    return e->uerror == *((int*) data);
}

static struct table_entry*
find(predicate pred, void* data)
{
    unsigned int i;

    for (i = 0; i < sizeof(status_table)/sizeof(status_table[0]); i++)
    {
        if (pred(&status_table[i], data))
            return &status_table[i];
    }

    return NULL;
}

int
NtStatusToWin32Error(NTSTATUS status)
{
    struct table_entry *e = find(match_status, &status);

    return e ? e->werror : -1;
}

int
NtStatusToErrno(NTSTATUS status)
{
    struct table_entry *e = find(match_status, &status);

    return e ? e->uerror : -1;
}

int
ErrnoToWin32Error(int uerror)
{
    struct table_entry *e = find(match_uerror, &uerror);

    return e ? e->werror : -1;
}

const char* NtStatusToName(NTSTATUS status)
{
    struct table_entry *e = find(match_status, &status);
    return (e) ? e->status_name : NULL;
}


const char* NtStatusToDesc(NTSTATUS status)
{
    struct table_entry *e = find(match_status, &status);
    return (e) ? e->desc : NULL;
}


const struct lderr_winerr* find_lderr(int lderr)
{
    unsigned int i;

    for (i = 0; ldaperr_winerr_map[i].lderrstr; i++) {
        if (ldaperr_winerr_map[i].lderr == lderr) {
            return &ldaperr_winerr_map[i];
        }
    }

    return NULL;
}


int
LdapErrToWin32Error(int lderr)
{
    const struct lderr_winerr *e = find_lderr(lderr);
    return (e) ? e->winerr : -1;
}

int
Win32ErrorToErrno(int winerr)
{
    struct table_entry *e = find(match_werror, &winerr);
    return (e) ? e->uerror : 0;
}

NTSTATUS
Win32ErrorToNtStatus(int winerr)
{
    struct table_entry *e = find(match_werror, &winerr);
    return (e) ? e->status : (NTSTATUS)-1;
}

const char
*Win32ErrorToName(int winerr)
{
    struct table_entry *e = find(match_werror, &winerr);
    return (e) ? e->werror_name : NULL;
}


const char
*Win32ErrorToDesc(int winerr)
{
    struct table_entry *e = find(match_werror, &winerr);
    return (e) ? e->desc : NULL;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
