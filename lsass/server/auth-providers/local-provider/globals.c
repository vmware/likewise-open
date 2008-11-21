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
 *        globals.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Global Variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "localprovider.h"

PCSTR gpszLocalProviderName = "lsa-local-provider";

PSTR gpszConfigFilePath = NULL;

LSA_PROVIDER_FUNCTION_TABLE gLocalProviderAPITable =
{
    &LsaProviderLocal_OpenHandle,
    &LsaProviderLocal_CloseHandle,
    &LsaProviderLocal_ServicesDomain,
    &LsaProviderLocal_AuthenticateUser,
    &LsaProviderLocal_ValidateUser,
    &LsaProviderLocal_CheckUserInList,
    &LsaProviderLocal_FindUserByName,
    &LsaProviderLocal_FindUserById,
    &LsaProviderLocal_BeginEnumUsers,
    &LsaProviderLocal_EnumUsers,
    &LsaProviderLocal_EndEnumUsers,
    &LsaProviderLocal_FindGroupByName,
    &LsaProviderLocal_FindGroupById,
    &LsaProviderLocal_GetGroupsForUser,
    &LsaProviderLocal_BeginEnumGroups,
    &LsaProviderLocal_EnumGroups,
    &LsaProviderLocal_EndEnumGroups,
    &LsaProviderLocal_ChangePassword,
    &LsaProviderLocal_AddUser,
    &LsaProviderLocal_ModifyUser,
    &LsaProviderLocal_DeleteUser,
    &LsaProviderLocal_AddGroup,
    &LsaProviderLocal_DeleteGroup,
    &LsaProviderLocal_OpenSession,
    &LsaProviderLocal_CloseSession,
    &LsaProviderLocal_GetNamesBySidList,
    &LsaProviderLocal_FindNSSArtefactByKey,
    &LsaProviderLocal_BeginEnumNSSArtefacts,
    &LsaProviderLocal_EnumNSSArtefacts,
    &LsaProviderLocal_EndEnumNSSArtefacts,
    &LsaProviderLocal_GetStatus,
    &LsaProviderLocal_FreeStatus,
    &LsaProviderLocal_RefreshConfiguration
};

pthread_rwlock_t g_dbLock;

pthread_rwlock_t gProviderLocalGlobalDataLock;

LOCAL_CONFIG gLocalConfig = {0};

PSTR gProviderLocal_Hostname = NULL;

