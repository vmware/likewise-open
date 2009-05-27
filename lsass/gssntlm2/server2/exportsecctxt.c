
DWORD
NtlmClientExportSecurityContext(
    PCtxtHandle phContext,
    ULONG fFlags,
    PSecBuffer pPackedContext,
    HANDLE *pToken
    )
{
    DWORD dwError = 0;
    dwError = NtlmTransactExportSecurityContext(
                    hServer,
                    phContext,
                    fFlags,
                    pPackedContext,
                    pToken
                    );
    return(dwError);
}
