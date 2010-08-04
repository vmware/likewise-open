/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#include "rdr.h"

NTSTATUS
SMBSrvClientSessionCreate(
    IN OUT PSMB_SOCKET* ppSocket,
    IN PIO_CREDS pCreds,
    uid_t uid,
    OUT PSMB_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SESSION pSession = NULL;
    BOOLEAN bInLock = FALSE;
    PSMB_SOCKET pSocket = *ppSocket;
    struct _RDR_SESSION_KEY key = {0};

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    switch (pCreds->type)
    {
    case IO_CREDS_TYPE_KRB5_TGT:
        ntStatus = LwRtlCStringAllocateFromWC16String(
            &key.pszPrincipal,
            pCreds->payload.krb5Tgt.pwszClientPrincipal);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    case IO_CREDS_TYPE_PLAIN:
        ntStatus = LwRtlCStringAllocateFromWC16String(
            &key.pszPrincipal,
            pCreds->payload.krb5Tgt.pwszClientPrincipal);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    default:
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

    key.uid = uid;

    ntStatus = SMBHashGetValue(
        pSocket->pSessionHashByPrincipal,
        &key,
        OUT_PPVOID(&pSession));

    if (!ntStatus)
    {
        pSession->refCount++;
        RdrSessionRevive(pSession);
        SMBSocketRelease(pSocket);
        *ppSocket = NULL;
    }
    else
    {
        ntStatus = SMBSessionCreate(&pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        pSession->pSocket = pSocket;

        ntStatus = SMBStrndup(
            key.pszPrincipal,
            strlen(key.pszPrincipal) + 1,
            &pSession->key.pszPrincipal);
        BAIL_ON_NT_STATUS(ntStatus);

        pSession->key.uid = key.uid;

        ntStatus = SMBHashSetValue(
            pSocket->pSessionHashByPrincipal,
            &pSession->key,
            pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        pSession->bParentLink = TRUE;

        *ppSocket = NULL;
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    *ppSession = pSession;

cleanup:

    LWIO_SAFE_FREE_STRING(key.pszPrincipal);

    return ntStatus;

error:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    if (pSession)
    {
        SMBSessionRelease(pSession);
    }

    *ppSession = NULL;

    goto cleanup;
}
