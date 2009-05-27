
DWORD
NtlmServerInitializeSecurityContext(
    PCredHandle phCredential,
    PCtxtHandle phContext,
    SEC_CHAR * pszTargetName,
    ULONG fContextReq,
    ULONG Reserverd1.
    ULONG TargetDataRep,
    PSecBufferDesc pInput,
    ULONG Reserved2,
    PCtxtHandle phNewContext,
    PSecBufferDesc pOutput,
    PULONG pfContextAttr,
    PTimeStamp ptsExpiry
    )
{
    DWORD dwError = 0;

    return(dwError);
}
