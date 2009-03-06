/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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


NTSTATUS LsaOpenPolicy2(handle_t b, const wchar16_t *sysname,
                        void *attrib, uint32 access_mask,
                        PolicyHandle *lsa_policy)
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t *system_name = NULL;
    PolicyHandle handle = {0};
    SECURITY_QUALITY_OF_SERVICE qos = {0};
    ObjectAttribute attr = {0};

    goto_if_invalid_param_ntstatus(b, cleanup);
    goto_if_invalid_param_ntstatus(sysname, cleanup);
    goto_if_invalid_param_ntstatus(lsa_policy, cleanup);

    system_name = wc16sdup(sysname);
    goto_if_no_memory_ntstatus(system_name, cleanup);

    /* ObjectAttribute argument is not used, so just pass an empty structure */
    qos.Length              = 0;
    qos.ImpersonationLevel  = SecurityImpersonation;
    qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    qos.EffectiveOnly       = 0;

    attr.len          = 0;
    attr.root_dir     = NULL;
    attr.object_name  = NULL;
    attr.attributes   = 0;
    attr.sec_desc     = NULL;
    // TODO-QOS field in object attributes should probaby be NULL.
    attr.sec_qos      = &qos;

    DCERPC_CALL(_LsaOpenPolicy2(b, system_name, &attr, access_mask, &handle));
    goto_if_ntstatus_not_success(status, cleanup);

    *lsa_policy = handle;

cleanup:
    SAFE_FREE(system_name);

    return status;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
