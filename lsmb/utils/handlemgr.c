#include "includes.h"
#include "handlemgr_p.h"

static
size_t
SMBHandleManagerHashKey(
    PCVOID pHandleId
    );

static
VOID
SMBHandleManagerFreeKey(
    const SMB_HASH_ENTRY *pEntry
    );

DWORD
SMBHandleManagerCreate(
    IN  SMBHANDLE dwHandleMax,
    OUT PSMB_HANDLE_MANAGER* ppHandleManager
    )
{
    DWORD dwError = 0;
    PSMB_HANDLE_MANAGER pHandleManager = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_HANDLE_MANAGER),
                    (PVOID*)&pHandleManager);
    BAIL_ON_SMB_ERROR(dwError);

    pHandleManager->dwHandleMax = dwHandleMax ? dwHandleMax : SMB_DEFAULT_HANDLE_MAX;

    dwError = SMBBitVectorCreate(
                    pHandleManager->dwHandleMax,
                    &pHandleManager->pFreeHandleIndex);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBHashCreate(
                    pHandleManager->dwHandleMax % 421,
                    &SMBHashCompareUINT32,
                    &SMBHandleManagerHashKey,
                    &SMBHandleManagerFreeKey,
                    &pHandleManager->pHandleTable);
    BAIL_ON_SMB_ERROR(dwError);

    *ppHandleManager = pHandleManager;

cleanup:

    return dwError;

error:

    *ppHandleManager = NULL;

    if (pHandleManager)
    {
        SMBHandleManagerFree(pHandleManager);
    }

    goto cleanup;
}

DWORD
SMBHandleManagerAddItem(
    IN  PSMB_HANDLE_MANAGER pHandleMgr,
    IN  SMBHandleType       hType,
    IN  PVOID               pItem,
    OUT PSMBHANDLE          phItem
    )
{
    DWORD dwError = 0;
    PSMB_HANDLE_TABLE_ENTRY pEntry = NULL;
    PDWORD pKey = NULL;
    DWORD dwAvailableIndex = 0;

    BAIL_ON_INVALID_POINTER(pHandleMgr);
    BAIL_ON_INVALID_POINTER(pItem);

    dwError = SMBBitVectorFirstUnsetBit(
                pHandleMgr->pFreeHandleIndex,
                &dwAvailableIndex);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBAllocateMemory(
                    sizeof(SMB_HANDLE_TABLE_ENTRY),
                    (PVOID*)&pEntry);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBAllocateMemory(
                    sizeof(DWORD),
                    (PVOID*)&pKey);
    BAIL_ON_SMB_ERROR(dwError);

    pEntry->hType = hType;
    pEntry->pItem = pItem;

    *pKey = dwAvailableIndex;

    dwError = SMBHashSetValue(
                    pHandleMgr->pHandleTable,
                    pKey,
                    pEntry);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBBitVectorSetBit(
                    pHandleMgr->pFreeHandleIndex,
                    dwAvailableIndex);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    return dwError;

error:

    if (dwError == SMB_ERROR_NO_BIT_AVAILABLE)
    {
        dwError = SMB_ERROR_OUT_OF_HANDLES;
    }

    SMB_SAFE_FREE_MEMORY(pEntry);
    SMB_SAFE_FREE_MEMORY(pKey);

    goto cleanup;
}

DWORD
SMBHandleManagerGetItem(
    IN  PSMB_HANDLE_MANAGER pHandleMgr,
    IN  SMBHANDLE           hItem,
    OUT SMBHandleType*      phType,
    OUT PVOID*              ppItem
    )
{
    DWORD dwError = 0;
    PSMB_HANDLE_TABLE_ENTRY pEntry = NULL;

    BAIL_ON_INVALID_POINTER(pHandleMgr);
    BAIL_ON_INVALID_POINTER(ppItem);
    BAIL_ON_INVALID_POINTER(phType);
    BAIL_ON_INVALID_SMBHANDLE(hItem);

    // Make sure this handle is currently assigned
    if (!SMBBitVectorIsSet(
                pHandleMgr->pFreeHandleIndex,
                hItem))
    {
        dwError = SMB_ERROR_INVALID_HANDLE;
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = SMBHashGetValue(
                pHandleMgr->pHandleTable,
                &hItem,
                (PVOID*)&pEntry);
    BAIL_ON_SMB_ERROR(dwError);

    *phType = pEntry->hType;
    *ppItem = pEntry->pItem;

cleanup:

    return dwError;

error:

    if (ppItem)
    {
        *ppItem = NULL;
    }

    if (phType)
    {
        *phType = SMB_HANDLE_TYPE_UNKNOWN;
    }

    if (dwError == ENOENT)
    {
        dwError = SMB_ERROR_INVALID_HANDLE;
    }

    goto cleanup;
}

DWORD
SMBHandleManagerDeleteItem(
    IN  PSMB_HANDLE_MANAGER     pHandleMgr,
    IN  SMBHANDLE               hItem,
    OUT OPTIONAL SMBHandleType* phType,
    OUT OPTIONAL PVOID*         ppItem
    )
{
    DWORD dwError = 0;
    PSMB_HANDLE_TABLE_ENTRY pEntry = NULL;
    PVOID pItem = NULL;
    SMBHandleType hType = SMB_HANDLE_TYPE_UNKNOWN;

    BAIL_ON_INVALID_POINTER(pHandleMgr);
    BAIL_ON_INVALID_SMBHANDLE(hItem);

    // Make sure this handle is currently assigned
    if (!SMBBitVectorIsSet(
                pHandleMgr->pFreeHandleIndex,
                hItem))
    {
        dwError = SMB_ERROR_INVALID_HANDLE;
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = SMBHashGetValue(
                pHandleMgr->pHandleTable,
                &hItem,
                (PVOID*)&pEntry);
    BAIL_ON_SMB_ERROR(dwError);

    hType = pEntry->hType;
    pItem = pEntry->pItem;

    dwError = SMBHashRemoveKey(
                    pHandleMgr->pHandleTable,
                    &hItem);
    BAIL_ON_SMB_ERROR(dwError);

    // Indicate that this id is available
    dwError = SMBBitVectorUnsetBit(
                pHandleMgr->pFreeHandleIndex,
                hItem);
    BAIL_ON_SMB_ERROR(dwError);

    if (ppItem)
    {
        *ppItem = pItem;
    }

    if (phType)
    {
        *phType = hType;
    }

cleanup:

    return dwError;

error:

    if (ppItem)
    {
        *ppItem = NULL;
    }

    if (phType)
    {
        *phType = SMB_HANDLE_TYPE_UNKNOWN;
    }

    if (dwError == ENOENT)
    {
    dwError = SMB_ERROR_INVALID_HANDLE;
    }

    goto cleanup;
}

VOID
SMBHandleManagerFree(
    IN PSMB_HANDLE_MANAGER pHandleManager
    )
{
    SMBHashSafeFree(&pHandleManager->pHandleTable);

    if (pHandleManager->pFreeHandleIndex)
    {
        SMBBitVectorFree(pHandleManager->pFreeHandleIndex);
    }

    SMBFreeMemory(pHandleManager);
}

static
size_t
SMBHandleManagerHashKey(
    PCVOID pHandleId
    )
{
    return (pHandleId ? *(uint32_t*)pHandleId : 0);
}

static
VOID
SMBHandleManagerFreeKey(
    const SMB_HASH_ENTRY *pEntry
    )
{
    PVOID pHandleTableEntry = (PVOID)pEntry->pKey;
    SMB_SAFE_FREE_MEMORY(pHandleTableEntry);
}

