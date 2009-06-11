#include "client.h"

DWORD
NtlmClientExportSecurityContext(
    IN PCtxtHandle phContext,
    IN ULONG fFlags,
    OUT PSecBuffer pPackedContext,
    OUT OPTIONAL HANDLE *pToken
    )
{
    DWORD dwError = 0;
    HANDLE hServer = INVALID_HANDLE;

    // We don't want to zero out the SecBuffers, but we may want to initialize
    // some of their members to zero.
    // In the long run, I don't think we need this token...
    if(pToken)
    {
        *pToken = INVALID_HANDLE;
    }

    dwError = NtlmOpenServer(&hServer);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmTransactExportSecurityContext(
        hServer,
        phContext,
        fFlags,
        pPackedContext,
        pToken
        );

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    if(INVALID_HANDLE != hServer)
    {
        NtlmCloseServer(hServer);
    }
    return(dwError);
error:
    // we may not want to clear the IN OUT params on error
    // also, I don't think we're going to be using this token, but put this
    // clean up code in for now... note the missing "CloseHandle" call we
    // would normally want here.
    if(pToken)
    {
        *pToken = INVALID_HANDLE;
    }
    goto cleanup;
}
