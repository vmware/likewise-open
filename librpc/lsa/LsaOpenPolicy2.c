/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

NTSTATUS
LsaOpenPolicy2(
    handle_t hBinding,
    PCWSTR pwszSysname,
    PVOID attrib,
    UINT32 AccessMask,
    PolicyHandle *phPolicy
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszSystemName = NULL;
    PolicyHandle hLsaPolicy = {0};
    SECURITY_QUALITY_OF_SERVICE SecQos = {0};
    ObjectAttribute ObjAttribute = {0};

    BAIL_ON_INVALID_PTR(hBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszSysname, ntStatus);
    BAIL_ON_INVALID_PTR(phPolicy, ntStatus);

    ntStatus = RtlWC16StringDuplicate(&pwszSystemName, pwszSysname);
    BAIL_ON_NT_STATUS(ntStatus);

    /* ObjectAttribute argument is not used, so just
       pass an empty structure */

    SecQos.Length              = 0;
    SecQos.ImpersonationLevel  = SecurityImpersonation;
    SecQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecQos.EffectiveOnly       = 0;

    ObjAttribute.len          = 0;
    ObjAttribute.root_dir     = NULL;
    ObjAttribute.object_name  = NULL;
    ObjAttribute.attributes   = 0;
    ObjAttribute.sec_desc     = NULL;
    // TODO-QOS field in object attributes should probaby be NULL.
    ObjAttribute.sec_qos      = &SecQos;

    DCERPC_CALL(ntStatus, _LsaOpenPolicy2(
                              hBinding,
                              pwszSystemName,
                              &ObjAttribute,
                              AccessMask,
                              &hLsaPolicy));
    BAIL_ON_NT_STATUS(ntStatus);

    *phPolicy = hLsaPolicy;

cleanup:
    RtlWC16StringFree(&pwszSystemName);

    return ntStatus;

error:
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
