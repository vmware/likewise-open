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
 *        lwnet-cachedb.c
 *
 * Abstract:
 *
 *        Caching for Likewise Netlogon
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#include "includes.h"

#define FILEDB_FORMAT_TYPE "LFLT"
#define FILEDB_FORMAT_VERSION 1

struct _LWNET_CACHE_DB_HANDLE_DATA {
    PDLINKEDLIST pCacheList;
    // This RW lock helps us to ensure that we don't stomp
    // ourselves while giving up good parallel access.
    // Note, however, that SQLite might still return busy errors
    // if some other process is trying to poke at the database
    // (which might happen with database debugging or maintenance tools).
    pthread_rwlock_t Lock;
    pthread_rwlock_t* pLock;
    BOOLEAN CanWrite;
};

static LWNET_CACHE_DB_HANDLE gDbHandle;

LWMsgTypeSpec gLWNetCacheEntrySpec[] =
{
    LWMSG_STRUCT_BEGIN(LWNET_CACHE_DB_ENTRY),
    LWMSG_MEMBER_PSTR(LWNET_CACHE_DB_ENTRY, pszDnsDomainName),
    LWMSG_MEMBER_PSTR(LWNET_CACHE_DB_ENTRY, pszSiteName),
    LWMSG_MEMBER_UINT32(LWNET_CACHE_DB_ENTRY, QueryType),
    LWMSG_MEMBER_UINT64(LWNET_CACHE_DB_ENTRY, LastDiscovered),
    LWMSG_MEMBER_UINT64(LWNET_CACHE_DB_ENTRY, LastPinged),
    LWMSG_MEMBER_STRUCT_BEGIN(LWNET_CACHE_DB_ENTRY, DcInfo),
    LWMSG_MEMBER_UINT32(LWNET_DC_INFO, dwPingTime),
    LWMSG_MEMBER_UINT32(LWNET_DC_INFO, dwDomainControllerAddressType),
    LWMSG_MEMBER_UINT32(LWNET_DC_INFO, dwFlags),
    LWMSG_MEMBER_UINT16(LWNET_DC_INFO, wLMToken),
    LWMSG_MEMBER_UINT16(LWNET_DC_INFO, wNTToken),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszDomainControllerName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszDomainControllerAddress),
    LWMSG_MEMBER_ARRAY_BEGIN(LWNET_DC_INFO, pucDomainGUID),
    LWMSG_UINT8(char),
    LWMSG_ARRAY_END,
    LWMSG_ATTR_LENGTH_STATIC(LWNET_GUID_SIZE),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszNetBIOSDomainName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszFullyQualifiedDomainName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszDnsForestName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszDCSiteName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszClientSiteName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszNetBIOSHostName),
    LWMSG_MEMBER_PSTR(LWNET_DC_INFO, pszUserName),
    LWMSG_STRUCT_END,
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

// ISSUE-2008/07/01-dalmeida -- For now, use exlusive locking as we need to
// verify actual thread safety wrt things like error strings and such.
#define RW_LOCK_ACQUIRE_READ(Lock) \
    pthread_rwlock_wrlock(Lock)

#define RW_LOCK_RELEASE_READ(Lock) \
    pthread_rwlock_unlock(Lock)

#define RW_LOCK_ACQUIRE_WRITE(Lock) \
    pthread_rwlock_wrlock(Lock)

#define RW_LOCK_RELEASE_WRITE(Lock) \
    pthread_rwlock_unlock(Lock)

static
DWORD
LWNetCacheDbReadFromFile(
    LWNET_CACHE_DB_HANDLE dbHandle
    );

static
DWORD
LWNetCacheDbWriteToFile(
    PDLINKEDLIST pCacheList
    );

static
VOID
LWNetCacheDbForEachEntryDestroy(
    IN PVOID pData,
    IN PVOID pContext
    );

static
VOID
LWNetCacheDbEntryFree(
    IN OUT PLWNET_CACHE_DB_ENTRY pEntry
    );

static
DWORD
LWNetCacheDbUpdate(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN LWNET_CACHE_DB_QUERY_TYPE QueryType,
    IN OPTIONAL PLWNET_UNIX_TIME_T LastDiscovered,
    IN OPTIONAL PLWNET_UNIX_TIME_T LastPinged,
    IN PLWNET_DC_INFO pDcInfo
    );

