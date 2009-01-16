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
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 */


#include "includes.h"


#ifdef FORWARD
#undefine FORWARD
#endif

#define FORWARD(position, str, len, sep)    \
    while (((position) - (str) < (len)) &&  \
           *((position)++) != (sep))


NTSTATUS
RtlParseSidStringA(
    PSID* ppSid,
    PCSTR pszSidStr
    )
{
    const char *sid_start = "S-";
    const char separator = '-';

    size_t sidstr_len;
    char *start, *pos;
    uint8 count, fields, i;
    unsigned int authid;
    SID *pSid = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_NULL_PTR_PARAM(pszSidStr);
    BAIL_ON_NULL_PTR_PARAM(ppSid)

    /* start of the actual sid string */
    sidstr_len = strlen(pszSidStr);
    if (sidstr_len == 0) {
        status = STATUS_INVALID_PARAMETER;
        goto error;
    }

    start = strstr(pszSidStr, sid_start);
    if (start == NULL) {
        status = STATUS_INVALID_SID;
        goto error;
    }

    pos = start;

    /* count the number of fields */
    fields = 0;
    while ((pos - start) < sidstr_len) {
        if ((*pos) == separator) fields++;
        pos++;
    }

    /* first two dashes separate fixed fields
       (S-1-5-21-123456-123456-123456-500) */
    fields -= 2;

    /* skip revision number */
    FORWARD(start, pszSidStr, sidstr_len, separator);

    status = SdAllocateMemory((void**)&pSid, SidGetRequiredSize(fields));
    BAIL_ON_NTSTATUS_ERROR(status);

    pSid->revision = (uint8) atol(start);
    if (pSid->revision != 1) {
        status = STATUS_INVALID_SID;
        goto error;
    }

    /* auth id */
    FORWARD(start, pszSidStr, sidstr_len, separator);

    authid = strtoul(start, &start, 10);
    memset(&pSid->authid, 0, sizeof(pSid->authid));

    /* decimal representation of authid is apparently 32-bit number */
    for (i = 0; i < sizeof(uint32); i++) {
	    unsigned long mask = 0xff << (i * 8);
	    pSid->authid[sizeof(pSid->authid) - (i + 1)] = (authid & mask) >> (i * 8);
    }

    FORWARD(start, pszSidStr, sidstr_len, separator);
    count = 0;

    while ((start - pszSidStr < sidstr_len) &&
           *start != 0) {
        pSid->subauth[count++] = (DWORD) strtoul(start, NULL, 10);
        FORWARD(start, pszSidStr, sidstr_len, separator);
    }

    pSid->subauth_count = count;
    *ppSid = pSid;

cleanup:
    return status;

error:
    SidFree(pSid);
    *ppSid = NULL;
    goto cleanup;
}


NTSTATUS
RtlParseSidStringW(
    PSID *ppSid,
    const wchar16_t *pwszSidStr
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszSidStr = NULL;
    SID* pSid = NULL;

    BAIL_ON_NULL_PTR_PARAM(pwszSidStr);

    pszSidStr = awc16stombs(pwszSidStr);
    BAIL_ON_NULL_PTR(pszSidStr);

    status = RtlParseSidStringA(&pSid, pszSidStr);
    BAIL_ON_NTSTATUS_ERROR(status);

    *ppSid = pSid;

cleanup:
    if (pszSidStr) {
        free(pszSidStr);
    }

    return status;

error:
    if (pSid) {
        SidFree(pSid);
    }

    *ppSid = NULL;
    goto cleanup;
}


