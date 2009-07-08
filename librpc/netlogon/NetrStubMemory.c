/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


VOID
NetrCleanStubDomainTrustList(
    NetrDomainTrustList *pTrustList
    )
{
    UINT32 i = 0;

    for (i = 0; i < pTrustList->count; i++) {
        NetrDomainTrust *pTrust = &pTrustList->array[i];

        SAFE_FREE(pTrust->netbios_name);
        SAFE_FREE(pTrust->dns_name);
        if (pTrust->sid) {
            MsRpcFreeSid(pTrust->sid);
        }
    }

    free(pTrustList->array);
}


static
VOID
NetrCleanSamBaseInfo(
    NetrSamBaseInfo *pInfo
    )
{
    FreeUnicodeStringEx(&pInfo->account_name);
    FreeUnicodeStringEx(&pInfo->full_name);
    FreeUnicodeStringEx(&pInfo->logon_script);
    FreeUnicodeStringEx(&pInfo->profile_path);
    FreeUnicodeStringEx(&pInfo->home_directory);
    FreeUnicodeStringEx(&pInfo->home_drive);

    pInfo->groups.count = 0;
    SAFE_FREE(pInfo->groups.rids);

    FreeUnicodeStringEx(&pInfo->logon_server);
    FreeUnicodeStringEx(&pInfo->domain);

    if (pInfo->domain_sid) {
        MsRpcFreeSid(pInfo->domain_sid);
        pInfo->domain_sid = NULL;
    }
}


static
VOID
NetrCleanSamInfo2(
    NetrSamInfo2 *pInfo
    )
{
    NetrCleanSamBaseInfo(&pInfo->base);
}


static
VOID
NetrFreeSamInfo2(
    NetrSamInfo2 *pInfo
    )
{
    if (pInfo == NULL) return;

    NetrCleanSamInfo2(pInfo);
    free(pInfo);
}


static
VOID
NetrCleanSidAttr(
    NetrSidAttr *pSidAttr,
    UINT32 Count
    )
{
    UINT32 i = 0;

    for (i = 0; pSidAttr && i < Count; i++) {
        if (pSidAttr[i].sid) {
            MsRpcFreeSid(pSidAttr[i].sid);
            pSidAttr[i].sid = NULL;
        }
    }
}


static
void
NetrFreeSidAttr(
    NetrSidAttr *pSidAttr,
    UINT32 Count)
{
    NetrCleanSidAttr(pSidAttr, Count);
    free(pSidAttr);
}


static
VOID
NetrCleanSamInfo3(
    NetrSamInfo3 *pInfo
    )
{
    NetrCleanSamBaseInfo(&pInfo->base);

    if (pInfo->sids) {
        NetrFreeSidAttr(pInfo->sids,
                        pInfo->sidcount);
    }
}


static
VOID
NetrFreeSamInfo3(
    NetrSamInfo3 *pInfo
    )
{
    if (pInfo == NULL) return;

    NetrCleanSamInfo3(pInfo);
    free(pInfo);
}


static
VOID
NetrCleanSamInfo6(
    NetrSamInfo6 *pInfo
    )
{
    NetrCleanSamBaseInfo(&pInfo->base);

    if (pInfo->sids) {
        NetrFreeSidAttr(pInfo->sids,
                        pInfo->sidcount);
    }

    FreeUnicodeString(&pInfo->forest);
    FreeUnicodeString(&pInfo->principal);
}


static
VOID
NetrFreeSamInfo6(
    NetrSamInfo6 *pInfo
    )
{
    if (pInfo == NULL) return;

    NetrCleanSamInfo6(pInfo);
    free(pInfo);
}


static
VOID
NetrCleanPacInfo(
    NetrPacInfo *pInfo
    )
{
    SAFE_FREE(pInfo->pac);
    SAFE_FREE(pInfo->auth);

    FreeUnicodeString(&pInfo->logon_domain);
    FreeUnicodeString(&pInfo->logon_server);
    FreeUnicodeString(&pInfo->principal_name);
    FreeUnicodeString(&pInfo->unknown1);
    FreeUnicodeString(&pInfo->unknown2);
    FreeUnicodeString(&pInfo->unknown3);
    FreeUnicodeString(&pInfo->unknown4);
}


static
VOID
NetrFreePacInfo(
    NetrPacInfo *pInfo
    )
{
    if (pInfo == NULL) return;

    NetrCleanPacInfo(pInfo);
    free(pInfo);
}


VOID
NetrCleanStubValidationInfo(
    NetrValidationInfo *pInfo,
    UINT16 Level
    )
{
    switch (Level) {
    case 2:
        NetrFreeSamInfo2(pInfo->sam2);
        break;
    case 3:
        NetrFreeSamInfo3(pInfo->sam3);
        break;
    case 4:
        NetrFreePacInfo(pInfo->pac4);
        break;
    case 5:
        NetrFreePacInfo(pInfo->pac5);
        break;
    case 6:
        NetrFreeSamInfo6(pInfo->sam6);
        break;
    default:
        break;
    }
}


static
VOID
NetrCleanDomainTrustInfo(
    NetrDomainTrustInfo *pInfo
    )
{
    UINT32 i = 0;

    if (pInfo == NULL) return;

    FreeUnicodeString(&pInfo->domain_name);
    FreeUnicodeString(&pInfo->full_domain_name);
    FreeUnicodeString(&pInfo->forest);
    MsRpcFreeSid(pInfo->sid);

    for (i = 0;
         i < sizeof(pInfo->unknown1)/sizeof(pInfo->unknown1[0]);
         i++)
    {
        FreeUnicodeString(&pInfo->unknown1[i]);
    }
}


static
VOID
NetrFreeDomainInfo1(
    NetrDomainInfo1 *pInfo
    )
{
    UINT32 i = 0;

    if (pInfo == NULL) return;

    NetrCleanDomainTrustInfo(&pInfo->domain_info);

    for (i = 0; i < pInfo->num_trusts; i++) {
        NetrCleanDomainTrustInfo(&pInfo->trusts[i]);
    }

    free(pInfo->trusts);
    free(pInfo);
}


VOID
NetrCleanStubDomainInfo(
    NetrDomainInfo *pInfo,
    UINT16 Level
    )
{
    if (pInfo == NULL) return;

    switch (Level) {
    case 1:
        NetrFreeDomainInfo1(pInfo->info1);
        break;
    case 2:
        NetrFreeDomainInfo1(pInfo->info2);
        break;
    }
}


VOID
NetrCleanStubDcNameInfo(
    DsrDcNameInfo *pInfo
    )
{
    SAFE_FREE(pInfo->dc_name);
    SAFE_FREE(pInfo->dc_address);
    SAFE_FREE(pInfo->domain_name);
    SAFE_FREE(pInfo->forest_name);
    SAFE_FREE(pInfo->dc_site_name);
    SAFE_FREE(pInfo->cli_site_name);
}


VOID
NetrFreeStubDcNameInfo(
    DsrDcNameInfo *pInfo
    )
{
    if (pInfo == NULL) return;

    NetrCleanStubDcNameInfo(pInfo);
    SAFE_FREE(pInfo);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
