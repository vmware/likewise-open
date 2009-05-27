
DWORD
NtlmClientDecryptMessage(
    PCtxtHandle phContext,
    PSecBufferDesc pMessage,
    ULONG MessageSeqNo,
    PULONG pfQoP
    )
{
    DWORD dwError = 0;

    dwError = NtlmTransactDecryptMessage(
                    hServer,
                    phContext,
                    pMessage,
                    MessageSeqNo,
                    pfQoP
                    );
    return(dwError);
}
