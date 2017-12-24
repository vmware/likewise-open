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
 *        netlogon.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Samr rpc server stub functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *          Adam Bernstein (abernstein@vmware.com)
 */

#include "includes.h"

NTSTATUS srv_netr_Function00(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function01(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


    /* function 0x02 */
NTSTATUS srv_NetrLogonSamLogon(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ NetrAuth *creds,
        /* [in] */ NetrAuth *ret_creds,
        /* [in] */ UINT16 logon_level,
        /* [in] */ NetrLogonInfo *logon,
        /* [in] */ UINT16 validation_level,
        /* [out] */ NetrValidationInfo *validation,
        /* [out] */ UINT8 *authoritative
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}

	/* function 0x03 */
NTSTATUS srv_NetrLogonSamLogoff(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ NetrAuth *creds,
        /* [in] */ NetrAuth *ret_creds,
        /* [in] */ UINT16 logon_level,
        /* [in] */ NetrLogonInfo *logon
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}



	/* function 0x04 */
NTSTATUS srv_NetrServerReqChallenge(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ NetrCred *credentials
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}



	/* function 0x05 */
NTSTATUS srv_NetrServerAuthenticate(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t account_name[],
        /* [in] */ UINT16 secure_channel_type,
        /* [in] */ wchar16_t computer_name[],
        /* [in] */ NetrCred *credentials
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}



NTSTATUS srv_netr_Function06(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function07(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function08(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function09(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0c(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0d(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function0e(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


	/* function 0x0f */
NTSTATUS srv_NetrServerAuthenticate2(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t account_name[],
        /* [in] */ UINT16 secure_channel_type,
        /* [in] */ wchar16_t computer_name[],
        /* [in] */ NetrCred *credentials,
        /* [in] */ UINT32 *negotiate_flags
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}



NTSTATUS srv_netr_Function10(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function11(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function12(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function13(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


    /* function 0x14 */
WINERROR srv_DsrGetDcName(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *domain_name,
        /* [in] */ GUID *domain_guid,
        /* [in] */ GUID *site_guid,
        /* [in] */ UINT32 get_dc_flags,
        /* [out] */ DsrDcNameInfo **info
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}



NTSTATUS srv_netr_Function15(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function16(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function17(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function18(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function19(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


	/* function 0x1a */
NTSTATUS srv_NetrServerAuthenticate3(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t account_name[],
        /* [in] */ UINT16 secure_channel_type,
        /* [in] */ wchar16_t computer_name[],
        /* [in] */ NetrCred *credentials,
        /* [in] */ UINT32 *negotiate_flags,
        /* [out] */ UINT32 *rid
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}



NTSTATUS srv_netr_Function1b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function1c(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


    /* function 0x1d */
NTSTATUS srv_NetrLogonGetDomainInfo(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ NetrAuth *creds,
        /* [in] */ NetrAuth *ret_creds,
        /* [in] */ UINT32 level,
        /* [in] */ NetrDomainQuery *query,
        /* [out] */ NetrDomainInfo *info
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}




NTSTATUS srv_netr_Function1e(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function1f(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function20(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function21(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function22(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function23(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


	/* function 0x24 */
NTSTATUS srv_NetrEnumerateTrustedDomainsEx(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [out] */ NetrDomainTrustList *domain_trusts
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}



NTSTATUS srv_netr_Function25(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function26(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


	/* function 0x27 */
NTSTATUS srv_NetrLogonSamLogonEx(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ wchar16_t *computer_name,
        /* [in] */ UINT16 logon_level,
        /* [in] */ NetrLogonInfo *logon,
        /* [in] */ UINT16 validation_level,
        /* [out] */ NetrValidationInfo *validation,
        /* [out] */ UINT8 *authoritative,
        /* [in] */ UINT32 *flags
        )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}




	/* function 0x28 */
WINERROR srv_DsrEnumerateDomainTrusts(
        /* [in] */ handle_t IDL_handle,
        /* [in] */ wchar16_t *server_name,
        /* [in] */ UINT32 trust_flags,
        /* [out] */ NetrDomainTrustList *trusts
        )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszServerName = NULL;
    DWORD dwDomainTrustCount = 1;
    DWORD dwNetBiosNameLen = 0;
    NetrDomainTrust *pDomainTrustArray = {0};
#if 1 /* TBD:Adam-Perform ldap queries to get this data; hard code now */
    CHAR szNetBiosName[256] = {0}; /* MAX hostname length */
    PSTR pszDnsDomainName = NULL;
#if 1
    /* TBD:Adam-Endian issues with this Guid. This is how wireshark parses below value */
    PSTR pszDomainGuid = NULL;
#else
    PSTR pszDomainGuid = "3c448826-516c-467a-adb9-f95252680c8a";
#endif

    PSTR pszDomainSid = NULL;

    /* 0x1d */
    DWORD dwTrustFlags = NETR_TRUST_FLAG_NATIVE  | NETR_TRUST_FLAG_PRIMARY |
                         NETR_TRUST_FLAG_TREEROOT | NETR_TRUST_FLAG_IN_FOREST;
    DWORD dwParentIndex = 0x00;
    DWORD dwTrustType = 0x02;
    DWORD dwTrustAttrs = 0x00;
#endif
    PWSTR pwszNetBiosName = NULL;
    PWSTR pwszDnsDomainName = NULL;
    PSID pDomainSid = NULL;
    uuid_t domainGuid;

    gethostname(szNetBiosName, sizeof(szNetBiosName));
    dwNetBiosNameLen = (DWORD) strlen(szNetBiosName);
    if (dwNetBiosNameLen > 16) /* TBD: Need to improve "Uniqueness test */
    {
        szNetBiosName[15] = '$';
        szNetBiosName[16] = '\0';
    }

#if 1 /* TBD:Adam-Perform ldap queries to get this data; hard code now */
    /* TBD:Adam-Still need to get promoted domain name pszDnsDomainName/DomainGuid/DomainSid */
    pszDnsDomainName = "lightwave.local";
    pszDomainGuid = "2688443c-6c51-7a46-adb9-f95252680c8a";
    pszDomainSid = "S-1-5-21-100314066-221396614-742840509";
#endif

/* I am the DC, so return data returned from calling
 *  NetrLogonGetDomainInfo()
 *
 * "If the server is a domain controller (section 3.1.4.8), it MUST perform 
 *  behavior equivalent to locally invoking NetrLogonGetDomainInfo with 
 *  the previously described parameters."
 */
    status = LwRtlCStringAllocateFromWC16String(
                 &pszServerName,
                 server_name);
    BAIL_ON_NTSTATUS_ERROR(status);

    LSA_LOG_ERROR("srv_NetrEnumerateTrustedDomainsEx: server_name=%s trusts=%x",
                  pszServerName, trust_flags);

    status = LwRtlWC16StringAllocateFromCString(
                 &pwszNetBiosName,
                 szNetBiosName);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = LwRtlWC16StringAllocateFromCString(
                 &pwszDnsDomainName,
                 pszDnsDomainName);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = RtlAllocateSidFromCString(
                 &pDomainSid,
                 pszDomainSid);
    BAIL_ON_NTSTATUS_ERROR(status);

    if (uuid_parse(pszDomainGuid, domainGuid))
    {
        status = STATUS_NO_GUID_TRANSLATION;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    /* Populate this value! */
    status = NetlogonSrvAllocateMemory(
                 (VOID **) &pDomainTrustArray,
                 sizeof(NetrDomainTrust) * dwDomainTrustCount);
    BAIL_ON_NTSTATUS_ERROR(status);


    pDomainTrustArray[0].netbios_name = pwszNetBiosName;
    pDomainTrustArray[0].dns_name = pwszDnsDomainName;
    pDomainTrustArray[0].sid = pDomainSid;
    memcpy(&pDomainTrustArray[0].guid, &domainGuid, sizeof(domainGuid));
    pDomainTrustArray[0].trust_flags = dwTrustFlags; /* TBD:Adam & [in] trust_flags ?? */
    pDomainTrustArray[0].parent_index = dwParentIndex;
    pDomainTrustArray[0].trust_type = dwTrustType;
    pDomainTrustArray[0].trust_attrs = dwTrustAttrs; /* ??? */ 
 
    trusts->count = dwDomainTrustCount;
    trusts->array = pDomainTrustArray;

cleanup:
    LW_SAFE_FREE_MEMORY(pszServerName);
    return status;

error:
    LW_SAFE_FREE_MEMORY(pwszNetBiosName);
    LW_SAFE_FREE_MEMORY(pwszDnsDomainName);
    LW_SAFE_FREE_MEMORY(pDomainSid);
    LW_SAFE_FREE_MEMORY(pDomainTrustArray);
    
    goto cleanup;
}



NTSTATUS srv_netr_Function29(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2c(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2d(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2e(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function2f(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function30(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function31(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function32(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function33(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function34(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function35(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function36(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function37(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function38(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function39(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function3a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS srv_netr_Function3b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
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