DWORD
LWNetCacheDbOpen(
    IN PCSTR Path,
    IN BOOLEAN bIsWrite,
    OUT PLWNET_CACHE_DB_HANDLE pDbHandle
    )
{
    DWORD dwError = 0;
    LWNET_CACHE_DB_HANDLE dbHandle = NULL;

    dwError = LWNetAllocateMemory(sizeof(*dbHandle), (PVOID *)&dbHandle);
    BAIL_ON_LWNET_ERROR(dwError);

    // TODO-dalmeida-2008/06/30 -- Convert error code
    dwError = pthread_rwlock_init(&dbHandle->Lock, NULL);
    BAIL_ON_LWNET_ERROR(dwError);

    dbHandle->pLock = &dbHandle->Lock;

    dwError = LWNetCacheDbReadFromFile(dbHandle);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    if (dwError)
    {
        LWNetCacheDbClose(&dbHandle);
    }    
    *pDbHandle = dbHandle;
    return dwError;
}

VOID
LWNetCacheDbClose(
    IN OUT PLWNET_CACHE_DB_HANDLE pDbHandle
    )
{
    LWNET_CACHE_DB_HANDLE dbHandle = *pDbHandle;

    if (dbHandle)
    {
        if (dbHandle->pCacheList)
        {
            LWNetCacheDbWriteToFile(dbHandle->pCacheList);

            LWNetDLinkedListForEach(
                dbHandle->pCacheList,
                LWNetCacheDbForEachEntryDestroy,
                NULL);
            LWNetDLinkedListFree(dbHandle->pCacheList);
        }
        if (dbHandle->pLock)
        {
            pthread_rwlock_destroy(dbHandle->pLock);
        }

        LWNET_SAFE_FREE_MEMORY(dbHandle);

        *pDbHandle = NULL;
    }
}

static
VOID
LWNetCacheDbForEachEntryDestroy(
    IN PVOID pData,
    IN PVOID pContext
    )
{
    PLWNET_CACHE_DB_ENTRY pEntry = (PLWNET_CACHE_DB_ENTRY)pData;
    LWNetCacheDbEntryFree(pEntry);
}

static
VOID
LWNetCacheDbEntryFree(
    IN OUT PLWNET_CACHE_DB_ENTRY pEntry
    )
{
    if (pEntry)
    {
        LWNET_SAFE_FREE_STRING(pEntry->pszDnsDomainName);
        LWNET_SAFE_FREE_STRING(pEntry->pszSiteName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszDomainControllerName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszDomainControllerAddress);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszNetBIOSDomainName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszFullyQualifiedDomainName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszDnsForestName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszDCSiteName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszClientSiteName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszNetBIOSHostName);
        LWNET_SAFE_FREE_STRING(pEntry->DcInfo.pszUserName);
        LWNET_SAFE_FREE_MEMORY(pEntry);
    }
}

