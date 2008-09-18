/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        lsamacclient.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Public Client API (For Mac OS X)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LSAMACCLIENT_H__
#define __LSAMACCLIENT_H__

#ifndef _WIN32

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifndef DWORD_DEFINED
#define DWORD_DEFINED 1

typedef uint32_t        DWORD, *PDWORD;

#endif

#ifndef INT_DEFINED
#define INT_DEFINED 1

typedef int             INT, *PINT;

#endif

#ifndef UINT64_DEFINED
#define UINT64_DEFINED 1

typedef uint64_t        UINT64, *PUINT64;

#endif

#ifndef UINT32_DEFINED
#define UINT32_DEFINED 1

typedef uint32_t        UINT32, *PUINT32;

#endif

#ifndef UINT16_DEFINED
#define UINT16_DEFINED 1

typedef uint16_t        UINT16, *PUINT16;

#endif

#ifndef WORD_DEFINED
#define WORD_DEFINED 1

typedef uint16_t WORD, *PWORD;

#endif

#ifndef USHORT_DEFINED
#define USHORT_DEFINED 1

typedef unsigned short  USHORT, *PUSHORT;

#endif

#ifndef ULONG_DEFINED
#define ULONG_DEFINED 1

typedef unsigned long   ULONG, *PULONG;

#endif

#ifndef ULONGLONG_DEFINED
#define ULONGLONG_DEFINED 1

typedef unsigned long long ULONGLONG, *PULONGLONG;

#endif

#ifndef UINT8_DEFINED
#define UINT8_DEFINED 1

typedef uint8_t         UINT8, *PUINT8;

#endif

#ifndef BYTE_DEFINED
#define BYTE_DEFINED

typedef uint8_t BYTE, *PBYTE;

#endif

#ifndef UCHAR_DEFINED
#define UCHAR_DEFINED 1

typedef uint8_t UCHAR, *PUCHAR;

#endif

#ifndef HANDLE_DEFINED
#define HANDLE_DEFINED 1

typedef unsigned long   HANDLE, *PHANDLE;

#endif

#ifndef CHAR_DEFINED
#define CHAR_DEFINED 1

typedef char            CHAR;

#endif

#ifndef PSTR_DEFINED
#define PSTR_DEFINED 1

typedef char *          PSTR;

#endif

#ifndef PCSTR_DEFINED
#define PCSTR_DEFINED 1

typedef const char *    PCSTR;

#endif

#ifndef VOID_DEFINED
#define VOID_DEFINED 1

typedef void            VOID, *PVOID;

#endif

#ifndef PCVOID_DEFINED
#define PCVOID_DEFINED 1

typedef const void      *PCVOID;

#endif

#ifndef BOOLEAN_DEFINED
#define BOOLEAN_DEFINED 1

typedef int             BOOLEAN, *PBOOLEAN;

#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif

typedef enum
{
    AccountType_NotFound = 0,
    AccountType_Group = 1,
    AccountType_User = 2,
} ADAccountType;

typedef struct __LSA_USER_INFO_0
{
    uid_t uid;
    gid_t gid;
    PSTR  pszName;
    PSTR  pszPasswd;
    PSTR  pszGecos;
    PSTR  pszShell;
    PSTR  pszHomedir;
    PSTR  pszSid;
} LSA_USER_INFO_0, *PLSA_USER_INFO_0;

typedef struct __LSA_USER_INFO_1
{
    union
    {
        struct
        {
            uid_t uid;
            gid_t gid;
            PSTR  pszName;
            PSTR  pszPasswd;
            PSTR  pszGecos;
            PSTR  pszShell;
            PSTR  pszHomedir;
            PSTR  pszSid;
        };
        LSA_USER_INFO_0 info0;
    };
    PSTR  pszUPN;
    DWORD bIsGeneratedUPN;
    DWORD bIsLocalUser;
    PBYTE pLMHash;
    DWORD dwLMHashLen;
    PBYTE pNTHash;
    DWORD dwNTHashLen;
} LSA_USER_INFO_1, *PLSA_USER_INFO_1;

typedef struct __LSA_USER_INFO_2
{
    union
    {
        struct
        {
            uid_t   uid;
            gid_t   gid;
            PSTR    pszName;
            PSTR    pszPasswd;
            PSTR    pszGecos;
            PSTR    pszShell;
            PSTR    pszHomedir;
            PSTR    pszSid;
            PSTR    pszUPN;
            DWORD   bIsGeneratedUPN;
            DWORD   bIsLocalUser;
            PBYTE   pLMHash;
            DWORD   dwLMHashLen;
            PBYTE   pNTHash;
            DWORD   dwNTHashLen;
        };
        LSA_USER_INFO_1 info1;
    };
    DWORD   dwDaysToPasswordExpiry;
    BOOLEAN bPasswordExpired;
    BOOLEAN bPasswordNeverExpires;
    BOOLEAN bPromptPasswordChange;
    BOOLEAN bUserCanChangePassword;
    BOOLEAN bAccountDisabled;
    BOOLEAN bAccountExpired;
    BOOLEAN bAccountLocked;
} LSA_USER_INFO_2, *PLSA_USER_INFO_2;

