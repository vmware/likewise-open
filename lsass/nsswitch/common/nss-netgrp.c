/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        nss-group.c
 *
 * Abstract:
 * 
 *        Name Server Switch (Likewise LSASS)
 * 
 *        Handle NSS NetGroup Information (Common)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "lsanss.h"

static const DWORD MAX_NUM_GROUPS = 500;

VOID
LsaNssClearEnumNetGroupsState(
    PLSA_ENUMGROUPS_STATE pState
    )
{
    if (pState->hLsaConnection != (HANDLE)NULL) {

        if (pState->ppNetGroupInfoList) {
            LsaFreeNetGroupInfoList(
                pState->dwNetGroupInfoLevel,
                pState->ppNetGroupInfoList,
                pState->dwNumNetGroups
                );
            pState->ppNetGroupInfoList = (HANDLE)NULL;
        }
        
        if (pState->hResume != (HANDLE)NULL) {
            LsaEndEnumNetGroups(pState->hLsaConnection, pState->hResume);
            pState->hResume = (HANDLE)NULL;
        }
        
        LsaCloseServer(pState->hLsaConnection);
        
        pState->hLsaConnection = (HANDLE)NULL;
    }

    memset(pState, 0, sizeof(LSA_ENUMGROUPS_STATE));
    
    pState->dwNetGroupInfoLevel = 1;
}



