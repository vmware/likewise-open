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

void
FreeUnicodeStringContents(UnicodeStringEx *pStr)
{
    if (pStr->string != NULL)
    {
        rpc_ss_client_free(pStr->string);
        pStr->string = NULL;
        pStr->len = 0;
        pStr->size = 0;
    }
}

void
FreeRidWithAttributeArrayContents(RidWithAttributeArray *pArr)
{
    if (pArr->rids != NULL)
    {
        rpc_ss_client_free(pArr->rids);
        pArr->rids = NULL;
        pArr->count = 0;
    }
}

void
FreeNetrSamBaseInfoContents(NetrSamBaseInfo *pBase)
{
    UnicodeStringEx *ppStrings[] = {
        &pBase->account_name,
        &pBase->full_name,
        &pBase->logon_script,
        &pBase->profile_path,
        &pBase->home_directory,
        &pBase->home_drive,
        &pBase->logon_server,
        &pBase->domain,
    };
    int i;

    for (i = 0; i < sizeof(ppStrings)/sizeof(ppStrings[0]); i++)
    {
        FreeUnicodeStringContents(ppStrings[i]);
    }

    FreeRidWithAttributeArrayContents(&pBase->groups);

    if (pBase->domain_sid != NULL)
    {
        rpc_ss_client_free(pBase->domain_sid);
        pBase->domain_sid = NULL;
    }
}

void
FreeSidAttrArray(NetrSidAttr *pSids, size_t sCount)
{
    size_t sIndex;
    if (pSids == NULL)
    {
        return;
    }

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        if (pSids[sIndex].sid != NULL)
        {
            rpc_ss_client_free(pSids[sIndex].sid);
        }
    }
    rpc_ss_client_free(pSids);
}

void
FreeNetrSamInfo3Contents(NetrSamInfo3 *pInfo)
{
    FreeNetrSamBaseInfoContents(&pInfo->base);
    if (pInfo->sids != NULL)
    {
        FreeSidAttrArray(pInfo->sids, pInfo->sidcount);
        pInfo->sids = NULL;
        pInfo->sidcount = 0;
    }
}

void
FreePacLogonInfo(
    PAC_LOGON_INFO *pInfo)
{
    if (pInfo != NULL)
    {
        FreeNetrSamInfo3Contents(&pInfo->info3);
        if (pInfo->res_group_dom_sid != NULL)
        {
            rpc_ss_client_free(pInfo->res_group_dom_sid);
        }
        FreeRidWithAttributeArrayContents(&pInfo->res_groups);
        rpc_ss_client_free(pInfo);
    }
}

error_status_t
DecodePacLogonInfo(
    const char *pchBuffer,
    size_t sBufferLen,
    PAC_LOGON_INFO **ppLogonInfo)
{
    idl_es_handle_t decodingHandle = NULL;
    error_status_t status;
    error_status_t status2;
    PAC_LOGON_INFO *pLogonInfo = NULL;

    idl_es_decode_buffer(
            (unsigned char *)pchBuffer,
            sBufferLen,
            &decodingHandle,
            &status);
    if (status != error_status_ok)
    {
        goto error;
    }

    idl_es_set_attrs(decodingHandle, IDL_ES_MIDL_COMPAT, &status);
    if (status != error_status_ok)
    {
        goto error;
    }

    PAC_LOGON_INFO_Decode(decodingHandle, &pLogonInfo);
    if (status != error_status_ok)
    {
        goto error;
    }

    idl_es_handle_free(&decodingHandle, &status);
    decodingHandle = NULL;
    if (status != error_status_ok)
    {
        goto error;
    }

    *ppLogonInfo = pLogonInfo;

cleanup:
    return status;

error:
    if (pLogonInfo != NULL)
    {
        FreePacLogonInfo(pLogonInfo);
    }
    if (decodingHandle != NULL)
    {
        // Do not return status2
        idl_es_handle_free(&decodingHandle, &status2);
    }
    goto cleanup;
}
