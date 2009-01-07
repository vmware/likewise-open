#include "includes.h"

DWORD
RdrGetSessionKey(
    HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    )
{
    DWORD dwError = 0;
    PSMB_CLIENT_FILE_HANDLE pFile = (PSMB_CLIENT_FILE_HANDLE)hFile;
    PSMB_SESSION pSession = NULL;
    PBYTE pSessionKey = NULL;
    BOOLEAN bInLock = FALSE;

    /* Because the handle keeps a reference count on the tree, and the tree on
       the session, and the session on the socket, it is safe to access the
       session structure without locking the socket hash mutex to protect
       against reaping. */

    BAIL_ON_INVALID_SMBHANDLE(hFile);
    BAIL_ON_INVALID_POINTER(pdwSessionKeyLength);
    BAIL_ON_INVALID_POINTER(ppSessionKey);

    pSession = pFile->pTree->pSession;

    SMB_LOCK_MUTEX(bInLock, &pSession->mutex);

    BAIL_ON_INVALID_POINTER(pSession->pSessionKey);

    dwError = SMBAllocateMemory(
                    pSession->dwSessionKeyLength,
                    (PVOID*)&pSessionKey);
    BAIL_ON_SMB_ERROR(dwError);

    memcpy(pSessionKey, pSession->pSessionKey, pSession->dwSessionKeyLength);

    *pdwSessionKeyLength = pSession->dwSessionKeyLength;
    *ppSessionKey = pSessionKey;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pSession->mutex);

    return dwError;

error:

    *pdwSessionKeyLength = 0;
    *ppSessionKey = NULL;

    goto cleanup;
}
