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

#include "includes.h"

/*
 * push functions/macros transfer: net userinfo -> samr userinfo
 * pull functions/macros transfer: net userinfo <- samr userinfo
 */


NTSTATUS
PullLocalGroupInfo0(
    void **buffer,
    AliasInfo *ai,
    int num
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
	LOCALGROUP_INFO_0 *info = NULL;
    int i = 0;

    goto_if_invalid_param_ntstatus(buffer, cleanup);
    goto_if_invalid_param_ntstatus(ai, cleanup);

    if (num <= 0) {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    status = NetAllocateMemory((void**)&info, sizeof(LOCALGROUP_INFO_0) * num,
                               NULL);
    goto_if_ntstatus_not_success(status, error);

    for (i = 0; i < num; i++) {
        PULL_UNICODE_STRING(info[i].lgrpi0_name, ai[i].all.name, info);
    }

	*buffer = (void*)info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *buffer = NULL;
    goto cleanup;
}


NTSTATUS
PullLocalGroupInfo1(
    void **buffer,
    AliasInfo *ai,
    int num
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
	LOCALGROUP_INFO_1 *info = NULL;
    int i = 0;

    goto_if_invalid_param_ntstatus(buffer, cleanup);
    goto_if_invalid_param_ntstatus(ai, cleanup);

    if (num <= 0) {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    status = NetAllocateMemory((void**)&info, sizeof(LOCALGROUP_INFO_1) * num,
                               NULL);
    goto_if_ntstatus_not_success(status, error);

    for (i = 0; i < num; i++) {
        PULL_UNICODE_STRING(info[i].lgrpi1_name, ai[i].all.name, info);
        PULL_UNICODE_STRING(info[i].lgrpi1_comment, ai[i].all.description,
                            info);
    }

    *buffer = info;

cleanup:
	return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *buffer = NULL;
    goto cleanup;
}


NTSTATUS
PushLocalGroupInfo0(
    AliasInfo **sinfo,
    uint32 *slevel,
    LOCALGROUP_INFO_0 *ninfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
	AliasInfo *info = NULL;

	*slevel = ALIAS_INFO_NAME;

    status = NetAllocateMemory((void**)&info, sizeof(AliasInfo), NULL);
    goto_if_ntstatus_not_success(status, error);
	
	PUSH_UNICODE_STRING_ALIASINFO(info->name, ninfo->lgrpi0_name, info);

    *sinfo = info;

cleanup:
	return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo = NULL;
    goto cleanup;
}


NTSTATUS
PushLocalGroupInfo1(
    AliasInfo **sinfo,
    uint32 *slevel,
    LOCALGROUP_INFO_1 *ninfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
	AliasInfo *info = NULL;

	*slevel = ALIAS_INFO_ALL;

    status = NetAllocateMemory((void**)&info, sizeof(AliasInfo), NULL);
    goto_if_ntstatus_not_success(status, error);
	
	PUSH_UNICODE_STRING_ALIASINFO(info->all.name, ninfo->lgrpi1_name, info);
	PUSH_UNICODE_STRING_ALIASINFO(info->all.description, ninfo->lgrpi1_comment,
                                  info);

    *sinfo = info;

cleanup:
	return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo = NULL;
    goto cleanup;
}


NTSTATUS
PushLocalGroupInfo1002(
    AliasInfo **sinfo,
    uint32 *slevel,
    LOCALGROUP_INFO_1002 *ninfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
	AliasInfo *info = NULL;

	*slevel = ALIAS_INFO_DESCRIPTION;

    status = NetAllocateMemory((void**)&info, sizeof(AliasInfo), NULL);
    goto_if_ntstatus_not_success(status, error);
	
	PUSH_UNICODE_STRING_ALIASINFO(info->description, ninfo->lgrpi1002_comment,
                                  info);

    *sinfo = info;

cleanup:
	return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo = NULL;
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
