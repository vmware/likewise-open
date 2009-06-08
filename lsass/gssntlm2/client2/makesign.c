#include "clientipc.h"

DWORD
NtlmClientMakeSignature(
    IN PCtxtHandle phContext,
    IN ULONG fQoP,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo
    )
{
    DWORD dwError = 0;
    HANDLE hServer = INVALID_HANDLE;

    dwError = NtlmOpenServer(
        &hServer
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmTransactMakeSignature(
        hServer,
        phContext,
        fQoP,
        pMessage,
        MessageSeqNo
        );

cleanup:
    if(INVALID_HANDLE != hServer)
    {
        NtlmCloseServer(hServer);
    }
    return(dwError);
error:
    // we may not want to clear the IN OUT params on error
    goto cleanup;
}
