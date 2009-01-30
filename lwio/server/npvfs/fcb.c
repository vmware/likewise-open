#include "npfs.h"


NTSTATUS
NpfsCreateFCB(
    PUNICODE_STRING pUnicodeString,
    PNPFS_FCB * ppFCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_FCB pFCB = NULL;

    ntStatus = NpfsAllocateMemory(
                        sizeof(NPFS_FCB),
                        &pFCB
                        );
    BAIL_ON_NT_STATUS(ntStatus);
    ntStatus = RtlUnicodeStringDuplicate(
                    &pFCB->PipeName,
                    pUnicodeString
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pFCB->pNext = gpFCB;
    gpFCB = pFCB;

    *ppFCB = pFCB;

    return(ntStatus);

error:

    if (pFCB) {
        NpfsFreeMemory(pFCB);
    }

    *ppFCB = NULL;

    return(ntStatus);
}

NTSTATUS
NpfsFindFCB(
    PUNICODE_STRING pUnicodeString,
    PNPFS_FCB * ppFCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_FCB pFCB = NULL;
    BOOLEAN bEqual = FALSE;

    pFCB = gpFCB;
    while (pFCB) {
         bEqual = RtlUnicodeStringIsEqual(
                            pUnicodeString,
                            &pFCB->PipeName,
                            FALSE
                            );
        if (bEqual) {
            *ppFCB = pFCB;
            return (ntStatus);
        }
        pFCB = pFCB->pNext;
    }
    ntStatus = STATUS_OBJECT_NAME_NOT_FOUND;
    *ppFCB = NULL;
    return(ntStatus);
}



