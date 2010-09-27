/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        srvgss.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        GSS Support
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"
#include "srvgss_p.h"

BOOLEAN
SrvGssNegotiateIsComplete(
    HANDLE hGssNegotiate
    )
{
    PSRV_GSS_NEGOTIATE_CONTEXT pGssNegotiate = (PSRV_GSS_NEGOTIATE_CONTEXT)hGssNegotiate;
    return pGssNegotiate->state == SRV_GSS_CONTEXT_STATE_COMPLETE;
}

static
NTSTATUS
SrvGssGetSessionKey(
    gss_ctx_id_t Context,
    PBYTE* ppSessionKey,
    PDWORD pdwSessionKeyLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBYTE pSessionKey = NULL;
    DWORD dwSessionKeyLength = 0;
    OM_uint32 gssMajor = GSS_S_COMPLETE;
    OM_uint32 gssMinor = 0;
    gss_buffer_set_t sessionKey = NULL;

    gssMajor = gss_inquire_sec_context_by_oid(
                    &gssMinor,
                    Context,
                    GSS_C_INQ_SSPI_SESSION_KEY,
                    &sessionKey);
    if (gssMajor != GSS_S_COMPLETE)
    {
        srv_display_status("gss_inquire_sec_context_by_oid", gssMajor, gssMinor);
        // TODO - error code conversion
        status = gssMajor;
        BAIL_ON_LWIO_ERROR(status);
    }

    // The key is in element 0 and the key type OID is in element 1
    if (!sessionKey ||
        (sessionKey->count < 1) ||
        !sessionKey->elements[0].value ||
        (0 == sessionKey->elements[0].length))
    {
        LWIO_ASSERT_MSG(FALSE, "Invalid session key");
        status = STATUS_ASSERTION_FAILURE;
        BAIL_ON_LWIO_ERROR(status);
    }

    status = LW_RTL_ALLOCATE(&pSessionKey, BYTE, sessionKey->elements[0].length);
    BAIL_ON_LWIO_ERROR(status);

    memcpy(pSessionKey, sessionKey->elements[0].value, sessionKey->elements[0].length);
    dwSessionKeyLength = sessionKey->elements[0].length;

cleanup:
    gss_release_buffer_set(&gssMinor, &sessionKey);

    *ppSessionKey = pSessionKey;
    *pdwSessionKeyLength = dwSessionKeyLength;

    return status;

error:
    LWIO_SAFE_FREE_MEMORY(pSessionKey);
    dwSessionKeyLength = 0;

    goto cleanup;
}


