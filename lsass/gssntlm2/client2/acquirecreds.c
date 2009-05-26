

DWORD
NTLMServerAcquireCredentialsHandle(
   ___n   SEC_CHAR *pszPrincipal,
  __in   SEC_CHAR *pszPackage,
  __in   ULONG fCredentialUse,
  __in   PLUID pvLogonID,
  __in   PVOID pAuthData,
  __in   SEC_GET_KEY_FN pGetKeyFn,
  __in   PVOID pvGetKeyArgument,
  __out  PCredHandle phCredential,
  __out  PTimeStamp ptsExpiry
    )
{
    DWORD dwError = 0;

    return(dwError);
}
