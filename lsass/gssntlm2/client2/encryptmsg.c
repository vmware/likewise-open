
DWORD
NtlmClientEncryptMessage(
    PCtxtHandle phContext,
    ULONG fQoP,
    PSecBufferDesc pMessage,
    ULONG MessageSeqNo
    )
{
    DWORD dwError = 0;
    dwError = NtlmTransactEncryptMessage(
                    hServer,
                    phContext,
                    fQoP,
                    pMessage,
                    MessageSeqNo
                    );
    return(dwError);
}
