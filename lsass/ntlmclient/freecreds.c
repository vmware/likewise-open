#include "clientipc.h"

DWORD
NtlmClientFreeCredentialsHandle(
    IN PCredHandle phCredential
    )
{
    DWORD dwError = 0;
    HANDLE hServer = INVALID_HANDLE;

    dwError = NtlmOpenServer(
        &hServer
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmTransactFreeCredentialsHandle(
        hServer,
        phCredential
        );

error:
    if(INVALID_HANDLE != hServer)
    {
        NtlmCloseServer(hServer);
    }
    return(dwError);
}
