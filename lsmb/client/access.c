#include "includes.h"

DWORD
SMBCreatePlainAccessTokenA(
    LPCSTR pszUsername,
    LPCSTR pszPassword,
    PHANDLE phAccessToken
    )
{
    DWORD dwError = 0;
    LPWSTR pwszUsername = NULL;
    LPWSTR pwszPassword = NULL;
    
    dwError = SMBMbsToWc16s(pszUsername, &pwszUsername);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBMbsToWc16s(pszPassword, &pwszPassword);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCreatePlainAccessTokenW(pwszUsername, pwszPassword, phAccessToken);
    BAIL_ON_SMB_ERROR(dwError);

error:
    
    SMB_SAFE_FREE_MEMORY(pwszUsername);
    SMB_SAFE_FREE_MEMORY(pwszPassword);

    return dwError;
}

DWORD
SMBCreatePlainAccessTokenW(
    LPCWSTR pwszUsername,
    LPCWSTR pwszPassword,
    PHANDLE phAccessToken
    )
{
    DWORD dwError = 0;
    PSMB_API_HANDLE pAPIHandle = NULL;
    PSMB_SECURITY_TOKEN_REP pSecurityToken = NULL;

    dwError = SMBAllocateMemory(sizeof(*pAPIHandle), (void**) (void*) &pAPIHandle);
    BAIL_ON_SMB_ERROR(dwError);

    pAPIHandle->type = SMB_API_HANDLE_ACCESS;

    pSecurityToken = &pAPIHandle->variant.securityToken;
    
    pSecurityToken->type = SMB_SECURITY_TOKEN_TYPE_PLAIN;
    
    dwError = SMBWc16sDup(pwszUsername, &pSecurityToken->payload.plain.pwszUsername);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBWc16sDup(pwszPassword, &pSecurityToken->payload.plain.pwszPassword);
    BAIL_ON_SMB_ERROR(dwError);

    *phAccessToken = (HANDLE) pAPIHandle;
    
cleanup:

    return dwError;

error:

    if (pAPIHandle)
    {
        SMBCloseHandle(NULL, pAPIHandle);
    }

    goto cleanup;
}

DWORD
SMBCreateKrb5AccessTokenA(
    LPCSTR pszPrincipal,
    LPCSTR pszCachePath,
    PHANDLE phAccessToken
    )
{
    DWORD dwError = 0;
    LPWSTR pwszPrincipal = NULL;
    LPWSTR pwszCachePath = NULL;
    
    dwError = SMBMbsToWc16s(pszPrincipal, &pwszPrincipal);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBMbsToWc16s(pszCachePath, &pwszCachePath);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCreateKrb5AccessTokenW(pwszPrincipal, pwszCachePath, phAccessToken);
    BAIL_ON_SMB_ERROR(dwError);

error:
    
    SMB_SAFE_FREE_MEMORY(pwszPrincipal);
    SMB_SAFE_FREE_MEMORY(pwszCachePath);

    return dwError;
}

DWORD
SMBCreateKrb5AccessTokenW(
    LPCWSTR pwszPrincipal,
    LPCWSTR pwszCachePath,
    PHANDLE phAccessToken
    )
{
    DWORD dwError = 0;
    PSMB_API_HANDLE pAPIHandle = NULL;
    PSMB_SECURITY_TOKEN_REP pSecurityToken = NULL;

    dwError = SMBAllocateMemory(sizeof(*pAPIHandle), (void**) (void*) &pAPIHandle);
    BAIL_ON_SMB_ERROR(dwError);

    pAPIHandle->type = SMB_API_HANDLE_ACCESS;

    pSecurityToken = &pAPIHandle->variant.securityToken;
    
    pSecurityToken->type = SMB_SECURITY_TOKEN_TYPE_KRB5;
    
    dwError = SMBWc16sDup(pwszPrincipal, &pSecurityToken->payload.krb5.pwszPrincipal);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBWc16sDup(pwszCachePath, &pSecurityToken->payload.krb5.pwszCachePath);
    BAIL_ON_SMB_ERROR(dwError);

    *phAccessToken = (HANDLE) pAPIHandle;
    
cleanup:

    return dwError;

error:

    if (pAPIHandle)
    {
        SMBCloseHandle(NULL, pAPIHandle);
    }

    goto cleanup;
}
