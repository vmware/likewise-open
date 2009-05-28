
DWORD
NtlmServerImportSecurityContext(
    PSECURITY_STRING *pszPackage,
    PSecBuffer pPackedContext,
    HANDLE pToken,
    PCtxtHandle phContext
    )
{
    DWORD dwError = 0;
    dwError = NtlmTransactImportSecurityContext(
                    hServer,
                    pszPackage,
                    pPackedContext,
                    pToken,
                    phContext
                    );
    return(dwError);
}
