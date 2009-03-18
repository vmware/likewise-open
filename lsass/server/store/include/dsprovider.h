/*
 * dsprovider.h
 *
 *  Created on: Mar 9, 2009
 *      Author: krishnag
 */

#ifndef DSPROVIDER_H_
#define DSPROVIDER_H_

typedef struct _OCTET_STRING {
    ULONG ulNumBytes;
    PBYTE pBytes;
} OCTET_STRING, *POCTET_STRING;

typedef struct _ATTRIBUTE_VALUE {
    ULONG Type;
    union {
        ULONG uLongValue;
        PWSTR pwszStringValue;
        BOOL  bBooleanValue;
        POCTET_STRING pOctetString;
    };
} ATTRIBUTE_VALUE, *PATTRIBUTE_VALUE;

typedef struct _DIRECTORY_ATTRIBUTE {
    PWSTR pwszAttributeName;
    ULONG ulNumValues;
    PATTRIBUTE_VALUE * ppAttributeValues;
} DIRECTORY_ATTRIBUTE, *PDIRECTORY_ATTRIBUTE;

typedef ULONG DIR_MOD_FLAGS;

#define DIR_MOD_FLAGS_ADD     0x0
#define DIR_MOD_FLAGS_REPLACE 0x1
#define DIR_MOD_FLAGS_DELETE  0x2

typedef struct _DIRECTORY_MOD {
    DIR_MOD_FLAGS    ulOperationFlags;
    PWSTR            pwszAttributeName;
    ULONG            ulNumValues;
    PATTRIBUTE_VALUE pAttributeValues;
} DIRECTORY_MOD, *PDIRECTORY_MOD;

typedef struct _DIRECTORY_ENTRY{
    ULONG ulNumAttributes;
    PDIRECTORY_ATTRIBUTE * ppDirectoryAttributes;
} DIRECTORY_ENTRY, *PDIRECTORY_ENTRY;

typedef DWORD (*PFNDIRECTORYOPEN)(
                    PHANDLE phDirectory
                    );

typedef DWORD (*PFNDIRECTORYBIND)(
                    HANDLE hDirectory,
                    PWSTR  pwszDistinguishedName,
                    PWSTR  pwszCredential,
                    ULONG  ulMethod
                    );

typedef DWORD (*PFNDIRECTORYADD)(
                    HANDLE hDirectory,
                    PWSTR  pwszObjectDN,
                    DIRECTORY_MOD Attributes[]
                    );

typedef DWORD (*PFNDIRECTORYMODIFY)(
                    HANDLE hDirectory,
                    PWSTR  pwszObjectDN,
                    DIRECTORY_MOD Modifications[]
                    );

typedef DWORD (*PFNDIRECTORYSEARCH)(
                    HANDLE hDirectory,
                    PWSTR  pwszBase,
                    ULONG  ulScope,
                    PWSTR  pwszFilter,
                    PWSTR  wszAttributes[],
                    ULONG  ulAttributesOnly,
                    PATTRIBUTE_VALUE * ppDirectoryValues,
                    PDWORD pdwNumValues
                    );

typedef DWORD (*PFNDIRECTORYDELETE)(
                    HANDLE hDirectory,
                    PWSTR  pwszObjectDN
                    );

typedef DWORD (*PFNDIRECTORYCLOSE)(
                    HANDLE hDirectory
                    );

typedef struct __LSA_DIRECTORY_PROVIDER_FUNCTION_TABLE
{
    PFNDIRECTORYOPEN   pfnDirectoryOpen;
    PFNDIRECTORYBIND   pfnDirectoryBind;
    PFNDIRECTORYADD    pfnDirectoryAdd;
    PFNDIRECTORYMODIFY pfnDirectoryModify;
    PFNDIRECTORYDELETE pfnDirectoryDelete;
    PFNDIRECTORYSEARCH pfnDirectorySearch;
    PFNDIRECTORYCLOSE  pfnDirectoryClose;

} DIRECTORY_PROVIDER_FUNCTION_TABLE, *PDIRECTORY_PROVIDER_FUNCTION_TABLE;

#define DIRECTORY_SYMBOL_NAME_INITIALIZE_PROVIDER "DirectoryInitializeProvider"

typedef DWORD (*PFNINITIALIZEDIRPROVIDER)(
                    PCSTR pszConfigFilePath,
                    PSTR* ppszProviderName,
                    PDIRECTORY_PROVIDER_FUNCTION_TABLE* ppFnTable
                    );

#define DIRECTORY_SYMBOL_NAME_SHUTDOWN_PROVIDER "DirectoryShutdownProvider"

typedef DWORD (*PFNSHUTDOWNDIRPROVIDER)(
                    PSTR pszProviderName,
                    PDIRECTORY_PROVIDER_FUNCTION_TABLE pFnTable
                    );

#endif /* DSPROVIDER_H_ */