static
DWORD
LWNetCacheDbReadFromFile(
    LWNET_CACHE_DB_HANDLE dbHandle
    )
{
    DWORD    dwError = 0;
    BOOLEAN  bExists = FALSE;
    FILE *   pFileDb = NULL;
    size_t   Cnt = 0;
    BYTE     FormatType[4];
    DWORD    dwVersion = 0;
    PVOID    pData = NULL;
    size_t   DataMaxSize = 0;
    size_t   DataSize = 0;
    LWMsgContext * pContext = NULL;
    LWMsgDataContext * pDataContext = NULL;
    PLWNET_CACHE_DB_ENTRY pCacheEntry = NULL;

    memset(FormatType, 0, sizeof(FormatType));

    dwError = LwCheckFileTypeExists(
                    NETLOGON_DB,
                    LWFILE_REGULAR,
                    &bExists);
    BAIL_ON_LWNET_ERROR(dwError);

    if (!bExists)
    {
       goto cleanup;
    }

    pFileDb = fopen(NETLOGON_DB, "r");
    if (pFileDb == NULL)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_LWNET_ERROR(dwError);

    Cnt = fread(&FormatType, sizeof(FormatType), 1, pFileDb);
    if (Cnt == 0)
    {
        dwError = ERROR_BAD_FORMAT;
    }
    else if (Cnt != 1)
    {
        dwError = ERROR_BAD_FORMAT;
    }
    BAIL_ON_LWNET_ERROR(dwError);

    if (memcmp(FormatType, FILEDB_FORMAT_TYPE, sizeof(FormatType)))
    {
        dwError = ERROR_BAD_FORMAT;
    }
    BAIL_ON_LWNET_ERROR(dwError);

    Cnt = fread(&dwVersion, sizeof(dwVersion), 1, pFileDb);
    if (Cnt == 0)
    {
        dwError = ERROR_BAD_FORMAT;
    }
    else if (Cnt != 1)
    {
        dwError = ERROR_BAD_FORMAT;
    }
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &pContext));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(pContext, &pDataContext));
    BAIL_ON_LWNET_ERROR(dwError);

    while (1)
    {
        Cnt = fread(&DataSize, sizeof(DataSize), 1, pFileDb);
        if (Cnt == 0)
        {
            break;
        }
        else if (Cnt != 1)
        {
            dwError = ERROR_BAD_FORMAT;
        }
        BAIL_ON_LWNET_ERROR(dwError);

        if (DataSize > DataMaxSize)
        {
            DataMaxSize = DataSize * 2;

            dwError = LWNetReallocMemory(
                          pData,
                          (PVOID*)&pData,
                          DataMaxSize);
            BAIL_ON_LWNET_ERROR(dwError);
        }

        Cnt = fread(pData, DataSize, 1, pFileDb);
        if (Cnt != 1)
        {
            dwError = ERROR_BAD_FORMAT;
        }
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                                  pDataContext,
                                  gLWNetCacheEntrySpec,
                                  pData,
                                  DataSize,
                                  (PVOID*)&pCacheEntry));
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LWNetCacheDbUpdate(
                      dbHandle,
                      pCacheEntry->pszDnsDomainName,
                      pCacheEntry->pszSiteName,
                      pCacheEntry->QueryType,
                      &pCacheEntry->LastDiscovered,
                      &pCacheEntry->LastPinged,
                      &pCacheEntry->DcInfo);
        BAIL_ON_LWNET_ERROR(dwError);

        lwmsg_data_free_graph(
            pDataContext,
            gLWNetCacheEntrySpec,
            pCacheEntry);

        pCacheEntry = NULL;
    }

cleanup:

    if (pFileDb != NULL)
    {
        fclose(pFileDb);
    }

    if (pCacheEntry)
    {
        lwmsg_data_free_graph(
            pDataContext,
            gLWNetCacheEntrySpec,
            pCacheEntry);
    }
    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }
    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    LWNET_SAFE_FREE_MEMORY(pData);

    return dwError;

error:

    // So long as there is network connectivity, errors are not fatal.
    LWNET_LOG_ERROR("Failed to read cache %s [%d]", NETLOGON_DB, dwError);
    dwError = 0;

    goto cleanup;
}

