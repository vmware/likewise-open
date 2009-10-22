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
 *        regshare.c
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 *        Server sub-system
 *
 *        Server share registry interface
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#include "includes.h"

static
void
SrvShareFreeStringArray(
    PSTR* ppszStr,
    ULONG ulLen
    )
{
    int iCount = 0;

    if (!ppszStr)
        return;

    for (iCount = 0; iCount < ulLen; iCount++)
    {
        if (ppszStr[iCount])
        {
            LwRtlCStringFree(&ppszStr[iCount]);
        }
    }
}

static
NTSTATUS
SrvShareRegWriteToShareInfo(
    IN REG_DATA_TYPE regDataType,
    IN PSTR pszValueName,
    IN PBYTE pData,
    IN ULONG ulDataLen,
    IN REG_DATA_TYPE regSecDataType,
    IN PBYTE pSecData,
    IN ULONG ulSecDataLen,
    OUT PSRV_SHARE_INFO* ppShareInfo
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_SHARE_INFO pShareInfo = NULL;
    PSTR* ppszOutMultiSz = NULL;
    ULONG ulMultiIndex = 0;

    ntStatus = SrvAllocateMemory(
                    sizeof(*pShareInfo),
                    (PVOID*)&pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    pShareInfo->refcount = 1;

    pthread_rwlock_init(&pShareInfo->mutex, NULL);
    pShareInfo->pMutex = &pShareInfo->mutex;

    ntStatus = SrvMbsToWc16s(pszValueName,
                             &pShareInfo->pwszName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RegByteArrayToMultiStrsA(pData,
                                        ulDataLen,
                                        &ppszOutMultiSz);
    BAIL_ON_NT_STATUS(ntStatus);

    for (ulMultiIndex=0; ppszOutMultiSz[ulMultiIndex]; ulMultiIndex++)
    {
        PSTR pszPtr = NULL;

        switch(ulMultiIndex)
        {
            case 0:
                pszPtr = strstr(ppszOutMultiSz[ulMultiIndex], "Path=");
                if (pszPtr)
                {
                    ntStatus = SrvMbsToWc16s(
                                   (PCSTR)pszPtr+strlen("Path="),
                                   &pShareInfo->pwszPath);
                    BAIL_ON_NT_STATUS(ntStatus);
                }
                else
                {
                    ntStatus = STATUS_INVALID_PARAMETER_3;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                break;

            case 1:
                pszPtr = strstr(ppszOutMultiSz[ulMultiIndex], "Comment=");
                if (pszPtr)
                {
                    ntStatus = SrvMbsToWc16s(
                                   (PCSTR)pszPtr+strlen("Comment="),
                                   &pShareInfo->pwszComment);
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                break;

            case 2:
                pszPtr = strstr(ppszOutMultiSz[ulMultiIndex], "Service=");
                if (pszPtr)
                {
                    ntStatus = SrvShareMapServiceStringToId(
                                                     (PCSTR)pszPtr+strlen("Service="),
                                                     &pShareInfo->service);
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                break;

            default:
                ntStatus = STATUS_INVALID_PARAMETER_3;
                BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    if (ulSecDataLen)
    {
        ntStatus = SrvAllocateMemory(
                       ulSecDataLen,
                       (PVOID*)&pShareInfo->pSecDesc);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pShareInfo->pSecDesc, pSecData, ulSecDataLen);
        pShareInfo->ulSecDescLen = ulSecDataLen;
    }

    *ppShareInfo = pShareInfo;

cleanup:
    RegMultiStrsFree(ppszOutMultiSz);

    return ntStatus;

error:
    if (pShareInfo)
    {
        SrvShareReleaseInfo(pShareInfo);
    }

    goto cleanup;
}

NTSTATUS
SrvShareRegInit(
    VOID
    )
{
    return 0;
}

NTSTATUS
SrvShareRegOpen(
    OUT PHANDLE phRepository
    )
{
    return RegOpenServer(phRepository);
}

NTSTATUS
SrvShareRegFindByName(
    HANDLE           hRepository,
    PWSTR            pwszShareName,
    PSRV_SHARE_INFO* ppShareInfo
    )
{
    NTSTATUS ntStatus = 0;
    PSTR pszShareName = NULL;
    HKEY hRootKey = NULL;
    HKEY hKey = NULL;
    HKEY hSecKey = NULL;
    REG_DATA_TYPE dataType = REG_UNKNOWN;
    ULONG ulValueLen = MAX_VALUE_LENGTH;
    BYTE pData[MAX_VALUE_LENGTH] = {0};
    REG_DATA_TYPE dataSecType = REG_UNKNOWN;
    ULONG ulSecValueLen = MAX_VALUE_LENGTH;
    BYTE pSecData[MAX_VALUE_LENGTH] = {0};
    PSRV_SHARE_INFO pShareInfo = NULL;

    ntStatus = RegOpenKeyExA(
            hRepository,
            NULL,
            LIKEWISE_ROOT_KEY,
            0,
            0,
            &hRootKey);
    BAIL_ON_NT_STATUS(ntStatus);

    // Open key "Services\\lwio\\Parameters\\Drivers\\srv\\shares"
    ntStatus = RegOpenKeyExA(
            hRepository,
            hRootKey,
            pszSharePath,
            0,
            0,
            &hKey);
    BAIL_ON_NT_STATUS(ntStatus);

    // "Services\\lwio\\Parameters\\Drivers\\srv\\shares\\security"
    ntStatus = RegOpenKeyExA(
                hRepository,
                hRootKey,
                pszShareSecPath,
                0,
                0,
                &hSecKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvWc16sToMbs(pwszShareName, &pszShareName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RegGetValueA(
            hRepository,
            hKey,
            NULL,
            pszShareName,
            RRF_RT_REG_MULTI_SZ,
            &dataType,
            pData,
            &ulValueLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RegGetValueA(
            hRepository,
            hSecKey,
            NULL,
            pszShareName,
            RRF_RT_REG_BINARY,
            &dataSecType,
            pSecData,
            &ulSecValueLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareRegWriteToShareInfo(
                  dataType,
                  pszShareName,
                  pData,
                  ulValueLen,
                  dataSecType,
                  pSecData,
                  ulSecValueLen,
                  &pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppShareInfo = pShareInfo;

cleanup:
    if (hRootKey)
    {
        RegCloseKey(hRepository, hRootKey);
    }
    if (hKey)
    {
        RegCloseKey(hRepository, hKey);
    }
    if (hSecKey)
    {
        RegCloseKey(hRepository, hSecKey);
    }
    SRV_SAFE_FREE_MEMORY(pszShareName);

    return ntStatus;

error:
    if (pShareInfo)
    {
        SrvShareReleaseInfo(pShareInfo);
    }

    goto cleanup;
}

NTSTATUS
SrvShareRegAdd(
    IN HANDLE hRepository,
    IN PWSTR  pwszShareName,
    IN PWSTR  pwszPath,
    IN PWSTR  pwszComment,
    IN PBYTE  pSecDesc,
    IN ULONG  ulSecDescLen,
    IN PWSTR  pwszService
    )
{
    NTSTATUS ntStatus = 0;
    HKEY hRootKey = NULL;
    HKEY hKey = NULL;
    HKEY hSecKey = NULL;
    PSTR pszShareName = NULL;
    PSTR pszPath = NULL;
    PSTR pszComment = NULL;
    PSTR pszService = NULL;
    PSTR* ppszValue = NULL;
    PBYTE pOutData = NULL;
    SSIZE_T cOutDataLen = 0;
    ULONG ulCount = 0;

    if (pwszShareName)
    {
        ntStatus = SrvWc16sToMbs(pwszShareName, &pszShareName);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    if (pwszPath)
    {
        ntStatus = SrvWc16sToMbs(pwszPath, &pszPath);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    if (pwszComment)
    {
        ntStatus = SrvWc16sToMbs(pwszComment, &pszComment);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    if (pwszService)
    {
        ntStatus = SrvWc16sToMbs(pwszService, &pszService);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (IsNullOrEmptyString(pszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER_2;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    if (IsNullOrEmptyString(pszPath))
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = RegOpenKeyExA(
            hRepository,
            NULL,
            LIKEWISE_ROOT_KEY,
            0,
            0,
            &hRootKey);
    BAIL_ON_NT_STATUS(ntStatus);

    // Add share value under "Services\\lwio\\Parameters\\Drivers\\srv\\shares"
    ntStatus = RegOpenKeyExA(
            hRepository,
            hRootKey,
            pszSharePath,
            0,
            0,
            &hKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateMemory(sizeof(*ppszValue)*4, (PVOID)&ppszValue);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateStringPrintf(
                 &ppszValue[ulCount++],
                 "Path=%s",
                 pszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!IsNullOrEmptyString(pszComment))
    {
        ntStatus = SrvAllocateStringPrintf(
                     &ppszValue[ulCount++],
                     "Comment=%s",
                     pszComment);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (!IsNullOrEmptyString(pszService))
    {
        ntStatus = SrvAllocateStringPrintf(
                     &ppszValue[ulCount++],
                     "Service=%s",
                     pszService);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = RegMultiStrsToByteArrayW(
                 ppszValue,
                 &pOutData,
                 &cOutDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RegSetValueExW(
            hRepository,
            hKey,
            pwszShareName,
            0,
            REG_MULTI_SZ,
            pOutData,
            cOutDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    if ((pSecDesc && !ulSecDescLen) || (!pSecDesc && ulSecDescLen))
    {
        ntStatus = STATUS_INVALID_PARAMETER_5;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Add share security value under
    // "Services\\lwio\\Parameters\\Drivers\\srv\\shares\\security"
    ntStatus = RegOpenKeyExA(
                hRepository,
                hRootKey,
                pszShareSecPath,
                0,
                0,
                &hSecKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RegSetValueExW(
            hRepository,
            hSecKey,
            pwszShareName,
            0,
            REG_BINARY,
            pSecDesc,
            ulSecDescLen);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    if (hRootKey)
    {
        RegCloseKey(hRepository, hRootKey);
    }
    if (hKey)
    {
        RegCloseKey(hRepository, hKey);
    }
    if (hSecKey)
    {
        RegCloseKey(hRepository, hSecKey);
    }
    LwRtlCStringFree(&pszPath);
    LwRtlCStringFree(&pszComment);
    LwRtlCStringFree(&pszService);
    SrvShareFreeStringArray(ppszValue, 4);
    SrvFreeMemory(pOutData);

    return ntStatus;

error:
    goto cleanup;
}

NTSTATUS
SrvShareRegBeginEnum(
    HANDLE  hRepository,
    ULONG   ulLimit,
    PHANDLE phResume
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulValueCount = 0;
    PSRV_SHARE_REG_ENUM_CONTEXT pEnumContext = NULL;
    HKEY hRootKey = NULL;
    HKEY hKey = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_SHARE_REG_ENUM_CONTEXT),
                    (PVOID*)&pEnumContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RegOpenKeyExA(
            hRepository,
            NULL,
            LIKEWISE_ROOT_KEY,
            0,
            0,
            &hRootKey);
    BAIL_ON_NT_STATUS(ntStatus);

    // Add share value under "Services\\lwio\\Parameters\\Drivers\\srv\\shares"
    ntStatus = RegOpenKeyExA(
            hRepository,
            hRootKey,
            pszSharePath,
            0,
            0,
            &hKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RegQueryInfoKey(
        hRepository,
        hKey,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &ulValueCount,
        &pEnumContext->ulMaxValueNameLen,
        &pEnumContext->ulMaxValueLen,
        NULL,
        NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (ulLimit > ulValueCount)
    {
        pEnumContext->ulMaxIndex = ulValueCount;
    }
    else
    {
        pEnumContext->ulMaxIndex = ulLimit;
    }

    *phResume = (HANDLE)pEnumContext;

cleanup:
    if (hRootKey)
    {
        RegCloseKey(hRepository, hRootKey);
    }
    if (hKey)
    {
        RegCloseKey(hRepository, hKey);
    }

    return ntStatus;

error:

    *phResume = NULL;

    SRV_SAFE_FREE_MEMORY(pEnumContext);

    goto cleanup;
}

NTSTATUS
SrvShareRegEnum(
    HANDLE            hRepository,
    HANDLE            hResume,
    PSRV_SHARE_INFO** pppShareInfoList,
    PULONG            pulNumSharesFound
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulIndex = 0;
    PSRV_SHARE_REG_ENUM_CONTEXT pResume = (PSRV_SHARE_REG_ENUM_CONTEXT)hResume;
    HKEY hRootKey = NULL;
    HKEY hKey = NULL;
    HKEY hSecKey = NULL;
    ULONG ulMaxValueNameLen = 0;
    PSTR pszValueName = NULL;
    ULONG ulMaxValueLen = 0;
    PBYTE pData = NULL;
    REG_DATA_TYPE dataType = REG_UNKNOWN;
    PSRV_SHARE_INFO* ppShareInfoList = NULL;
    REG_DATA_TYPE dataSecType = REG_UNKNOWN;
    ULONG ulMaxSecValueLen = 0;
    BYTE pSecData[MAX_VALUE_LENGTH] = {0};
    ULONG ulNumSharesFound = 0;

    ntStatus = RegOpenKeyExA(
            hRepository,
            NULL,
            LIKEWISE_ROOT_KEY,
            0,
            0,
            &hRootKey);
    BAIL_ON_NT_STATUS(ntStatus);

    // Open key "Services\\lwio\\Parameters\\Drivers\\srv\\shares"
    ntStatus = RegOpenKeyExA(
            hRepository,
            hRootKey,
            pszSharePath,
            0,
            0,
            &hKey);
    BAIL_ON_NT_STATUS(ntStatus);

    // "Services\\lwio\\Parameters\\Drivers\\srv\\shares\\security"
    ntStatus = RegOpenKeyExA(
                hRepository,
                hRootKey,
                pszShareSecPath,
                0,
                0,
                &hSecKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateMemory(
                  pResume->ulMaxIndex*sizeof(*ppShareInfoList),
                  (PVOID) &ppShareInfoList);
    BAIL_ON_NT_STATUS(ntStatus);

    for (ulIndex = 0; ulIndex < pResume->ulMaxIndex; ulIndex++)
    {
        ulMaxValueNameLen = (pResume->ulMaxValueNameLen + 1) *
                            sizeof(*pszValueName);
        ulMaxValueLen = pResume->ulMaxValueLen;
        ulMaxSecValueLen = MAX_VALUE_LENGTH;

        if (ulMaxValueNameLen)
        {
            ntStatus = SrvAllocateMemory(
                      ulMaxValueNameLen,
                      (PVOID) &pszValueName);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (ulMaxValueLen)
        {
            ntStatus = SrvAllocateMemory(
                      ulMaxValueLen,
                      (PVOID) &pData);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = RegEnumValueA(
                      hRepository,
                      hKey,
                      ulIndex,
                      pszValueName,
                      &ulMaxValueNameLen,
                      NULL,
                      &dataType,
                      pData,
                      &ulMaxValueLen);
        BAIL_ON_NT_STATUS(ntStatus);

        if (REG_MULTI_SZ != dataType)
        {
            continue;
        }

        ntStatus = RegGetValueA(
                hRepository,
                hSecKey,
                NULL,
                pszValueName,
                RRF_RT_REG_BINARY,
                &dataSecType,
                pSecData,
                &ulMaxSecValueLen);
        if (LW_ERROR_NO_SUCH_VALUENAME == ntStatus)
        {
            ntStatus = 0;
            ulMaxSecValueLen = 0;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvShareRegWriteToShareInfo(
                      dataType,
                      pszValueName,
                      pData,
                      ulMaxValueLen,
                      dataSecType,
                      pSecData,
                      ulMaxSecValueLen,
                      &ppShareInfoList[ulNumSharesFound++]);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pppShareInfoList = ppShareInfoList;
    *pulNumSharesFound = ulNumSharesFound;

cleanup:
    if (hRootKey)
    {
        RegCloseKey(hRepository, hRootKey);
    }
    if (hKey)
    {
        RegCloseKey(hRepository, hKey);
    }
    if (hSecKey)
    {
        RegCloseKey(hRepository, hSecKey);
    }
    SRV_SAFE_FREE_MEMORY(pszValueName);
    SRV_SAFE_FREE_MEMORY(pData);

    return ntStatus;

error:
    if (ppShareInfoList)
    {
        SrvShareFreeInfoList(ppShareInfoList, ulNumSharesFound);
    }

    goto cleanup;
}

NTSTATUS
SrvShareRegEndEnum(
    IN HANDLE hRepository,
    IN HANDLE hResume
    )
{
    // TODO
    return STATUS_SUCCESS;
}

NTSTATUS
SrvShareRegDelete(
    IN HANDLE hRepository,
    IN PWSTR  pwszShareName
    )
{
    NTSTATUS ntStatus = 0;
    HKEY hRootKey = NULL;
    PWSTR pwszSharePath = NULL;
    PWSTR pwszShareSecPath = NULL;

    ntStatus = RegOpenKeyExA(
            hRepository,
            NULL,
            LIKEWISE_ROOT_KEY,
            0,
            0,
            &hRootKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMbsToWc16s(pszSharePath, &pwszSharePath);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RegDeleteKeyValue(hRepository,
                                 hRootKey,
                                 pwszSharePath,
                                 pwszShareName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMbsToWc16s(pszShareSecPath, &pwszShareSecPath);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RegDeleteKeyValue(hRepository,
                                hRootKey,
                                pwszShareSecPath,
                                pwszShareName);
    if (LW_ERROR_NO_SUCH_VALUENAME == ntStatus)
    {
        ntStatus = 0;
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    if (hRootKey)
    {
        RegCloseKey(hRepository, hRootKey);
    }
    SRV_SAFE_FREE_MEMORY(pwszSharePath);
    SRV_SAFE_FREE_MEMORY(pwszShareSecPath);

    return ntStatus;

error:
    goto cleanup;
}

NTSTATUS
SrvShareRegGetCount(
    IN     HANDLE  hRepository,
    IN OUT PULONG  pulNumShares
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
SrvShareRegClose(
    IN HANDLE hRepository
    )
{
    // TODO
}

VOID
SrvShareRegShutdown(
    VOID
    )
{
    // TODO
}
