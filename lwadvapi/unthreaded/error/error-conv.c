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

#include <stdlib.h>
#include <errno.h>

#include <lw/ntstatus.h>
#include <winerror.h>
#include <winerror-conv.h>
#include <ldaperror-table.h>


#define STATUS_CODE(status, werror, errno, desc)             \
    {status, werror, errno, #status, #werror, #errno, desc },
#define __ERROR_XMACRO__

struct table_entry
{
    NTSTATUS ntStatus;
    DWORD    werror;
    int      uerror;
    PCSTR    pszStatusName;
    PCSTR    pszWinerrName;
    PCSTR    pszErrnoName;
    PCSTR    pszDescription;

} status_table[] =
{
#include <winerror-table.h>
    {-1, 0, 0}
};

#undef STATUS_CODE
#undef __ERROR_XMACRO__

typedef int (*predicate) (struct table_entry* e, void* data);

static int
match_status(
    struct table_entry* e,
    void *data
    )
{
    return e->ntStatus == *((NTSTATUS*) data);
}


static int
match_werror(
    struct table_entry* e,
    void *data
    )
{
    return e->werror == *((int*) data);
}


static int
match_uerror(
    struct table_entry* e,
    void *data
    )
{
    return e->uerror == *((int*) data);
}


static struct table_entry*
find(
    predicate pred,
    void* data
    )
{
    unsigned int i;

    for (i = 0; i < sizeof(status_table)/sizeof(status_table[0]); i++)
    {
        if (pred(&status_table[i], data))
            return &status_table[i];
    }

    return NULL;
}


DWORD
LwNtStatusToWin32Error(
    NTSTATUS ntStatus
    )
{
    struct table_entry *e = find(match_status, &ntStatus);
    return e ? e->werror : -1;
}


int
LwNtStatusToErrno(
    NTSTATUS ntStatus
    )
{
    struct table_entry *e = find(match_status, &ntStatus);
    return e ? e->uerror : -1;
}


NTSTATUS
LwErrnoToNtStatus(
    int uerror
    )
{
    struct table_entry *e = find(match_uerror, &uerror);
    return (e) ? e->ntStatus : (NTSTATUS)-1;
}


DWORD
LwErrnoToWin32Error(
    int uerror
    )
{
    struct table_entry *e = find(match_uerror, &uerror);
    return e ? e->werror : -1;
}


PCSTR
LwNtStatusToName(
    NTSTATUS ntStatus
    )
{
    struct table_entry *e = find(match_status, &ntStatus);
    return (e) ? e->pszStatusName : NULL;
}


PCSTR
LwNtStatusToDesc(
    NTSTATUS ntStatus
    )
{
    struct table_entry *e = find(match_status, &ntStatus);
    return (e) ? e->pszDescription : NULL;
}


const struct lderr_winerr*
find_lderr(
    int lderr
    )
{
    unsigned int i;

    for (i = 0; ldaperr_winerr_map[i].pszLderrStr; i++)
    {
        if (ldaperr_winerr_map[i].lderr == lderr)
        {
            return &ldaperr_winerr_map[i];
        }
    }

    return NULL;
}


DWORD
LwLdapErrToWin32Error(
    int lderr
    )
{
    const struct lderr_winerr *e = find_lderr(lderr);
    return (e) ? e->winerr : -1;
}

int
LwErrnoToLdapErr(
    int uerror
    )
{
    unsigned int i = 0;
    DWORD dwError = ErrnoToWin32Error(uerror);

    for (i = 0; ldaperr_winerr_map[i].pszLderrStr; i++)
    {
        if (ldaperr_winerr_map[i].winerr == dwError)
        {
            return ldaperr_winerr_map[i].lderr;
        }
    }

    return -1;
}

int
LwWin32ErrorToErrno(
    DWORD winerr
    )
{
    struct table_entry *e = find(match_werror, &winerr);
    return (e) ? e->uerror : 0;
}

NTSTATUS
LwWin32ErrorToNtStatus(
    DWORD winerr
    )
{
    struct table_entry *e = find(match_werror, &winerr);
    return (e) ? e->ntStatus : (NTSTATUS)-1;
}


PCSTR
LwWin32ErrorToName(
    int winerr
    )
{
    struct table_entry *e = find(match_werror, &winerr);
    return (e) ? e->pszWinerrName : NULL;
}


PCSTR
LwWin32ErrorToDesc(
    DWORD winerr
    )
{
    struct table_entry *e = find(match_werror, &winerr);
    return (e) ? e->pszDescription : NULL;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
