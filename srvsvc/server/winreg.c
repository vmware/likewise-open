/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * WinReg Server
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *
 */

#include "includes.h"

#define LW_REG_NONE     0
#define LW_REG_SZ       1

/********************************************************************
 *******************************************************************/

void
REGISTRY_HANDLE_rundown(
    void *hContext
    )
{
    UINT32 *h = (UINT32*)hContext;

    if (h)
    {
        LW_SAFE_FREE_MEMORY(h);

    }

    return;
}


/********************************************************************
 *******************************************************************/

void _winreg_Function0x0(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x1(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

WINERR
_RegOpenHKLM(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ WCHAR *system_name,
    /* [in] */ UINT32 access_mask,
    /* [out] */ REGISTRY_HANDLE *handle
    )
{
    WINERR dwError = ERROR_SUCCESS;

cleanup:
    return dwError;

error:
    goto cleanup;
}



/********************************************************************
 *******************************************************************/

void _winreg_Function0x3(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x4(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

WINERR
_RegCloseKey(
    /* [in] */ handle_t IDL_handle,
    /* [in, out] */ REGISTRY_HANDLE *handle
    )
{
    WINERR dwError = ERROR_SUCCESS;
    UINT32 *h = NULL;

    dwError = LwAllocateMemory(sizeof(UINT32), &h);
    BAIL_ON_ERROR(dwError);

    *handle = (REGISTRY_HANDLE)h;

cleanup:
    return dwError;

error:
    goto cleanup;
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x6(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x7(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x8(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x9(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0xa(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0xb(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0xc(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0xd(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0xe(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

WINERR
_RegOpenKey(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ REGISTRY_HANDLE parent_handle,
    /* [in] */ RegString key_name,
    /* [in] */ UINT32 unknown,
    /* [in] */ UINT32 access_mask,
    /* [out] */ REGISTRY_HANDLE *handle
    )
{
    WINERR dwError = ERROR_FILE_NOT_FOUND;
    PSTR pszKeyName = NULL;
    UINT32 *h = NULL;

    dwError = LwWc16sToMbs(key_name.string, &pszKeyName);
    BAIL_ON_ERROR(dwError);

    if (LwRtlCStringIsEqual(
            pszKeyName,
            "system\\currentcontrolset\\control\\productoptions",
            FALSE))
    {
        dwError = ERROR_SUCCESS;
    }
    else if (LwRtlCStringIsEqual(
            pszKeyName,
            "software\\microsoft\\windows nt\\currentversion",
            FALSE))
    {
        dwError = ERROR_SUCCESS;
    }

    if (dwError == ERROR_SUCCESS)
    {
        dwError = LwAllocateMemory(sizeof(UINT32), &h);
        BAIL_ON_ERROR(dwError);

        *handle = (REGISTRY_HANDLE)h;
    }

cleanup:
    LW_SAFE_FREE_STRING(pszKeyName);

    return dwError;

error:
    goto error;
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x10(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

WINERR
_RegQueryValue(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ REGISTRY_HANDLE handle,
    /* [in] */ RegString value_name,
    /* [in, out] */ UINT32 *type,
    /* [in, out] */ UINT8 *buffer,
    /* [in, out] */ UINT32 *buffer_size_ptr,
    /* [in, out] */ UINT32 *buffer_size,
    /* [in, out] */ UINT32 *buffer_len_ptr,
    /* [in, out] */ UINT32 *buffer_len
    )
{
    WINERR dwError = ERROR_FILE_NOT_FOUND;
    PSTR pszValueName = NULL;

    *buffer_size_ptr = 1;
    *buffer_len_ptr = 1;

    dwError = LwWc16sToMbs(value_name.string, &pszValueName);
    BAIL_ON_ERROR(dwError);

    if (LwRtlCStringIsEqual(pszValueName, "ProductType", FALSE))
    {
        PCSTR pszProductTypeValue = "WinNT";
        DWORD dwPTValueLen = strlen(pszProductTypeValue)+1;
        PWSTR pwszProductTypeValue = NULL;

        *type = LW_REG_SZ;

        if ((*buffer_size > 0) && (dwPTValueLen <= *buffer_size))
        {
            //dwError = SrvSvcSrvAllocateMemory(*buffer_size, (PVOID*)buffer);
            //BAIL_ON_ERROR(dwError);

            dwError = LwMbsToWc16s(pszProductTypeValue, &pwszProductTypeValue);
            BAIL_ON_ERROR(dwError);

            memcpy(buffer, pwszProductTypeValue, sizeof(WCHAR)*dwPTValueLen);
            *buffer_len = dwPTValueLen * sizeof(WCHAR);

            LW_SAFE_FREE_STRING(pwszProductTypeValue);
        }
        else
        {
            *buffer_size = dwPTValueLen * sizeof(WCHAR);
            *buffer_len = 0;
        }

        dwError = ERROR_SUCCESS;
    }

cleanup:
    LW_SAFE_FREE_STRING(pszValueName);

    return dwError;

error:
    goto cleanup;
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x12(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x13(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x14(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x15(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x16(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x17(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x18(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x19(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

WINERR
_RegGetVersion(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ REGISTRY_HANDLE handle,
    /* [out] */ UINT32 *version
    )
{
    *version = 0x05;

    return ERROR_SUCCESS;
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x1b(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x1c(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x1d(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x1e(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x1f(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x20(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x21(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _winreg_Function0x22(
    /* [in] */ handle_t IDL_handle
    )
{
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
