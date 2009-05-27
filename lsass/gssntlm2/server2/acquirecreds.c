
DWORD
NtlmClientAcquireCredentialsHandle(
    SEC_CHAR *pszPrincipal,
    SEC_CHAR *pszPackage,
    ULONG fCredentialUse,
    PLUID pvLogonID,
    PVOID pAuthData,
    SEC_GET_KEY_FN pGetKeyFn,
    PVOID pvGetKeyArgument,
    PCredHandle phCredential,
    PTimeStamp ptsExpiry
    )
{
    DWORD dwError = 0;
    dwError = NtlmTransactAcquireCredentialsHandle(
                    hServer,
                    pszPrincipal,
                    pszPackage,
                    fCredentialUse,
                    pvLogonID,
                    pAuthData,
                    pGetKeyFn,
                    pvGetKeyArgument,
                    phCredential,
                    ptsExpiry
                    );
    return(dwError);
}
