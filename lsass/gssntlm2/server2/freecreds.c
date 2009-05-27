
DWORD
NtlmClientFreeCredentialsHandle(
    PCredHandle phCredential
    )
{
    DWORD dwError = 0;
    dwError = NtlmTransactFreeCredentialsHandle(
                    hServer,
                    phCredential
                    );
    return(dwError);
}