static
NTSTATUS
SrvGssGetClientPrincipalName(
    gss_ctx_id_t Context,
    PSTR *ppszClientName
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    OM_uint32 gssMajor = GSS_S_COMPLETE;
    OM_uint32 gssMinor = 0;
    gss_buffer_desc nameBuffer = {0};
    gss_buffer_set_t ClientName = NULL;
    PSTR pszClientPrincipalName = NULL;
    gss_name_t initiatorName = {0};
    gss_buffer_desc clientNameBuffer = {0};
    gss_OID nameOid = {0};

    /* Try the old way first */

    gssMajor = gss_inquire_context(
                   &gssMinor,
                   Context,
                   &initiatorName,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL);
    if (gssMajor == GSS_S_COMPLETE)
    {
        gssMajor = gss_display_name(
                       &gssMinor,
                       initiatorName,
                       &clientNameBuffer,
                       &nameOid);
        BAIL_ON_SEC_ERROR(gssMajor);

        nameBuffer = clientNameBuffer;
    }
    else
    {
        /* Fallback to using the newer inquire_by_oid() method */

        gssMajor = gss_inquire_sec_context_by_oid(
                        &gssMinor,
                        Context,
                        GSS_C_NT_STRING_UID_NAME,
                        &ClientName);
        BAIL_ON_SEC_ERROR(gssMajor);

        if (!ClientName || (ClientName->count == 0))
        {
            ntStatus = STATUS_NONE_MAPPED;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        nameBuffer = ClientName->elements[0];
    }

    ntStatus = SrvAllocateMemory(
                   (nameBuffer.length + 1) * sizeof(CHAR),
                   (PVOID*)&pszClientPrincipalName);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(pszClientPrincipalName, nameBuffer.value, nameBuffer.length);
    pszClientPrincipalName[nameBuffer.length] = '\0';

    *ppszClientName = pszClientPrincipalName;

cleanup:

    gss_release_buffer_set(&gssMinor, &ClientName);
    gss_release_name(&gssMinor, &initiatorName);
    gss_release_buffer(&gssMinor, &clientNameBuffer);

    return ntStatus;

sec_error:

    ntStatus = LWIO_ERROR_GSS;

error:

    goto cleanup;
}

NTSTATUS
SrvGssGetSessionDetails(
    HANDLE hGssNegotiate,
    PBYTE* ppSessionKey,
    PULONG pulSessionKeyLength,
    PSTR* ppszClientPrincipalName,
    LW_MAP_SECURITY_GSS_CONTEXT* pContextHandle
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_GSS_NEGOTIATE_CONTEXT pGssNegotiate = (PSRV_GSS_NEGOTIATE_CONTEXT)hGssNegotiate;
    PBYTE pSessionKey = NULL;
    DWORD dwSessionKeyLength = 0;
    PSTR pszClientPrincipalName = NULL;

    if (!SrvGssNegotiateIsComplete(hGssNegotiate))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ppSessionKey)
    {
        ntStatus = SrvGssGetSessionKey(
                        *pGssNegotiate->pGssContext,
                        &pSessionKey,
                        &dwSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ppszClientPrincipalName)
    {
        ntStatus = SrvGssGetClientPrincipalName(
                       *pGssNegotiate->pGssContext,
                       &pszClientPrincipalName);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ppszClientPrincipalName)
    {
        *ppszClientPrincipalName = pszClientPrincipalName;
    }

    if (ppSessionKey)
    {
        *ppSessionKey = pSessionKey;
        *pulSessionKeyLength = dwSessionKeyLength;
    }

    if (pContextHandle)
    {
        *pContextHandle = *pGssNegotiate->pGssContext;
    }

cleanup:

    return ntStatus;

error:

    if (ppSessionKey)
    {
        *ppSessionKey = NULL;
    }
    if (pulSessionKeyLength)
    {
        *pulSessionKeyLength = 0;
    }

    if (pSessionKey)
    {
        SrvFreeMemory(pSessionKey);
    }
    if (pszClientPrincipalName)
    {
        SrvFreeMemory(pszClientPrincipalName);
    }

    goto cleanup;
}

NTSTATUS
SrvGssBeginNegotiate(
    PHANDLE phGssResume
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_GSS_NEGOTIATE_CONTEXT pGssNegotiate = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_GSS_NEGOTIATE_CONTEXT),
                    (PVOID*)&pGssNegotiate);
    BAIL_ON_NT_STATUS(ntStatus);

    pGssNegotiate->state = SRV_GSS_CONTEXT_STATE_INITIAL;

    ntStatus = SrvAllocateMemory(
                    sizeof(gss_ctx_id_t),
                    (PVOID*)&pGssNegotiate->pGssContext);
    BAIL_ON_NT_STATUS(ntStatus);

    *pGssNegotiate->pGssContext = GSS_C_NO_CONTEXT;

    *phGssResume = (HANDLE)pGssNegotiate;

cleanup:

    return ntStatus;

error:

    *phGssResume = NULL;

    if (pGssNegotiate)
    {
        SrvGssEndNegotiate((HANDLE)pGssNegotiate);
    }

    goto cleanup;
}