DWORD
LsaNssWriteNetGroupInfo(
    DWORD       dwNetGroupInfoLevel,
    PVOID       pNetGroupInfo,
    group_ptr_t pResultNetGroup,
    char**      ppszBuf,
    int         bufLen)
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1 pNetGroupInfo_1 = NULL;
    PSTR  pszMarker = *ppszBuf;
    DWORD dwLen = 0;
    DWORD dwAlignBytes = 0;
    DWORD dwNumMembers = 0;
    
    memset(pResultNetGroup, 0, sizeof(struct group));
    
    if ((dwNetGroupInfoLevel != 0) && (dwGroupInfoLevel != 1)) {
        dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    pNetGroupInfo_1 = (PLSA_GROUP_INFO_1)pGroupInfo;
    
    dwNumMembers = LsaNssGetNumberNetGroupMembers(pGroupInfo_1->ppszMembers);
    
    dwAlignBytes = (dwNumMembers ? ((((HANDLE)pszMarker) % sizeof(ULONG)) * sizeof(ULONG)) : 0);

    if (LsaNssComputeNetGroupStringLength(dwAlignBytes, pGroupInfo_1) > bufLen) {
       dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
       BAIL_ON_LSA_ERROR(dwError);
    }
    
    pResultNetGroup->gr_gid = pGroupInfo_1->gid;

    memset(pszMarker, 0, bufLen);
    
    pszMarker += dwAlignBytes;
    pResultNetGroup->gr_mem = (PSTR*)pszMarker;
    
    //
    // Handle NetGroup Members first, because we computed the
    // alignment adjustment based on the first pointer position
    //
    if (!dwNumMembers) {        
       *(pResultNetGroup->gr_mem) = NULL;
       pszMarker += sizeof(ULONG) + 1;
       
    } else {
        PSTR pszMemberMarker = NULL;
        DWORD iMember = 0;
        
        // This is where we start writing the members
        pszMemberMarker = pszMarker + (sizeof(PSTR) * (dwNumMembers + 1));

        for (iMember = 0; iMember < dwNumMembers; iMember++)
        {
            *(pResultNetGroup->gr_mem+iMember) = pszMemberMarker;
            pszMarker += sizeof(PSTR);
            
            dwLen = strlen(*(pNetGroupInfo_1->ppszMembers + iMember));
            memcpy(pszMemberMarker, *(pNetGroupInfo_1->ppszMembers + iMember), dwLen);
            pszMemberMarker += dwLen + 1;
        }
        // Handle the terminating NULL
        *(pResultNetGroup->gr_mem+iMember) = NULL;
        pszMarker = ++pszMemberMarker; // skip NULL
    }
    
    if (!IsNullOrEmptyString(pNetGroupInfo_1->pszName)) {
       dwLen = strlen(pNetGroupInfo_1->pszName);
       memcpy(pszMarker, pNetGroupInfo_1->pszName, dwLen);
       pResultNetGroup->gr_name = pszMarker;
       pszMarker += dwLen + 1;
    }

    if (!IsNullOrEmptyString(pNetGroupInfo_1->pszPasswd)) {
       dwLen = strlen(pNetGroupInfo_1->pszPasswd);
       memcpy(pszMarker, pNetGroupInfo_1->pszPasswd, dwLen);
       pResultNetGroup->gr_passwd = pszMarker;
       pszMarker += dwLen + 1;
    }
    else{
        dwLen = sizeof("x") - 1;
        *pszMarker = 'x';
        pResultNetGroup->gr_passwd = pszMarker;
        pszMarker += dwLen + 1;
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

NSS_STATUS
LsaNssCommonNetGroupSetgrent(
    PLSA_ENUMGROUPS_STATE     pEnumNetGroupsState
    )
{
    int                       ret = NSS_STATUS_SUCCESS;
    
    LsaNssClearEnumNetGroupsState(pEnumGroupsState);
    
    ret = MAP_LSA_ERROR(NULL,
                        LsaOpenServer(&pEnumNetGroupsState->hLsaConnection));
    BAIL_ON_NSS_ERROR(ret);
    
    ret = MAP_LSA_ERROR(NULL,
                        LsaBeginEnumNetGroups(
                            pEnumNetGroupsState->hLsaConnection,
                            pEnumNetGroupsState->dwGroupInfoLevel,
                            MAX_NUM_GROUPS,
                            &pEnumNetGroupsState->hResume));
    BAIL_ON_NSS_ERROR(ret);

cleanup:

    return ret;
    
error:

    LsaNssClearEnumNetGroupsState(pEnumGroupsState);

    goto cleanup;
}

NSS_STATUS
LsaNssCommonNetGroupGetgrent(
    PLSA_ENUMGROUPS_STATE     pEnumNetGroupsState,
    struct group*             pResultNetGroup,
    char *                    pszBuf,
    size_t                    bufLen,
    int*                      pErrorNumber
    )
{
    int                       ret = NSS_STATUS_NOTFOUND;
    
    if (pEnumNetGroupsState->hLsaConnection == (HANDLE)NULL)
    {
        ret = MAP_LSA_ERROR(pErrorNumber, LSA_ERROR_INVALID_LSA_CONNECTION);
        BAIL_ON_NSS_ERROR(ret);
    }
    
    if (!pEnumNetGroupsState->bTryAgain)
    {
        if (!pEnumNetGroupsState->idxGroup ||
            (pEnumNetGroupsState->idxGroup >= pEnumGroupsState->dwNumGroups))
        {    
            if (pEnumNetGroupsState->ppGroupInfoList) {
                LsaFreeNetGroupInfoList(
                   pEnumNetGroupsState->dwGroupInfoLevel,
                   pEnumNetGroupsState->ppGroupInfoList,
                   pEnumNetGroupsState->dwNumGroups);
                pEnumNetGroupsState->ppGroupInfoList = NULL;
                pEnumNetGroupsState->dwNumGroups = 0;
                pEnumNetGroupsState->idxGroup = 0;
            }
            
            ret = MAP_LSA_ERROR(pErrorNumber,
                           LsaEnumNetGroups(
                               pEnumNetGroupsState->hLsaConnection,
                               pEnumNetGroupsState->hResume,
                               &pEnumNetGroupsState->dwNumGroups,
                               &pEnumNetGroupsState->ppGroupInfoList));
            BAIL_ON_NSS_ERROR(ret);
        }
        
    }

    if (pEnumNetGroupsState->dwNumGroups) {
        PLSA_GROUP_INFO_1 pNetGroupInfo = 
            (PLSA_GROUP_INFO_1)*(pEnumNetGroupsState->ppGroupInfoList+pEnumGroupsState->idxGroup);
        ret = MAP_LSA_ERROR(pErrorNumber,
                            LsaNssWriteNetGroupInfo(
                                pEnumNetGroupsState->dwGroupInfoLevel,
                                pNetGroupInfo,
                                pResultNetGroup,
                                &pszBuf,
                                bufLen));
        BAIL_ON_NSS_ERROR(ret);
        pEnumNetGroupsState->idxGroup++;
        
        ret = NSS_STATUS_SUCCESS;
    } else {
        ret = NSS_STATUS_UNAVAIL;
        
        if (pErrorNumber) {
            *pErrorNumber = ENOENT;
        }
    }   
    
    pEnumNetGroupsState->bTryAgain = FALSE;
    
cleanup:

    return ret;
     
error:

    if ((ret == NSS_STATUS_TRYAGAIN) && pErrorNumber && (*pErrorNumber == ERANGE))
    {
        pEnumNetGroupsState->bTryAgain = TRUE;
    }
    else
    {
       LsaNssClearEnumNetGroupsState(pEnumGroupsState);
    }
    
    if (bufLen && pszBuf) {
        memset(pszBuf, 0, bufLen);
    }

    goto cleanup;
}

NSS_STATUS
LsaNssCommonNetGroupEndgrent(
    PLSA_ENUMGROUPS_STATE     pEnumNetGroupsState
    )
{
    LsaNssClearEnumNetGroupsState(pEnumGroupsState);

    return NSS_STATUS_SUCCESS;
}

