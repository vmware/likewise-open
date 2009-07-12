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

#include "includes.h"


WINERR
WNetCancelConnection2(
    wchar16_t *name,
    uint16 flags,
    bool force
    )
{
    WINERR err = 0;
    char *hostname = NULL;
    char *slash = NULL;

    if (name == NULL)
    {
        err = ERROR_INVALID_PARAMETER;
        goto done;
    }

    hostname = awc16stombs(name + 2);
    if (hostname == NULL)
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }

    slash = strchr(hostname, '\\');

    if (!slash)
    {
        err = ERROR_INVALID_PARAMETER;
        goto done;
    }

    *slash = '\0';

#if 0
    if (result = NpcClearAuthInfo(hostname))
    {
        status = ErrnoToWin32Error(result);
        goto done;
    }
#endif

done:
    if (hostname) {
        free(hostname);
    }

    return err;

#if 0
    /* ! ! ! FIXME-LSMB ! ! !
       Tear down impersonation/connection here
    */
    return 0;
#endif
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
