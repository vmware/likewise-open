/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        lsaadprovider.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Client API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __LSAADPROVIDER_H__
#define __LSAADPROVIDER_H__

#include "lsautils.h"

#define LSA_AD_TAG_PROVIDER "lsa-activedirectory-provider"

#define LSA_AD_IO_EMPTYCACHE             1
#define LSA_AD_IO_REMOVEUSERBYNAMECACHE  2
#define LSA_AD_IO_REMOVEUSERBYIDCACHE    3
#define LSA_AD_IO_REMOVEGROUPBYNAMECACHE 4
#define LSA_AD_IO_REMOVEGROUPBYIDCACHE   5
#define LSA_AD_IO_ENUMUSERSCACHE         6
#define LSA_AD_IO_ENUMGROUPSCACHE        7

typedef struct __LSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ {
    PCSTR pszResume;
    DWORD dwInfoLevel;
    DWORD dwMaxNumUsers;
} LSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ, *PLSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ;

typedef struct __LSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP {
    PSTR pszResume;
    PLSA_USER_INFO_LIST pUserInfoList;
} LSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP, *PLSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP;

typedef struct __LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ {
    PCSTR pszResume;
    DWORD dwInfoLevel;
    DWORD dwMaxNumGroups;
} LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ, *PLSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ;

typedef struct __LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP {
    PSTR pszResume;
    PLSA_GROUP_INFO_LIST pGroupInfoList;
} LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP, *PLSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP;

LWMsgTypeSpec*
LsaAdIPCGetEnumUsersFromCacheReqSpec(
    void
    );

LWMsgTypeSpec*
LsaAdIPCGetEnumUsersFromCacheRespSpec(
    void
    );

LWMsgTypeSpec*
LsaAdIPCGetEnumGroupsFromCacheReqSpec(
    void
    );

LWMsgTypeSpec*
LsaAdIPCGetEnumGroupsFromCacheRespSpec(
    void
    );

#endif /* __LSAADPROVIDER_H__ */
