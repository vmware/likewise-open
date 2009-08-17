/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        state_store.c
 *
 * Abstract:
 *
 *        Caching for AD Provider Database Interface
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */

#include "adprovider.h"

// The database consists of several tables.  Because we're
// using a flat file and the amount of data is small, the
// easiest way to update it is to replace the entire file.
// However, we may be asked to write some of it before we
// have been asked to read all of it.  The simplest solution
// is to preload everything into memory and hold it until
// either we're asked for it or we determine it isn't needed.
typedef struct _ADSTATE_CONNECTION
{
    pthread_rwlock_t lock;
} ADSTATE_CONNECTION, *PADSTATE_CONNECTION;

// This is the maximum number of characters necessary to store a guid in
// string form.
#define UUID_STR_SIZE 37

#define ADSTATE_DB       LSASS_DB_DIR "/lsass-adstate.filedb"
#define ADSTATE_DB_TEMP  ADSTATE_DB ".temp"

#define FILEDB_FORMAT_TYPE "LFLT"
#define FILEDB_FORMAT_VERSION 1

#define FILEDB_DATA_TYPE_PROVIDER    1
#define FILEDB_DATA_TYPE_LINKEDCELL  2
#define FILEDB_DATA_TYPE_DOMAINTRUST 3

#define ENTER_ADSTATE_DB_RW_READER_LOCK(pLock, bInLock) \
    if (!bInLock) {                                 \
        pthread_rwlock_rdlock(pLock);               \
        bInLock = TRUE;                             \
    }
#define LEAVE_ADSTATE_DB_RW_READER_LOCK(pLock, bInLock) \
    if (bInLock) {                                  \
        pthread_rwlock_unlock(pLock);               \
        bInLock = FALSE;                            \
    }

#define ENTER_ADSTATE_DB_RW_WRITER_LOCK(pLock, bInLock) \
    if (!bInLock) {                                 \
        pthread_rwlock_wrlock(pLock);               \
        bInLock = TRUE;                             \
    }
#define LEAVE_ADSTATE_DB_RW_WRITER_LOCK(pLock, bInLock) \
    if (bInLock) {                                  \
        pthread_rwlock_unlock(pLock);               \
        bInLock = FALSE;                            \
    }

typedef struct _AD_FILEDB_PROVIDER_DATA
{
    DWORD dwDirectoryMode;
    ADConfigurationMode adConfigurationMode;
    UINT64 adMaxPwdAge;
    PSTR  pszDomain;
    PSTR  pszShortDomain;
    PSTR  pszComputerDN;
    PSTR  pszCellDN;
} AD_FILEDB_PROVIDER_DATA, *PAD_FILEDB_PROVIDER_DATA;

typedef struct _AD_FILEDB_DOMAIN_INFO
{
    PSTR pszDnsDomainName;
    PSTR pszNetbiosDomainName;
    PSTR pszSid;
    PSTR pszGuid;
    PSTR pszTrusteeDnsDomainName;
    DWORD dwTrustFlags;
    DWORD dwTrustType;
    DWORD dwTrustAttributes;
    LSA_TRUST_DIRECTION dwTrustDirection;
    LSA_TRUST_MODE dwTrustMode;
    // Can be NULL (e.g. external trust)
    PSTR pszForestName;
    PSTR pszClientSiteName;
    LSA_DM_DOMAIN_FLAGS Flags;
    PLSA_DM_DC_INFO DcInfo;
    PLSA_DM_DC_INFO GcInfo;
} AD_FILEDB_DOMAIN_INFO, *PAD_FILEDB_DOMAIN_INFO;

