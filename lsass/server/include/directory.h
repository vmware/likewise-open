/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        directory.h
 *
 * Abstract:
 *
 *
 *      Likewise Directory Wrapper Interface
 *
 *      Directory Interface API
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__

typedef ULONG DIRECTORY_ATTR_TYPE;

#define DIRECTORY_ATTR_TYPE_BOOLEAN                 1
#define DIRECTORY_ATTR_TYPE_INTEGER                 2
#define DIRECTORY_ATTR_TYPE_LARGE_INTEGER           3
#define DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR  4
#define DIRECTORY_ATTR_TYPE_OCTET_STREAM            5
#define DIRECTORY_ATTR_TYPE_UNICODE_STRING          6
#define DIRECTORY_ATTR_TYPE_ANSI_STRING             7

typedef struct _OCTET_STRING {
    ULONG ulNumBytes;
    PBYTE pBytes;
} OCTET_STRING, *POCTET_STRING;

typedef struct _ATTRIBUTE_VALUE
{
    DIRECTORY_ATTR_TYPE Type;

    union _ATTRIBUTE_VALUE_DATA
    {
        ULONG  ulValue;
        LONG64 llValue;
        PWSTR  pwszStringValue;
        PSTR   pszStringValue;
        BOOL   bBooleanValue;
        POCTET_STRING pOctetString;
    } data;

} ATTRIBUTE_VALUE, *PATTRIBUTE_VALUE;

typedef struct _DIRECTORY_ATTRIBUTE
{
    PWSTR            pwszName;

    ULONG            ulNumValues;
    PATTRIBUTE_VALUE pValues;

} DIRECTORY_ATTRIBUTE, *PDIRECTORY_ATTRIBUTE;

typedef struct _DIRECTORY_ENTRY
{
    ULONG                ulNumAttributes;
    PDIRECTORY_ATTRIBUTE pAttributes;

} DIRECTORY_ENTRY, *PDIRECTORY_ENTRY;

typedef ULONG DIR_MOD_FLAGS;

#define DIR_MOD_FLAGS_ADD     0x0
#define DIR_MOD_FLAGS_REPLACE 0x1
#define DIR_MOD_FLAGS_DELETE  0x2

typedef struct _DIRECTORY_MOD
{
    DIR_MOD_FLAGS    ulOperationFlags;
    PWSTR            pwszAttrName;
    ULONG            ulNumValues;
    PATTRIBUTE_VALUE pAttrValues;

} DIRECTORY_MOD, *PDIRECTORY_MOD;

#define ENTRY(i, ppDirectoryEntry) = *(ppDirectoryEntry + i)
#define ATTRIBUTE(i, ppDirectoryAttributes) = *(ppDirectoryAttributes + i)

#define VALUE(i, pAttribute) = *(pAttribute->ppAttributeValues + i)
#define NUM_VALUES(pAttribute) = pAttribute->ulNumValues
#define INTEGER(pValue) =  pValue->ulValue;
#define PRINTABLE_STRING(pValue) = pValue->pPrintableString;
#define IA5_STRING(pValue) = pValue->pIA5String;
#define OCTET_STRING_LENGTH(pValue) = pValue->pOctetString->ulNumBytes;
#define OCTET_STRING_DATA(pValue) = pValue->pOctetString->pBytes;
#define NT_SECURITY_DESCRIPTOR_LENGTH(pValue) = pValue->pNTSecurityDescriptor->ulNumBytes;
#define NT_SECURITY_DESCRIPTOR_DATA(pValue) = pValue->pNTSecurityDescriptor->pBytes;

#define DIRECTORY_FREE_STRING(pszStr) \
    if (pszStr) { \
        DirectoryFreeString(pszStr); \
    }

#define DIRECTORY_FREE_STRING_AND_RESET(pszStr) \
    if (pszStr) { \
        DirectoryFreeString(pszStr); \
        (pszStr) = NULL; \
    }

#define DIRECTORY_FREE_MEMORY(pMem) \
    if (pMem) { \
        DirectoryFreeMemory(pMem); \
    }

#define DIRECTORY_FREE_MEMORY_AND_RESET(pMem) \
    if (pMem) { \
        DirectoryFreeMemory(pMem); \
        (pMem) = NULL; \
    }

DWORD
DirectoryOpen(
    PHANDLE phDirectory
    );

