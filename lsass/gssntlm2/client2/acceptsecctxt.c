
DWORD
NtlmClientAcceptSecurityContext(
    )
{
    DWORD dwError = 0;
    dwError = NtlmTransactAcceptSecurityContext(
                    hServer,
                    phCredential,
                    phContext,
                    pInput,
                    fContextReq,
                    TargetDataRep,
                    phNewContext,
                    pOutput,
                    pfContextAttr,
                    ptsTimeStamp
                    );
    return(dwError);
}
