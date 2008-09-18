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
 *        crypt.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Cryptography API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "localprovider.h"

DWORD
LsaSrvComputeLMHash(
    PCSTR pszPassword,
    PBYTE* ppHash,
    PDWORD pdwHashLen
    )
{
    DWORD dwError = 0;
    PBYTE pHash = NULL;
    DWORD dwLen = 0;
    LSA_STRING wszPassword = {0};
    
    if (!IsNullOrEmptyString(pszPassword)) {
    
        dwError = LsaInitializeLsaStringA(
                        pszPassword, 
                        &wszPassword);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwLen = 16;
        
        dwError = LsaAllocateMemory(
                        dwLen * sizeof(BYTE),
                        (PVOID*)&pHash);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = ComputeLMOWF(&wszPassword, pHash);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppHash = pHash;
    *pdwHashLen = dwLen;
    
cleanup:

    LsaFreeLsaString(&wszPassword);

    return dwError;
    
error:

    *pdwHashLen = 0;
    *ppHash = NULL;
    
    LSA_SAFE_FREE_MEMORY(pHash);

    goto cleanup;
}

DWORD
LsaSrvComputeNTHash(
    PCSTR pszPassword,
    PBYTE* ppHash,
    PDWORD pdwHashLen
    )
{
    DWORD dwError = 0;
    PBYTE pHash = NULL;
    DWORD dwLen = 0;
    LSA_STRING wszPassword = {0};
    
    if (!IsNullOrEmptyString(pszPassword)) {
    
        dwError = LsaInitializeLsaStringA(
                        pszPassword, 
                        &wszPassword);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwLen = 16;
        
        dwError = LsaAllocateMemory(
                        dwLen * sizeof(BYTE),
                        (PVOID*)&pHash);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = ComputeNTOWF(&wszPassword, pHash);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppHash = pHash;
    *pdwHashLen = dwLen;
    
cleanup:

    LsaFreeLsaString(&wszPassword);

    return dwError;
    
error:

    *pdwHashLen = 0;
    *ppHash = NULL;
    
    LSA_SAFE_FREE_MEMORY(pHash);

    goto cleanup;
}

DWORD
ComputeNTOWF(
    PLSA_STRING password,
    UCHAR owf[16]
    )
{
    /* @todo what to do here?  Hash '/0' - I think */
    if (password->length == 0 || password->buffer == NULL)
        return LSA_ERROR_PASSWORD_MISMATCH;

    MD4((UCHAR*) password->buffer, password->length, owf);
    return LSA_ERROR_SUCCESS;

}

DWORD
ComputeLMOWF(
    PLSA_STRING password,
    UCHAR owf[16]
    )
{
    return LSA_ERROR_NOT_IMPLEMENTED;
}
 
