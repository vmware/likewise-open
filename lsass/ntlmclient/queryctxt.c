#include "client.h"

DWORD
NtlmClientQuerySecurityContextAttributes(
    IN PCtxtHandle phContext,
    IN ULONG ulAttribute,
    OUT PVOID pBuffer
    )
{
    DWORD dwError = 0;
    HANDLE hServer = INVALID_HANDLE;

    dwError = NtlmOpenServer(
        &hServer
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmTransactQuerySecurityContextAttributes(
        hServer,
        phContext,
        ulAttribute,
        pBuffer
        );

cleanup:
    if(INVALID_HANDLE != hServer)
    {
        NtlmCloseServer(hServer);
    }
    return(dwError);
error:
    goto cleanup;
}
