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
 *        batch_marshal.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Wei Fu (wfu@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "adprovider.h"
#include "batch_marshal.h"

static
DWORD
LsaAdBatchMarshalUserInfoFixHomeDirectory(
    IN OUT PSTR* ppszHomeDirectory,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName
    )
{
    DWORD dwError = 0;
    PSTR pszHomeDirectory = *ppszHomeDirectory;
    PSTR pszNewHomeDirectory = NULL;

    if (!pszHomeDirectory)
    {
        dwError = AD_GetUnprovisionedModeHomedirTemplate(&pszHomeDirectory);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pszHomeDirectory);
    }

    if (strstr(pszHomeDirectory, "%"))
    {
        dwError = AD_BuildHomeDirFromTemplate(
                    pszHomeDirectory,
                    pszNetbiosDomainName,
                    pszSamAccountName,
                    &pszNewHomeDirectory);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_SAFE_FREE_STRING(pszHomeDirectory);
        LSA_XFER_STRING(pszNewHomeDirectory, pszHomeDirectory);
    }

    LsaStrCharReplace(pszHomeDirectory, ' ', '_');

cleanup:
    *ppszHomeDirectory = pszHomeDirectory;
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalUserInfoFixShell(
    IN OUT PSTR* ppszShell
    )
{
    DWORD dwError = 0;
    PSTR pszShell = *ppszShell;

    if (!pszShell)
    {
        dwError = AD_GetUnprovisionedModeShell(&pszShell);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pszShell);
    }

cleanup:
    *ppszShell = pszShell;
    return dwError;

error:
    goto cleanup;
}

// This function fillls in all of the booleans in pObjectUserInfo except for
// bPromptPasswordChange and bAccountExpired
static
VOID
LsaAdBatchMarshalUserInfoAccountControl(
    IN UINT32 AccountControl,
    IN OUT PAD_SECURITY_OBJECT_USER_INFO pObjectUserInfo
    )
{
    pObjectUserInfo->bPasswordNeverExpires = IsSetFlag(AccountControl, LSA_AD_UF_DONT_EXPIRE_PASSWD);
    if (pObjectUserInfo->bPasswordNeverExpires)
    {
        pObjectUserInfo->bPasswordExpired = FALSE;
    }
    else
    {
        pObjectUserInfo->bPasswordExpired = IsSetFlag(AccountControl, LSA_AD_UF_PASSWORD_EXPIRED);
    }
    pObjectUserInfo->bUserCanChangePassword = !IsSetFlag(AccountControl, LSA_AD_UF_CANT_CHANGE_PASSWD);
    pObjectUserInfo->bAccountDisabled = IsSetFlag(AccountControl, LSA_AD_UF_ACCOUNTDISABLE);
    pObjectUserInfo->bAccountLocked = IsSetFlag(AccountControl, LSA_AD_UF_LOCKOUT);
}

