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
 *        utils.c
 *
 * Abstract:
 *
 * NTLM utilities
 *
 * Author: Todd Stecher (2007)
 *
 */
#include <ntlmcommon.h>

PVOID
NTLMAllocateMemory(DWORD dwSize)
{
    PVOID pMemory;

    pMemory = malloc(dwSize);
    if (!pMemory) return NULL;
    memset(pMemory,0, dwSize);
    return pMemory;
}

PVOID
NTLMReallocMemory(
    PVOID  pMemory,
    DWORD dwSize
    )
{
    PVOID pNewMemory = NULL;

    if (pMemory == NULL) {
       pNewMemory = NTLMAllocateMemory(dwSize);
    }else {
       pNewMemory = realloc(pMemory, dwSize);
    }

    return pNewMemory;
}

void
NTLMFreeMemory(PVOID pMemory)
{
    if (pMemory)
    {
        free(pMemory);
    }
}

void
NTLMSafeFreeMemory( PVOID *ppMemory)
{
    NTLMFreeMemory(*ppMemory);
    *ppMemory = NULL;
}

DWORD
NTLMAllocCopySecBuffer(
    PSEC_BUFFER dst,
    PSEC_BUFFER src
    )
{
    /* handle null sec buffer */
    if (!src || src->length == 0 || src->buffer == NULL) {
        dst->buffer = NULL;
        dst->length = dst->maxLength = 0;
        return LSA_ERROR_SUCCESS;
    }

    dst->buffer = (PBYTE) NTLMAllocateMemory(src->maxLength);
    if (!dst->buffer) {
        return LSA_ERROR_OUT_OF_MEMORY;
    }

    SEC_BUFFER_COPY(dst,src);
    dst->length = src->length;
    dst->maxLength = src->maxLength;

    return LSA_ERROR_SUCCESS;
}

void
NTLMFreeSecBuffer(
    PSEC_BUFFER freeme
    )
{
    NTLM_SAFE_FREE(freeme->buffer);
    ZERO_STRUCTP(freeme);
}

void
NTLMFreeSecBufferBase(
    PSEC_BUFFER *freeme
    )
{
    if (!freeme)
        return;

    NTLMFreeSecBuffer(*freeme);
    NTLM_SAFE_FREE(*freeme);
    *freeme = NULL;
}


DWORD
NTLMAllocTransferSecBuffer(
    PSEC_BUFFER *dst,
    PSEC_BUFFER src
    )
{
    *dst = (PSEC_BUFFER) NTLMAllocateMemory(sizeof(SEC_BUFFER));
    if (!*dst)
        return LSA_ERROR_OUT_OF_MEMORY;

    if (!src || src->length == 0 || src->buffer == NULL) {
        (*dst)->buffer = NULL;
        (*dst)->length = (*dst)->maxLength = 0;
    } else {
        (*dst)->length = src->length;
        (*dst)->maxLength = src->maxLength;
        (*dst)->buffer = src->buffer;
        src->buffer = NULL;
        src->length = src->maxLength = 0;
    }

    return LSA_ERROR_SUCCESS;
}


/*
 * @brief NTLMValidateMarshalledSecBuffer
 *
 * Useful routine for validating secbuffers embedded in
 * other messages.
 *
 * @param base - base of embedding structure
 * @param length - length of embedding structure
 * @param secBuf - sec_buffer contained in structure
 *
 * @returns true / false
 */
BOOLEAN
NTLMValidateMarshalledSecBuffer(
    PBYTE base,
    ULONG length,
    PSEC_BUFFER secBuf
    )
{

    if (secBuf->maxLength < secBuf->length)
        goto error;

#if 0
/* FIXME: what is this supposed to actually check for? */
    if (length < (ULONG) secBuf->buffer + secBuf->maxLength)
        goto error;
#endif

    if (secBuf->buffer == NULL &&
        (secBuf->length || secBuf->maxLength))
        goto error;

    return true;

error:

    DBGDumpSecBuffer(D_ERROR,"Invalid secbuffer",secBuf);
    return false;
}

/*
 * @brief NTLMValidateMarshalledLsaString
 *
 * Useful routine for validating lsa_strings embedded in
 * other messages.
 *
 * @param base - base of embedding structure
 * @param length - length of embedding structure
 * @param string - string embedded in structure
 *
 * @returns true / false
 */
BOOLEAN
NTLMValidateMarshalledLsaString(
    PBYTE base,
    ULONG length,
    PLSA_STRING string
    )
{

    /* this routine assumes strings contain offsets */
    /* note that's nearly == to a marshalled sec buffer */
    return NTLMValidateMarshalledSecBuffer(base,length, (PSEC_BUFFER) string);
}




/*
 * Global access methods
 */
DWORD
NTLMInitWorkstationName(
    VOID
    )
{
    DWORD dwError;
    char host[NTLM_MAX_WORKSTATION_NAME];

    if (gethostname(host, NTLM_MAX_WORKSTATION_NAME))
        return LSA_ERROR_INTERNAL;

    NTLM_GLOBALS_LOCK();
    dwError = LsaInitializeLsaStringA(
                host,
                &g_workstationName
                );
    NTLM_GLOBALS_UNLOCK();

    return dwError;
}

DWORD
NTLMTeardownWorkstationName(
    VOID
    )
{
    NTLM_GLOBALS_LOCK();

    LsaFreeLsaString(&g_workstationName);

    NTLM_GLOBALS_UNLOCK();

    return 0;
}

