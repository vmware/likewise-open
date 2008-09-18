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
 *        marshal_gss.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Marshal/Unmarshal GSSNTLM messages
 *
 * Authors: Todd Stecher (v-todds@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#include "ipc.h"

#define ZERO_STRUCT(_s_) memset((char*)&(_s_),0,sizeof(_s_))


DWORD
LsaUnMarshalSecBuffer(
    PCSTR base,
    DWORD length,
    PSEC_BUFFER dest,
    PLSADATACOORDINATES src,
    BOOLEAN alloc
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    ULONGLONG end = src->offset + src->length;

    if (src->length == 0)
    {
        dest->length = dest->maxLength = 0;
        dest->buffer = NULL;
        return LSA_ERROR_SUCCESS;
    }

    if (end > length ||
        src->length > length ||
        src->offset > length )
        return (LSA_ERROR_INSUFFICIENT_BUFFER);

    if (alloc)
    {
        dwError = LsaAllocateMemory(
                        src->length, 
                        (PVOID*)&dest->buffer
                        );

        BAIL_ON_LSA_ERROR(dwError);

        memcpy(
            dest->buffer, 
            OFFSET_TO_PTR(base, src->offset), 
            src->length
            );
    }
    else
        dest->buffer = OFFSET_TO_PTR(base, src->offset);

    dest->length = dest->maxLength = src->length;

error:

    return dwError;
}

DWORD
LsaUnMarshalSecBufferS(
    PCSTR base,
    DWORD length,
    PSEC_BUFFER_S dest,
    PLSADATACOORDINATES src
    )
{
    DWORD dwError;
    SEC_BUFFER tmp;
    ZERO_STRUCT(tmp);

    dwError = LsaUnMarshalSecBuffer(
                    base,
                    length,
                    &tmp,
                    src,
                    false /* no alloc */
                    );

    INIT_SEC_BUFFER_S_VAL(dest, tmp.length, tmp.buffer);
    return dwError;
}

DWORD
LsaUnMarshalString(
    PCSTR               pszMsg,
    DWORD               dwLength,
    PSTR*               ppszStr,
    PLSADATACOORDINATES pSrc 
    )
{
    DWORD dwError = 0;
    PSTR pszStr = NULL;
    ULONGLONG end = pSrc->offset + pSrc->length;

    if (end > dwLength ||
    	pSrc->length > dwLength ||
    	pSrc->offset > dwLength )
    {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pSrc->length == 0)
    {
        *ppszStr = NULL;
        goto cleanup;
    }

    dwError = LsaAllocateMemory(
    				pSrc->length + 1,
    				(PVOID*)&pszStr);
    BAIL_ON_LSA_ERROR(dwError);
    
    memcpy(pszStr, pszMsg + pSrc->offset, pSrc->length);
    
    *ppszStr = pszStr;
    
cleanup:

    return dwError;
    
error:

	*ppszStr = NULL;
	
	LSA_SAFE_FREE_STRING(pszStr);

	goto cleanup;
}

VOID
LsaMarshalSecBuffer(
    PSTR                pszBase,
    DWORD *             pdwBufOfs,
    PSEC_BUFFER         pSrc,
    PLSADATACOORDINATES pDest
    )
{
    pDest->offset = (*pdwBufOfs);

    if (!pSrc || pSrc->length == 0)
        pDest->length = 0;
    else 
    {
        pDest->length = pSrc->length;
        memcpy(&pszBase[(*pdwBufOfs)], pSrc->buffer, pSrc->length);
        (*pdwBufOfs) += pSrc->length;
    }
}

VOID
LsaMarshalSecBufferS(
    PSTR                pszBase,
    DWORD *             pdwBufOfs,
    PSEC_BUFFER_S       pSrc,
    PLSADATACOORDINATES pDest
    ) 
{
    SEC_BUFFER tmp;
    SEC_BUFFER_S_CONVERT(&tmp, pSrc);

    return LsaMarshalSecBuffer(
                    pszBase,
                    pdwBufOfs,
                    &tmp,
                    pDest
                    );
}


VOID
LsaMarshalString(
    PSTR                pszMsg,
    DWORD *             pdwBufOfs,
    PCSTR               pszSrc,
    PLSADATACOORDINATES pDest
    )
{
    DWORD dwLen;

    pDest->offset = (*pdwBufOfs);

    if (pszSrc) 
    {
        dwLen = strlen(pszSrc);
        memcpy(&pszMsg[(*pdwBufOfs)], pszSrc, dwLen);
        (*pdwBufOfs) += dwLen;
    }
    else
        dwLen = 0;

    pDest->length = dwLen;
}

/*
 * LSA_GSS_Q_MAKE_AUTH_MESSAGE
 */

DWORD
LsaMarshalGSSMakeAuthMsgQ(
    PSEC_BUFFER     credentials,
    PSEC_BUFFER_S   serverChallenge,
    PSEC_BUFFER     targetInfo,
    ULONG           negotiateFlags,
    PSTR            pszMsg,
    DWORD *         pdwMsgLen
    )
{
    DWORD   dwError = 0;
    DWORD   bufOfs = sizeof(LSA_GSS_Q_MAKE_AUTH_MSG);
    DWORD   len =  sizeof(LSA_GSS_Q_MAKE_AUTH_MSG) + serverChallenge->length;

    PLSA_GSS_Q_MAKE_AUTH_MSG pMAM = (PLSA_GSS_Q_MAKE_AUTH_MSG) pszMsg;

    if (credentials)
        len += credentials->length;

    if (targetInfo)
        len+= targetInfo->length;

    /* size only */
    if (!pszMsg) {
        (*pdwMsgLen) = len;
        goto cleanup;
    }
    
    if (len > (*pdwMsgLen))
    {
    	dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pMAM->negotiateFlags = negotiateFlags;

    LsaMarshalSecBuffer(
    		pszMsg,
    		&bufOfs,
    		credentials, 
    		&pMAM->marshalledAuthUser);

    LsaMarshalSecBufferS(
    		pszMsg,
    		&bufOfs,
    		serverChallenge,
    		&pMAM->serverChallenge);

    LsaMarshalSecBuffer(
    		pszMsg,
    		&bufOfs,
    		targetInfo,
    		&pMAM->targetInfo);

cleanup:

    return dwError;
    
error:

	goto cleanup;
}

DWORD
LsaUnMarshalGSSMakeAuthMsgQ(
    PCSTR           pszMsg,
    DWORD           dwMsgLen,
    PSEC_BUFFER     credentials,
    PSEC_BUFFER_S   serverChallenge,
    PSEC_BUFFER     targetInfo,
    ULONG  *        negotiateFlags
    )
{
    DWORD   dwError = 0;

    PLSA_GSS_Q_MAKE_AUTH_MSG pMAM = (PLSA_GSS_Q_MAKE_AUTH_MSG) pszMsg;
    
    BAIL_ON_INVALID_POINTER(credentials);
    BAIL_ON_INVALID_POINTER(serverChallenge);
    BAIL_ON_INVALID_POINTER(targetInfo);
    BAIL_ON_INVALID_POINTER(negotiateFlags);

    if (!pszMsg || (dwMsgLen < sizeof(LSA_GSS_Q_MAKE_AUTH_MSG)))
    {
    	dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    (*negotiateFlags) = pMAM->negotiateFlags;
    
    dwError = LsaUnMarshalSecBuffer(
    	            pszMsg,
                    dwMsgLen, 
                    credentials, 
                    &pMAM->marshalledAuthUser,
                    FALSE /* use in memory copy */
                    );

    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaUnMarshalSecBufferS(
    	            pszMsg,
                    dwMsgLen, 
                    serverChallenge,
                    &pMAM->serverChallenge
                    );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUnMarshalSecBuffer(
    	            pszMsg,
                    dwMsgLen, 
                    targetInfo, 
                    &pMAM->targetInfo,
                    FALSE /* use in memory copy */
                    );

    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

/*
 * LSA_GSS_R_MAKE_AUTH_MESSAGE
 */

DWORD
LsaMarshalGSSMakeAuthMsgR(
    DWORD           msgError,
    PSEC_BUFFER     authenticateMessage,
    PSEC_BUFFER_S   sessionKey,
    PSTR            pszMsg,
    DWORD *         pdwMsgLen
    )
{
    DWORD   dwError = 0;
    DWORD   bufOfs = sizeof(LSA_GSS_R_MAKE_AUTH_MSG);
    DWORD   len =  sizeof(LSA_GSS_R_MAKE_AUTH_MSG) +
        authenticateMessage->length + sessionKey->length;
    PLSA_GSS_R_MAKE_AUTH_MSG pMAMR = (PLSA_GSS_R_MAKE_AUTH_MSG) pszMsg;

    /* size only */
    if (!pszMsg) {
        (*pdwMsgLen) = len;
        goto cleanup;
    }
    
    if (len > (*pdwMsgLen))
    {
    	dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pMAMR->msgError = msgError;

    LsaMarshalSecBuffer(
    		pszMsg,
    		&bufOfs,
    		authenticateMessage, 
    		&pMAMR->authenticateMessage);

    LsaMarshalSecBufferS(
    		pszMsg,
    		&bufOfs,
    		sessionKey,
    		&pMAMR->baseSessionKey);

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaUnMarshalGSSMakeAuthMsgR(
    PCSTR       pszMsg,
    DWORD       dwMsgLen,
    DWORD *     dwMsgError,
    PSEC_BUFFER authenticateMessage,
    PSEC_BUFFER_S baseSessionKey
    )
{
    DWORD   dwError = 0;
    PLSA_GSS_R_MAKE_AUTH_MSG pMAM = (PLSA_GSS_R_MAKE_AUTH_MSG) pszMsg;
    
    BAIL_ON_INVALID_POINTER(authenticateMessage);
    BAIL_ON_INVALID_POINTER(baseSessionKey);

    if (!pszMsg || (dwMsgLen < sizeof(LSA_GSS_R_MAKE_AUTH_MSG)))
    {
    	dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
    	BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaUnMarshalSecBuffer(
    	            pszMsg,
                    dwMsgLen, 
                    authenticateMessage, 
                    &pMAM->authenticateMessage,
                    TRUE /* alloc */
                    );

    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaUnMarshalSecBufferS(
    	            pszMsg,
                    dwMsgLen, 
                    baseSessionKey,
                    &pMAM->baseSessionKey
                    );

    BAIL_ON_LSA_ERROR(dwError);

    (*dwMsgError) = pMAM->msgError;

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

/*
 * LSA_GSS_Q_CHECK_AUTH_MSG
 */

DWORD
LsaMarshalGSSCheckAuthMsgQ(
    ULONG           negotiateFlags,
    PSEC_BUFFER_S   serverChallenge,
    PSEC_BUFFER     targetInfo,
    PSEC_BUFFER     authenticateMessage,
    PSTR            pszMsg,
    DWORD *         pdwMsgLen
    )
{
    DWORD   dwError = 0;
    DWORD   bufOfs = sizeof(LSA_GSS_Q_CHECK_AUTH_MSG);
    DWORD   len =  sizeof(LSA_GSS_Q_CHECK_AUTH_MSG) +
        authenticateMessage->length + serverChallenge->length;

    PLSA_GSS_Q_CHECK_AUTH_MSG pCAM = (PLSA_GSS_Q_CHECK_AUTH_MSG) pszMsg;

    if (targetInfo)
        len+= targetInfo->length;

    /* size only */
    if (!pszMsg) {
        (*pdwMsgLen) = len;
        goto cleanup;
    }
    
    if (len > (*pdwMsgLen))
    {
    	dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pCAM->negotiateFlags = negotiateFlags;

    LsaMarshalSecBufferS(
    		pszMsg,
    		&bufOfs,
    		serverChallenge,
    		&pCAM->serverChallenge);

    LsaMarshalSecBuffer(
    		pszMsg,
    		&bufOfs,
    		targetInfo, 
    		&pCAM->targetInfo);

    LsaMarshalSecBuffer(
    		pszMsg,
    		&bufOfs,
    		authenticateMessage, 
    		&pCAM->authenticateMessage);

cleanup:

    return dwError;
    
error:

	goto cleanup;
}

DWORD
LsaUnMarshalGSSCheckAuthMsgQ(
    PCSTR           pszMsg,
    DWORD           dwMsgLen,
    ULONG  *        negotiateFlags,
    PSEC_BUFFER_S   serverChallenge,
    PSEC_BUFFER     targetInfo,
    PSEC_BUFFER     authenticateMessage
    )
{
    DWORD   dwError = 0;

    PLSA_GSS_Q_CHECK_AUTH_MSG pCAM = (PLSA_GSS_Q_CHECK_AUTH_MSG) pszMsg;
    
    BAIL_ON_INVALID_POINTER(authenticateMessage);
    BAIL_ON_INVALID_POINTER(targetInfo);
    BAIL_ON_INVALID_POINTER(negotiateFlags);

    if (!pszMsg || (dwMsgLen < sizeof(LSA_GSS_Q_CHECK_AUTH_MSG)))
    {
    	dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    (*negotiateFlags) = pCAM->negotiateFlags;
    
    dwError = LsaUnMarshalSecBuffer(
    	            pszMsg,
                    dwMsgLen, 
                    targetInfo, 
                    &pCAM->targetInfo,
                    FALSE /* use in memory copy */
                    );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUnMarshalSecBuffer(
    	            pszMsg,
                    dwMsgLen, 
                    authenticateMessage, 
                    &pCAM->authenticateMessage,
                    FALSE /* use in memory copy */
                    );

    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaUnMarshalSecBufferS(
    	            pszMsg,
                    dwMsgLen, 
                    serverChallenge,
                    &pCAM->serverChallenge
                    );

    BAIL_ON_LSA_ERROR(dwError);
    

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

/*
 * LSA_GSS_R_CHECK_AUTH_MSG
 */
DWORD
LsaMarshalGSSCheckAuthMsgR(
    DWORD           msgError,
    PSEC_BUFFER_S   baseSessionKey,
    PSTR            pszMsg,
    DWORD *         pdwMsgLen
    )
{
    DWORD   dwError = 0;
    DWORD   bufOfs = sizeof(LSA_GSS_R_CHECK_AUTH_MSG);
    DWORD   len =  sizeof(LSA_GSS_R_CHECK_AUTH_MSG) + baseSessionKey->length;

    PLSA_GSS_R_CHECK_AUTH_MSG pCAMR = (PLSA_GSS_R_CHECK_AUTH_MSG) pszMsg;

    BAIL_ON_INVALID_POINTER(baseSessionKey);

    /* size only */
    if (!pszMsg) {
        (*pdwMsgLen) = len;
        goto cleanup;
    }
    
    if (len > (*pdwMsgLen))
    {
    	dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pCAMR->msgError = msgError;

    LsaMarshalSecBufferS(
    		pszMsg,
    		&bufOfs,
    		baseSessionKey, 
    		&pCAMR->baseSessionKey);

cleanup:

    return dwError;
    
error:

	goto cleanup;
}

DWORD
LsaUnMarshalGSSCheckAuthMsgR(
    PCSTR           pszMsg,
    DWORD           dwMsgLen,
    DWORD          *msgError,
    PSEC_BUFFER_S   baseSessionKey
    )
{
    DWORD   dwError = 0;

    PLSA_GSS_R_CHECK_AUTH_MSG pCAMR = (PLSA_GSS_R_CHECK_AUTH_MSG) pszMsg;
    
    BAIL_ON_INVALID_POINTER(baseSessionKey);
    BAIL_ON_INVALID_POINTER(msgError);

    if (!pszMsg || (dwMsgLen < sizeof(LSA_GSS_R_CHECK_AUTH_MSG)))
    {
    	dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    (*msgError) = pCAMR->msgError;
    
    dwError = LsaUnMarshalSecBufferS(
    	            pszMsg,
                    dwMsgLen, 
                    baseSessionKey,
                    &pCAMR->baseSessionKey
                    );

    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

