#include "client.h"

DWORD
NtlmClientImportSecurityContext(
    IN PSECURITY_STRING *pszPackage,
    IN PSecBuffer pPackedContext,
    IN OPTIONAL HANDLE pToken,
    OUT PCtxtHandle phContext
    )
{
    DWORD dwError = 0;
    HANDLE hServer = INVALID_HANDLE;

    memset(phContext, 0, sizeof(CtxtHandle));

    dwError = NtlmOpenServer(&hServer);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmTransactImportSecurityContext(
        hServer,
        pszPackage,
        pPackedContext,
        pToken,
        phContext
        );

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    if(INVALID_HANDLE != hServer)
    {
        NtlmCloseServer(hServer);
    }
    return(dwError);
error:
    memset(phContext, 0, sizeof(CtxtHandle));
    goto cleanup;
}
