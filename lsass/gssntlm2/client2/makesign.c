
DWORD
NtlmClientMakeSignature(
    PCtxtHandle phContext,
    ULONG fQoP,
    PSecBufferDesc pMessage,
    ULONG MessageSeqNo
    )
{
    DWORD dwError = 0;
    dwError = NtlmTransactMakeSignature(
                    hServer,
                    phContext,
                    fQoP,
                    pMessage,
                    MessageSeqNo
                    );
    return(dwError);
}
