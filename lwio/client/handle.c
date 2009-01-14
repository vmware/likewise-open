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

DWORD
SMBAPIHandleGetSecurityToken(
    HANDLE hHandle,
    PIO_ACCESS_TOKEN* ppSecurityToken
    )
{
    DWORD dwError = 0;
    PSMB_API_HANDLE pAPIHandle = (PSMB_API_HANDLE) hHandle;
    
    if (pAPIHandle != NULL)
    {
        if (pAPIHandle->type != SMB_API_HANDLE_ACCESS)
        {
            dwError = SMB_ERROR_INVALID_HANDLE;
            BAIL_ON_SMB_ERROR(dwError);
        }
        
        *ppSecurityToken = &pAPIHandle->variant.securityToken;
    }
    else
    {
        *ppSecurityToken = NULL;
    }

error:

    return dwError;
}

DWORD
SMBAPIHandleFreeSecurityToken(
    HANDLE hHandle
    )
{
    DWORD dwError = 0;
    PIO_ACCESS_TOKEN pSecurityToken = NULL;
 
    dwError = SMBAPIHandleGetSecurityToken(
        hHandle,
        &pSecurityToken);
    BAIL_ON_SMB_ERROR(dwError);

    switch (pSecurityToken->type)
    {
    case IO_ACCESS_TOKEN_TYPE_PLAIN:
        SMB_SAFE_FREE_MEMORY(pSecurityToken->payload.plain.pwszUsername);
        SMB_SAFE_FREE_MEMORY(pSecurityToken->payload.plain.pwszPassword);
        break;
    case IO_ACCESS_TOKEN_TYPE_KRB5:
        SMB_SAFE_FREE_MEMORY(pSecurityToken->payload.krb5.pwszPrincipal);
        SMB_SAFE_FREE_MEMORY(pSecurityToken->payload.krb5.pwszCachePath);
        break;
    }

error:

    return dwError;
}

DWORD
SMBCopyHandle(
    HANDLE hHandle,
    PHANDLE phHandleCopy
    )
{
    DWORD dwError = 0;
    PSMB_API_HANDLE pAPIHandle = (PSMB_API_HANDLE) hHandle;
    PSMB_API_HANDLE pAPIHandleCopy = NULL;
    PIO_ACCESS_TOKEN pSecurityToken = NULL;
    PIO_ACCESS_TOKEN pSecurityTokenCopy = NULL;

    if (hHandle)
    {
        dwError = SMBAllocateMemory(sizeof(*pAPIHandleCopy), (void**) (void*) &pAPIHandleCopy);
        BAIL_ON_SMB_ERROR(dwError);
        
        pAPIHandleCopy->type = pAPIHandle->type;
        
        switch (pAPIHandle->type)
        {
        case SMB_API_HANDLE_FILE:
            /* Can't duplicate file handles as of yet */
            dwError = SMB_ERROR_INVALID_HANDLE;
            BAIL_ON_SMB_ERROR(dwError);
            break;
        case SMB_API_HANDLE_ACCESS:
            pSecurityToken = &pAPIHandle->variant.securityToken;
            pSecurityTokenCopy = &pAPIHandleCopy->variant.securityToken;
            pSecurityTokenCopy->type = pSecurityToken->type;
            
            switch (pSecurityToken->type)
            {
            case IO_ACCESS_TOKEN_TYPE_PLAIN:
                dwError = SMBWc16sDup(
                    pSecurityToken->payload.plain.pwszUsername,
                    &pSecurityTokenCopy->payload.plain.pwszUsername);
                BAIL_ON_SMB_ERROR(dwError);
                dwError = SMBWc16sDup(
                    pSecurityToken->payload.plain.pwszPassword,
                    &pSecurityTokenCopy->payload.plain.pwszPassword);
                BAIL_ON_SMB_ERROR(dwError);
                break;
            case IO_ACCESS_TOKEN_TYPE_KRB5:
                dwError = SMBWc16sDup(
                    pSecurityToken->payload.krb5.pwszPrincipal,
                    &pSecurityTokenCopy->payload.krb5.pwszPrincipal);
                BAIL_ON_SMB_ERROR(dwError);
                dwError = SMBWc16sDup(
                    pSecurityToken->payload.krb5.pwszCachePath,
                    &pSecurityTokenCopy->payload.krb5.pwszCachePath);
                BAIL_ON_SMB_ERROR(dwError);
                break;
            }
            break;
        }

        *phHandleCopy = (HANDLE) pAPIHandleCopy;
    }
    else
    {
        *phHandleCopy = NULL;
    }
    
cleanup:

    return dwError;

error:

    if (pAPIHandleCopy)
    {
        SMBCloseHandle(NULL, pAPIHandleCopy);
    }

    goto cleanup;
}

DWORD
SMBCompareHandles(
    HANDLE hHandleOne,
    HANDLE hHandleTwo,
    BOOL* pbEqual
    )
{
    DWORD dwError = 0;
    PSMB_API_HANDLE pAPIOne = (PSMB_API_HANDLE) hHandleOne;
    PSMB_API_HANDLE pAPITwo = (PSMB_API_HANDLE) hHandleTwo;
    PIO_ACCESS_TOKEN pSecurityOne = NULL;
    PIO_ACCESS_TOKEN pSecurityTwo = NULL;

    BAIL_ON_INVALID_POINTER(pAPIOne);
    BAIL_ON_INVALID_POINTER(pAPITwo);
    
    *pbEqual = FALSE;

    if (pAPIOne == pAPITwo)
    {
        *pbEqual = TRUE;
    }
    else if (pAPIOne->type == pAPITwo->type)
    {
        switch (pAPIOne->type)
        {
        case SMB_API_HANDLE_FILE:
            /* Equality of files not currently supported */
            dwError = SMB_ERROR_NOT_IMPLEMENTED;
            BAIL_ON_SMB_ERROR(dwError);
        case SMB_API_HANDLE_ACCESS:
            pSecurityOne = &pAPIOne->variant.securityToken;
            pSecurityTwo = &pAPITwo->variant.securityToken;
            
            if (pSecurityOne->type == pSecurityTwo->type)
            {
                switch (pSecurityOne->type)
                {
                case IO_ACCESS_TOKEN_TYPE_PLAIN:
                    if (!SMBWc16sCmp(pSecurityOne->payload.plain.pwszUsername,
                                     pSecurityTwo->payload.plain.pwszUsername) &&
                        !SMBWc16sCmp(pSecurityOne->payload.plain.pwszPassword,
                                     pSecurityTwo->payload.plain.pwszPassword))
                    {
                        *pbEqual = TRUE;
                    }
                    break;
                case IO_ACCESS_TOKEN_TYPE_KRB5:
                    if (!SMBWc16sCmp(pSecurityOne->payload.krb5.pwszPrincipal,
                                     pSecurityTwo->payload.krb5.pwszPrincipal) &&
                        !SMBWc16sCmp(pSecurityOne->payload.krb5.pwszCachePath,
                                     pSecurityTwo->payload.krb5.pwszCachePath))
                    {
                        *pbEqual = TRUE;
                    }
                    break;
                }
            }
        }
    }
    

error:

    return dwError;
}
