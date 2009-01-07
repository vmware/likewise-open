#include "includes.h"

DWORD
SMBAPIHandleGetSecurityToken(
    HANDLE hHandle,
    PSMB_SECURITY_TOKEN_REP* ppSecurityToken
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
    PSMB_SECURITY_TOKEN_REP pSecurityToken = NULL;
 
    dwError = SMBAPIHandleGetSecurityToken(
        hHandle,
        &pSecurityToken);
    BAIL_ON_SMB_ERROR(dwError);

    switch (pSecurityToken->type)
    {
    case SMB_SECURITY_TOKEN_TYPE_PLAIN:
        SMB_SAFE_FREE_MEMORY(pSecurityToken->payload.plain.pwszUsername);
        SMB_SAFE_FREE_MEMORY(pSecurityToken->payload.plain.pwszPassword);
        break;
    case SMB_SECURITY_TOKEN_TYPE_KRB5:
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
    PSMB_SECURITY_TOKEN_REP pSecurityToken = NULL;
    PSMB_SECURITY_TOKEN_REP pSecurityTokenCopy = NULL;

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
            case SMB_SECURITY_TOKEN_TYPE_PLAIN:
                dwError = SMBWc16sDup(
                    pSecurityToken->payload.plain.pwszUsername,
                    &pSecurityTokenCopy->payload.plain.pwszUsername);
                BAIL_ON_SMB_ERROR(dwError);
                dwError = SMBWc16sDup(
                    pSecurityToken->payload.plain.pwszPassword,
                    &pSecurityTokenCopy->payload.plain.pwszPassword);
                BAIL_ON_SMB_ERROR(dwError);
                break;
            case SMB_SECURITY_TOKEN_TYPE_KRB5:
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
    PSMB_SECURITY_TOKEN_REP pSecurityOne = NULL;
    PSMB_SECURITY_TOKEN_REP pSecurityTwo = NULL;

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
                case SMB_SECURITY_TOKEN_TYPE_PLAIN:
                    if (!SMBWc16sCmp(pSecurityOne->payload.plain.pwszUsername,
                                     pSecurityTwo->payload.plain.pwszUsername) &&
                        !SMBWc16sCmp(pSecurityOne->payload.plain.pwszPassword,
                                     pSecurityTwo->payload.plain.pwszPassword))
                    {
                        *pbEqual = TRUE;
                    }
                    break;
                case SMB_SECURITY_TOKEN_TYPE_KRB5:
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
