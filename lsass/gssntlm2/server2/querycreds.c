
DWORD
NtlmServerQueryCredentialsAttributes(
    PCredHandle phCredential,
    ULONG ulAttribute,
    PVOID pBuffer
    )
{
    DWORD dwError = 0;
    dwError = NtlmTransactQueryCredentialsAttributes(
                    hServer,
                    phCredential,
                    ulAttribute,
                    pBuffer
                    );
    return(dwError);
}