typedef struct __LSA_GROUP_INFO_0
{
    gid_t gid;
    PSTR  pszName;
    PSTR  pszSid;
} LSA_GROUP_INFO_0, *PLSA_GROUP_INFO_0;

typedef struct __LSA_GROUP_INFO_1
{
    union
    {
        struct
        {
            gid_t gid;
            PSTR  pszName;
            PSTR  pszSid;
        };
        LSA_GROUP_INFO_0 info0;
    };
    PSTR  pszPasswd;
    PSTR* ppszMembers;
} LSA_GROUP_INFO_1, *PLSA_GROUP_INFO_1;

typedef struct __LSA_SID_INFO
{
    ADAccountType accountType;
    PSTR          pszSamAccountName;
    PSTR          pszDomainName;
} LSA_SID_INFO, *PLSA_SID_INFO;

typedef DWORD (*PFN_LSA_OPEN_SERVER)(
                PHANDLE phLsaConnection
                );

typedef DWORD (*PFN_LSA_GET_GROUPS_FOR_USERNAME)(
                HANDLE  hLsaConnection,
                PCSTR   pszUserName,    
                PDWORD  pdwGroupFound,
                gid_t** ppGidResults
                );

typedef DWORD (*PFN_LSA_FIND_GROUP_BY_NAME)(
                HANDLE hLsaConnection,
                PCSTR  pszGroupName,
                DWORD  dwGroupInfoLevel,
                PVOID* ppGroupInfo
                );

typedef DWORD (*PFN_LSA_FIND_GROUP_BY_ID)(
                HANDLE hLsaConnection,
                gid_t  gid,
                DWORD  dwGroupInfoLevel,
                PVOID* ppGroupInfo
                );

typedef DWORD (*PFN_LSA_BEGIN_ENUM_GROUPS)(
                HANDLE  hLsaConnection,
                DWORD   dwGroupInfoLevel,
                DWORD   dwMaxNumGroups,
                PHANDLE phResume
                );

typedef DWORD (*PFN_LSA_ENUM_GROUPS)(
                HANDLE  hLsaConnection,
                HANDLE  hResume,
                PDWORD  pdwNumGroupsFound,
                PVOID** pppGroupsInfoList
                );

typedef DWORD (*PFN_LSA_END_ENUM_GROUPS)(
                HANDLE  hLsaConnection,
                HANDLE  hResume
                );

typedef DWORD (*PFN_LSA_FIND_USER_BY_NAME)(
                HANDLE hLsaConnection,
                PCSTR  pszName,
                DWORD  dwUserInfoLevel,
                PVOID* ppUserInfo
                );

typedef DWORD (*PFN_LSA_FIND_USER_BY_ID)(
                HANDLE hLsaConnection,
                uid_t  uid,
                DWORD  dwUserInfoLevel,
                PVOID* ppUserInfo
                );

typedef DWORD (*PFN_LSA_GET_NAMES_BY_SID_LIST)(
                HANDLE          hLsaConnection,
                size_t          sCount,
                PSTR*           ppszSidList,
                PLSA_SID_INFO*  ppSIDInfoList
                );

typedef DWORD (*PFN_LSA_BEGIN_ENUM_USERS)(
                HANDLE  hLsaConnection,
                DWORD   dwUserInfoLevel,
                DWORD   dwMaxNumUsers,
                PHANDLE phResume
                );

typedef DWORD (*PFN_LSA_ENUM_USERS)(
                HANDLE  hLsaConnection,
                HANDLE  hResume,
                PDWORD  pdwNumUsersFound,
                PVOID** pppUserInfoList
                );

typedef DWORD (*PFN_LSA_END_ENUM_USERS)(
                HANDLE hLsaConnection,
                HANDLE hResume
                );

typedef DWORD (*PFN_LSA_AUTHENTICATE_USER)(
                HANDLE hLsaConnection,
                PCSTR  pszLoginName,
                PCSTR  pszPassword
                );

typedef DWORD (*PFN_LSA_VALIDATE_USER)(
                HANDLE hLsaConnection,
                PCSTR  pszLoginName,
                PCSTR  pszPassword
                );

typedef DWORD (*PFN_LSA_CHANGE_PASSWORD)(
                HANDLE hLsaConnection,
                PCSTR  pszLoginName,
                PCSTR  pszNewPassword,
                PCSTR  pszOldPassword
                );

