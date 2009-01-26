/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        ntstatus.c
 *
 * Abstract:
 *
 *        NT status codes
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "includes.h"

typedef struct _TABLE_ENTRY
{
    NTSTATUS code;
    int unixErrno;
    PCSTR pszSymbolicName;
    PCSTR pszDescription;
} const TABLE_ENTRY, *PTABLE_ENTRY;

static
PTABLE_ENTRY
LwNtLookupCode(
    NTSTATUS code
    );

#define NTSTATUS_CODE(code, err, desc) { code, err, #code, desc },
static
TABLE_ENTRY LwNtStatusCodeTable[] =
{
#include "ntstatus-table.h"
    {-1, -1, NULL, NULL}
};
#undef NTSTATUS_CODE

PCSTR
LwNtStatusToSymbolicName(
    NTSTATUS code
    )
{
    PTABLE_ENTRY pEntry = LwNtLookupCode(code);

    if (pEntry)
    {
        return pEntry->pszSymbolicName;
    }
    else
    {
        return "UNKNOWN";
    }
}

PCSTR
LwNtStatusToDescription(
    NTSTATUS code
    )
{
    PTABLE_ENTRY pEntry = LwNtLookupCode(code);

    if (pEntry)
    {
        if (pEntry->pszDescription)
        {
            return pEntry->pszDescription;
        }
        else
        {
            return "No description available";
        }
    }
    else
    {
        return "Unknown error";
    }
}

int
LwNtStatusToUnixErrno(
    NTSTATUS code
    )
{
    PTABLE_ENTRY pEntry = LwNtLookupCode(code);

    if (pEntry)
    {
        return pEntry->unixErrno;
    }
    else
    {
        return -1;
    }
}

static
PTABLE_ENTRY
LwNtLookupCode(
    NTSTATUS code
    )
{
    ULONG index;

    for (index = 0; index < sizeof(LwNtStatusCodeTable) / sizeof(*LwNtStatusCodeTable); index++)
    {
        if (LwNtStatusCodeTable[index].code == code)
        {
            return &LwNtStatusCodeTable[index];
        }
    }

    return NULL;
}