static
DWORD
LsaAdBatchMarshalUserInfoAccountExpires(
    IN UINT64 AccountExpires,
    IN OUT PAD_SECURITY_OBJECT_USER_INFO pObjectUserInfo
    )
{
    DWORD dwError = 0;

    if (AccountExpires == 0LL ||
        AccountExpires == 9223372036854775807LL)
    {
        // This means the account will never expire.
        pObjectUserInfo->bAccountExpired = FALSE;
    }
    else
    {
        // in 100ns units:
        UINT64 currentNtTime = 0;

        dwError = ADGetCurrentNtTime(&currentNtTime);
        BAIL_ON_LSA_ERROR(dwError);

        if (currentNtTime <= AccountExpires)
        {
            pObjectUserInfo->bAccountExpired = FALSE;
        }
        else
        {
            pObjectUserInfo->bAccountExpired = TRUE;
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


static
DWORD
LsaAdBatchMarshalUserInfoPasswordLastSet(
    IN UINT64 PasswordLastSet,
    IN OUT PAD_SECURITY_OBJECT_USER_INFO pObjectUserInfo
    )
{
    DWORD dwError = 0;
    // in 100ns units:
    int64_t passwordExpiry = 0;
    UINT64 currentNtTime = 0;

    dwError = ADGetCurrentNtTime(&currentNtTime);
    BAIL_ON_LSA_ERROR(dwError);

    // ISSUE-2008/11/18-dalmeida -- Don't we need to check
    // for the max password age of the user domain rather
    // than the machine domain?
    passwordExpiry = gpADProviderData->adMaxPwdAge -
        (currentNtTime - PasswordLastSet);
    // ISSUE-2008/11/18-dalmeida -- The number of days
    // should be a setting.
    if (passwordExpiry / (10000000LL * 24*60*60) <= 14)
    {
        //The password will expire in 14 days or less
        pObjectUserInfo->bPromptPasswordChange = TRUE;
    }
    else
    {
        pObjectUserInfo->bPromptPasswordChange = FALSE;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalUnprovisionedUser(
    IN OUT PLSA_AD_BATCH_ITEM_USER_INFO pUserInfo,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    DWORD dwId = 0;
    PLSA_SECURITY_IDENTIFIER pSid = 0;

    // uid
    dwError = LsaAllocSecurityIdentifierFromString(pszSid, &pSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashSecurityIdentifierToId(pSid, &dwId);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->uid = (uid_t)dwId;

    // gid
    dwError = LsaSetSecurityIdentifierRid(pSid, pUserInfo->dwPrimaryGroupRid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashSecurityIdentifierToId(pSid, &dwId);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->gid = (gid_t)dwId;

    // can add alias here

cleanup:
    if (pSid)
    {
        LsaFreeSecurityIdentifier(pSid);
    }
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalUnprovisionedGroup(
    IN OUT PLSA_AD_BATCH_ITEM_GROUP_INFO pGroupInfo,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    DWORD dwId = 0;

    // gid
    dwError = LsaHashSidStringToId(pszSid, &dwId);
    BAIL_ON_LSA_ERROR(dwError);

    pGroupInfo->gid = (gid_t)dwId;

    // can add alias here

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalUserInfo(
    IN OUT PLSA_AD_BATCH_ITEM_USER_INFO pUserInfo,
    OUT PAD_SECURITY_OBJECT_USER_INFO pObjectUserInfo,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;

    pObjectUserInfo->bIsGeneratedUPN = FALSE;

    if (LsaAdBatchIsUnprovisionedMode())
    {
        dwError = LsaAdBatchMarshalUnprovisionedUser(
                        pUserInfo,
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        pszSamAccountName,
                        pszSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pObjectUserInfo->uid = pUserInfo->uid;
    pObjectUserInfo->gid = pUserInfo->gid;

    LSA_XFER_STRING(pUserInfo->pszAlias, pObjectUserInfo->pszAliasName);
    LSA_XFER_STRING(pUserInfo->pszPasswd, pObjectUserInfo->pszPasswd);
    LSA_XFER_STRING(pUserInfo->pszGecos, pObjectUserInfo->pszGecos);
    LSA_XFER_STRING(pUserInfo->pszShell, pObjectUserInfo->pszShell);
    LSA_XFER_STRING(pUserInfo->pszHomeDirectory, pObjectUserInfo->pszHomedir);
    LSA_XFER_STRING(pUserInfo->pszUserPrincipalName, pObjectUserInfo->pszUPN);

    pObjectUserInfo->qwPwdLastSet = pUserInfo->PasswordLastSet;
    pObjectUserInfo->qwAccountExpires = pUserInfo->AccountExpires;

    // Handle shell.
    dwError = LsaAdBatchMarshalUserInfoFixShell(&pObjectUserInfo->pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    // Handle home directory.
    dwError = LsaAdBatchMarshalUserInfoFixHomeDirectory(
                    &pObjectUserInfo->pszHomedir,
                    pszNetbiosDomainName,
                    pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    // Handle UPN.
    if (!pObjectUserInfo->pszUPN)
    {
        dwError = ADGetLDAPUPNString(
                        0,
                        NULL,
                        pszDnsDomainName,
                        pszSamAccountName,
                        &pObjectUserInfo->pszUPN,
                        &pObjectUserInfo->bIsGeneratedUPN);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Decode account control flags.
    LsaAdBatchMarshalUserInfoAccountControl(
            pUserInfo->UserAccountControl,
            pObjectUserInfo);

    // Figure out account expiration.
    dwError = LsaAdBatchMarshalUserInfoAccountExpires(
                    pUserInfo->AccountExpires,
                    pObjectUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    // Figure out password prompting.
    dwError = LsaAdBatchMarshalUserInfoPasswordLastSet(
                    pUserInfo->PasswordLastSet,
                    pObjectUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalGroupInfo(
    IN OUT PLSA_AD_BATCH_ITEM_GROUP_INFO pGroupInfo,
    OUT PAD_SECURITY_OBJECT_GROUP_INFO pObjectGroupInfo,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;

    if (LsaAdBatchIsUnprovisionedMode())
    {
        dwError = LsaAdBatchMarshalUnprovisionedGroup(
                        pGroupInfo,
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        pszSamAccountName,
                        pszSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pObjectGroupInfo->gid = pGroupInfo->gid;

    LSA_XFER_STRING(pGroupInfo->pszAlias, pObjectGroupInfo->pszAliasName);
    LSA_XFER_STRING(pGroupInfo->pszPasswd, pObjectGroupInfo->pszPasswd);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshal(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    OUT PAD_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PAD_SECURITY_OBJECT pObject = NULL;

    // To marshal, the following conditions to be satisfied:
    //
    // 1) Object must have user or group type.
    // 2) Object must have real information.
    // 3) Object must have either:
    //    a) Pseudo information.
    //    b) Or be in unprovisioned mode.
    if ((LSA_AD_BATCH_OBJECT_TYPE_UNDEFINED == pItem->ObjectType) ||
        !IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_REAL) ||
        !(IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO) || LsaAdBatchIsUnprovisionedMode()))
    {
        PCSTR pszType = NULL;
        BOOLEAN bIsString = FALSE;
        PCSTR pszString = NULL;
        DWORD dwId = 0;

        LsaAdBatchQueryTermDebugInfo(
                &pItem->QueryTerm,
                &pszType,
                &bIsString,
                &pszString,
                &dwId);
        if (bIsString)
        {
            LSA_LOG_DEBUG("Did not find object by %s '%s'", pszType, pszString);
        }
        else
        {
            LSA_LOG_DEBUG("Did not find object by %s %u", pszType, dwId);
        }
        dwError = 0;
        goto cleanup;
    }

    LSA_ASSERT(LSA_IS_XOR(IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO), LsaAdBatchIsUnprovisionedMode()));

    if (IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED) &&
        (pItem->ObjectType != LSA_AD_BATCH_OBJECT_TYPE_GROUP))
    {
        // Skip any disabled non-groups.
        dwError = 0;
        goto cleanup;
    }

    dwError = LsaAllocateMemory(sizeof(*pObject), (PVOID*)&pObject);
    BAIL_ON_LSA_ERROR(dwError);

    pObject->cache.qwCacheId = -1;

    pObject->enabled = !IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED);

    // Transfer the data
    LSA_XFER_STRING(pItem->pszSid, pObject->pszObjectSid);
    LSA_XFER_STRING(pItem->pszSamAccountName, pObject->pszSamAccountName);
    LSA_XFER_STRING(pItem->pszDn, pObject->pszDN);

    dwError = LsaAllocateString(
                    pItem->pDomainEntry->pszNetbiosDomainName,
                    &pObject->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pItem->ObjectType)
    {
        case LSA_AD_BATCH_OBJECT_TYPE_USER:
            pObject->type = AccountType_User;
            dwError = LsaAdBatchMarshalUserInfo(
                            &pItem->UserInfo,
                            &pObject->userInfo,
                            pItem->pDomainEntry->pszDnsDomainName,
                            pObject->pszNetbiosDomainName,
                            pObject->pszSamAccountName,
                            pObject->pszObjectSid);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
            pObject->type = AccountType_Group;
            dwError = LsaAdBatchMarshalGroupInfo(
                            &pItem->GroupInfo,
                            &pObject->groupInfo,
                            pItem->pDomainEntry->pszDnsDomainName,
                            pObject->pszNetbiosDomainName,
                            pObject->pszSamAccountName,
                            pObject->pszObjectSid);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *ppObject = pObject;
    return dwError;

error:
    if (pObject)
    {
        ADCacheDB_SafeFreeObject(&pObject);
    }
    goto cleanup;
}

DWORD
LsaAdBatchMarshalList(
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    IN DWORD dwAvailableCount,
    OUT PAD_SECURITY_OBJECT* ppObjects,
    OUT PDWORD pdwUsedCount
    )
{
    DWORD dwError = 0;
    PLSA_LIST_LINKS pLinks = NULL;
    DWORD dwIndex = 0;

    for (pLinks = pBatchItemList->Next;
         pLinks != pBatchItemList;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        if (dwIndex >= dwAvailableCount)
        {
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAdBatchMarshal(pItem, &ppObjects[dwIndex]);
        BAIL_ON_LSA_ERROR(dwError);
        if (ppObjects[dwIndex])
        {
            dwIndex++;
        }
    }

cleanup:
    *pdwUsedCount = dwIndex;
    return dwError;

error:
    goto cleanup;
}