DWORD
NTLMInitializeGlobals(
    VOID
    )
{

    DWORD dwError;

    if (pthread_mutex_init(&g_globalsLock, NULL))
        return LSA_ERROR_INTERNAL;

    dwError = NTLMInitWorkstationName();
    BAIL_ON_NTLM_ERROR(dwError);

error:

    return dwError;

}

DWORD
NTLMTeardownGlobals(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = NTLMTeardownWorkstationName();
    BAIL_ON_NTLM_ERROR(dwError);

error:

    return dwError;
}

DWORD
NTLMGetDNSDomainName(PLSA_STRING dnsDomainName)
{

    DWORD dwError;
    NTLM_GLOBALS_LOCK();

    /* @todo - right now, we don't know our domain */
    dwError = LsaCopyLsaString(
                dnsDomainName,
                &g_workstationName
                );

    NTLM_GLOBALS_UNLOCK();

    return dwError;

}

DWORD
NTLMGetNBDomainName(PLSA_STRING nbDomainName)
{

    DWORD dwError;
    NTLM_GLOBALS_LOCK();

    /* @todo - right now, we don't know our domain */
    dwError = LsaCopyLsaString(
                nbDomainName,
                &g_workstationName
                );

    NTLM_GLOBALS_UNLOCK();

    return dwError;

}

DWORD
NTLMGetDNSWorkstationName(PLSA_STRING workstationDNSName)
{

    DWORD dwError;
    NTLM_GLOBALS_LOCK();

    /* @todo - build dns machine name */
    dwError = LsaCopyLsaString(
                workstationDNSName,
                &g_workstationName
                );

    NTLM_GLOBALS_UNLOCK();

    return dwError;

}

DWORD
NTLMGetWorkstationName(PLSA_STRING workstationName)
{

    DWORD dwError;
    NTLM_GLOBALS_LOCK();

    dwError = LsaCopyLsaString(
                workstationName,
                &g_workstationName
                );

    NTLM_GLOBALS_UNLOCK();

    return dwError;

}

DWORD
NTLMInitUtilityFunctions(
    VOID
    )
{
    DWORD dwError;

   if (pthread_mutex_init(&g_globalsLock, NULL))
        return LSA_ERROR_INTERNAL;

    dwError = NTLMInitWorkstationName();
    BAIL_ON_NTLM_ERROR(dwError);

error:

    return dwError;
}

DWORD
NTLMTeardownUtilityFunctions(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = NTLMTeardownWorkstationName();
    BAIL_ON_NTLM_ERROR(dwError);

error:

    return dwError;
}

DWORD
NTLMGetPackedContextLength(
    PNTLM_PACKED_CONTEXT packedContext
)
{

    DWORD dwLen = sizeof(NTLM_PACKED_CONTEXT);

    dwLen += packedContext->baseSessionKey.length;
    dwLen += packedContext->peerName.length;
    dwLen += packedContext->peerDomain.length;

    return dwLen;
}



DWORD
NTLMUnpackContext(
    PSEC_BUFFER contextData,
    PNTLM_PACKED_CONTEXT *packedContext
)
{
    DWORD dwError;
    ULONG ofs = ( 2 * sizeof(ULONG) );
    PNTLM_PACKED_CONTEXT tmp = (PNTLM_PACKED_CONTEXT) contextData->buffer;

    dwError = NTLMGetSecBuffer(
                &tmp->baseSessionKey,
                contextData,
                &ofs
                );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMGetLsaString(
                &tmp->peerName,
                contextData,
                &ofs
                );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMGetLsaString(
                &tmp->peerDomain,
                contextData,
                &ofs
                );

    BAIL_ON_NTLM_ERROR(dwError);

    (*packedContext) = tmp;

error:

    return dwError;
}


DWORD
NTLMPackContext(
    PNTLM_PACKED_CONTEXT packedContext,
    PSEC_BUFFER contextData
)
{
    DWORD dwError;
    SEC_BUFFER tmp;
    ULONG ofs = (2 * sizeof(ULONG) );
    ULONG bufofs = sizeof(NTLM_PACKED_CONTEXT);

    tmp.length = tmp.maxLength = NTLMGetPackedContextLength(packedContext);
    tmp.buffer = NTLMAllocateMemory(tmp.length);

    if (!tmp.buffer)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    memcpy(tmp.buffer, packedContext, sizeof(NTLM_PACKED_CONTEXT));

    NTLMPutSecBuffer(
        &packedContext->baseSessionKey,
        tmp.buffer,
        &bufofs,
        &ofs
        );

    NTLMPutLsaString(
        &packedContext->peerName,
        tmp.buffer,
        &bufofs,
        &ofs
        );

    NTLMPutLsaString(
        &packedContext->peerDomain,
        tmp.buffer,
        &bufofs,
        &ofs
        );

    (*contextData) = tmp;

error:

    return LSA_ERROR_SUCCESS;

}

#define TIME_TO_NTTIME_CORRECTION 11644473600LL

VOID
NTLMGetNTTime(
    PULONG64 ntTime
)
{
    ULONG64 t;
    /* @todo - this may not have sufficient granularity */
    t = time(NULL);
    t += TIME_TO_NTTIME_CORRECTION;
    t *= (1000*1000*10);

    *ntTime = t;
}
