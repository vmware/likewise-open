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
 *        ipc_group.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process communication (Server) API for NSSArtefacts
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "ipc.h"


DWORD
LsaSrvIpcBeginEnumNSSArtefacts(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD dwError = 0;
    DWORD dwNSSArtefactInfoLevel = 0;
    DWORD dwMapType = 0;
    DWORD dwNumMaxNSSArtefacts = 0;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    PSTR   pszGUID = NULL;

    dwError = LsaUnmarshalBeginEnumNSSArtefactRecordsQuery(
                        pMessage->pData,
                        pMessage->header.messageLength,
                        &dwNSSArtefactInfoLevel,
                        &dwMapType,
                        &dwNumMaxNSSArtefacts);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvBeginEnumNSSArtefacts(
                        hServer,
                        dwMapType,
                        dwNSSArtefactInfoLevel,
                        dwNumMaxNSSArtefacts,
                        &pszGUID);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwError) {

       dwError = LsaMarshalNSSArtefactInfoError(
                       dwError,
                       &pResponse);
       BAIL_ON_LSA_ERROR(dwError);

    } else {

       dwError = LsaMarshalEnumRecordsToken(
                           pszGUID,
                           NULL,
                           &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);

       dwError = LsaBuildMessage(
                        LSA_R_BEGIN_ENUM_NSS_ARTEFACTS,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse
                        );
       BAIL_ON_LSA_ERROR(dwError);

       dwError = LsaMarshalEnumRecordsToken(
                        pszGUID,
                        pResponse->pData,
                        &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);

    }

    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_MESSAGE(pResponse);

    LSA_SAFE_FREE_STRING(pszGUID);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvIpcEnumNSSArtefacts(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
)
{

    DWORD dwError = 0;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD  dwNSSArtefactInfoLevel = 0;
    DWORD  dwNumNSSArtefactsFound = 0;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    PSTR pszGUID = NULL;


    dwError = LsaUnmarshalEnumRecordsToken(
                        pMessage->pData,
                        pMessage->header.messageLength,
                        &pszGUID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);


    dwError = LsaSrvEnumNSSArtefacts(
                    hServer,
                    pszGUID,
                    &dwNSSArtefactInfoLevel,
                    &ppNSSArtefactInfoList,
                    &dwNumNSSArtefactsFound);



    if (dwError) {

       dwError = LsaMarshalNSSArtefactInfoError(
                       dwError,
                       &pResponse);
       BAIL_ON_LSA_ERROR(dwError);

    } else {

       dwError = LsaMarshalNSSArtefactInfoList(
                           ppNSSArtefactInfoList,
                           dwNSSArtefactInfoLevel,
                           dwNumNSSArtefactsFound,
                           NULL,
                           &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);



       dwError = LsaBuildMessage(
                        LSA_R_ENUM_NSS_ARTEFACTS,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse);
       BAIL_ON_LSA_ERROR(dwError);

       dwError = LsaMarshalNSSArtefactInfoList(
                        ppNSSArtefactInfoList,
                        dwNSSArtefactInfoLevel,
                        dwNumNSSArtefactsFound,
                        pResponse->pData,
                        &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);

    }


    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (ppNSSArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }

    LSA_SAFE_FREE_MESSAGE(pResponse);

    LSA_SAFE_FREE_STRING(pszGUID);

    return dwError;

error:

    goto cleanup;

}

DWORD
LsaSrvIpcEndEnumNSSArtefacts(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    PSTR   pszGUID = NULL;

    dwError = LsaUnmarshalEnumRecordsToken(
                        pMessage->pData,
                        pMessage->header.messageLength,
                        &pszGUID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvEndEnumNSSArtefacts(hServer, pszGUID);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwError) {

       dwError = LsaMarshalNSSArtefactInfoError(
                       dwError,
                       &pResponse);
       BAIL_ON_LSA_ERROR(dwError);

    } else {

       dwError = LsaBuildMessage(
                        LSA_R_END_ENUM_NSS_ARTEFACTS,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse
                        );
       BAIL_ON_LSA_ERROR(dwError);

    }

    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_MESSAGE(pResponse);

    LSA_SAFE_FREE_STRING(pszGUID);

    return dwError;

error:

    goto cleanup;
}


DWORD
LsaMarshalNSSArtefactInfoError(
    DWORD dwErrCode,
    PLSAMESSAGE* ppMessage
    )
{
    DWORD dwError = 0;
    DWORD dwMsgLen = 0;
    PLSAMESSAGE pMessage = NULL;

    dwError = LsaMarshalError(dwErrCode, NULL, NULL, &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBuildMessage(
                 LSA_ERROR,
                 dwMsgLen,
                 1,
                 1,
                 &pMessage
                 );
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMarshalError(dwErrCode, NULL, pMessage->pData, &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    *ppMessage = pMessage;

cleanup:

    return dwError;

error:

    *ppMessage = NULL;

    LSA_SAFE_FREE_MESSAGE(pMessage);

    goto cleanup;
}
