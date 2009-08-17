/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        provider-main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        AD LDAP Group Marshalling
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "adprovider.h"

DWORD
ADMarshalGetCanonicalName(
    PLSA_SECURITY_OBJECT     pObject,
    PSTR*                   ppszResult)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR    pszResult = NULL;

    if(pObject->type == AccountType_Group &&
            !LW_IS_NULL_OR_EMPTY_STR(pObject->groupInfo.pszAliasName))
    {
        dwError = LwAllocateString(
            pObject->groupInfo.pszAliasName,
            &pszResult);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if(pObject->type == AccountType_User &&
            !LW_IS_NULL_OR_EMPTY_STR(pObject->userInfo.pszAliasName))
    {
        dwError = LwAllocateString(
            pObject->userInfo.pszAliasName,
            &pszResult);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateStringPrintf(
            &pszResult,
            "%s%c%s",
            pObject->pszNetbiosDomainName,
            LsaGetDomainSeparator(),
            pObject->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);

        LwStrCharReplace(
            pszResult,
            ' ',
            AD_GetSpaceReplacement());

        LwStrnToUpper(
            pszResult,
            strlen(pObject->pszNetbiosDomainName));

        LwStrToLower(
            pszResult + strlen(pObject->pszNetbiosDomainName) + 1);
    }

    *ppszResult = pszResult;

cleanup:
    return dwError;

error:
    *ppszResult = NULL;
    LW_SAFE_FREE_STRING(pszResult);
    goto cleanup;
}

DWORD
ADMarshalFromGroupCache(
    PLSA_SECURITY_OBJECT     pGroup,
    size_t                  sMembers,
    PLSA_SECURITY_OBJECT*    ppMembers,
    DWORD                   dwGroupInfoLevel,
    PVOID*                  ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    /* The variable represents pGroupInfo casted to different types. Do not
     * free these values directly, free pGroupInfo instead.
     */   
    size_t sIndex = 0;
    size_t sEnabled = 0;

    *ppGroupInfo = NULL;

    BAIL_ON_INVALID_POINTER(pGroup);

    if(pGroup->type != AccountType_Group)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (!pGroup->enabled)
    {
        /* Unenabled groups can be represented in cache format, but not in
         * group info format. So when marshalling an unenabled group, pretend
         * like it doesn't exist.
         */
        dwError = LW_ERROR_OBJECT_NOT_ENABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch(dwGroupInfoLevel)
    {
        case 0:
        {
            PLSA_GROUP_INFO_0 pGroupInfo0 = NULL;

            dwError = LwAllocateMemory(
                            sizeof(LSA_GROUP_INFO_0),
                            (PVOID*)&pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
            
            pGroupInfo0 = (PLSA_GROUP_INFO_0) pGroupInfo;
            
            pGroupInfo0->gid = pGroup->groupInfo.gid;
            
            dwError = ADMarshalGetCanonicalName(
                            pGroup,
                            &pGroupInfo0->pszName);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwError = LwAllocateString(
                                pGroup->pszObjectSid,
                                &pGroupInfo0->pszSid);
            BAIL_ON_LSA_ERROR(dwError);
            
            break;
        }
        case 1:
        {
            PLSA_GROUP_INFO_1 pGroupInfo1 = NULL; 

            dwError = LwAllocateMemory(
                            sizeof(LSA_GROUP_INFO_1),
                            (PVOID*)&pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
            
            pGroupInfo1 = (PLSA_GROUP_INFO_1) pGroupInfo;
     
            pGroupInfo1->gid = pGroup->groupInfo.gid;
            dwError = ADMarshalGetCanonicalName(
                            pGroup,
                            &pGroupInfo1->pszName);
            BAIL_ON_LSA_ERROR(dwError);

            // Optional values use LwStrDupOrNull. Required values use
            // LwAllocateString.
            dwError = LwStrDupOrNull(
                        pGroup->groupInfo.pszPasswd,
                        &pGroupInfo1->pszPasswd);
            BAIL_ON_LSA_ERROR(dwError);        

            dwError = LwAllocateString(
                                pGroup->pszObjectSid,
                                &pGroupInfo1->pszSid);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateString(
                                pGroup->pszDN,
                                &pGroupInfo1->pszDN);
            BAIL_ON_LSA_ERROR(dwError);

            for (sIndex = 0; sIndex < sMembers; sIndex++)
            {
                if (ppMembers[sIndex]->enabled)
                {
                    sEnabled++;
                }

                if (ppMembers[sIndex]->type != AccountType_User)
                {
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(dwError);
                }
            }

            dwError = LwAllocateMemory(
                            //Leave room for terminating null pointer
                            sizeof(PSTR) * (sEnabled+1),
                            (PVOID*)&pGroupInfo1->ppszMembers);
            BAIL_ON_LSA_ERROR(dwError);

            sEnabled = 0;

            for (sIndex = 0; sIndex < sMembers; sIndex++)
            {
                if (ppMembers[sIndex]->enabled)
                {
                    dwError = ADMarshalGetCanonicalName(
                            ppMembers[sIndex],
                            &pGroupInfo1->ppszMembers[sEnabled++]);
                    BAIL_ON_LSA_ERROR(dwError);
                }
            }
            
            break;
        }

        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }
    
    *ppGroupInfo = pGroupInfo;
    
cleanup:
    
    return dwError;
    
error:
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
        pGroupInfo = NULL;
    }
    
    *ppGroupInfo = NULL;
    
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