typedef DWORD (*PFN_LSA_OPEN_SESSION)(
                HANDLE hLsaConnection,
                PCSTR  pszLoginId
                );

typedef DWORD (*PFN_LSA_CLOSE_SESSION)(
                HANDLE hLsaConnection,
                PCSTR  pszLoginId
                );

typedef VOID (*PFN_LSA_FREE_GROUP_INFO_LIST)(
                DWORD  dwLevel,
                PVOID* pGroupInfoList,
                DWORD  dwNumGroups
                );

typedef VOID (*PFN_LSA_FREE_GROUP_INFO)(
                DWORD dwLevel,
                PVOID pGroupInfo
                );

typedef VOID (*PFN_LSA_FREE_USER_INFO_LIST)(
                DWORD  dwLevel,
                PVOID* pUserInfoList,
                DWORD  dwNumUsers
                );

typedef VOID (*PFN_LSA_FREE_USER_INFO)(
                DWORD dwLevel,
                PVOID pUserInfo
                );

typedef VOID (*PFN_LSA_FREE_SID_INFO_LIST)(
                PLSA_SID_INFO  ppSIDInfoList,
                size_t         stNumSID
                );

typedef VOID (*PFN_LSA_FREE_SID_INFO)(
                PLSA_SID_INFO pSIDInfo
                );

typedef DWORD (*PFN_LSA_CLOSE_SERVER)(
                HANDLE hLsaConnection
                );

typedef struct __LSAMACCLIENT_FUNC_TABLE
{
    PFN_LSA_OPEN_SERVER              pfnLsaOpenServer;
    
    /* Groups */
    PFN_LSA_GET_GROUPS_FOR_USERNAME  pfnLsaGetGroupsForUsername;
    PFN_LSA_FIND_GROUP_BY_NAME       pfnLsaFindGroupByName;
    PFN_LSA_FIND_GROUP_BY_ID         pfnLsaFindGroupById;
    PFN_LSA_BEGIN_ENUM_GROUPS        pfnLsaBeginEnumGroups;
    PFN_LSA_ENUM_GROUPS              pfnLsaEnumGroups;
    PFN_LSA_END_ENUM_GROUPS          pfnLsaEndEnumGroups;

    /* Users */
    PFN_LSA_FIND_USER_BY_NAME        pfnLsaFindUserByName;
    PFN_LSA_FIND_USER_BY_ID          pfnLsaFindUserById;
    PFN_LSA_GET_NAMES_BY_SID_LIST    pfnLsaGetNamesBySIDList;
    PFN_LSA_BEGIN_ENUM_USERS         pfnLsaBeginEnumUsers;
    PFN_LSA_ENUM_USERS               pfnLsaEnumUsers;
    PFN_LSA_END_ENUM_USERS           pfnLsaEndEnumUsers;

    /* Authentication */
    PFN_LSA_AUTHENTICATE_USER        pfnLsaAuthenticateUser;
    PFN_LSA_VALIDATE_USER            pfnLsaValidateUser;
    PFN_LSA_CHANGE_PASSWORD          pfnLsaChangePassword;
    PFN_LSA_OPEN_SESSION             pfnLsaOpenSession;
    PFN_LSA_CLOSE_USER_LOGON_SESSION pfnLsaCloseUserLogonSession;
    
    /* Memory management */
    PFN_LSA_FREE_GROUP_INFO_LIST     pfnLsaFreeGroupInfoList;
    PFN_LSA_FREE_GROUP_INFO          pfnLsaFreeGroupInfo;
    PFN_LSA_FREE_USER_INFO_LIST      pfnLsaFreeUserInfoList;
    PFN_LSA_FREE_USER_INFO           pfnLsaFreeUserInfo;
    PFN_LSA_FREE_SID_INFO_LIST       pfnLsaFreeSIDInfoList;
    PFN_LSA_FREE_SID_INFO            pfnLsaFreeSIDInfo;

    PFN_LSA_CLOSE_SERVER             pfnLsaCloseServer;
    
} LSAMACCLIENT_FUNC_TABLE, *PLSAMACCLIENT_FUNC_TABLE;

typedef DWORD (*PFN_LSACLIENTMACINITIALIZE)(
    PLSAMACCLIENT_FUNC_TABLE* ppFnTable
    );

#ifndef LSA_CLIENT_MAC_INITIALIZE
#define LSA_CLIENT_MAC_INITIALIZE "LsaClientMacInitialize"
#endif

typedef DWORD (*PFN_LSACLIENTMACSHUTDOWN)(
    PLSAMACCLIENT_FUNC_TABLE pFnTable
    );

#ifndef LSA_CLIENT_MAC_SHUTDOWN
#define LSA_CLIENT_MAC_SHUTDOWN "LsaClientMacShutdown"
#endif

#endif /* __LSAMACCLIENT_H__ */
