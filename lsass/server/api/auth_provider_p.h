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
 *        auth_provider_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Private header (Library)
 * 
 *        Authentication Provider Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __AUTH_PROVIDER_P_H__
#define __AUTH_PROVIDER_P_H__

#define LSA_CFG_TAG_AUTH_PROVIDER "auth provider:"

typedef struct __LSA_AUTH_PROVIDER
{
    PSTR pszId;
    PSTR pszProviderLibpath;
    PSTR pszName;
    PVOID pLibHandle;
    PFNSHUTDOWNPROVIDER pFnShutdown;
    PLSA_PROVIDER_FUNCTION_TABLE pFnTable;
    
    struct __LSA_AUTH_PROVIDER *pNext;
    
} LSA_AUTH_PROVIDER, *PLSA_AUTH_PROVIDER;

VOID
LsaSrvFreeAuthProvider(
    PLSA_AUTH_PROVIDER pProvider
    );

VOID
LsaSrvFreeAuthProviderStack(
    PLSA_STACK pProviderStack
    );

VOID
LsaSrvFreeAuthProviderList(
    PLSA_AUTH_PROVIDER pProviderList
    );

DWORD
LsaSrvValidateProvider(
    PLSA_AUTH_PROVIDER pProvider
    );

DWORD
LsaSrvInitAuthProvider(
    PCSTR pszConfigFilePath,
    PLSA_AUTH_PROVIDER pProvider
    );

DWORD
LsaSrvInitAuthProviders(
    PCSTR pszConfigFilePath
    );

DWORD
LsaCfgFreeAuthProviderInStack(
    PVOID pItem,
    PVOID pUserData
    );

DWORD
LsaSrvAuthProviderConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    );

DWORD
LsaSrvAuthProviderConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );

VOID
LsaSrvFreeAuthProviders(
    VOID
    );

DWORD
LsaSrvIsLocalDomain(
    PCSTR pszDomain,
    PBOOLEAN pbLocalDomain
    );

DWORD
LsaSrvCrackName(
    PCSTR pszId,
    PSTR* ppszDomain,
    PSTR* ppszName
    );

DWORD
LsaSrvCrackUserName(
    PCSTR pszLoginId,
    PSTR* ppszDomain,
    PSTR* ppszUserName
    );

DWORD
LsaSrvCrackGroupName(
    PCSTR pszGroup,
    PSTR* ppszDomain,
    PSTR* ppszGroupName
    );

DWORD
LsaGetNumberOfProviders_inlock(
    VOID
    );

#endif /* __AUTH_PROVIDER_P_H__ */
