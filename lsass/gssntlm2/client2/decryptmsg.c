#include "clientipc.h"

DWORD
NtlmClientDecryptMessage(
    IN PCtxtHandle phContext,
    IN OUT PSecBufferDesc pMessage,
    IN ULONG MessageSeqNo,
    OUT PULONG pfQoP
    )
{
    DWORD dwError = 0;
    HANDLE hServer = INVALID_HANDLE;

    *pfQoP = 0;

    dwError = NtlmOpenServer(&hServer);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmTransactDecryptMessage(
        hServer,
        phContext,
        pMessage,
        MessageSeqNo,
        pfQoP
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
    //memset(pMessage, 0, sizeof(SecBufferDesc));
    *pfQoP = 0;
    goto cleanup;
}
