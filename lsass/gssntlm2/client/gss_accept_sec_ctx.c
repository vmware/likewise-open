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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        context.c
 *
 * Abstract:
 *
 *        GSS context management and APIs
 *
 * Author: Todd Stecher (2007)
 *
 */

#include "client.h"

NTLMGssAcceptSecContext(
    DWORD          *pdwMinorStatus,
    PVOID           pCredential,
    PVOID          *pContext,
    PSEC_BUFFER     inputToken,
    PLSA_STRING     pSrcName,
    PSEC_BUFFER     pOutputToken,
    DWORD          *pFlags,
    DWORD          *pTimeValid
    )
{
    DWORD dwError = 0;
    DWORD msgError = LSA_ERROR_SUCCESS;
    NTLM_CREDENTIAL *pCred = NULL;
    NTLM_CONTEXT *pCtxt = NULL;
    SEC_BUFFER outputToken;

    ZERO_STRUCT(outputToken);

    if (!pContext)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_CONTEXT);

    /* locate, and reference credential handle */
    pCred = NTLMValidateCredential(
                (NTLM_CREDENTIAL*) pCredential,
                true
                );

    if (NULL == pCred) {
        dwError = LSA_ERROR_INVALID_CREDENTIAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* check credential usage */
    if ((pCred->flags & GSS_C_ACCEPT) == 0) {
        dwError = LSA_ERROR_INVALID_CREDENTIAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* validate oid, possibly alter mech */

    /* validate secbuffer data */

    /* @todo validate uid, pid */

    /* if null context, create a new one */
    if (*pContext == GSS_C_NO_CONTEXT) {

        dwError = NTLMCreateContext(
                        pCred,
                        CONTEXT_SERVER,
                        &pCtxt
                        );

        BAIL_ON_NTLM_ERROR(dwError);

    } else {

        pCtxt = NTLMLocateContext(
                    (PNTLM_CONTEXT) *pContext,
                    pCred,
                    CONTEXT_SERVER
                    );

        if (pCtxt == NULL) {
            dwError = LSA_ERROR_NO_CONTEXT;
            BAIL_ON_NTLM_ERROR(dwError);
        }
    }

    switch (pCtxt->dwState) {

        case ACCEPT_CONTEXT_RECEIVE_NEGOTIATE_MSG:
            dwError =  SrvProcessNegotiateMessage();
            break;

        case ACCEPT_CONTEXT_RECEIVE_AUTH_MSG:
            dwError = SrvProcessAuthMessage();
            ProcessAuthMessage();
    }

    /* process message */
    msgError = pCtxt->processNextMessage(
                        pCtxt,
                        inputToken,
                        &outputToken
                        );

    if (msgError != LSA_WARNING_CONTINUE_NEEDED)
        BAIL_ON_NTLM_ERROR(msgError);

    if (msgError == GSS_S_COMPLETE)
    {
        NTLM_LOCK_CONTEXTS();
        dwError = NTLMCreateKeys(pCtxt);
        NTLM_UNLOCK_CONTEXTS();

        BAIL_ON_NTLM_ERROR(dwError);
    }

    *pContext = pCtxt;
    memcpy(pOutputToken, &outputToken, sizeof(SEC_BUFFER));
    ZERO_STRUCT(outputToken);

    NTLMDumpContext(D_ERROR, pCtxt); /*@todo - dumb this down to D_CTXT */

error:

    if (dwError == LSA_ERROR_SUCCESS && msgError)
        dwError = msgError;

    (*pdwMinorStatus) = dwError;

    /* context is invalid after hard error */
    if (LSA_ERROR_MASK(dwError) && pCtxt)
        NTLMRemoveContext(pCtxt);

    NTLMFreeSecBuffer(&outputToken);
    NTLMDereferenceCredential(pCred);
    NTLMDereferenceContext(pCtxt);

    return dwError;

}

