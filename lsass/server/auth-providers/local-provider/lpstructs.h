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
 *        lpstructs.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider (Structures)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LPSTRUCTS_H__
#define __LPSTRUCTS_H__

typedef struct __LOCAL_PROVIDER_ENUM_STATE
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    DWORD dwInfoLevel;

    PSTR  pszGUID;

    PDIRECTORY_ENTRY pEntries;
    DWORD            dwNumEntries;

    DWORD dwNextStartingId;

    struct __LOCAL_PROVIDER_ENUM_STATE* pNext;

} LOCAL_PROVIDER_ENUM_STATE, *PLOCAL_PROVIDER_ENUM_STATE;

typedef struct __LOCAL_PROVIDER_CONTEXT
{
    uid_t uid;
    gid_t gid;

    LOCAL_ACCESS_FLAG accessFlags;

    HANDLE hDirectory;

} LOCAL_PROVIDER_CONTEXT, *PLOCAL_PROVIDER_CONTEXT;

typedef struct __LOCAL_CONFIG
{
    BOOLEAN   bEnableEventLog;
} LOCAL_CONFIG, *PLOCAL_CONFIG;

typedef struct _LOCAL_PROVIDER_GLOBALS
{
    pthread_mutex_t mutex;

    PSTR            pszConfigFilePath;
    PSTR            pszLocalDomain;
    PSTR            pszNetBIOSName;

    LONG64          llMaxPwdAge;
    LONG64          llPwdChangeTime;

    LOCAL_CONFIG    cfg;

} LOCAL_PROVIDER_GLOBALS, *PLOCAL_PROVIDER_GLOBALS;

#endif /* __LPSTRUCTS_H__ */

