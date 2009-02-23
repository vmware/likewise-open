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


void NetrCleanStubDomainTrustList(NetrDomainTrustList *r)
{
    int i = 0;

    for (i = 0; i < r->count; i++) {
        NetrDomainTrust *trust = &r->array[i];

        SAFE_FREE(trust->netbios_name);
        SAFE_FREE(trust->dns_name);
        if (trust->sid) SidFree(trust->sid);
    }

    free(r->array);
}


static void NetrCleanSamBaseInfo(NetrSamBaseInfo *r)
{
    FreeUnicodeStringEx(&r->account_name);
    FreeUnicodeStringEx(&r->full_name);
    FreeUnicodeStringEx(&r->logon_script);
    FreeUnicodeStringEx(&r->profile_path);
    FreeUnicodeStringEx(&r->home_directory);
    FreeUnicodeStringEx(&r->home_drive);

    r->groups.count = 0;
    SAFE_FREE(r->groups.rids);

    FreeUnicodeStringEx(&r->logon_server);
    FreeUnicodeStringEx(&r->domain);

    if (r->domain_sid) {
        SidFree(r->domain_sid);
        r->domain_sid = NULL;
    }
}


static void NetrCleanSamInfo2(NetrSamInfo2 *r)
{
    NetrCleanSamBaseInfo(&r->base);
}


static void NetrFreeSamInfo2(NetrSamInfo2 *ptr)
{
    if (ptr == NULL) return;

    NetrCleanSamInfo2(ptr);
    free(ptr);
}


static void NetrCleanSidAttr(NetrSidAttr *r, uint32 count)
{
    int i = 0;

    for (i = 0; r && i < count; i++) {
        if (r[i].sid) {
            SidFree(r[i].sid);
            r[i].sid = NULL;
        }
    }
}


static void NetrFreeSidAttr(NetrSidAttr *ptr, uint32 count)
{
    NetrCleanSidAttr(ptr, count);
    free(ptr);
}

static void NetrCleanSamInfo3(NetrSamInfo3 *r)
{
    NetrCleanSamBaseInfo(&r->base);

    if (r->sids) {
        NetrFreeSidAttr(r->sids, r->sidcount);
    }
}


static void NetrFreeSamInfo3(NetrSamInfo3 *ptr)
{
    if (ptr == NULL) return;

    NetrCleanSamInfo3(ptr);
    free(ptr);
}


static void NetrCleanSamInfo6(NetrSamInfo6 *r)
{
    NetrCleanSamBaseInfo(&r->base);

    if (r->sids) {
        NetrFreeSidAttr(r->sids, r->sidcount);
    }

    FreeUnicodeString(&r->forest);
    FreeUnicodeString(&r->principal);
}


static void NetrFreeSamInfo6(NetrSamInfo6 *ptr)
{
    if (ptr == NULL) return;

    NetrCleanSamInfo6(ptr);
    free(ptr);
}


static void NetrCleanPacInfo(NetrPacInfo *r)
{
    SAFE_FREE(r->pac);
    SAFE_FREE(r->auth);

    FreeUnicodeString(&r->logon_domain);
    FreeUnicodeString(&r->logon_server);
    FreeUnicodeString(&r->principal_name);
    FreeUnicodeString(&r->unknown1);
    FreeUnicodeString(&r->unknown2);
    FreeUnicodeString(&r->unknown3);
    FreeUnicodeString(&r->unknown4);
}


static void NetrFreePacInfo(NetrPacInfo *ptr)
{
    if (ptr == NULL) return;

    NetrCleanPacInfo(ptr);
    free(ptr);
}


void NetrCleanStubValidationInfo(NetrValidationInfo *r, uint16 level)
{
    switch (level) {
    case 2:
        NetrFreeSamInfo2(r->sam2);
        break;
    case 3:
        NetrFreeSamInfo3(r->sam3);
        break;
    case 4:
        NetrFreePacInfo(r->pac4);
        break;
    case 5:
        NetrFreePacInfo(r->pac5);
        break;
    case 6:
        NetrFreeSamInfo6(r->sam6);
        break;
    default:
        break;
    }
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
