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
 *        lsa2.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Public Client API (version 2)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 */

#ifndef __LSA2_H__
#define __LSA2_H__

#include <inttypes.h>
#include <lsa/lsa.h>

typedef struct __LSA_SECURITY_OBJECT_VERSION_INFO
{
    // This value is set to -1 if the value is not stored in the
    // database (it only exists in memory). Otherwise, this is an index into
    // the database.
    int64_t qwDbId;
    time_t tLastUpdated;
    // Sum of the size of all objects that use this version info (only used by
    // memory backend)
    DWORD dwObjectSize;
    // Importance of this object (for internal use by the memory backend)
    float fWeight;
} LSA_SECURITY_OBJECT_VERSION_INFO, *PLSA_SECURITY_OBJECT_VERSION_INFO;

typedef struct _LSA_SECURITY_OBJECT_USER_INFO
{
    uid_t uid;
    gid_t gid;
    PSTR pszUPN;
    PSTR pszAliasName;
    PSTR pszPasswd;
    PSTR pszGecos;
    PSTR pszShell;
    PSTR pszHomedir;
    uint64_t qwPwdLastSet;
    uint64_t qwAccountExpires;

    BOOLEAN bIsGeneratedUPN;
    BOOLEAN bIsAccountInfoKnown;
    // Calculated from userAccountControl, accountExpires, and pwdLastSet
    // attributes from AD.
    BOOLEAN bPasswordExpired;
    BOOLEAN bPasswordNeverExpires;
    BOOLEAN bPromptPasswordChange;
    BOOLEAN bUserCanChangePassword;
    BOOLEAN bAccountDisabled;
    BOOLEAN bAccountExpired;
    BOOLEAN bAccountLocked;
} LSA_SECURITY_OBJECT_USER_INFO, *PLSA_SECURITY_OBJECT_USER_INFO;

typedef struct _LSA_SECURITY_OBJECT_GROUP_INFO
{
    gid_t gid;
    PSTR pszAliasName;
    PSTR pszPasswd;
} LSA_SECURITY_OBJECT_GROUP_INFO, *PLSA_SECURITY_OBJECT_GROUP_INFO;

typedef struct __LSA_DB_SECURITY_OBJECT
{
    LSA_SECURITY_OBJECT_VERSION_INFO version;
    PSTR    pszDN;
    // The object SID is stored in printed form
    PSTR    pszObjectSid;
    //This is false if the object has not been enabled in the cell
    BOOLEAN enabled;

    PSTR    pszNetbiosDomainName;
    PSTR    pszSamAccountName;

    ADAccountType type;

    // These fields are only set if the object is enabled base on the type.
    union
    {
        LSA_SECURITY_OBJECT_USER_INFO userInfo;
        LSA_SECURITY_OBJECT_GROUP_INFO groupInfo;
        union
        {
            LSA_SECURITY_OBJECT_USER_INFO userInfo;
            LSA_SECURITY_OBJECT_GROUP_INFO groupInfo;
        } typeInfo;
    };
} LSA_SECURITY_OBJECT, *PLSA_SECURITY_OBJECT;

typedef const LSA_SECURITY_OBJECT * PCLSA_SECURITY_OBJECT;

#endif
