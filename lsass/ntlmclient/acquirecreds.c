#include "clientipc.h"

DWORD
NtlmClientAcquireCredentialsHandle(
    IN SEC_CHAR *pszPrincipal,
    IN SEC_CHAR *pszPackage,
    IN ULONG fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    // NOT USED BY NTLM - IN SEC_GET_KEY_FN pGetKeyFn,
    // NOT USED BY NTLM - IN PVOID pvGetKeyArgument,
    OUT PCredHandle phCredential,
    OUT PTimeStamp ptsExpiry
    )
{
    DWORD dwError = 0;
    HANDLE hServer = INVALID_HANDLE;

    memset(phCredential, 0, sizeof(CredHandle));
    memset(ptsExpiry, 0, sizeof(TimeStamp));

    dwError = NtlmOpenServer(&hServer);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmTransactAcquireCredentialsHandle(
        hServer,
        pszPrincipal,
        pszPackage,
        fCredentialUse,
        pvLogonID,
        pAuthData,
        //pGetKeyFn,
        //pvGetKeyArgument,
        phCredential,
        ptsExpiry
        );

cleanup:
    if(INVALID_HANDLE != hServer)
    {
        NtlmCloseServer(hServer);
    }
    return(dwError);
error:
    memset(phCredential, 0, sizeof(CredHandle));
    memset(ptsExpiry, 0, sizeof(TimeStamp));
    goto cleanup;
}