static
DWORD
LWNetCacheDbWriteToFile(
    PDLINKEDLIST pCacheList
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    FILE * pFileDb = NULL;
    size_t Cnt = 0;
    PCSTR FormatType = FILEDB_FORMAT_TYPE;
    DWORD dwVersion = FILEDB_FORMAT_VERSION;
    LWMsgContext * pContext = NULL;
    LWMsgDataContext * pDataContext = NULL;
    PVOID pData = NULL;
    size_t DataSize = 0;

    dwError = LwCheckFileTypeExists(
                    NETLOGON_DB_DIR,
                    LWFILE_DIRECTORY,
                    &bExists);
    BAIL_ON_LWNET_ERROR(dwError);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        /* Allow go+rx to the base cache folder */
        dwError = LwCreateDirectory(NETLOGON_DB_DIR, cacheDirMode);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    /* restrict access to u+rwx to the db folder */
    dwError = LwChangeOwnerAndPermissions(NETLOGON_DB_DIR, 0, 0, S_IRWXU);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LwCheckFileTypeExists(
                    NETLOGON_DB,
                    LWFILE_REGULAR,
                    &bExists);
    BAIL_ON_LWNET_ERROR(dwError);

    pFileDb = fopen(NETLOGON_DB, "w");
    if (pFileDb == NULL)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_LWNET_ERROR(dwError);

    Cnt = fwrite(FormatType, sizeof(BYTE) * 4, 1, pFileDb);
    if (Cnt != 1)
    {
        dwError = ERROR_BAD_FORMAT;
    }
    BAIL_ON_LWNET_ERROR(dwError);

    Cnt = fwrite(&dwVersion, sizeof(dwVersion), 1, pFileDb);
    if (Cnt != 1)
    {
        dwError = ERROR_BAD_FORMAT;
    }
    BAIL_ON_LWNET_ERROR(dwError);

    // Make sure that we are secured
    dwError = LwChangePermissions(NETLOGON_DB, S_IRWXU);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &pContext));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(pContext, &pDataContext));
    BAIL_ON_LWNET_ERROR(dwError);

    while (pCacheList)
    {
        dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                                  pDataContext,
                                  gLWNetCacheEntrySpec,
                                  pCacheList->pItem,
                                  &pData,
                                  &DataSize));
        BAIL_ON_LWNET_ERROR(dwError);

        Cnt = fwrite(&DataSize, sizeof(DataSize), 1, pFileDb);
        if (Cnt != 1)
        {
            dwError = ERROR_BAD_FORMAT;
        }
        BAIL_ON_LWNET_ERROR(dwError);

        Cnt = fwrite(pData, DataSize, 1, pFileDb);
        if (Cnt != 1)
        {
            dwError = ERROR_BAD_FORMAT;
        }
        BAIL_ON_LWNET_ERROR(dwError);

        LWNET_SAFE_FREE_MEMORY(pData);

        pCacheList = pCacheList->pNext;
    }

    fclose(pFileDb);
    pFileDb = NULL;

    if (!bExists)
    {
        dwError = LwChangePermissions(NETLOGON_DB, S_IRWXU);
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    LWNET_SAFE_FREE_MEMORY(pData);

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }
    if (pContext)
    {
        lwmsg_context_delete(pContext);
    }

    if (pFileDb != NULL)
    {
        fclose(pFileDb);
    }

    return dwError;

error:

    LWNET_LOG_ERROR("Failed to save cache %s [%d]", NETLOGON_DB, dwError);

    if (pFileDb != NULL)
    {
        fclose(pFileDb);
        pFileDb = NULL;
    }
    LwRemoveFile(NETLOGON_DB);

    goto cleanup;
}

static
LWNET_CACHE_DB_QUERY_TYPE
LWNetCacheDbQueryToQueryType(
    IN DWORD dwDsFlags
    )
{
    LWNET_CACHE_DB_QUERY_TYPE result = LWNET_CACHE_DB_QUERY_TYPE_DC;
    if (dwDsFlags & DS_PDC_REQUIRED)
    {
        result = LWNET_CACHE_DB_QUERY_TYPE_PDC;
    }
    if (dwDsFlags & DS_GC_SERVER_REQUIRED)
    {
        result = LWNET_CACHE_DB_QUERY_TYPE_GC;
    }
    return result;
}

