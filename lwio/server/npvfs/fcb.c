NTSTATUS
CreateNamedPipe(
    )
{
    NTSTATUS ntStatus = 0;

    ENTER_CRITICAL_SECTION(&pFcbLock);

    ntStatus = NpfsFindFcb(
                    pIrpContext,
                    &pFcb
                    );
    if (ntStatus! = 0  && ntStatus!= STATUS_OBJECT_EXISTS)
        BAIL_ON_NT_STATUS(ntStatus);
    }else if (ntStatus == STATUS_OBJECT_NOT_FOUND){

        ntStatus = ValidateServerSecurity(
                        hAccessToken,
                        gGlobals->pSecurityDescriptor
                        );
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NpfsCreateFCB_FirstInstance(
                        pIrpContext,
                        &pFcb,
                        &pScb
                        );
        BAIL_ON_NT_STATUS(ntStatus);
    }else {

        ntStatus = NpfsCreateScb(
                        pIrpContext,
                        pFcb,
                        &pScb
                        );
        BAIL_ON_NT_STATUS(ntStatus);
    }

    IoFileSetContext(hFile, pScb);

error:

    LEAVE_CRITICAL_SECTION(&pFcbLock);

    return(ntStatus);

}