NTSTATUS
SrvGssNegotiate(
    HANDLE  hGssResume,
    PBYTE   pSecurityInputBlob,
    ULONG   ulSecurityInputBlobLen,
    PBYTE*  ppSecurityOutputBlob,
    ULONG*  pulSecurityOutputBloblen
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_GSS_NEGOTIATE_CONTEXT pGssNegotiate = (PSRV_GSS_NEGOTIATE_CONTEXT)hGssResume;
    PBYTE pSecurityBlob = NULL;
    ULONG ulSecurityBlobLen = 0;

    switch(pGssNegotiate->state)
    {
        case SRV_GSS_CONTEXT_STATE_INITIAL:

            if (pSecurityInputBlob)
            {
                ntStatus = STATUS_INVALID_PARAMETER_3;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            // intentional fall through

        case SRV_GSS_CONTEXT_STATE_HINTS:

            ntStatus = SrvGssContinueNegotiate(
                            pGssNegotiate,
                            NULL,
                            0,
                            &pSecurityBlob,
                            &ulSecurityBlobLen);

            break;

        case SRV_GSS_CONTEXT_STATE_NEGOTIATE:

            if (!pSecurityInputBlob)
            {
                ntStatus = STATUS_INVALID_PARAMETER_3;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ntStatus = SrvGssContinueNegotiate(
                            pGssNegotiate,
                            pSecurityInputBlob,
                            ulSecurityInputBlobLen,
                            &pSecurityBlob,
                            &ulSecurityBlobLen);

            break;

        case SRV_GSS_CONTEXT_STATE_COMPLETE:

            break;

    }

    BAIL_ON_NT_STATUS(ntStatus);

error:

    *ppSecurityOutputBlob = pSecurityBlob;
    *pulSecurityOutputBloblen = ulSecurityBlobLen;

    return ntStatus;
}

VOID
SrvGssEndNegotiate(
    HANDLE hGssResume
    )
{
    ULONG ulMinorStatus = 0;
    PSRV_GSS_NEGOTIATE_CONTEXT pGssNegotiateContext = NULL;

    pGssNegotiateContext = (PSRV_GSS_NEGOTIATE_CONTEXT)hGssResume;

    if (pGssNegotiateContext->pGssContext &&
        (*pGssNegotiateContext->pGssContext != GSS_C_NO_CONTEXT))
    {
        gss_delete_sec_context(
                        &ulMinorStatus,
                        pGssNegotiateContext->pGssContext,
                        GSS_C_NO_BUFFER);
    }

    if (pGssNegotiateContext->pGssContext)
        SrvFreeMemory(pGssNegotiateContext->pGssContext);

    SrvFreeMemory(pGssNegotiateContext);
}

static
NTSTATUS
SrvGssContinueNegotiate(
    PSRV_GSS_NEGOTIATE_CONTEXT pGssNegotiate,
    PBYTE                      pSecurityInputBlob,
    ULONG                      ulSecurityInputBlobLen,
    PBYTE*                     ppSecurityOutputBlob,
    ULONG*                     pulSecurityOutputBloblen
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulMajorStatus = 0;
    ULONG ulMinorStatus = 0;
    gss_buffer_desc input_desc = {0};
    gss_buffer_desc output_desc = {0};
    ULONG ret_flags = 0;
    PBYTE pSecurityBlob = NULL;
    ULONG ulSecurityBlobLength = 0;

    static gss_OID_desc gss_spnego_mech_oid_desc =
                              {6, (void *)"\x2b\x06\x01\x05\x05\x02"};
    static gss_OID gss_spnego_mech_oid = &gss_spnego_mech_oid_desc;

    input_desc.length = ulSecurityInputBlobLen;
    input_desc.value = pSecurityInputBlob;

    ulMajorStatus = gss_accept_sec_context(
                        (OM_uint32 *)&ulMinorStatus,
                        pGssNegotiate->pGssContext,
                        NULL,
                        &input_desc,
                        NULL,
                        NULL,
                        &gss_spnego_mech_oid,
                        &output_desc,
                        &ret_flags,
                        NULL,
                        NULL);

    srv_display_status("gss_accept_sec_context", ulMajorStatus, ulMinorStatus);

    BAIL_ON_SEC_ERROR(ulMajorStatus);

    switch (ulMajorStatus)
    {
        case GSS_S_CONTINUE_NEEDED:

            pGssNegotiate->state = SRV_GSS_CONTEXT_STATE_NEGOTIATE;

            break;

        case GSS_S_COMPLETE:

            pGssNegotiate->state = SRV_GSS_CONTEXT_STATE_COMPLETE;

            break;

        default:

            ntStatus = LWIO_ERROR_GSS;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    if (output_desc.length)
    {
        ntStatus = SrvAllocateMemory(
                        output_desc.length,
                        (PVOID*)&pSecurityBlob);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pSecurityBlob, output_desc.value, output_desc.length);

        ulSecurityBlobLength = output_desc.length;
    }

    *ppSecurityOutputBlob = pSecurityBlob;
    *pulSecurityOutputBloblen = ulSecurityBlobLength;

cleanup:

    gss_release_buffer(&ulMinorStatus, &output_desc);

    return ntStatus;

sec_error:

    ntStatus = STATUS_LOGON_FAILURE;

error:

    *ppSecurityOutputBlob = NULL;
    *pulSecurityOutputBloblen = 0;

    if (pSecurityBlob)
    {
        SrvFreeMemory(pSecurityBlob);
    }

    goto cleanup;
}

static
void
srv_display_status(
    PCSTR     pszId,
    OM_uint32 maj_stat,
    OM_uint32 min_stat
    )
{
     srv_display_status_1(pszId, maj_stat, GSS_C_GSS_CODE);
     srv_display_status_1(pszId, min_stat, GSS_C_MECH_CODE);
}

static
void
srv_display_status_1(
    PCSTR     pszId,
    OM_uint32 code,
    int       type
    )
{
    OM_uint32 maj_stat, min_stat;
    gss_buffer_desc msg;
    OM_uint32 msg_ctx;

    if ( code == 0 )
    {
        return;
    }

    msg_ctx = 0;
    while (1)
    {
        maj_stat = gss_display_status(&min_stat, code,
                                      type, GSS_C_NULL_OID,
                                      &msg_ctx, &msg);

        switch(code)
        {
#ifdef WIN32
            case SEC_E_OK:
            case SEC_I_CONTINUE_NEEDED:
#else
            case GSS_S_COMPLETE:
            case GSS_S_CONTINUE_NEEDED:
            case 40157:   /* What minor code is this? */
#endif
                LWIO_LOG_VERBOSE("GSS-API error calling %s: %d (%s)\n",
                        pszId, code,
                        (char *)msg.value);
                break;

            default:

                LWIO_LOG_ERROR("GSS-API error calling %s: %d (%s)\n",
                        pszId, code,
                        (char *)msg.value);
        }

        (void) gss_release_buffer(&min_stat, &msg);

        if (!msg_ctx)
            break;
    }
}

/**
 * Caller must not free the memory since it is reused
 **/

NTSTATUS
SrvGssNegHints(
    PBYTE *ppNegHints,
    ULONG *pulNegHintsLength
    )
{
    NTSTATUS ntStatus      = STATUS_SUCCESS;
    HANDLE   hGssNegotiate = NULL;
    BOOLEAN  bInLock       = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gSrvElements.mutex);

    if (!gSrvElements.pHintsBuffer)
    {
        ntStatus = SrvGssBeginNegotiate(&hGssNegotiate);
        BAIL_ON_NT_STATUS(ntStatus);

        /* MIT Krb5 1.7 returns the NegHints blob if you call
           gss_accept_sec_context() with a NULL input buffer */

        ((PSRV_GSS_NEGOTIATE_CONTEXT)hGssNegotiate)->state = SRV_GSS_CONTEXT_STATE_HINTS;
        ntStatus = SrvGssNegotiate(
                       hGssNegotiate,
                       NULL,
                       0,
                       &gSrvElements.pHintsBuffer,
                       &gSrvElements.ulHintsLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppNegHints        = gSrvElements.pHintsBuffer;
    *pulNegHintsLength = gSrvElements.ulHintsLength;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &gSrvElements.mutex);

    if (hGssNegotiate)
    {
        SrvGssEndNegotiate(hGssNegotiate);
    }

    return ntStatus;

error:

    *ppNegHints = NULL;
    *pulNegHintsLength = 0;

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
