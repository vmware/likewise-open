/*
 * Copyright (c) Likewise Software
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

#include <lw/types.h>
#include <lw/attrs.h>

#include <lwerror.h>

#include <lwps/lwps.h>

typedef DWORD (*UpParseConfigSectionHandler)(
    BOOLEAN bStartOfSection,
    PCSTR pszSectionName, 
    PVOID pData,
    PBOOLEAN pbContinue
    );

typedef DWORD (*UpParseConfigNameValueHandler)(
    PCSTR pszSectionName, 
    PCSTR pszName,
    PCSTR pszValue,
    PVOID pData,
    PBOOLEAN pbContinue
    );

typedef DWORD (*UPCONVPFN)(PCSTR pszConfFilename, PCSTR pszRegFilename);


typedef struct _LWPS_PASSWORD_INFOA
{
    PSTR pszDomainName;
    PSTR pszDnsDomainName;
    PSTR pszSid;
    PSTR pszHostname;
    PSTR pszHostDnsDomain;
    PSTR pszMachineAccount;
    PSTR pszMachinePassword;
    time_t last_change_time;
    DWORD dwSchannelType;
} LWPS_PASSWORD_INFOA, *PLWPS_PASSWORD_INFOA;

DWORD
UpParseConfigFile(
    PCSTR                           pszFilePath,
    UpParseConfigSectionHandler     pfnSectionHandler,
    UpParseConfigNameValueHandler   pfnNameValuePairHandler,
    PVOID                           pData
    );

DWORD
UpParseSambaConfigFile(
    PCSTR                           pszFilePath,
    UpParseConfigSectionHandler     pfnSectionHandler,
    UpParseConfigNameValueHandler   pfnNameValuePairHandler,
    PVOID                           pData
    );


VOID
UpFreeString(
    PSTR pszStr
    );

DWORD
UpFormatBoolean(
    PCSTR pszName,
    BOOLEAN bValue,
    PSTR *ppszBoolean
    );

DWORD
UpFormatDword(
    PCSTR pszName,
    DWORD dwValue,
    PSTR *ppszDword
    );

DWORD
UpFormatString(
    PCSTR pszName,
    PCSTR pszValue,
    PSTR *ppszString
    );

DWORD
UpPrintBoolean(
    FILE *fp,
    PCSTR pszName,
    BOOLEAN bValue
    );

DWORD
UpPrintDword(
    FILE *fp,
    PCSTR pszName,
    DWORD dwValue
    );

DWORD
UpPrintString(
    FILE *fp,
    PCSTR pszName,
    PCSTR pszValue
    );

DWORD
UpParseDateString(
    PCSTR  pszTimeInterval,
    PDWORD pdwTimeInterval
    );

VOID
UpFreeMachineInformationContentsA(
    PLWPS_PASSWORD_INFOA pInfo
    );

DWORD
UpAllocateMachineInformationContentsW(
    PLWPS_PASSWORD_INFOA pInfo,
    PLWPS_PASSWORD_INFO pPasswordInfo
    );

VOID
UpFreeMachineInformationContentsW(
    PLWPS_PASSWORD_INFO pInfo
    );