LWMsgTypeSpec gADStateProviderDataCacheSpec[] =
{
    LWMSG_STRUCT_BEGIN(AD_FILEDB_PROVIDER_DATA),
    LWMSG_MEMBER_UINT32(AD_FILEDB_PROVIDER_DATA, dwDirectoryMode),
    LWMSG_MEMBER_UINT8(AD_FILEDB_PROVIDER_DATA, adConfigurationMode),
    LWMSG_MEMBER_PSTR(AD_FILEDB_PROVIDER_DATA, pszDomain),
    LWMSG_MEMBER_PSTR(AD_FILEDB_PROVIDER_DATA, pszShortDomain),
    LWMSG_MEMBER_PSTR(AD_FILEDB_PROVIDER_DATA, pszComputerDN),
    LWMSG_MEMBER_PSTR(AD_FILEDB_PROVIDER_DATA, pszCellDN),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec gADStateLinkedCellCacheSpec[] =
{
    LWMSG_STRUCT_BEGIN(AD_LINKED_CELL_INFO),
    LWMSG_MEMBER_PSTR(AD_LINKED_CELL_INFO, pszCellDN),
    LWMSG_MEMBER_PSTR(AD_LINKED_CELL_INFO, pszDomain),
    LWMSG_MEMBER_UINT8(AD_LINKED_CELL_INFO, bIsForestCell),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec gADStateDomainTrustCacheSpec[] =
{
    LWMSG_STRUCT_BEGIN(AD_FILEDB_DOMAIN_INFO),
    LWMSG_MEMBER_PSTR(AD_FILEDB_DOMAIN_INFO, pszDnsDomainName),
    LWMSG_MEMBER_PSTR(AD_FILEDB_DOMAIN_INFO, pszNetbiosDomainName),
    LWMSG_MEMBER_PSTR(AD_FILEDB_DOMAIN_INFO, pszSid),
    LWMSG_MEMBER_PSTR(AD_FILEDB_DOMAIN_INFO, pszGuid),
    LWMSG_MEMBER_PSTR(AD_FILEDB_DOMAIN_INFO, pszTrusteeDnsDomainName),
    LWMSG_MEMBER_UINT32(AD_FILEDB_DOMAIN_INFO, dwTrustFlags),
    LWMSG_MEMBER_UINT32(AD_FILEDB_DOMAIN_INFO, dwTrustType),
    LWMSG_MEMBER_UINT32(AD_FILEDB_DOMAIN_INFO, dwTrustAttributes),
    LWMSG_MEMBER_UINT32(AD_FILEDB_DOMAIN_INFO, dwTrustDirection),
    LWMSG_MEMBER_UINT32(AD_FILEDB_DOMAIN_INFO, dwTrustMode),
    LWMSG_MEMBER_PSTR(AD_FILEDB_DOMAIN_INFO, pszForestName),
    LWMSG_MEMBER_PSTR(AD_FILEDB_DOMAIN_INFO, pszClientSiteName),
    LWMSG_MEMBER_UINT32(AD_FILEDB_DOMAIN_INFO, Flags),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static
DWORD
ADState_ReadFromFile(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    OUT OPTIONAL PAD_PROVIDER_DATA* ppResult,
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    OUT OPTIONAL PDLINKEDLIST* ppDomainList
    );

static
DWORD
ADState_UnmarshalProviderData(
    IN LWMsgDataContext * pDataContext,
    IN PVOID pData,
    IN size_t DataSize,
    OUT PAD_PROVIDER_DATA * ppProviderData
    );

static
DWORD
ADState_UnmarshalLinkedCellData(
    IN LWMsgDataContext * pDataContext,
    IN PVOID pData,
    IN size_t DataSize,
    IN OUT PDLINKEDLIST * ppCellList
    );

static
DWORD
ADState_UnmarshalDomainTrustData(
    IN LWMsgDataContext * pDataContext,
    IN PVOID pData,
    IN size_t DataSize,
    IN OUT PDLINKEDLIST * ppDomainList
    );

static
VOID
ADState_FreeEnumDomainInfoCallback(
    IN OUT PVOID pData,
    IN PVOID pUserData
    );

VOID
ADState_FreeEnumDomainInfo(
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo
    );

static
DWORD
ADState_WriteToFile(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    IN OPTIONAL PAD_PROVIDER_DATA pProviderData,
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo,
    IN OPTIONAL DWORD dwDomainInfoCount,
    IN PLSA_DM_ENUM_DOMAIN_INFO pDomainInfoAppend
    );

static
DWORD
ADState_WriteProviderData(
    IN FILE * pFileDb,
    IN LWMsgDataContext * pDataContext,
    IN PAD_PROVIDER_DATA pProviderData
    );

static
DWORD
ADState_WriteCellEntry(
    IN FILE * pFileDb,
    IN LWMsgDataContext * pDataContext,
    IN PAD_LINKED_CELL_INFO pCellEntry
    );

static
DWORD
ADState_WriteDomainEntry(
    IN FILE * pFileDb,
    IN LWMsgDataContext * pDataContext,
    IN PLSA_DM_ENUM_DOMAIN_INFO pDomainInfoEntry
    );

static
DWORD
ADState_CopyFromFile(
    IN FILE * pFileDb,
    IN BOOLEAN bCopyProviderData,
    IN BOOLEAN bCopyDomainTrusts
    );

static
DWORD
ADState_WriteOneEntry(
    IN FILE * pFileDb,
    IN DWORD dwType,
    IN size_t DataSize,
    IN PVOID pData
    );

DWORD
ADState_OpenDb(
    ADSTATE_CONNECTION_HANDLE* phDb
    )
{
    DWORD dwError = 0;
    BOOLEAN bLockCreated = FALSE;
    PADSTATE_CONNECTION pConn = NULL;
    BOOLEAN bExists = FALSE;
    FILE * pFileDb = NULL;
    size_t Cnt = 0;
    PBYTE FormatType = (PBYTE)FILEDB_FORMAT_TYPE;
    DWORD dwVersion = FILEDB_FORMAT_VERSION;

    dwError = LwAllocateMemory(
                    sizeof(ADSTATE_CONNECTION),
                    (PVOID*)&pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pthread_rwlock_init(&pConn->lock, NULL);
    BAIL_ON_LSA_ERROR(dwError);
    bLockCreated = TRUE;

    dwError = LsaCheckDirectoryExists(LSASS_DB_DIR, &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU;

        dwError = LsaCreateDirectory(LSASS_DB_DIR, cacheDirMode);
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* restrict access to u+rwx to the db folder */
    dwError = LsaChangeOwnerAndPermissions(LSASS_DB_DIR, 0, 0, S_IRWXU);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckFileExists(
        ADSTATE_DB,
        &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists)
    {
        pFileDb = fopen(ADSTATE_DB, "w");
        if (pFileDb == NULL)
        {
            dwError = errno;
        }
        BAIL_ON_LSA_ERROR(dwError);

        Cnt = fwrite(FormatType, sizeof(BYTE) * 4, 1, pFileDb);
        if (Cnt != 1)
        {
            dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
        }
        BAIL_ON_LSA_ERROR(dwError);

        Cnt = fwrite(&dwVersion, sizeof(dwVersion), 1, pFileDb);
        if (Cnt != 1)
        {
            dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaChangeOwnerAndPermissions(ADSTATE_DB, 0, 0, S_IRWXU);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *phDb = pConn;

cleanup:

    if (pFileDb != NULL)
    {
        fclose(pFileDb);
    }

    return dwError;

error:

    if (pConn != NULL)
    {
        if (bLockCreated)
        {
            pthread_rwlock_destroy(&pConn->lock);
        }

        LW_SAFE_FREE_MEMORY(pConn);
    }
    *phDb = NULL;

    goto cleanup;
}

void
ADState_SafeCloseDb(
    ADSTATE_CONNECTION_HANDLE* phDb
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    ADSTATE_CONNECTION_HANDLE hDb = NULL;

    if (phDb == NULL || *phDb == NULL)
    {
        goto cleanup;
    }

    hDb = *phDb;

    dwError = pthread_rwlock_destroy(&hDb->lock);
    if (dwError != LW_ERROR_SUCCESS)
    {
        LSA_LOG_ERROR("Error destroying lock [%d]", dwError);
        dwError = LW_ERROR_SUCCESS;
    }

    LW_SAFE_FREE_MEMORY(hDb);

cleanup:
    return;
}

DWORD
ADState_GetProviderData(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    OUT PAD_PROVIDER_DATA* ppResult
    )
{
    return ADState_ReadFromFile(
               hDb,
               ppResult,
               NULL);
}

DWORD
ADState_StoreProviderData(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    IN PAD_PROVIDER_DATA pProvider
    )
{
    return ADState_WriteToFile(
               hDb,
               pProvider,
               NULL,
               0,
               NULL);
}

DWORD
ADState_GetDomainTrustList(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    OUT PDLINKEDLIST* ppList
    )
{
    return ADState_ReadFromFile(
               hDb,
               NULL,
               ppList);
}

DWORD
ADState_AddDomainTrust(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    IN PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo
    )
{
    return ADState_WriteToFile(
               hDb,
               NULL,
               NULL,
               0,
               pDomainInfo);
}

DWORD
ADState_StoreDomainTrustList(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    IN PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo,
    IN DWORD dwDomainInfoCount
    )
{
    return ADState_WriteToFile(
               hDb,
               NULL,
               ppDomainInfo,
               dwDomainInfoCount,
               NULL);
}

static
DWORD
ADState_ReadFromFile(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    OUT OPTIONAL PAD_PROVIDER_DATA* ppProviderData,
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    OUT OPTIONAL PDLINKEDLIST* ppDomainList
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    FILE * pFileDb = NULL;
    size_t Cnt = 0;
    BYTE FormatType[4];
    DWORD dwVersion = 0;
    DWORD dwType = 0;
    size_t DataMaxSize = 0;
    size_t DataSize = 0;
    PVOID pData = NULL;
    LWMsgContext * pContext = NULL;
    LWMsgDataContext * pDataContext = NULL;
    PAD_PROVIDER_DATA pProviderData = NULL;
    PDLINKEDLIST pCellList = NULL;
    PDLINKEDLIST pDomainList = NULL;

    memset(FormatType, 0, sizeof(FormatType));

    ENTER_ADSTATE_DB_RW_READER_LOCK(&hDb->lock, bInLock);

    pFileDb = fopen(ADSTATE_DB, "r");
    if (pFileDb == NULL)
    {
        dwError = errno;
    }
    BAIL_ON_LSA_ERROR(dwError);

    Cnt = fread(&FormatType, sizeof(FormatType), 1, pFileDb);
    if (Cnt == 0)
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    else if (Cnt != 1)
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LSA_ERROR(dwError);

    Cnt = fread(&dwVersion, sizeof(dwVersion), 1, pFileDb);
    if (Cnt != 1)
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &pContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(pContext, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    while (1)
    {
        Cnt = fread(&dwType, sizeof(dwType), 1, pFileDb);
        if (Cnt == 0)
        {
            break;
        }
        else if (Cnt != 1)
        {
            dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
        }
        BAIL_ON_LSA_ERROR(dwError);

        Cnt = fread(&DataSize, sizeof(DataSize), 1, pFileDb);
        if (Cnt == 0)
        {
            break;
        }
        else if (Cnt != 1)
        {
            dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
        }
        BAIL_ON_LSA_ERROR(dwError);

        if (DataSize > DataMaxSize)
        {
            DataMaxSize = DataSize * 2;

            dwError = LwReallocMemory(
                          pData,
                          (PVOID*)&pData,
                          DataMaxSize);
            BAIL_ON_LSA_ERROR(dwError);
        }

        Cnt = fread(pData, DataSize, 1, pFileDb);
        if (Cnt != 1)
        {
            dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
        }
        BAIL_ON_LSA_ERROR(dwError);

        switch (dwType)
        {
            case FILEDB_DATA_TYPE_PROVIDER:
                if (ppProviderData)
                {
                    if (pProviderData)
                    {
                        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
                    }
                    else
                    {
                        dwError = ADState_UnmarshalProviderData(
                                      pDataContext,
                                      pData,
                                      DataSize,
                                      &pProviderData);
                    }
                }
                break;
            case FILEDB_DATA_TYPE_LINKEDCELL:
                if (ppProviderData)
                {
                    dwError = ADState_UnmarshalLinkedCellData(
                                  pDataContext,
                                  pData,
                                  DataSize,
                                  &pCellList);
                }
                break;
            case FILEDB_DATA_TYPE_DOMAINTRUST:
                if (ppDomainList)
                {
                    dwError = ADState_UnmarshalDomainTrustData(
                                  pDataContext,
                                  pData,
                                  DataSize,
                                  &pDomainList);
                }
                break;
            default:
                dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
                break;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (ppProviderData)
    {
        if (pProviderData)
        {
            pProviderData->pCellList = pCellList;
            pCellList = NULL;
        }
        *ppProviderData = pProviderData;
        pProviderData = NULL;
    }

    if (ppDomainList)
    {
        *ppDomainList = pDomainList;
        pDomainList = NULL;
    }

cleanup:

    if (pFileDb != NULL)
    {
        fclose(pFileDb);
    }

    LEAVE_ADSTATE_DB_RW_READER_LOCK(&hDb->lock, bInLock);

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }
    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    LW_SAFE_FREE_MEMORY(pData);

    if (pProviderData)
    {
        ADProviderFreeProviderData(pProviderData);
    }
    if (pCellList)
    {
        ADProviderFreeCellList(pCellList);
    }
    if (pDomainList)
    {
        ADState_FreeEnumDomainInfoList(pDomainList);
    }

    return dwError;

error:

    if (ppProviderData)
    {
        *ppProviderData = NULL;
    }
    if (ppDomainList)
    {
        *ppDomainList = NULL;
    }

    goto cleanup;
}

static
DWORD
ADState_UnmarshalProviderData(
    IN LWMsgDataContext * pDataContext,
    IN PVOID pData,
    IN size_t DataSize,
    OUT PAD_PROVIDER_DATA * ppProviderData
    )
{
    DWORD dwError = 0;
    PAD_FILEDB_PROVIDER_DATA pFileDbData = NULL;
    PAD_PROVIDER_DATA pProviderData = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                  pDataContext,
                  gADStateProviderDataCacheSpec,
                  pData,
                  DataSize,
                  (PVOID*)&pFileDbData));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                  sizeof(*pProviderData),
                  (PVOID*)&pProviderData);
    BAIL_ON_LSA_ERROR(dwError);

    pProviderData->dwDirectoryMode = pFileDbData->dwDirectoryMode;
    pProviderData->adConfigurationMode = pFileDbData->adConfigurationMode;
    pProviderData->adMaxPwdAge = pFileDbData->adMaxPwdAge;

    if (pFileDbData->pszDomain)
    {
        strncpy(
            pProviderData->szDomain,
            pFileDbData->pszDomain,
            sizeof(pProviderData->szDomain));
    }

    if (pFileDbData->pszShortDomain)
    {
        strncpy(
            pProviderData->szShortDomain,
            pFileDbData->pszShortDomain,
            sizeof(pProviderData->szShortDomain));
    }

    if (pFileDbData->pszComputerDN)
    {
        strncpy(
            pProviderData->szComputerDN,
            pFileDbData->pszComputerDN,
            sizeof(pProviderData->szComputerDN));
    }

    if (pFileDbData->pszCellDN)
    {
        strncpy(
            pProviderData->cell.szCellDN,
            pFileDbData->pszCellDN,
            sizeof(pProviderData->cell.szCellDN));
    }

    *ppProviderData = pProviderData;

cleanup:

    if (pFileDbData)
    {
        lwmsg_data_free_graph(
            pDataContext,
            gADStateProviderDataCacheSpec,
            pFileDbData);
    }

    return dwError;

error:

    *ppProviderData = NULL;

    LW_SAFE_FREE_MEMORY(pProviderData);

    goto cleanup;
}

static
DWORD
ADState_UnmarshalLinkedCellData(
    IN LWMsgDataContext * pDataContext,
    IN PVOID pData,
    IN size_t DataSize,
    IN OUT PDLINKEDLIST * ppCellList
    )
{
    DWORD dwError = 0;
    PAD_LINKED_CELL_INFO pCellEntry = NULL;
    PAD_LINKED_CELL_INFO pListEntry = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                  pDataContext,
                  gADStateLinkedCellCacheSpec,
                  pData,
                  DataSize,
                  (PVOID*)&pCellEntry));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                  sizeof(*pListEntry),
                  (PVOID*)&pListEntry);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                  pCellEntry->pszCellDN,
                  &pListEntry->pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                  pCellEntry->pszDomain,
                  &pListEntry->pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    pListEntry->bIsForestCell = pCellEntry->bIsForestCell;

    dwError = LsaDLinkedListAppend(
                  ppCellList,
                  pListEntry);
    BAIL_ON_LSA_ERROR(dwError);
    pListEntry = NULL;

cleanup:

    if (pCellEntry)
    {
        lwmsg_data_free_graph(
            pDataContext,
            gADStateLinkedCellCacheSpec,
            pCellEntry);
    }

    return dwError;

error:

    if (pListEntry)
    {
        ADProviderFreeCellInfo(pListEntry);
    }

    goto cleanup;
}

static
DWORD
ADState_UnmarshalDomainTrustData(
    IN LWMsgDataContext * pDataContext,
    IN PVOID pData,
    IN size_t DataSize,
    IN OUT PDLINKEDLIST * ppDomainList
    )
{
    DWORD dwError = 0;
    PAD_FILEDB_DOMAIN_INFO pDomainInfo = NULL;
    PLSA_DM_ENUM_DOMAIN_INFO pListEntry = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                  pDataContext,
                  gADStateDomainTrustCacheSpec,
                  pData,
                  DataSize,
                  (PVOID*)&pDomainInfo));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                  sizeof(*pListEntry),
                  (PVOID*)&pListEntry);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                  pDomainInfo->pszDnsDomainName,
                  &pListEntry->pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                  pDomainInfo->pszNetbiosDomainName,
                  &pListEntry->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    if (pDomainInfo->pszSid)
    {
        dwError = LsaAllocateSidFromCString(
                      &pListEntry->pSid,
                      pDomainInfo->pszSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pDomainInfo->pszGuid)
    {
        dwError = LwAllocateMemory(
                      UUID_STR_SIZE,
                      (PVOID*)&pListEntry->pGuid);
        BAIL_ON_LSA_ERROR(dwError);

        if (uuid_parse(
                pDomainInfo->pszGuid,
                *pListEntry->pGuid) < 0)
        {
            // uuid_parse returns -1 on error, but does not set errno
            dwError = LW_ERROR_INVALID_OBJECTGUID;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = LwStrDupOrNull(
                  pDomainInfo->pszTrusteeDnsDomainName,
                  &pListEntry->pszTrusteeDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    pListEntry->dwTrustFlags = pDomainInfo->dwTrustFlags;
    pListEntry->dwTrustType = pDomainInfo->dwTrustType;
    pListEntry->dwTrustAttributes = pDomainInfo->dwTrustAttributes;
    pListEntry->dwTrustDirection = pDomainInfo->dwTrustDirection;

    dwError = LwStrDupOrNull(
                  pDomainInfo->pszForestName,
                  &pListEntry->pszForestName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                  pDomainInfo->pszClientSiteName,
                  &pListEntry->pszClientSiteName);
    BAIL_ON_LSA_ERROR(dwError);

    pListEntry->Flags = pDomainInfo->Flags;
    pListEntry->DcInfo = NULL;
    pListEntry->GcInfo = NULL;

    dwError = LsaDLinkedListAppend(
                  ppDomainList,
                  pListEntry);
    BAIL_ON_LSA_ERROR(dwError);
    pListEntry = NULL;

cleanup:

    if (pDomainInfo)
    {
        lwmsg_data_free_graph(
            pDataContext,
            gADStateDomainTrustCacheSpec,
            pDomainInfo);
    }

    return dwError;

error:

    if (pListEntry)
    {
        ADState_FreeEnumDomainInfo(pListEntry);
    }

    goto cleanup;
}

VOID
ADState_FreeEnumDomainInfoList(
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    IN OUT PDLINKEDLIST pList
    )
{
    LsaDLinkedListForEach(
        pList,
        ADState_FreeEnumDomainInfoCallback,
        NULL);

    LsaDLinkedListFree(pList);
}

static
VOID
ADState_FreeEnumDomainInfoCallback(
    IN OUT PVOID pData,
    IN PVOID pUserData
    )
{
    PLSA_DM_ENUM_DOMAIN_INFO pInfo =
        (PLSA_DM_ENUM_DOMAIN_INFO)pData;

    ADState_FreeEnumDomainInfo(pInfo);
}

VOID
ADState_FreeEnumDomainInfo(
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo
    )
{
    if (pDomainInfo)
    {
        LW_SAFE_FREE_STRING(pDomainInfo->pszDnsDomainName);
        LW_SAFE_FREE_STRING(pDomainInfo->pszNetbiosDomainName);
        LW_SAFE_FREE_MEMORY(pDomainInfo->pSid);
        LW_SAFE_FREE_MEMORY(pDomainInfo->pGuid);
        LW_SAFE_FREE_STRING(pDomainInfo->pszTrusteeDnsDomainName);
        LW_SAFE_FREE_STRING(pDomainInfo->pszForestName);
        LW_SAFE_FREE_STRING(pDomainInfo->pszClientSiteName);
        if (pDomainInfo->DcInfo)
        {
            // ISSUE-2008/09/10-dalmeida -- need ASSERT macro
            LSA_LOG_ALWAYS("ASSERT!!! - DcInfo should never be set by DB code!");
        }
        if (pDomainInfo->GcInfo)
        {
            // ISSUE-2008/09/10-dalmeida -- need ASSERT macro
            LSA_LOG_ALWAYS("ASSERT!!! - GcInfo should never be set by DB code!");
        }
        LwFreeMemory(pDomainInfo);
    }
}

static
DWORD
ADState_WriteToFile(
    IN ADSTATE_CONNECTION_HANDLE hDb,
    IN OPTIONAL PAD_PROVIDER_DATA pProviderData,
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo,
    IN OPTIONAL DWORD dwDomainInfoCount,
    IN OPTIONAL PLSA_DM_ENUM_DOMAIN_INFO pDomainInfoAppend
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bExists = FALSE;
    FILE * pFileDb = NULL;
    size_t Cnt = 0;
    PBYTE FormatType = (PBYTE)FILEDB_FORMAT_TYPE;
    DWORD dwVersion = FILEDB_FORMAT_VERSION;
    DWORD dwCount = 0;
    LWMsgContext * pContext = NULL;
    LWMsgDataContext * pDataContext = NULL;

    ENTER_ADSTATE_DB_RW_WRITER_LOCK(&hDb->lock, bInLock);

    dwError = LsaCheckDirectoryExists(LSASS_DB_DIR, &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU;

        dwError = LsaCreateDirectory(LSASS_DB_DIR, cacheDirMode);
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* restrict access to u+rwx to the db folder */
    dwError = LsaChangeOwnerAndPermissions(LSASS_DB_DIR, 0, 0, S_IRWXU);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckFileExists(
        ADSTATE_DB_TEMP,
        &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExists)
    {
        dwError = LsaRemoveFile(ADSTATE_DB_TEMP);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaCheckFileExists(
        ADSTATE_DB,
        &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    pFileDb = fopen(ADSTATE_DB_TEMP, "w");
    if (pFileDb == NULL)
    {
        dwError = errno;
    }
    BAIL_ON_LSA_ERROR(dwError);

    Cnt = fwrite(FormatType, sizeof(BYTE) * 4, 1, pFileDb);
    if (Cnt != 1)
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LSA_ERROR(dwError);

    Cnt = fwrite(&dwVersion, sizeof(dwVersion), 1, pFileDb);
    if (Cnt != 1)
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &pContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(pContext, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    if (pProviderData)
    {
        PDLINKEDLIST pCellList = pProviderData->pCellList;

        dwError = ADState_WriteProviderData(
                      pFileDb,
                      pDataContext,
                      pProviderData);
        BAIL_ON_LSA_ERROR(dwError);

        while (pCellList)
        {
            dwError = ADState_WriteCellEntry(
                          pFileDb,
                          pDataContext,
                          pCellList->pItem);
            BAIL_ON_LSA_ERROR(dwError);

            pCellList = pCellList->pNext;
        }
    }
    else
    {
        if (bExists)
        {
            dwError = ADState_CopyFromFile(
                          pFileDb,
                          TRUE,
                          FALSE);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (ppDomainInfo)
    {
        for (dwCount = 0 ; dwCount < dwDomainInfoCount ; dwCount++)
        {
            dwError = ADState_WriteDomainEntry(
                          pFileDb,
                          pDataContext,
                          ppDomainInfo[dwCount]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        if (bExists)
        {
            dwError = ADState_CopyFromFile(
                          pFileDb,
                          FALSE,
                          TRUE);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (pDomainInfoAppend)
    {
        dwError = ADState_WriteDomainEntry(
                      pFileDb,
                      pDataContext,
                      pDomainInfoAppend);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pFileDb != NULL)
    {
        fclose(pFileDb);
        pFileDb = NULL;
    }

    dwError = LsaChangeOwnerAndPermissions(ADSTATE_DB_TEMP, 0, 0, S_IRWXU);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMoveFile(ADSTATE_DB_TEMP, ADSTATE_DB);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pFileDb != NULL)
    {
        fclose(pFileDb);
    }

    LEAVE_ADSTATE_DB_RW_WRITER_LOCK(&hDb->lock, bInLock);

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }
    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ADState_WriteProviderData(
    IN FILE * pFileDb,
    IN LWMsgDataContext * pDataContext,
    IN PAD_PROVIDER_DATA pProviderData
    )
{
    DWORD dwError = 0;
    DWORD dwType = FILEDB_DATA_TYPE_PROVIDER;
    PVOID pData = NULL;
    size_t DataSize = 0;
    AD_FILEDB_PROVIDER_DATA FileDbData;

    memset(&FileDbData, 0, sizeof(FileDbData));

    FileDbData.dwDirectoryMode = pProviderData->dwDirectoryMode;
    FileDbData.adConfigurationMode = pProviderData->adConfigurationMode;
    FileDbData.adMaxPwdAge = pProviderData->adMaxPwdAge;

    dwError = LwStrDupOrNull(
                  pProviderData->szDomain,
                  &FileDbData.pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                  pProviderData->szShortDomain,
                  &FileDbData.pszShortDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                  pProviderData->szComputerDN,
                  &FileDbData.pszComputerDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                  pProviderData->cell.szCellDN,
                  &FileDbData.pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataContext,
                              gADStateProviderDataCacheSpec,
                              &FileDbData,
                              &pData,
                              &DataSize));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_WriteOneEntry(
                  pFileDb,
                  dwType,
                  DataSize,
                  pData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(FileDbData.pszDomain);
    LW_SAFE_FREE_STRING(FileDbData.pszShortDomain);
    LW_SAFE_FREE_STRING(FileDbData.pszComputerDN);
    LW_SAFE_FREE_STRING(FileDbData.pszCellDN);
    LW_SAFE_FREE_MEMORY(pData);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ADState_WriteCellEntry(
    IN FILE * pFileDb,
    IN LWMsgDataContext * pDataContext,
    IN PAD_LINKED_CELL_INFO pCellEntry
    )
{
    DWORD dwError = 0;
    DWORD dwType = FILEDB_DATA_TYPE_LINKEDCELL;
    PVOID pData = NULL;
    size_t DataSize = 0;

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataContext,
                              gADStateLinkedCellCacheSpec,
                              pCellEntry,
                              &pData,
                              &DataSize));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_WriteOneEntry(
                  pFileDb,
                  dwType,
                  DataSize,
                  pData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_MEMORY(pData);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ADState_WriteDomainEntry(
    IN FILE * pFileDb,
    IN LWMsgDataContext * pDataContext,
    IN PLSA_DM_ENUM_DOMAIN_INFO pDomainInfoEntry
    )
{
    DWORD dwError = 0;
    DWORD dwType = FILEDB_DATA_TYPE_DOMAINTRUST;
    PVOID pData = NULL;
    size_t DataSize = 0;
    AD_FILEDB_DOMAIN_INFO FileDbData;
    char szGuid[UUID_STR_SIZE];

    memset(&FileDbData, 0, sizeof(FileDbData));

    FileDbData.pszDnsDomainName = pDomainInfoEntry->pszDnsDomainName;
    FileDbData.pszNetbiosDomainName = pDomainInfoEntry->pszNetbiosDomainName;

    if (pDomainInfoEntry->pSid != NULL)
    {
        dwError = LsaAllocateCStringFromSid(
                &FileDbData.pszSid,
                pDomainInfoEntry->pSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pDomainInfoEntry->pGuid)
    {
        // Writes into a 37-byte caller allocated string
        uuid_unparse(*pDomainInfoEntry->pGuid, szGuid);

        FileDbData.pszGuid = szGuid;
    }

    FileDbData.pszTrusteeDnsDomainName = pDomainInfoEntry->pszTrusteeDnsDomainName;
    FileDbData.dwTrustFlags = pDomainInfoEntry->dwTrustFlags;
    FileDbData.dwTrustType = pDomainInfoEntry->dwTrustType;
    FileDbData.dwTrustAttributes = pDomainInfoEntry->dwTrustAttributes;
    FileDbData.dwTrustDirection = pDomainInfoEntry->dwTrustDirection;
    FileDbData.pszForestName = pDomainInfoEntry->pszForestName;
    FileDbData.pszClientSiteName = pDomainInfoEntry->pszClientSiteName;
    FileDbData.Flags = pDomainInfoEntry->Flags;

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataContext,
                              gADStateDomainTrustCacheSpec,
                              &FileDbData,
                              &pData,
                              &DataSize));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_WriteOneEntry(
                  pFileDb,
                  dwType,
                  DataSize,
                  pData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_STRING(FileDbData.pszSid);
    LW_SAFE_FREE_MEMORY(pData);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ADState_CopyFromFile(
    IN FILE * pFileDb,
    IN BOOLEAN bCopyProviderData,
    IN BOOLEAN bCopyDomainTrusts
    )
{
    DWORD dwError = 0;
    FILE * pOldFileDb = NULL;
    size_t Cnt = 0;
    BYTE FormatType[4];
    DWORD dwVersion = 0;
    DWORD dwType = 0;
    size_t DataMaxSize = 0;
    size_t DataSize = 0;
    PVOID pData = NULL;

    memset(FormatType, 0, sizeof(FormatType));

    pOldFileDb = fopen(ADSTATE_DB, "r");
    if (pOldFileDb == NULL)
    {
        dwError = errno;
    }
    BAIL_ON_LSA_ERROR(dwError);

    Cnt = fread(&FormatType, sizeof(FormatType), 1, pOldFileDb);
    if (Cnt == 0)
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    else if (Cnt != 1)
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LSA_ERROR(dwError);

    Cnt = fread(&dwVersion, sizeof(dwVersion), 1, pOldFileDb);
    if (Cnt != 1)
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LSA_ERROR(dwError);

    while (1)
    {
        Cnt = fread(&dwType, sizeof(dwType), 1, pOldFileDb);
        if (Cnt == 0)
        {
            break;
        }
        else if (Cnt != 1)
        {
            dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
        }
        BAIL_ON_LSA_ERROR(dwError);

        Cnt = fread(&DataSize, sizeof(DataSize), 1, pOldFileDb);
        if (Cnt == 0)
        {
            break;
        }
        else if (Cnt != 1)
        {
            dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
        }
        BAIL_ON_LSA_ERROR(dwError);

        if (DataSize > DataMaxSize)
        {
            DataMaxSize = DataSize * 2;

            dwError = LwReallocMemory(
                          pData,
                          (PVOID*)&pData,
                          DataMaxSize);
            BAIL_ON_LSA_ERROR(dwError);
        }

        Cnt = fread(pData, DataSize, 1, pOldFileDb);
        if (Cnt != 1)
        {
            dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
        }
        BAIL_ON_LSA_ERROR(dwError);

        switch (dwType)
        {
            case FILEDB_DATA_TYPE_PROVIDER:
            case FILEDB_DATA_TYPE_LINKEDCELL:
                if (bCopyProviderData)
                {
                    dwError = ADState_WriteOneEntry(
                                  pFileDb,
                                  dwType,
                                  DataSize,
                                  pData);
                    BAIL_ON_LSA_ERROR(dwError);
                }
                break;
            case FILEDB_DATA_TYPE_DOMAINTRUST:
                if (bCopyDomainTrusts)
                {
                    dwError = ADState_WriteOneEntry(
                                  pFileDb,
                                  dwType,
                                  DataSize,
                                  pData);
                    BAIL_ON_LSA_ERROR(dwError);
                }
                break;
            default:
                dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
                break;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pData);

    if (pOldFileDb != NULL)
    {
        fclose(pOldFileDb);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ADState_WriteOneEntry(
    IN FILE * pFileDb,
    IN DWORD dwType,
    IN size_t DataSize,
    IN PVOID pData
    )
{
    DWORD dwError = 0;
    size_t Cnt = 0;

    Cnt = fwrite(&dwType, sizeof(dwType), 1, pFileDb);
    if (Cnt != 1)
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LSA_ERROR(dwError);

    Cnt = fwrite(&DataSize, sizeof(DataSize), 1, pFileDb);
    if (Cnt != 1)
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LSA_ERROR(dwError);

    Cnt = fwrite(pData, DataSize, 1, pFileDb);
    if (Cnt != 1)
    {
        dwError = LW_ERROR_UNEXPECTED_DB_RESULT;
    }
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}
