/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#include "domainjoin.h"

CENTERROR
QueryInformation(
    PDOMAINJOININFO* ppDomainJoinInfo
    )
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    PDOMAINJOININFO pDomainJoinInfo = NULL;

    if (geteuid() != 0) {
       ceError = CENTERROR_DOMAINJOIN_NON_ROOT_USER;
       BAIL_ON_CENTERIS_ERROR(ceError);
    }

    ceError = CTAllocateMemory(sizeof(DOMAINJOININFO), (PVOID*)&pDomainJoinInfo);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetComputerName(&pDomainJoinInfo->pszName);
    BAIL_ON_CENTERIS_ERROR(ceError);

    ceError = DJGetConfiguredDescription(&pDomainJoinInfo->pszDescription);
    if (ceError != CENTERROR_DOMAINJOIN_DESCRIPTION_NOT_FOUND) {
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else {
        ceError = CENTERROR_SUCCESS;
    }

    ceError = DJGetConfiguredDomain(&pDomainJoinInfo->pszDomainName);
    if (ceError != CENTERROR_DOMAINJOIN_DOMAIN_NOT_FOUND) {
        BAIL_ON_CENTERIS_ERROR(ceError);
    } else {
        ceError = CENTERROR_SUCCESS;
    }

    if (IsNullOrEmptyString(pDomainJoinInfo->pszDomainName)) {

        ceError = DJGetConfiguredWorkgroup(&pDomainJoinInfo->pszWorkgroupName);
        BAIL_ON_CENTERIS_ERROR(ceError);

    } else {

        ceError = DJGetConfiguredWorkgroup(&pDomainJoinInfo->pszDomainShortName);
        BAIL_ON_CENTERIS_ERROR(ceError);

    }

    *ppDomainJoinInfo = pDomainJoinInfo;

    return ceError;

error:

    if (pDomainJoinInfo)
        FreeDomainJoinInfo(pDomainJoinInfo);

    return ceError;
}


void
FreeDomainJoinInfo(
    PDOMAINJOININFO pDomainJoinInfo
    )
{
    if (pDomainJoinInfo) {

        if (pDomainJoinInfo->pszName)
            CTFreeString(pDomainJoinInfo->pszName);

        if (pDomainJoinInfo->pszDescription)
            CTFreeString(pDomainJoinInfo->pszDescription);

        if (pDomainJoinInfo->pszDnsDomain)
            CTFreeString(pDomainJoinInfo->pszDnsDomain);

        if (pDomainJoinInfo->pszDomainName)
            CTFreeString(pDomainJoinInfo->pszDomainName);

        if (pDomainJoinInfo->pszDomainShortName)
            CTFreeString(pDomainJoinInfo->pszDomainShortName);

        if (pDomainJoinInfo->pszWorkgroupName)
            CTFreeString(pDomainJoinInfo->pszWorkgroupName);

        if (pDomainJoinInfo->pszLogFilePath)
            CTFreeString(pDomainJoinInfo->pszLogFilePath);

        CTFreeMemory(pDomainJoinInfo);
    }
}
