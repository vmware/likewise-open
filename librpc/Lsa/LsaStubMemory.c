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
 * Abstract: Lsa rpc stub memory cleanup routines (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


void
LsaCleanStubTranslatedSidArray(
    TranslatedSidArray *r
    )
{
    uint32 st = 0;

    rpc_sm_client_free(r->sids, &st);
}


void
LsaCleanStubTranslatedSidArray2(
    TranslatedSidArray2 *r
    )
{
    uint32 st = 0;

    rpc_sm_client_free(r->sids, &st);
}


void
LsaCleanStubTranslatedSidArray3(
    TranslatedSidArray3 *r
    )
{
    uint32 st = 0;
    int i = 0;

    for (i = 0; i < r->count; i++) {
        rpc_sm_client_free(r->sids[i].sid, &st);
    }

    rpc_sm_client_free(r->sids, &st);
}


void
LsaCleanStubTranslatedNameArray(
    TranslatedNameArray *r
    )
{
    uint32 st = 0;
    int i = 0;

    for (i = 0; i < r->count; i++) {
        TranslatedName *ptr = &(r->names[i]);

        rpc_sm_client_free(ptr->name.string, &st);
    }

    rpc_sm_client_free(r->names, &st);
}


void
LsaCleanStubRefDomainList(
    RefDomainList *r
    )
{
    uint32 st = 0;
    int i = 0;

    for (i = 0; i < r->count; i++) {
        LsaDomainInfo *ptr = &(r->domains[i]);

        rpc_sm_client_free(ptr->name.string, &st);
        if (ptr->sid) {
            rpc_sm_client_free(ptr->sid, &st);
        }
    }

    rpc_sm_client_free(r->domains, &st);
}


void
LsaFreeStubRefDomainList(
    RefDomainList *ptr
    )
{
    uint32 st = 0;

    LsaCleanStubRefDomainList(ptr);
    rpc_sm_client_free(ptr, &st);
}


void
LsaCleanStubPolicyInformation(
    LsaPolicyInformation *r,
    uint32 level
    )
{
    uint32 st = 0;

    switch (level) {
    case LSA_POLICY_INFO_AUDIT_EVENTS:
        rpc_sm_client_free(r->audit_events.settings, &st);
        break;

    case LSA_POLICY_INFO_DOMAIN:
    case LSA_POLICY_INFO_ACCOUNT_DOMAIN:
        rpc_sm_client_free(r->domain.name.string, &st);
        rpc_sm_client_free(r->domain.sid, &st);
        break;

    case LSA_POLICY_INFO_PD:
        rpc_sm_client_free(r->pd.name.string, &st);
        break;

    case LSA_POLICY_INFO_REPLICA:
        rpc_sm_client_free(r->replica.source.string, &st);
        rpc_sm_client_free(r->replica.account.string, &st);
        break;

    case LSA_POLICY_INFO_DNS:
        rpc_sm_client_free(r->dns.name.string, &st);
        rpc_sm_client_free(r->dns.dns_domain.string, &st);
        rpc_sm_client_free(r->dns.dns_forest.string, &st);
        rpc_sm_client_free(r->dns.sid, &st);
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


void
LsaFreeStubPolicyInformation(
    LsaPolicyInformation *ptr,
    uint32 level)
{
    uint32 st = 0;

    LsaCleanStubPolicyInformation(ptr, level);
    rpc_sm_client_free(ptr, &st);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