DWORD
DirectoryBind(
    HANDLE hDirectory,
    PWSTR  pwszDistinguishedName,
    PWSTR  pwszCredentials,
    ULONG  ulMethod
    );

DWORD
DirectoryAddObject(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD attributes[]
    );

DWORD
DirectoryModifyObject(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
DirectorySearch(
    HANDLE            hDirectory,
    PWSTR             pwszBase,
    ULONG             ulScope,
    PWSTR             pwszFilter,
    PWSTR             wszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
DirectoryDeleteObject(
    HANDLE hBindHandle,
    PWSTR  pwszObjectDN
    );

DWORD
DirectorySetPassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    );

DWORD
DirectoryVerifyPassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    );

DWORD
DirectoryGetGroupMembers(
    HANDLE            hDirectory,
    PWSTR             pwszGroupDN,
    PWSTR             pwszAttrs[],
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
DirectoryGetMemberships(
    HANDLE            hDirectory,
    PWSTR             pwszUserDN,
    PWSTR             pwszAttrs[],
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
DirectoryAddToGroup(
    HANDLE            hDirectory,
    PWSTR             pwszGroupDN,
    PDIRECTORY_ENTRY  pDirectoryEntries
    );

DWORD
DirectoryRemoveFromGroup(
    HANDLE            hDirectory,
    PWSTR             pwszGroupDN,
    PDIRECTORY_ENTRY  pDirectoryEntries
    );

DWORD
DirectoryGetUserCount(
    HANDLE hBindHandle,
    PDWORD pdwNumUsers
    );

DWORD
DirectoryGetGroupCount(
    HANDLE hBindHandle,
    PDWORD pdwNumGroups
    );

DWORD
DirectoryChangePassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszOldPassword,
    PWSTR  pwszNewPassword
    );

DWORD
DirectoryGetPasswordChangeInterval(
    HANDLE hBindHandle,
    PDWORD pdwInterval
    );

VOID
DirectoryClose(
    HANDLE hDirectory
    );

VOID
DirectoryFreeEntries(
    PDIRECTORY_ENTRY pEntries,
    DWORD            dwNumEntries
    );

VOID
DirectoryFreeAttributeValues(
    PATTRIBUTE_VALUE pAttrValues,
    DWORD            dwNumValues
    );

DWORD
DirectoryGetEntryAttributeSingle(
    PDIRECTORY_ENTRY pEntry,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    );

DWORD
DirectoryGetEntryAttributeByName(
    PDIRECTORY_ENTRY pEntry,
    PCWSTR pwszAttributeName,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    );

DWORD
DirectoryGetEntryAttributeByNameA(
    PDIRECTORY_ENTRY pEntry,
    PCSTR pszAttributeName,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    );

DWORD
DirectoryGetAttributeValue(
    PDIRECTORY_ATTRIBUTE pAttribute,
    PATTRIBUTE_VALUE *ppAttrValue
    );

DWORD
DirectoryGetEntryAttrValueByName(
    PDIRECTORY_ENTRY pEntry,
    PCWSTR pwszAttrName,
    DIRECTORY_ATTR_TYPE AttrType,
    void *pValue
    );

DWORD
DirectoryGetEntryAttrValueByNameA(
    PDIRECTORY_ENTRY pEntry,
    PCSTR pszAttrName,
    DIRECTORY_ATTR_TYPE AttrType,
    void *pValue
    );

DWORD
DirectoryAllocateMemory(
    size_t sSize,
    PVOID* ppMemory
    );

DWORD
DirectoryReallocMemory(
    PVOID  pMemory,
    PVOID* ppNewMemory,
    size_t sSize
    );

VOID
DirectoryFreeMemory(
    PVOID pMemory
    );

DWORD
DirectoryAllocateStringW(
    PWSTR  pwszInputString,
    PWSTR* ppwszOutputString
    );

DWORD
DirectoryAllocateString(
    PCSTR pszInputString,
    PSTR* ppszOutputString
    );

VOID
DirectoryFreeStringW(
    PWSTR pwszString
    );

VOID
DirectoryFreeString(
    PSTR pszString
    );

VOID
DirectoryFreeStringArray(
    PWSTR* ppStringArray,
    DWORD  dwCount
    );

#endif /* __DIRECTORY_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
