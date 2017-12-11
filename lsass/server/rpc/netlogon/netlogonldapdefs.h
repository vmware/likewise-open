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
 *        netlogonldapdefs.h
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      Provider macros and definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *          Adam Bernstein (abernstein@vmware.com)
 */

#ifndef __NETLOGON_LDAPDEFS_H__
#define __NETLOGON_LDAPDEFS_H__

#if 0

#define SAM_DB_DIR CACHEDIR   "/db"
#define SAM_DB     SAM_DB_DIR "/sam.db"

#define SAM_DB_CONTEXT_POOL_MAX_ENTRIES 10

#define SAM_DB_DEFAULT_ADMINISTRATOR_SHELL   "/bin/sh"
#define SAM_DB_DEFAULT_ADMINISTRATOR_HOMEDIR "/"

#define SAM_DB_DEFAULT_GUEST_SHELL           "/bin/sh"
#define SAM_DB_DEFAULT_GUEST_HOMEDIR         "/tmp"
#endif

#define NETLOGON_LDAP_LOG_ERROR(pszFormat, ...) LSA_LOG_ERROR(pszFormat, ## __VA_ARGS__)
#define NETLOGON_LDAP_LOG_WARNING(pszFormat, ...) LSA_LOG_WARNING(pszFormat, ## __VA_ARGS__)
#define NETLOGON_LDAP_LOG_INFO(pszFormat, ...) LSA_LOG_INFO(pszFormat, ## __VA_ARGS__)
#define NETLOGON_LDAP_LOG_VERBOSE(pszFormat, ...) LSA_LOG_VERBOSE(pszFormat, ## __VA_ARGS__)
#define NETLOGON_LDAP_LOG_DEBUG(pszFormat, ...) LSA_LOG_DEBUG(pszFormat, ## __VA_ARGS__)

#define BAIL_ON_NETLOGON_LDAP_ERROR(dwError) \
    BAIL_ON_LSA_ERROR((dwError))


typedef enum
{
    NETLOGON_LDAP_BIND_PROTOCOL_UNSET,
    NETLOGON_LDAP_BIND_PROTOCOL_KERBEROS,
    NETLOGON_LDAP_BIND_PROTOCOL_SRP,
    NETLOGON_LDAP_BIND_PROTOCOL_SPNEGO
} NETLOGON_LDAP_BIND_PROTOCOL;

#if 0

typedef enum
{
    NETLOGON_LDAP_ENTRY_TYPE_UNKNOWN = 0,
    NETLOGON_LDAP_ENTRY_TYPE_USER_OR_GROUP,
    NETLOGON_LDAP_ENTRY_TYPE_DOMAIN

} NETLOGON_LDAP_ENTRY_TYPE, *PNETLOGON_LDAP_ENTRY_TYPE;

#define ATTR_IS_MANDATORY     TRUE
#define ATTR_IS_NOT_MANDATORY FALSE
#define ATTR_IS_MUTABLE       TRUE
#define ATTR_IS_IMMUTABLE     FALSE

typedef enum
{
    NETLOGON_LDAP_DN_TOKEN_TYPE_UNKNOWN = 0,
    NETLOGON_LDAP_DN_TOKEN_TYPE_DC,
    NETLOGON_LDAP_DN_TOKEN_TYPE_CN,
    NETLOGON_LDAP_DN_TOKEN_TYPE_OU

} NETLOGON_LDAP_DN_TOKEN_TYPE;
#endif


#endif /* __NETLOGON_LDAPDEFS_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
