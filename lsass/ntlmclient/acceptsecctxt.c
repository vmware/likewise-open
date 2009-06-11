#include "client.h"

DWORD
NtlmClientAcceptSecurityContext(
    IN PCredHandle phCredential,
    IN OUT PCtxtHandle phContext,
    IN PSecBufferDesc pInput,
    IN ULONG fContextReq,
    IN ULONG TargetDataRep,
    IN OUT PCtxtHandle phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT PULONG  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    )
{
    DWORD dwError = 0;
    HANDLE hServer = INVALID_HANDLE;

    memset(ptsTimeStamp, 0, sizeof(TimeStamp));
    *pfContextAttr = 0;

    dwError = NtlmOpenServer(&hServer);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmTransactAcceptSecurityContext(
        hServer,
        phCredential,
        phContext,
        pInput,
        fContextReq,
        TargetDataRep,
        phNewContext,
        pOutput,
        pfContextAttr,
        ptsTimeStamp
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
    memset(phContext, 0, sizeof(CtxtHandle));
    memset(phNewContext, 0, sizeof(CtxtHandle));
    memset(pOutput, 0, sizeof(SecBufferDesc));
    memset(ptsTimeStamp, 0, sizeof(TimeStamp));
    *pfContextAttr = 0;
    goto cleanup;
}
