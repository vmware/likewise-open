/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 *        ipc_protocol.c
 *
 * Abstract:
 *
 *        Registry Interprocess Communication
 *
 *
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include "ipc.h"

#if defined(WORDS_BIGENDIAN)
    #define UCS2_NATIVE "UCS-2BE"
#else
    #define UCS2_NATIVE "UCS-2LE"
#endif

#define LWMSG_MEMBER_PBYTE(_type, _field)           \
    LWMSG_MEMBER_POINTER_BEGIN(_type, _field),      \
    LWMSG_UINT8(BYTE),                              \
    LWMSG_POINTER_END

#define LWMSG_MEMBER_PWSTR(_type, _field)           \
    LWMSG_MEMBER_POINTER_BEGIN(_type, _field),      \
    LWMSG_UINT16(WCHAR),                        \
    LWMSG_POINTER_END,                              \
    LWMSG_ATTR_ZERO_TERMINATED,                     \
    LWMSG_ATTR_ENCODING(UCS2_NATIVE)

#define LWMSG_PWSTR       \
    LWMSG_POINTER_BEGIN, \
    LWMSG_UINT16(WCHAR),  \
    LWMSG_POINTER_END,   \
    LWMSG_ATTR_STRING

/******************************************************************************/

// This one is odd because Windows has decided that a security descriptor
// (that's not a typo... descriptor) changes too much, so the nLength member
// actually is the size of SECURITY_ATTRIBUTE... so this puts us in a bit of
// a bind not knowing how big the pSecurityDescriptor member really is.  What
// saves us (a little) is that NULL is usually passed in that member, so we may
// have to add that as a limitation.
//
// In the mean time, we won't use the nLength as a LWMSG_ATTR_LENGTH_MEMBER
static LWMsgTypeSpec gRegSecAttrSpec[] =
{
    // DWORD  nLength;
    // PVOID  pSecurityDescriptor;
    // BOOL   bInheritHandle;

    LWMSG_STRUCT_BEGIN(SECURITY_ATTRIBUTES),

    LWMSG_MEMBER_UINT32(SECURITY_ATTRIBUTES, nLength),

    LWMSG_MEMBER_POINTER_BEGIN(SECURITY_ATTRIBUTES, pSecurityDescriptor),
    LWMSG_INT64(VOID),
    LWMSG_POINTER_END,
    //LWMSG_ATTR_LENGTH_MEMBER(SECURITY_ATTRIBUTES, nLength), see above comment

    LWMSG_MEMBER_UINT32(SECURITY_ATTRIBUTES, bInheritHandle),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegIPCErrorSpec[] =
{
    // DWORD dwError;

    LWMSG_STRUCT_BEGIN(REG_IPC_ERROR),
    LWMSG_MEMBER_UINT32(REG_IPC_ERROR, dwError),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegCreateKeyExSpec[] =
{
    // IN HKEY hKey,
    // IN PCWSTR pSubKey,
    // IN DWORD Reserved,
    // IN OPTIONAL PWSTR pClass,
    // IN DWORD dwOptions,
    // IN REGSAM samDesired,
    // IN OPTIONAL PSECURITY_ATTRIBUTES pSecurityAttributes,

    LWMSG_STRUCT_BEGIN(REG_IPC_CREATE_KEY_EX_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_CREATE_KEY_EX_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_CREATE_KEY_EX_REQ, pSubKey),

    LWMSG_MEMBER_PWSTR(REG_IPC_CREATE_KEY_EX_REQ, pClass),

    LWMSG_MEMBER_UINT32(REG_IPC_CREATE_KEY_EX_REQ, dwOptions),

    LWMSG_MEMBER_UINT32(REG_IPC_CREATE_KEY_EX_REQ, samDesired),

    // This member may be off... see the comments for the gRegSecAttrSpec above
    LWMSG_MEMBER_POINTER_BEGIN(REG_IPC_CREATE_KEY_EX_REQ, pSecurityAttributes),
    LWMSG_TYPESPEC(gRegSecAttrSpec),
    LWMSG_POINTER_END,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegOpenRootKeySpec[] =
{
    //PSTR pszRootKeyName;

    LWMSG_STRUCT_BEGIN(REG_IPC_OPEN_ROOT_KEY_REQ),

    LWMSG_MEMBER_PSTR(REG_IPC_OPEN_ROOT_KEY_REQ, pszRootKeyName),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegOpenRootKeyRespSpec[] =
{
    // OUT HKEY hkResult,

    LWMSG_STRUCT_BEGIN(REG_IPC_OPEN_ROOT_KEY_RESPONSE),

    LWMSG_MEMBER_HANDLE(REG_IPC_OPEN_ROOT_KEY_RESPONSE, hRootKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegCreateKeyExRespSpec[] =
{
    // OUT HKEY hkResult,
    // OUT OPTIONAL DWORD dwDisposition

    LWMSG_STRUCT_BEGIN(REG_IPC_CREATE_KEY_EX_RESPONSE),

    LWMSG_MEMBER_HANDLE(REG_IPC_CREATE_KEY_EX_RESPONSE, hkResult, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER,

    LWMSG_MEMBER_UINT32(REG_IPC_CREATE_KEY_EX_RESPONSE, dwDisposition),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegEnumRootKeysRespSpec[] =
{
    //PSTR* ppszRootKeyNames;
    //DWORD dwNumRootKeys;

    LWMSG_STRUCT_BEGIN(REG_IPC_ENUM_ROOTKEYS_RESPONSE),
    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_ROOTKEYS_RESPONSE, dwNumRootKeys),
    LWMSG_MEMBER_POINTER_BEGIN(REG_IPC_ENUM_ROOTKEYS_RESPONSE, ppszRootKeyNames),
    LWMSG_PSTR,
    LWMSG_POINTER_END,
    LWMSG_ATTR_ZERO_TERMINATED,
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_ENUM_ROOTKEYS_RESPONSE, dwNumRootKeys),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


/******************************************************************************/

static LWMsgTypeSpec gRegCloseKeySpec[] =
{
    // HKEY hKey;

    LWMSG_STRUCT_BEGIN(REG_IPC_CLOSE_KEY_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_CLOSE_KEY_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegDeleteKeySpec[] =
{
    // HKEY hKey;
    // PCTSTR pSubKey;

    LWMSG_STRUCT_BEGIN(REG_IPC_DELETE_KEY_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_DELETE_KEY_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_DELETE_KEY_REQ, pSubKey),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegDeleteKeyValueSpec[] =
{
    // HKEY hKey;
    // PCTSTR pSubKey;
    // PCTSTR pValueName;

    LWMSG_STRUCT_BEGIN(REG_IPC_DELETE_KEY_VALUE_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_DELETE_KEY_VALUE_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_DELETE_KEY_VALUE_REQ, pSubKey),

    LWMSG_MEMBER_PWSTR(REG_IPC_DELETE_KEY_VALUE_REQ, pValueName),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegDeleteTreeSpec[] =
{
    // HKEY hKey;
    // PCTSTR pSubKey;

    LWMSG_STRUCT_BEGIN(REG_IPC_DELETE_TREE_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_DELETE_TREE_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_DELETE_TREE_REQ, pSubKey),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegDeleteValueSpec[] =
{
    // HKEY hKey;
    // PCTSTR pValueName;

    LWMSG_STRUCT_BEGIN(REG_IPC_DELETE_VALUE_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_DELETE_VALUE_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_DELETE_VALUE_REQ, pValueName),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegEnumKeyExSpec[] =
{
    //HKEY hKey;
    //DWORD dwIndex;
    //PWSTR pName;
    //DWORD cName;
    //PWSTR pClass;
    //PDWORD pcClass;

    LWMSG_STRUCT_BEGIN(REG_IPC_ENUM_KEY_EX_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_ENUM_KEY_EX_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_KEY_EX_REQ, dwIndex),

    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_KEY_EX_REQ, cName),
    LWMSG_MEMBER_PWSTR(REG_IPC_ENUM_KEY_EX_REQ, pName),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_ENUM_KEY_EX_REQ, cName),


    LWMSG_MEMBER_PWSTR(REG_IPC_ENUM_KEY_EX_REQ, pClass),

    LWMSG_MEMBER_POINTER(REG_IPC_ENUM_KEY_EX_REQ, pcClass, LWMSG_UINT32(DWORD)),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegEnumKeyExRespSpec[] =
{
    //PWSTR pName;
    //DWORD cName;

    LWMSG_STRUCT_BEGIN(REG_IPC_ENUM_KEY_EX_RESPONSE),

    LWMSG_MEMBER_PWSTR(REG_IPC_ENUM_KEY_EX_RESPONSE, pName),
    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_KEY_EX_RESPONSE, cName),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegEnumValueSpec[] =
{
    //HKEY hKey;
    //DWORD dwIndex;
    //PWSTR pName;
    //DWORD cName;
    //REG_DATA_TYPE type;
    //PBYTE pValue;
    //DWORD cValue;

    LWMSG_STRUCT_BEGIN(REG_IPC_ENUM_VALUE_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_ENUM_VALUE_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_VALUE_REQ, dwIndex),

    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_VALUE_REQ, cName),
    LWMSG_MEMBER_PWSTR(REG_IPC_ENUM_VALUE_REQ, pName),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_ENUM_VALUE_REQ, cName),


    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_VALUE_REQ, cValue),
    LWMSG_MEMBER_PBYTE(REG_IPC_ENUM_VALUE_REQ, pValue),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_ENUM_VALUE_REQ, cValue),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegEnumValueRespSpec[] =
{
    //PWSTR pName;
    //DWORD cName;
    //PBYTE pValue;
    //DWORD cValue;
    //REG_DATA_TYPE type;

    LWMSG_STRUCT_BEGIN(REG_IPC_ENUM_VALUE_RESPONSE),

    LWMSG_MEMBER_PWSTR(REG_IPC_ENUM_VALUE_RESPONSE, pName),
    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_VALUE_RESPONSE, cName),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_ENUM_VALUE_RESPONSE, cName),

    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_VALUE_RESPONSE, type),

    LWMSG_MEMBER_UINT32(REG_IPC_ENUM_VALUE_RESPONSE, cValue),
    LWMSG_MEMBER_PBYTE(REG_IPC_ENUM_VALUE_RESPONSE, pValue),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_ENUM_VALUE_RESPONSE, cValue),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


/******************************************************************************/

static LWMsgTypeSpec gRegGetValueSpec[] =
{
    //HKEY hKey;
    //PCWSTR pSubKey;
    //PCWSTR pValue;
    //DWORD dwFlags;
    //PBYTE pData;
    //DWORD cbData;

    LWMSG_STRUCT_BEGIN(REG_IPC_GET_VALUE_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_GET_VALUE_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_GET_VALUE_REQ, pSubKey),
    LWMSG_MEMBER_PWSTR(REG_IPC_GET_VALUE_REQ, pValue),
    LWMSG_MEMBER_UINT32(REG_IPC_GET_VALUE_REQ, dwFlags),

    LWMSG_MEMBER_UINT32(REG_IPC_GET_VALUE_REQ, cbData),
    LWMSG_MEMBER_PBYTE(REG_IPC_GET_VALUE_REQ, pData),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_GET_VALUE_REQ, cbData),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegGetValueRespSpec[] =
{
    //DWORD dwType;
    //PBYTE pvData;
    //DWORD cbData;

    LWMSG_STRUCT_BEGIN(REG_IPC_GET_VALUE_RESPONSE),

    LWMSG_MEMBER_UINT32(REG_IPC_GET_VALUE_RESPONSE, dwType),
    LWMSG_MEMBER_UINT32(REG_IPC_GET_VALUE_RESPONSE, cbData),
    LWMSG_MEMBER_PBYTE(REG_IPC_GET_VALUE_RESPONSE, pvData),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_GET_VALUE_RESPONSE, cbData),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegOpenKeyExSpec[] =
{
    // HKEY hKey;
    // PCTSTR pSubKey;
    // REGSAM samDesired;

    LWMSG_STRUCT_BEGIN(REG_IPC_OPEN_KEY_EX_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_OPEN_KEY_EX_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_OPEN_KEY_EX_REQ, pSubKey),

    LWMSG_MEMBER_UINT32(REG_IPC_OPEN_KEY_EX_REQ, samDesired),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegOpenKeyExRespSpec[] =
{
    // OUT HKEY hkResult;

    LWMSG_STRUCT_BEGIN(REG_IPC_OPEN_KEY_EX_RESPONSE),

    LWMSG_MEMBER_HANDLE(REG_IPC_OPEN_KEY_EX_RESPONSE, hkResult, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER,

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegValueEntSpec[] =
{
    // PWSTR     ve_valuename;
    // DWORD     ve_valuelen;
    // PDWORD    ve_valueptr;
    // DWORD     ve_type;

    LWMSG_STRUCT_BEGIN(VALENT),

    LWMSG_MEMBER_PWSTR(VALENT, ve_valuename),
    LWMSG_MEMBER_UINT32(VALENT, ve_valuelen),
    LWMSG_MEMBER_POINTER(VALENT, ve_valueptr, LWMSG_UINT32(DWORD)),
    LWMSG_MEMBER_UINT32(VALENT, ve_type),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegQueryMultipleValuesSpec[] =
{
    // HKEY hKey;
    // DWORD num_vals;
    // PVALENT val_list;
    // DWORD dwTotalsize;
    // PWSTR pValue;

    LWMSG_STRUCT_BEGIN(REG_IPC_QUERY_MULTIPLE_VALUES_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_QUERY_MULTIPLE_VALUES_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_MULTIPLE_VALUES_REQ, num_vals),
    LWMSG_MEMBER_POINTER_BEGIN(REG_IPC_QUERY_MULTIPLE_VALUES_REQ, val_list),
    LWMSG_TYPESPEC(gRegValueEntSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_QUERY_MULTIPLE_VALUES_REQ, num_vals),

    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_MULTIPLE_VALUES_REQ, dwTotalsize),
    LWMSG_MEMBER_PWSTR(REG_IPC_QUERY_MULTIPLE_VALUES_REQ, pValue),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_QUERY_MULTIPLE_VALUES_REQ, dwTotalsize),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gRegQueryMultipleValuesRespSpec[] =
{
    // DWORD num_vals;
    // PVALENT val_list;
    // DWORD dwTotalsize;
    // PWSTR pValue;

    LWMSG_STRUCT_BEGIN(REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE),

    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE, num_vals),
    LWMSG_MEMBER_POINTER_BEGIN(REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE, val_list),
    LWMSG_TYPESPEC(gRegValueEntSpec),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE, num_vals),

    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE, dwTotalsize),
    LWMSG_MEMBER_PWSTR(REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE, pValue),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE, dwTotalsize),


    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


/******************************************************************************/

static LWMsgTypeSpec gRegQueryInfoKeySpec[] =
{
    // HKEY hKey;
    // PDWORD pcClass;

    LWMSG_STRUCT_BEGIN(REG_IPC_QUERY_INFO_KEY_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_QUERY_INFO_KEY_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_POINTER(REG_IPC_QUERY_INFO_KEY_REQ, pcClass, LWMSG_UINT32(DWORD)),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


/******************************************************************************/

static LWMsgTypeSpec gRegQueryInfoKeyRespSpec[] =
{
     //DWORD cSubKeys;
     //DWORD cMaxSubKeyLen;

     //DWORD cValues;
     //DWORD cMaxValueNameLen;
     //DWORD cMaxValueLen;


    LWMSG_STRUCT_BEGIN(REG_IPC_QUERY_INFO_KEY_RESPONSE),

    LWMSG_MEMBER_PWSTR(REG_IPC_QUERY_INFO_KEY_RESPONSE, pClass),
    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_INFO_KEY_RESPONSE, cSubKeys),
    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_INFO_KEY_RESPONSE, cMaxSubKeyLen),

    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_INFO_KEY_RESPONSE, cValues),
    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_INFO_KEY_RESPONSE, cMaxValueNameLen),
    LWMSG_MEMBER_UINT32(REG_IPC_QUERY_INFO_KEY_RESPONSE, cMaxValueLen),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegSetKeyValueSpec[] =
{
    // HKEY hKey;
    // PCTSTR pSubKey;
    // PCTSTR pValueName;
    // DWORD dwType;
    // PCVOID pData;
    // DWORD cbData;

    LWMSG_STRUCT_BEGIN(REG_IPC_SET_KEY_VALUE_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_SET_KEY_VALUE_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_SET_KEY_VALUE_REQ, pSubKey),

    LWMSG_MEMBER_PWSTR(REG_IPC_SET_KEY_VALUE_REQ, pValueName),

    LWMSG_MEMBER_UINT32(REG_IPC_SET_KEY_VALUE_REQ, dwType),

    LWMSG_MEMBER_UINT32(REG_IPC_SET_KEY_VALUE_REQ, cbData),

    LWMSG_MEMBER_POINTER_BEGIN(REG_IPC_SET_KEY_VALUE_REQ, pData),
    LWMSG_UINT8(BYTE),
    LWMSG_POINTER_END,
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_SET_KEY_VALUE_REQ, cbData),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgTypeSpec gRegSetValueExSpec[] =
{
    // HKEY hKey;
    // PCTSTR pValueName;
    //DWORD dwType;
    // const PBYTE pData;
    // DWORD cbData;

    LWMSG_STRUCT_BEGIN(REG_IPC_SET_VALUE_EX_REQ),

    LWMSG_MEMBER_HANDLE(REG_IPC_SET_VALUE_EX_REQ, hKey, HKEY),
    LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER,

    LWMSG_MEMBER_PWSTR(REG_IPC_SET_VALUE_EX_REQ, pValueName),

    LWMSG_MEMBER_UINT32(REG_IPC_SET_VALUE_EX_REQ, dwType),

    LWMSG_MEMBER_UINT32(REG_IPC_SET_VALUE_EX_REQ, cbData),

    LWMSG_MEMBER_POINTER(REG_IPC_SET_VALUE_EX_REQ, pData, LWMSG_UINT8(BYTE)),
    LWMSG_ATTR_LENGTH_MEMBER(REG_IPC_SET_VALUE_EX_REQ, cbData),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

/******************************************************************************/

static LWMsgProtocolSpec gRegIPCSpec[] =
{
    /*Key Operation APIS*/
    LWMSG_MESSAGE(REG_Q_ENUM_ROOT_KEYS, NULL),
    LWMSG_MESSAGE(REG_R_ENUM_ROOT_KEYS_SUCCESS, gRegEnumRootKeysRespSpec),
    LWMSG_MESSAGE(REG_R_ENUM_ROOT_KEYS_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_OPEN_ROOT_KEY, gRegOpenRootKeySpec),
    LWMSG_MESSAGE(REG_R_OPEN_ROOT_KEY_SUCCESS, gRegOpenRootKeyRespSpec),
    LWMSG_MESSAGE(REG_R_OPEN_ROOT_KEY_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_OPEN_KEY_EX, gRegOpenKeyExSpec),
    LWMSG_MESSAGE(REG_R_OPEN_KEY_EX_SUCCESS, gRegOpenKeyExRespSpec),
    LWMSG_MESSAGE(REG_R_OPEN_KEY_EX_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_CREATE_KEY_EX, gRegCreateKeyExSpec),
    LWMSG_MESSAGE(REG_R_CREATE_KEY_EX_SUCCESS, gRegCreateKeyExRespSpec),
    LWMSG_MESSAGE(REG_R_CREATE_KEY_EX_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_CLOSE_KEY, gRegCloseKeySpec),
    LWMSG_MESSAGE(REG_R_CLOSE_KEY_SUCCESS, NULL),
    LWMSG_MESSAGE(REG_R_CLOSE_KEY_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_DELETE_KEY, gRegDeleteKeySpec),
    LWMSG_MESSAGE(REG_R_DELETE_KEY_SUCCESS, NULL),
    LWMSG_MESSAGE(REG_R_DELETE_KEY_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_QUERY_INFO_KEY, gRegQueryInfoKeySpec),
    LWMSG_MESSAGE(REG_R_QUERY_INFO_KEY_SUCCESS, gRegQueryInfoKeyRespSpec),
    LWMSG_MESSAGE(REG_R_QUERY_INFO_KEY_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_ENUM_KEY_EX, gRegEnumKeyExSpec),
    LWMSG_MESSAGE(REG_R_ENUM_KEY_EX_SUCCESS, gRegEnumKeyExRespSpec),
    LWMSG_MESSAGE(REG_R_ENUM_KEY_EX_FAILURE, gRegIPCErrorSpec),
    /*Value Operation APIs*/
    LWMSG_MESSAGE(REG_Q_SET_VALUEA_EX, gRegSetValueExSpec),
    LWMSG_MESSAGE(REG_R_SET_VALUEA_EX_SUCCESS, NULL),
    LWMSG_MESSAGE(REG_R_SET_VALUEA_EX_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_SET_VALUEW_EX, gRegSetValueExSpec),
    LWMSG_MESSAGE(REG_R_SET_VALUEW_EX_SUCCESS, NULL),
    LWMSG_MESSAGE(REG_R_SET_VALUEW_EX_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_GET_VALUEA, gRegGetValueSpec),
    LWMSG_MESSAGE(REG_R_GET_VALUEA_SUCCESS, gRegGetValueRespSpec),
    LWMSG_MESSAGE(REG_R_GET_VALUEA_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_GET_VALUEW, gRegGetValueSpec),
    LWMSG_MESSAGE(REG_R_GET_VALUEW_SUCCESS, gRegGetValueRespSpec),
    LWMSG_MESSAGE(REG_R_GET_VALUEW_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_QUERY_VALUEA_EX, gRegGetValueSpec),
    LWMSG_MESSAGE(REG_R_QUERY_VALUEA_EX_SUCCESS, gRegGetValueRespSpec),
    LWMSG_MESSAGE(REG_R_QUERY_VALUEA_EX_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_QUERY_VALUEW_EX, gRegGetValueSpec),
    LWMSG_MESSAGE(REG_R_QUERY_VALUEW_EX_SUCCESS, gRegGetValueRespSpec),
    LWMSG_MESSAGE(REG_R_QUERY_VALUEW_EX_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_DELETE_KEY_VALUE, gRegDeleteKeyValueSpec),
    LWMSG_MESSAGE(REG_R_DELETE_KEY_VALUE_SUCCESS, NULL),
    LWMSG_MESSAGE(REG_R_DELETE_KEY_VALUE_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_DELETE_TREE, gRegDeleteTreeSpec),
    LWMSG_MESSAGE(REG_R_DELETE_TREE_SUCCESS, NULL),
    LWMSG_MESSAGE(REG_R_DELETE_TREE_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_DELETE_VALUE, gRegDeleteValueSpec),
    LWMSG_MESSAGE(REG_R_DELETE_VALUE_SUCCESS, NULL),
    LWMSG_MESSAGE(REG_R_DELETE_VALUE_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_ENUM_VALUEA, gRegEnumValueSpec),
    LWMSG_MESSAGE(REG_R_ENUM_VALUEA_SUCCESS, gRegEnumValueRespSpec),
    LWMSG_MESSAGE(REG_R_ENUM_VALUEA_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_ENUM_VALUEW, gRegEnumValueSpec),
    LWMSG_MESSAGE(REG_R_ENUM_VALUEW_SUCCESS, gRegEnumValueRespSpec),
    LWMSG_MESSAGE(REG_R_ENUM_VALUEW_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_QUERY_MULTIPLE_VALUES, gRegQueryMultipleValuesSpec),
    LWMSG_MESSAGE(REG_R_QUERY_MULTIPLE_VALUES_SUCCESS, gRegQueryMultipleValuesRespSpec),
    LWMSG_MESSAGE(REG_R_QUERY_MULTIPLE_VALUES_FAILURE, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_Q_SET_KEY_VALUE, gRegSetKeyValueSpec),
    LWMSG_MESSAGE(REG_R_SET_KEY_VALUE_SUCCESS, gRegIPCErrorSpec),
    LWMSG_MESSAGE(REG_R_SET_KEY_VALUE_FAILURE, gRegIPCErrorSpec),
    LWMSG_PROTOCOL_END
};

LWMsgProtocolSpec*
RegIPCGetProtocolSpec(
    void
    )
{
    return gRegIPCSpec;
}

DWORD
RegMapLwmsgStatus(
    LWMsgStatus status
    )
{
    switch (status)
    {
    default:
        return LW_ERROR_INTERNAL;
    case LWMSG_STATUS_SUCCESS:
        return LW_ERROR_SUCCESS;
    case LWMSG_STATUS_ERROR:
        return LW_ERROR_INTERNAL;
    case LWMSG_STATUS_MEMORY:
        return LW_ERROR_OUT_OF_MEMORY;
    case LWMSG_STATUS_MALFORMED:
    case LWMSG_STATUS_OVERFLOW:
    case LWMSG_STATUS_UNDERFLOW:
    case LWMSG_STATUS_EOF:
        return LW_ERROR_INVALID_MESSAGE;
    case LWMSG_STATUS_INVALID_PARAMETER:
        return EINVAL;
    case LWMSG_STATUS_INVALID_STATE:
        return EINVAL;
    case LWMSG_STATUS_UNIMPLEMENTED:
        return LW_ERROR_NOT_IMPLEMENTED;
    case LWMSG_STATUS_SYSTEM:
        return LW_ERROR_INTERNAL;
    case LWMSG_STATUS_SECURITY:
        return EACCES;
    case LWMSG_STATUS_CANCELLED:
        return EINTR;
    case LWMSG_STATUS_FILE_NOT_FOUND:
        return ENOENT;
    case LWMSG_STATUS_CONNECTION_REFUSED:
        return ECONNREFUSED;
    case LWMSG_STATUS_PEER_RESET:
        return ECONNRESET;
    case LWMSG_STATUS_PEER_ABORT:
        return ECONNABORTED;
    case LWMSG_STATUS_PEER_CLOSE:
        return EPIPE;
    case LWMSG_STATUS_SESSION_LOST:
        return EPIPE;
    }
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
