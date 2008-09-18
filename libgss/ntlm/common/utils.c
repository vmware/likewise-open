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
#include "includes.h"


pthread_mutex_t g_globalsLock;
#define NTLM_GLOBALS_LOCK(_x_)      pthread_mutex_lock(&g_globalsLock);
#define NTLM_GLOBALS_UNLOCK(_x_)    pthread_mutex_unlock(&g_globalsLock);

/*
 * Global data
 */

LSA_STRING g_workstationName;


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
    if (pMemory) return;
    free(pMemory);
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
bool 
NTLMValidateMarshalledSecBuffer(
    PBYTE base,
    ULONG length,
    PSEC_BUFFER secBuf
    )
{

    if (secBuf->maxLength < secBuf->length)
        goto error;

    if (length < (ULONG) secBuf->buffer + secBuf->maxLength)
        goto error;

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
bool
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
 * @brief NTLMBuildSupplementalCredentials
 *
 * Useful routine for building a supplemental credentials
 * structure.
 * 
 * @param username - duh!
 * @param domain - duh2!
 * @param password - duh3!
 * @param credBlob - user allocated sec_buffer, filled in by this routine.
 *
 * A properly formatted cred looks like this
 *
 *  @todo - alignment requirements
 * 
 *  *----------------------------*
 *  * base NTLMGSS_SUPPLIED_CRED *
 *  *----------------------------*
 *  * string 1                   *
 *  * string 2                   *
 *  * string 3                   *
 *  *----------------------------*
 *
 *
 * @returns errors due to validation, or alloc failure
 */ 
DWORD
NTLMBuildSupplementalCredentials(
    char *username,
    char *domain,
    char *password,
    PSEC_BUFFER credBlob
    )
{
   
    DWORD dwError;
    DWORD dwSize;
    PBYTE copyTo;

    LSA_STRING lsaUser;
    LSA_STRING lsaDomain;
    LSA_STRING lsaPassword;
    PNTLMGSS_SUPPLIED_CREDS creds = NULL;

    ZERO_STRUCT(creds);

    if (!username || !domain || !password) {
        return LSA_ERROR_INVALID_PARAMETER;
    }

    dwError = LsaInitializeLsaStringA(
                    username,
                    &lsaUser
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaInitializeLsaStringA(
                    domain,
                    &lsaDomain
                    );

    BAIL_ON_NTLM_ERROR(dwError);
    dwError = LsaInitializeLsaStringA(
                    password,
                    &lsaPassword
                    );

    BAIL_ON_NTLM_ERROR(dwError);
    
    /* note the max length is NOT used here - null termination isn't needed */
    /* note2:  we should also verify NULL term strings work */
    dwSize = sizeof(NTLMGSS_SUPPLIED_CREDS) + ALIGN_UP_8(lsaUser.length) + 
        ALIGN_UP_8(lsaDomain.length) + ALIGN_UP_8(lsaPassword.length);
    
    creds = (PNTLMGSS_SUPPLIED_CREDS) NTLMAllocateMemory(dwSize);
    if (!creds)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    copyTo = ((PBYTE) creds) + sizeof(NTLMGSS_SUPPLIED_CREDS);

    memcpy(copyTo, &lsaUser.buffer, lsaUser.length);
    creds->user.buffer = (wchar16_t*) PTR_TO_OFFSET(creds,copyTo);
    creds->user.length = creds->user.max = lsaUser.length;
    copyTo += ALIGN_UP_8(lsaUser.length);

    memcpy(copyTo, &lsaDomain.buffer, lsaDomain.length);
    creds->domain.buffer = (wchar16_t*) PTR_TO_OFFSET(creds,copyTo);
    creds->domain.length = creds->domain.max = lsaDomain.length;
    copyTo += ALIGN_UP_8(lsaDomain.length);

    memcpy(copyTo, &lsaPassword.buffer, lsaPassword.length);
    creds->password.buffer = (wchar16_t*) PTR_TO_OFFSET(creds,copyTo);
    creds->password.length = creds->password.max = lsaPassword.length;

    credBlob->buffer = (PBYTE) creds;
    credBlob->length = credBlob->maxLength = dwSize;

    creds = NULL;

error:

    LsaFreeLsaString(&lsaUser);
    LsaFreeLsaString(&lsaDomain);
    LsaFreeLsaString(&lsaPassword);
    NTLM_SAFE_FREE(creds);

    return dwError;
}


/*
 * Global access methods
 */
DWORD
NTLMInitWorkstationName()
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
NTLMInitializeGlobals()
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
NTLMInitUtilityFunctions()
{
    DWORD dwError;

   if (pthread_mutex_init(&g_globalsLock, NULL))
        return LSA_ERROR_INTERNAL; 

    dwError = NTLMInitWorkstationName();
    BAIL_ON_NTLM_ERROR(dwError);

error:

    return dwError;
}

void
NTLMUpperCase(
    PLSA_STRING s
)
{
    wchar16_t *tmp = s->buffer;
    ULONG cch;

    for (cch = 0; 
        (*tmp != '\0') && (cch <  LSASTR_WCHAR_COUNT(s->length));
        tmp++, cch ++) {
        *tmp = towupper(*tmp);
    }
}

