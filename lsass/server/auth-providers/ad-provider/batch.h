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
 *        batch.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Wei Fu (wfu@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 */
#ifndef __BATCH_H__
#define __BATCH_H__

DWORD
ADLdap_FindObjectsByDNListBatched(
    IN HANDLE hProvider,
    IN DWORD dwCount,
    IN PSTR* ppszDnList,
    OUT PDWORD pdwCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    );

DWORD
ADLdap_FindObjectsBySidListBatched(
    IN HANDLE hProvider,
    IN DWORD dwCount,
    IN PSTR* ppszSidList,
    OUT PDWORD pdwCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    );

DWORD
ADLdap_FindObjectsByNameListBatched(
    IN HANDLE hProvider,
    IN DWORD dwCount,
    IN PSTR* ppszNameList,
    OUT PDWORD pdwCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    );

DWORD
ADLdap_FindObjectsByUidListBatched(
    IN HANDLE hProvider,
    IN DWORD dwCount,
    IN PDWORD pdwId,
    OUT PDWORD pdwCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    );

DWORD
ADLdap_FindObjectsByGidListBatched(
    IN HANDLE hProvider,
    IN DWORD dwCount,
    IN PDWORD pdwId,
    OUT PDWORD pdwCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    );

DWORD
ADLdap_FindObjectsByUserAliasListBatched(
    IN HANDLE hProvider,
    IN DWORD dwCount,
    IN PSTR* ppszUserAliasList,
    OUT PDWORD pdwCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    );

DWORD
ADLdap_FindObjectsByGroupAliasListBatched(
    IN HANDLE hProvider,
    IN DWORD dwCount,
    IN PSTR* ppszGroupAliasList,
    OUT PDWORD pdwCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    );

typedef UINT8 LSA_AD_BATCH_QUERY_TYPE, *PLSA_AD_BATCH_QUERY_TYPE;
#define LSA_AD_BATCH_QUERY_TYPE_UNDEFINED      0
#define LSA_AD_BATCH_QUERY_TYPE_BY_DN          1
#define LSA_AD_BATCH_QUERY_TYPE_BY_SID         2
#define LSA_AD_BATCH_QUERY_TYPE_BY_NT4         3
#define LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS  4
#define LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS 5
#define LSA_AD_BATCH_QUERY_TYPE_BY_UID         6
#define LSA_AD_BATCH_QUERY_TYPE_BY_GID         7

DWORD
LsaAdBatchFindObjects(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwQueryItemsCount,
    IN OPTIONAL PSTR* ppszQueryList,
    IN OPTIONAL PDWORD pdwId,
    OUT PDWORD pdwObjectsCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    );

DWORD
LsaAdBatchFindSingleObject(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN OPTIONAL PCSTR pszQueryTerm,
    IN OPTIONAL PDWORD dwId,
    OUT PAD_SECURITY_OBJECT* ppObject
    );

#endif /* __BATCH_H__ */