NTSTATUS
RtlSidToStringA(
    PSID pSid,
    PSTR* ppszSidStr
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszResult = NULL;
    char  sidPrefix[64];
    PSTR pszSidSuffix = NULL;
    DWORD dwMemAllocated = 0;
    DWORD dwMemAvailable = 0;
    DWORD dwCurOffset = 0;
    DWORD dwStrSize = 0;
    int   iSubAuth = 0;

    BAIL_ON_NULL_PTR_PARAM(pSid);
    BAIL_ON_NULL_PTR_PARAM(ppszSidStr);

    if (pSid->authid[0] || pSid->authid[1])
    {
        sprintf(sidPrefix, "S-%u-0x%.2X%.2X%.2X%.2X%.2X%.2X",
                pSid->revision,
                pSid->authid[0],
                pSid->authid[1],
                pSid->authid[2],
                pSid->authid[3],
                pSid->authid[4],
                pSid->authid[5]);
    }
    else
    {
        uint32 authid =
            ((uint32)pSid->authid[2] << 24) |
            ((uint32)pSid->authid[3] << 16) |
            ((uint32)pSid->authid[4] << 8)  |
            ((uint32)pSid->authid[5] << 0);

        sprintf(sidPrefix, "S-%u-%lu", pSid->revision, (unsigned long) authid);
    }

    for (iSubAuth = 0; iSubAuth < pSid->subauth_count; iSubAuth++)
    {
        char  sidPart[64];
        size_t len = 0;

        sprintf(sidPart, "-%u", pSid->subauth[iSubAuth]);

        len = strlen(sidPart);

        if (dwMemAvailable < (len+1)) {
           DWORD dwMemIncrement = 64;
           PSTR pszNewSuffix = NULL;

           status = SdReallocMemory((void**)&pszNewSuffix,
                                     dwMemAllocated + dwMemIncrement,
                                     pszSidSuffix);
           BAIL_ON_NTSTATUS_ERROR(status);

           pszSidSuffix    = pszNewSuffix;
           dwMemAllocated += dwMemIncrement;
           dwMemAvailable += dwMemIncrement;

           memset(pszSidSuffix + dwCurOffset, 0, dwMemAvailable);
        }

        memcpy(pszSidSuffix + dwCurOffset, sidPart, len);
        dwCurOffset    += len;
        dwMemAvailable -= len;
    }

    dwStrSize = strlen(sidPrefix) + (pszSidSuffix ? strlen(pszSidSuffix) : 0) + 1;
    status = SdAllocateMemory((void*)&pszResult, dwStrSize);
    BAIL_ON_NTSTATUS_ERROR(status);

    sprintf(pszResult, "%s%s", sidPrefix, (pszSidSuffix ? pszSidSuffix : ""));

    *ppszSidStr = pszResult;

cleanup:

    if (pszSidSuffix) {
        SdFreeMemory((void*)pszSidSuffix);
        pszSidSuffix = NULL;
    }

    return status;

error:

    if (pszResult) {
        SdFreeMemory((void*)pszResult);
        pszResult = NULL;
    }

    *ppszSidStr = NULL;
    goto cleanup;
}


void
SidStrFreeA(
    PSTR pszSidStr
    )
{
    SdFreeMemory((void*)pszSidStr);
}


NTSTATUS
RtlSidToStringW(
    PSID pSid,
    PWSTR *ppwszSidStr
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszSidStr = NULL;
    PWSTR pwszSidStr = NULL;

    BAIL_ON_NULL_PTR_PARAM(pSid);
    BAIL_ON_NULL_PTR_PARAM(ppwszSidStr);

    status = RtlSidToStringA(pSid, &pszSidStr);
    BAIL_ON_NTSTATUS_ERROR(status);

    pwszSidStr = ambstowc16s(pszSidStr);
    BAIL_ON_NULL_PTR(pwszSidStr);

    *ppwszSidStr = pwszSidStr;

cleanup:
    if (pszSidStr) {
        free(pszSidStr);
    }

    return status;

error:
    if (pwszSidStr) {
        free(pwszSidStr);
    }

    *ppwszSidStr = NULL;
    goto error;
}


void
SidStrFreeW(
    PWSTR pwszSidStr
    )
{
    SdFreeMemory((void*)pwszSidStr);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
