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
 * Module Name:
 *
 *        api2.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Client API (version 2)
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 */

#include "client.h"

DWORD
LsaFindObjects(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    return LsaTransactFindObjects(
        hLsa,
        pszTargetProvider,
        FindFlags,
        ObjectType,
        QueryType,
        dwCount,
        QueryList,
        pppObjects);
}

DWORD
LsaOpenEnumObjects(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    return LsaTransactOpenEnumObjects(
        hLsa,
        pszTargetProvider,
        phEnum,
        FindFlags,
        ObjectType,
        pszDomainName);
}

DWORD
LsaEnumObjects(
    IN HANDLE hLsa,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    return LsaTransactEnumObjects(
        hLsa,
        hEnum,
        dwMaxObjectsCount,
        pdwObjectsCount,
        pppObjects);
}

DWORD
LsaOpenEnumMembers(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid
    )
{
    return LsaTransactOpenEnumMembers(
        hLsa,
        pszTargetProvider,
        phEnum,
        FindFlags,
        pszSid);
}

DWORD
LsaEnumMembers(
    IN HANDLE hLsa,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PSTR** pppszMember
    )
{
    return LsaTransactEnumMembers(
        hLsa,
        hEnum,
        dwMaxObjectsCount,
        pdwObjectsCount,
        pppszMember);
}

DWORD
LsaQueryMemberOf(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    )
{
    return LsaTransactQueryMemberOf(
        hLsa,
        pszTargetProvider,
        FindFlags,
        dwSidCount,
        ppszSids,
        pdwGroupSidCount,
        pppszGroupSids);
}

DWORD
LsaCloseEnum(
    IN HANDLE hLsa,
    IN OUT HANDLE hEnum
    )
{
    return LsaTransactCloseEnum(
        hLsa,
        hEnum);
}

VOID
LsaFreeSidList(
    IN DWORD dwSidCount,
    IN OUT PSTR* ppszSids
    )
{
    LwFreeStringArray(ppszSids, dwSidCount);
}

VOID
LsaFreeSecurityObjectList(
    IN DWORD dwObjectCount,
    IN OUT PLSA_SECURITY_OBJECT* ppObjects
    )
{
    LsaUtilFreeSecurityObjectList(dwObjectCount, ppObjects);
}
