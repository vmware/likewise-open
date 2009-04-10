/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        lpdefs.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider (Defines)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LPDEFS_H__
#define __LPDEFS_H__

#define LOCAL_CFG_TAG_LOCAL_PROVIDER "lsa-local-provider"
#define LOCAL_CFG_TAG_AUTH_PROVIDER  "auth provider"

#define LOCAL_PASSWD_CHANGE_INTERVAL_MINIMUM LSA_SECONDS_IN_DAY
#define LOCAL_PASSWD_CHANGE_INTERVAL_DEFAULT (30 * LSA_SECONDS_IN_DAY)
#define LOCAL_PASSWD_CHANGE_INTERVAL_MAXIMUM (180 * LSA_SECONDS_IN_DAY)

#define LOCAL_PASSWD_CHANGE_WARNING_TIME_MINIMUM LSA_SECONDS_IN_HOUR
#define LOCAL_PASSWD_CHANGE_WARNING_TIME_DEFAULT (14 * LSA_SECONDS_IN_DAY)
#define LOCAL_PASSWD_CHANGE_WARNING_TIME_MAXIMUM (30 * LSA_SECONDS_IN_DAY)

#define LOCAL_LOCK_MUTEX(bInLock, pMutex)  \
        if (!bInLock) {                    \
           pthread_mutex_lock(pMutex);     \
           bInLock = TRUE;                 \
        }

#define LOCAL_UNLOCK_MUTEX(bInLock, pMutex) \
        if (bInLock) {                      \
           pthread_mutex_unlock(pMutex);    \
           bInLock = FALSE;                 \
        }

#define LOCAL_OBJECT_CLASS_DOMAIN          1
#define LOCAL_OBJECT_CLASS_GROUP           4
#define LOCAL_OBJECT_CLASS_USER            5

#define LOCAL_DB_DIR_ATTR_OBJECT_CLASS         "ObjectClass"
#define LOCAL_DB_DIR_ATTR_OBJECT_SID           "ObjectSID"
#define LOCAL_DB_DIR_ATTR_DOMAIN               "Domain"
#define LOCAL_DB_DIR_ATTR_NETBIOS_NAME         "NetBIOSName"
#define LOCAL_DB_DIR_ATTR_COMMON_NAME          "CommonName"
#define LOCAL_DB_DIR_ATTR_SAM_ACCOUNT_NAME     "SamAccountName"
#define LOCAL_DB_DIR_ATTR_USER_PRINCIPAL_NAME  "UserPrincipalName"
#define LOCAL_DB_DIR_ATTR_UID                  "UID"
#define LOCAL_DB_DIR_ATTR_GID                  "GID"
#define LOCAL_DB_DIR_ATTR_PASSWORD             "Password"
#define LOCAL_DB_DIR_ATTR_USER_INFO_FLAGS      "UserInfoFlags"
#define LOCAL_DB_DIR_ATTR_PASSWORD_CHANGE_TIME "PasswdChangeTime"
#define LOCAL_DB_DIR_ATTR_FULL_NAME            "FullName"
#define LOCAL_DB_DIR_ATTR_ACCOUNT_EXPIRY       "AccountExpiry"
#define LOCAL_DB_DIR_ATTR_GECOS                "Gecos"
#define LOCAL_DB_DIR_ATTR_HOME_DIR             "Homedir"
#define LOCAL_DB_DIR_ATTR_SHELL                "LoginShell"

#define LOCAL_DIR_ATTR_OBJECT_CLASS  \
    {'O','b','j','e','c','t','C','l','a','s','s',0}
#define LOCAL_DIR_ATTR_OBJECT_SID \
    {'O','b','j','e','c','t','S','I','D',0}
#define LOCAL_DIR_ATTR_DOMAIN \
    {'D','o','m','a','i','n',0}
#define LOCAL_DIR_ATTR_NETBIOS_NAME \
    {'N','e','t','B','I','O','S','N','a','m','e',0}
#define LOCAL_DIR_ATTR_COMMON_NAME \
    {'C','o','m','m','o','n','N','a','m','e',0}
#define LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME \
    {'S','a','m','A','c','c','o','u','n','t','N','a','m','e',0}
#define LOCAL_DIR_ATTR_USER_PRINCIPAL_NAME \
    {'U','s','e','r','P','r','i','n','c','i','p','a','l','N','a','m','e',0}
#define LOCAL_DIR_ATTR_UID \
    {'U','I','D',0}
#define LOCAL_DIR_ATTR_GID \
    {'G','I','D',0}
#define LOCAL_DIR_ATTR_PASSWORD \
    {'P','a','s','s','w','o','r','d',0}
#define LOCAL_DIR_ATTR_USER_INFO_FLAGS \
    {'U','s','e','r','I','n','f','o','F','l','a','g','s',0}
#define LOCAL_DIR_ATTR_PASSWORD_CHANGE_TIME \
    {'P','a','s','s','w','d','C','h','a','n','g','e','T','i','m','e',0}
#define LOCAL_DIR_ATTR_FULL_NAME \
    {'F','u','l','l','N','a','m','e',0}
#define LOCAL_DIR_ATTR_ACCOUNT_EXPIRY \
    {'A','c','c','o','u','n','t','E','x','p','i','r','y',0}
#define LOCAL_DIR_ATTR_GECOS \
    {'G','e','c','o','s',0}
#define LOCAL_DIR_ATTR_HOME_DIR \
    {'H','o','m','e','d','i','r',0}
#define LOCAL_DIR_ATTR_SHELL \
    {'L','o','g','i','n','S','h','e','l','l',0}
#define LOCAL_DIR_CN_PREFIX \
    {'C','N','=',0}
#define LOCAL_DIR_OU_PREFIX \
    {'O','U','=',0}
#define LOCAL_DIR_DC_PREFIX \
    {'D','C','=',0}

typedef DWORD LOCAL_ACCESS_FLAG;

#define LOCAL_ACCESS_FLAG_ALLOW_NONE   0x00000000
#define LOCAL_ACCESS_FLAG_ALLOW_QUERY  0x00000001
#define LOCAL_ACCESS_FLAG_ALLOW_ADD    0x00000002
#define LOCAL_ACCESS_FLAG_ALLOW_DELETE 0x00000004
#define LOCAL_ACCESS_FLAG_ALLOW_MODIFY 0x00000008


#endif /* __LPDEFS_H__ */

