/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

SMB_API
DWORD
SMBGetSessionKey(
    HANDLE hConnection,
    HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    )
{
    DWORD  dwError = 0;
    PSMB_API_HANDLE pAPIHandle = (PSMB_API_HANDLE)hFile;
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PSMB_SERVER_CONNECTION pConnection = (PSMB_SERVER_CONNECTION) hConnection;
    PSMB_GET_SESSION_KEY_RESPONSE pSessResponse = NULL;

    BAIL_IF_NOT_FILE_HANDLE(hFile);

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                                   pConnection->pAssoc,
                                   SMB_GET_SESSION_KEY,
                                   pAPIHandle->variant.hIPCHandle,
                                   &replyType,
                                   &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
    case SMB_GET_SESSION_KEY_SUCCESS:
        break;
        
    case SMB_GET_SESSION_KEY_FAILED:
        dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;
        break;

    default:
        dwError = EINVAL;
        break;
    }
    BAIL_ON_SMB_ERROR(dwError);

    pSessResponse = pResponse;

    *pdwSessionKeyLength = pSessResponse->dwSessionKeyLength;
    *ppSessionKey = pSessResponse->pSessionKey;

    /* Unset pointer to session key in message so it does not get freed */
    pSessResponse->pSessionKey = NULL;

cleanup:

    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, replyType, pResponse);
    }

    return dwError;

error:

    goto cleanup;
}

SMB_API
VOID
SMBFreeSessionKey(
    PBYTE pSessionKey
    )
{
    SMBFreeMemory(pSessionKey);
}
