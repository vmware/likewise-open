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

#include <reg/reg.h>

#define BAIL_ON_NON_LWREG_ERROR(dwError) \
        if (!(40700 <= dwError && dwError <= 41200)) {  \
           BAIL_ON_LWIO_ERROR(dwError);            \
        }

typedef struct __SMB_CONFIG_REG
{
    HANDLE hConnection;
    HKEY hKey;
    PSTR pszConfigKey;
    PSTR pszPolicyKey;
} SMB_CONFIG_REG, *PSMB_CONFIG_REG;

typedef enum
{
    SMBTypeString,
    SMBTypeDword,
    SMBTypeBoolean,
    SMBTypeChar,
    SMBTypeEnum
} SMB_CONFIG_TYPE;

typedef struct __SMB_CONFIG_TABLE
{
    PCSTR   pszName;
    BOOLEAN bUsePolicy;
    SMB_CONFIG_TYPE Type;
    DWORD dwMin;
    DWORD dwMax;
    const PCSTR *ppszEnumNames;
    PVOID pValue;
} SMB_CONFIG_TABLE, *PSMB_CONFIG_TABLE;

DWORD
SMBProcessConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PSMB_CONFIG_TABLE pConfig,
    DWORD dwConfigEntries
    );

DWORD
SMBOpenConfig(
    PCSTR pszConfigKey,
    PCSTR pszPolicyKey,
    PSMB_CONFIG_REG *ppReg
    );

VOID
SMBCloseConfig(
    PSMB_CONFIG_REG pReg
    );

DWORD
SMBReadConfigString(
    PSMB_CONFIG_REG pReg,
    PCSTR   pszName,
    BOOLEAN bUsePolicy,
    PSTR    *ppszValue
    );

DWORD
SMBReadConfigDword(
    PSMB_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD   dwMin,
    DWORD   dwMax,
    PDWORD pdwValue
    );

DWORD
SMBReadConfigBoolean(
    PSMB_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    PBOOLEAN pbValue
    );

DWORD
SMBReadConfigEnum(
    PSMB_CONFIG_REG pReg,
    PCSTR pszName,
    BOOLEAN bUsePolicy,
    DWORD dwMin,
    DWORD dwMax,
    const PCSTR *ppszEnumNames,
    PDWORD pdwValue
    );