DWORD
LWNetCacheDbQuery(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT PLWNET_UNIX_TIME_T LastDiscovered,
    OUT PLWNET_UNIX_TIME_T LastPinged
    )
{
    DWORD dwError = 0;
    LWNET_CACHE_DB_QUERY_TYPE queryType = 0;
    BOOLEAN isAcquired = FALSE;
    PLWNET_DC_INFO pDcInfo = NULL;
    LWNET_UNIX_TIME_T lastDiscovered = 0;
    LWNET_UNIX_TIME_T lastPinged = 0;
    PSTR pszDnsDomainNameLower = NULL;
    PSTR pszSiteNameLower = NULL;
    PLWNET_CACHE_DB_ENTRY pEntry = NULL;
    PDLINKEDLIST pListEntry = NULL;

    if (pszDnsDomainName)
    {
        dwError = LWNetAllocateString(
                      pszDnsDomainName,
                      &pszDnsDomainNameLower);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    else
    {
        LWNET_LOG_DEBUG("Cached entry not found: %s, %s, %u",
                        "",
                        pszSiteNameLower ? pszSiteNameLower : "",
                        queryType);
        goto error;
    }

    LwStrToLower(pszDnsDomainNameLower);

    if (pszSiteName)
    {
        dwError = LWNetAllocateString(
                      pszSiteName,
                      &pszSiteNameLower);
        BAIL_ON_LWNET_ERROR(dwError);

        LwStrToLower(pszSiteNameLower);
    }

    queryType = LWNetCacheDbQueryToQueryType(dwDsFlags);

    RW_LOCK_ACQUIRE_READ(DbHandle->pLock);
    isAcquired = TRUE;

    for (pListEntry = DbHandle->pCacheList ;
         pListEntry ;
         pListEntry = pListEntry->pNext)
    {
        pEntry = (PLWNET_CACHE_DB_ENTRY)pListEntry->pItem;

        if ( pEntry->QueryType == queryType &&
             !strcmp(pEntry->pszDnsDomainName, pszDnsDomainNameLower))
        {
            if (!pEntry->pszSiteName && !pszSiteNameLower)
            {
                break;
            }
            if (pEntry->pszSiteName && pszSiteNameLower &&
                !strcmp(pEntry->pszSiteName, pszSiteNameLower))
            {
                break;
            }
        }
    }

    if (!pListEntry)
    {
        LWNET_LOG_DEBUG("Cached entry not found: %s, %s, %u",
                        pszDnsDomainNameLower,
                        pszSiteNameLower ? pszSiteNameLower : "",
                        queryType);
        goto error;
    }

    dwError = LWNetAllocateMemory(sizeof(*pDcInfo), (PVOID*)&pDcInfo);
    BAIL_ON_LWNET_ERROR(dwError);

    lastDiscovered = pEntry->LastDiscovered;
    lastPinged = pEntry->LastPinged;

    pDcInfo->dwPingTime = pEntry->DcInfo.dwPingTime;
    pDcInfo->dwDomainControllerAddressType = pEntry->DcInfo.dwDomainControllerAddressType;
    pDcInfo->dwFlags = pEntry->DcInfo.dwFlags;
    pDcInfo->dwVersion = pEntry->DcInfo.dwVersion;
    pDcInfo->wLMToken = pEntry->DcInfo.wLMToken;
    pDcInfo->wNTToken = pEntry->DcInfo.wNTToken;

    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszDomainControllerName,
                  &pDcInfo->pszDomainControllerName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszDomainControllerAddress,
                  &pDcInfo->pszDomainControllerAddress);
    BAIL_ON_LWNET_ERROR(dwError);

    memcpy(pDcInfo->pucDomainGUID,
           pEntry->DcInfo.pucDomainGUID,
           LWNET_GUID_SIZE);

    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszNetBIOSDomainName,
                  &pDcInfo->pszNetBIOSDomainName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszFullyQualifiedDomainName,
                  &pDcInfo->pszFullyQualifiedDomainName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszDnsForestName,
                  &pDcInfo->pszDnsForestName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszDCSiteName,
                  &pDcInfo->pszDCSiteName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszClientSiteName,
                  &pDcInfo->pszClientSiteName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszNetBIOSHostName,
                  &pDcInfo->pszNetBIOSHostName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pEntry->DcInfo.pszUserName,
                  &pDcInfo->pszUserName);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    if (isAcquired)
    {
        RW_LOCK_RELEASE_READ(DbHandle->pLock);
    }
    if (dwError)
    {
        LWNET_SAFE_FREE_DC_INFO(pDcInfo);
        lastPinged = 0;
        lastDiscovered = 0;
    }

    LWNET_SAFE_FREE_STRING(pszDnsDomainNameLower);
    LWNET_SAFE_FREE_STRING(pszSiteNameLower);

    *ppDcInfo = pDcInfo;
    *LastDiscovered = lastDiscovered;
    *LastPinged = lastPinged;

    return dwError;
}

