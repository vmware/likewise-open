/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ipc_state.c
 *
 * Abstract:
 *
 *        Likewise Security and Authorization Subsystem (LSASS)
 *
 *        Server Connection State API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

static
DWORD
LsaSrvIpcCheckPermissions(
    LWMsgSecurityToken* token,
    uid_t* puid,
    gid_t* pgid
    )
{
    DWORD dwError = 0;
    uid_t euid;
    gid_t egid;

    if (strcmp(lwmsg_security_token_get_type(token), "local"))
    {
        LSA_LOG_WARNING("Unsupported authentication type");
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_local_token_get_eid(token, &euid, &egid));
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_VERBOSE("Permission granted for (uid = %i, gid = %i) to open LsaIpcServer",
                    (int) euid,
                    (int) egid);

    *puid = euid;
    *pgid = egid;

error:
    return dwError;
}

void
LsaSrvIpcDestructSession(
    LWMsgSecurityToken* pToken,
    void* pSessionData
    )
{
    LsaSrvCloseServer(pSessionData);
}

LWMsgStatus
LsaSrvIpcConstructSession(
    LWMsgSecurityToken* pToken,
    void* pData,
    void** ppSessionData
    )
{
    DWORD dwError = 0;
    HANDLE Handle = (HANDLE)NULL;
    uid_t UID;
    gid_t GID;

    dwError = LsaSrvIpcCheckPermissions(pToken, &UID, &GID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvOpenServer(UID, GID, &Handle);
    BAIL_ON_LSA_ERROR(dwError);

    *ppSessionData = Handle;

cleanup:

    return MAP_LSA_ERROR_IPC(dwError);

error:

    goto cleanup;
}
