
DWORD
NtlmClientQuerySecurityContextAttributes(
    PCtxtHandle phContext,
    ULONG ulAttribute,
    PVOID pBuffer
    )
{
    DWORD dwError = 0;
    dwError = NtlmTransactQuerySecurityContextAttributes(
                    hServer,
                    phContext,
                    ulAttribute,
                    pBuffer
                    );
    return(dwError);
}