static
DWORD
LWNetCacheDbUpdate(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN LWNET_CACHE_DB_QUERY_TYPE QueryType,
    IN OPTIONAL PLWNET_UNIX_TIME_T LastDiscovered,
    IN OPTIONAL PLWNET_UNIX_TIME_T LastPinged,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    DWORD dwError = 0;
    LWNET_UNIX_TIME_T now = 0;
    PLWNET_CACHE_DB_ENTRY pNewEntry = NULL;
    PLWNET_CACHE_DB_ENTRY pOldEntry = NULL;
    PDLINKEDLIST pOldListEntry = NULL;
    BOOLEAN isAcquired = FALSE;

    dwError = LWNetAllocateMemory(sizeof(*pNewEntry), (PVOID *)&pNewEntry);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pszDnsDomainName,
                  &pNewEntry->pszDnsDomainName);
    BAIL_ON_LWNET_ERROR(dwError);

    LwStrToLower(pNewEntry->pszDnsDomainName);

    if (pszSiteName)
    {
        dwError = LWNetAllocateString(
                      pszSiteName,
                      &pNewEntry->pszSiteName);
        BAIL_ON_LWNET_ERROR(dwError);

        LwStrToLower(pNewEntry->pszSiteName);
    }

    pNewEntry->QueryType = QueryType;

    dwError = LWNetGetSystemTime(&now);
    BAIL_ON_LWNET_ERROR(dwError);

    pNewEntry->LastDiscovered = LastDiscovered ? *LastDiscovered : now;
    pNewEntry->LastPinged = LastPinged ? *LastPinged : now;
    pNewEntry->DcInfo.dwPingTime = pDcInfo->dwPingTime;
    pNewEntry->DcInfo.dwDomainControllerAddressType = pDcInfo->dwDomainControllerAddressType;
    pNewEntry->DcInfo.dwFlags = pDcInfo->dwFlags;
    pNewEntry->DcInfo.dwVersion = pDcInfo->dwVersion;
    pNewEntry->DcInfo.wLMToken = pDcInfo->wLMToken;
    pNewEntry->DcInfo.wNTToken = pDcInfo->wNTToken;

    dwError = LWNetAllocateString(
                  pDcInfo->pszDomainControllerName,
                  &pNewEntry->DcInfo.pszDomainControllerName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pDcInfo->pszDomainControllerAddress,
                  &pNewEntry->DcInfo.pszDomainControllerAddress);
    BAIL_ON_LWNET_ERROR(dwError);

    memcpy(pNewEntry->DcInfo.pucDomainGUID,
           pDcInfo->pucDomainGUID,
           LWNET_GUID_SIZE);

    dwError = LWNetAllocateString(
                  pDcInfo->pszNetBIOSDomainName,
                  &pNewEntry->DcInfo.pszNetBIOSDomainName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pDcInfo->pszFullyQualifiedDomainName,
                  &pNewEntry->DcInfo.pszFullyQualifiedDomainName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pDcInfo->pszDnsForestName,
                  &pNewEntry->DcInfo.pszDnsForestName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pDcInfo->pszDCSiteName,
                  &pNewEntry->DcInfo.pszDCSiteName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pDcInfo->pszClientSiteName,
                  &pNewEntry->DcInfo.pszClientSiteName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pDcInfo->pszNetBIOSHostName,
                  &pNewEntry->DcInfo.pszNetBIOSHostName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetAllocateString(
                  pDcInfo->pszUserName,
                  &pNewEntry->DcInfo.pszUserName);
    BAIL_ON_LWNET_ERROR(dwError);

    RW_LOCK_ACQUIRE_WRITE(DbHandle->pLock);
    isAcquired = TRUE;

    for (pOldListEntry = DbHandle->pCacheList ;
         pOldListEntry ;
         pOldListEntry = pOldListEntry->pNext)
    {
        pOldEntry = (PLWNET_CACHE_DB_ENTRY)pOldListEntry->pItem;

        if (pOldEntry->QueryType == pNewEntry->QueryType &&
             !strcmp(pOldEntry->pszDnsDomainName, pNewEntry->pszDnsDomainName))
        {
            if (!pOldEntry->pszSiteName && !pNewEntry->pszSiteName)
            {
                break;
            }
            if (pOldEntry->pszSiteName && pNewEntry->pszSiteName &&
                !strcmp(pOldEntry->pszSiteName, pNewEntry->pszSiteName))
            {
                break;
            }
        }
    }

    if (pOldListEntry)
    {
        LWNetDLinkedListDelete(
            &DbHandle->pCacheList,
            pOldEntry);

        LWNetCacheDbEntryFree(pOldEntry);
    }

    dwError = LWNetDLinkedListAppend(
                  &DbHandle->pCacheList,
                  pNewEntry);
    BAIL_ON_LWNET_ERROR(dwError);

    pNewEntry = NULL;

error:
    if (isAcquired)
    {
        RW_LOCK_RELEASE_WRITE(DbHandle->pLock);
    }

    LWNetCacheDbEntryFree(pNewEntry);

    return dwError;
}

