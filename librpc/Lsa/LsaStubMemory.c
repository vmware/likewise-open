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

#include "includes.h"


void LsaCleanStubTranslatedSidArray(TranslatedSidArray *r)
{
    SAFE_FREE(r->sids);
}


void LsaCleanStubTranslatedSidArray2(TranslatedSidArray2 *r)
{
    SAFE_FREE(r->sids);
}


void LsaCleanStubTranslatedNameArray(TranslatedNameArray *r)
{
    int i = 0;

    for (i = 0; i < r->count; i++) {
        TranslatedName *ptr = &(r->names[i]);

        FreeUnicodeString(&ptr->name);
    }

    free(r->names);
}


void LsaCleanStubRefDomainList(RefDomainList *r)
{
    int i = 0;

    for (i = 0; i < r->count; i++) {
        LsaDomainInfo *ptr = &(r->domains[i]);

        FreeUnicodeStringEx(&ptr->name);
        if (ptr->sid) SidFree(ptr->sid);
    }

    free(r->domains);
}


void LsaFreeStubRefDomainList(RefDomainList *ptr)
{
    LsaCleanStubRefDomainList(ptr);
    free(ptr);
}


void LsaCleanStubPolicyInformation(LsaPolicyInformation *r, uint32 level)
{
    switch (level) {
    case LSA_POLICY_INFO_AUDIT_EVENTS:
        SAFE_FREE(r->audit_events.settings);
        break;

    case LSA_POLICY_INFO_DOMAIN:
    case LSA_POLICY_INFO_ACCOUNT_DOMAIN:
        SAFE_FREE(r->domain.name.string);
        SAFE_FREE(r->domain.sid);
        break;

    case LSA_POLICY_INFO_PD:
        SAFE_FREE(r->pd.name.string);
        break;

    case LSA_POLICY_INFO_REPLICA:
        SAFE_FREE(r->replica.source.string);
        SAFE_FREE(r->replica.account.string);
        break;

    case LSA_POLICY_INFO_DNS:
        SAFE_FREE(r->dns.name.string);
        SAFE_FREE(r->dns.dns_domain.string);
        SAFE_FREE(r->dns.dns_forest.string);
        SAFE_FREE(r->dns.sid);
        break;

    case LSA_POLICY_INFO_AUDIT_LOG:
    case LSA_POLICY_INFO_ROLE:
    case LSA_POLICY_INFO_QUOTA:
    case LSA_POLICY_INFO_DB:
    case LSA_POLICY_INFO_AUDIT_FULL_SET:
    case LSA_POLICY_INFO_AUDIT_FULL_QUERY:
    default:
        break;
    }
}


void LsaFreeStubPolicyInformation(LsaPolicyInformation *ptr, uint32 level)
{
    LsaCleanStubPolicyInformation(ptr, level);
    free(ptr);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
