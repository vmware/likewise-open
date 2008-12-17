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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <lwrpc/types.h>
#include <lwrpc/winerror.h>
#include <wc16str.h>
#include <npc.h>
#include <npctypes.h>

#include <lwrpc/mpr.h>


int WNetAddConnection2(NETRESOURCE* netResource,
                       const wchar16_t* password16,
                       const wchar16_t* username16)
{
    const NPC_AUTH_FLAGS authflags = NPC_AUTH_FLAG_KERBEROS |
                                     NPC_AUTH_FLAG_FALLBACK |
                                     NPC_AUTH_FLAG_NO_ANONYMOUS;
    char *password = NULL;
    char *username = NULL;
    char* host = NULL;
    char* slash = NULL;
    char* resource = NULL;
    int result = 0;
    int winerr = ERROR_SUCCESS;

    if (netResource == NULL ||
        password16 == NULL ||
        username16 == NULL)
    {
        winerr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }


    password = awc16stombs(password16);
    if (password == NULL)
    {
        winerr = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    username = awc16stombs(username16);
    if (username == NULL)
    {
        winerr = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    host = awc16stombs(netResource->RemoteName + 2);
    if (host == NULL)
    {
        winerr = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    slash = strchr(host, '\\');
    if (!slash)
    {
        winerr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    resource = strdup(slash);
    if (resource == NULL)
    {
        winerr = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    *slash = '\0';
    
    if ((result = NpcConnectCheckCreds("np",
                                       host,
                                       "\\pipe\\srvsvc",
                                       NULL,
                                       authflags,
                                       username,
                                       password)))
    {
        winerr = ErrnoToWin32Error(result);
        goto cleanup;
    }

    if ((result = NpcSetAuthInfo(host,
                                 authflags,
                                 username,
                                 password)))
    {
        winerr = ErrnoToWin32Error(result);
        goto cleanup;
    }

cleanup:

    if (host)
    {
        free(host);
    }

    if (username)
    {
        free(username);
    }

    if (password)
    {
        free (password);
    }

    if (resource)
    {
        free(resource);
    }

    return winerr;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