DWORD
LWNetCacheDbScavenge(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN LWNET_UNIX_TIME_T PositiveCacheAge,
    IN LWNET_UNIX_TIME_T NegativeCacheAge
    )
{
    DWORD dwError = 0;
    LWNET_UNIX_TIME_T now = 0;
    LWNET_UNIX_TIME_T positiveTimeLimit = 0;
    PLWNET_CACHE_DB_ENTRY pEntry = NULL;
    PDLINKEDLIST pListEntry = NULL;
    PDLINKEDLIST pNextListEntry = NULL;
    BOOLEAN isAcquired = FALSE;

    dwError = LWNetGetSystemTime(&now);
    positiveTimeLimit = now + PositiveCacheAge;

    RW_LOCK_ACQUIRE_WRITE(DbHandle->pLock);
    isAcquired = TRUE;

    pListEntry = DbHandle->pCacheList;
    while(pListEntry)
    {
        pNextListEntry = pListEntry->pNext;
        pEntry = pListEntry->pItem;

        if (pEntry->LastPinged < positiveTimeLimit)
        {
            LWNetDLinkedListDelete(
                &DbHandle->pCacheList,
                pEntry);

            LWNetCacheDbEntryFree(pEntry);
        }

        pListEntry = pNextListEntry;
    }

    if (isAcquired)
    {
        RW_LOCK_RELEASE_WRITE(DbHandle->pLock);
    }

    return dwError;
}

DWORD
LWNetCacheDbExport(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    OUT PLWNET_CACHE_DB_ENTRY* ppEntries,
    OUT PDWORD pdwCount
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
LWNetCacheInitialize(
    )
{
    DWORD dwError = 0;

    dwError = LWNetCacheDbOpen(NETLOGON_DB, TRUE, &gDbHandle);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    if (dwError)
    {
        LWNetCacheCleanup();
    }
    return dwError;
}

VOID
LWNetCacheCleanup(
    )
{
    LWNetCacheDbClose(&gDbHandle);
}

DWORD
LWNetCacheQuery(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT PLWNET_UNIX_TIME_T LastDiscovered,
    OUT PLWNET_UNIX_TIME_T LastPinged
    )
{
    return LWNetCacheDbQuery(gDbHandle,
                             pszDnsDomainName, pszSiteName, dwDsFlags,
                             ppDcInfo, LastDiscovered, LastPinged);
}


DWORD
LWNetCacheUpdatePing(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    IN LWNET_UNIX_TIME_T LastDiscovered,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    return LWNetCacheDbUpdate(
               gDbHandle,
               pszDnsDomainName,
               pszSiteName,
               LWNetCacheDbQueryToQueryType(dwDsFlags),
               &LastDiscovered,
               NULL,
               pDcInfo);
}

DWORD
LWNetCacheUpdateDiscover(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    return LWNetCacheDbUpdate(
               gDbHandle,
               pszDnsDomainName,
               pszSiteName,
               LWNetCacheDbQueryToQueryType(dwDsFlags),
               NULL,
               NULL,
               pDcInfo);
}

DWORD
LWNetCacheScavenge(
    IN LWNET_UNIX_TIME_T PositiveCacheAge,
    IN LWNET_UNIX_TIME_T NegativeCacheAge
    )
{
    return LWNetCacheDbScavenge(gDbHandle, PositiveCacheAge, NegativeCacheAge);
}
