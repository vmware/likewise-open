
DWORD
NtlmClientVerifySignature(
    PCtxtHandle phContext,
    PSecBufferDesc pMessage,
    ULONG MessageSeqNo
    )
{
    DWORD dwError = 0;
    dwError = NtlmTransactVerifySignature(
                    hServer,
                    phContext,
                    pMessage,
                    MessageSeqNo
                    );
    return(dwError);
}
