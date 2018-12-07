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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        vmpac.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Kerberos 5 PAC decoding functions
 *
 */

#include "includes.h"

VOID
FreeVmPacAccessInfo(
    VMPAC_ACCESS_INFO *pInfo)
{
    if (pInfo == NULL)
    {
        return;
    }

    rpc_ss_client_free(pInfo);

    return;
}

#ifdef BAIL_ON_ERR_STATUS
#undef BAIL_ON_ERR_STATUS
#endif

#define BAIL_ON_ERR_STATUS(err)                 \
    if ((err) != error_status_ok) {             \
        goto error;                             \
    }


NTSTATUS
DecodeVmPacAccessInfo(
    IN PVOID pBuffer,
    IN size_t sBufferLen,
    OUT VMPAC_ACCESS_INFO** ppAccessInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    idl_es_handle_t decodingHandle = NULL;
    error_status_t status;
    error_status_t status2;
    PVMPAC_ACCESS_INFO pAccessInfo = NULL;

    idl_es_decode_buffer(
            (unsigned char *)pBuffer,
            sBufferLen,
            &decodingHandle,
            &status);
    BAIL_ON_ERR_STATUS(status);


    idl_es_set_attrs(decodingHandle, IDL_ES_MIDL_COMPAT, &status);
    BAIL_ON_ERR_STATUS(status);

    VMPAC_ACCESS_INFO_Decode(decodingHandle, &pAccessInfo);
    BAIL_ON_ERR_STATUS(status);

    idl_es_handle_free(&decodingHandle, &status);
    decodingHandle = NULL;
    BAIL_ON_ERR_STATUS(status);

    *ppAccessInfo = pAccessInfo;

cleanup:
    if (status != error_status_ok)
    {
        ntStatus = LwRpcStatusToNtStatus(status);
    }

    return ntStatus;

error:
    if (pAccessInfo != NULL)
    {
        FreeVmPacAccessInfo(pAccessInfo);
    }
    if (decodingHandle != NULL)
    {
        // Do not return status2
        idl_es_handle_free(&decodingHandle, &status2);
    }
    goto cleanup;
}


NTSTATUS
EncodeVmPacAccessInfo(
    IN VMPAC_ACCESS_INFO* pAccessInfo,
    OUT PDWORD pdwEncodedSize,
    OUT PVOID* ppEncodedBuffer
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    idl_es_handle_t decodingHandle = NULL;
    error_status_t status;
    error_status_t status2;

    idl_es_encode_dyn_buffer(
        (idl_byte**) (void*) ppEncodedBuffer,
        (idl_ulong_int*) pdwEncodedSize,
        &decodingHandle,
        &status);
    BAIL_ON_ERR_STATUS(status);

    idl_es_set_attrs(decodingHandle, IDL_ES_MIDL_COMPAT, &status);
    BAIL_ON_ERR_STATUS(status);

    VMPAC_ACCESS_INFO_Encode(decodingHandle, pAccessInfo);
    BAIL_ON_ERR_STATUS(status);

    idl_es_handle_free(&decodingHandle, &status);
    decodingHandle = NULL;
    BAIL_ON_ERR_STATUS(status);

cleanup:
    if (status != error_status_ok)
    {
        ntStatus = LwRpcStatusToNtStatus(status);
    }

    return ntStatus;

error:

    if (decodingHandle != NULL)
    {
        // Do not return status2
        idl_es_handle_free(&decodingHandle, &status2);
    }

    goto cleanup;
}
