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
 *        event.h
 *
 * Abstract:
 *
 *        Likewise Security And Authentication Subsystem (LSASS)
 *
 *        Eventlog API
 *
 * Authors: Wei Fu (wfu@likewisesoftware.com)
 */

#include "includes.h"

DWORD
LsaSrvOpenEventLog(
    PSTR pszCategoryType,
    PHANDLE phEventLog
    )
{
    return LWIOpenEventLogEx(
                  NULL,
                  pszCategoryType,
                  "Likewise LSASS",
                  0x8000,
                  "lsassd",
                  NULL,
                  phEventLog);
}

DWORD
LsaSrvCloseEventLog(
    HANDLE hEventLog
    )
{
    return LWICloseEventLog(hEventLog);
}

DWORD
LsaSrvLogInformationEvent(
    HANDLE hEventLog,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
    return LWIWriteEventLog(
                   hEventLog,
                   INFORMATION_EVENT_TYPE,
                   pszCategory,
                   pszDescription,
                   pszData);
}

DWORD
LsaSrvLogWarningEvent(
    HANDLE hEventLog,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
    return LWIWriteEventLog(
                   hEventLog,
                   WARNING_EVENT_TYPE,
                   pszCategory,
                   pszDescription,
                   pszData);
}

DWORD
LsaSrvLogErrorEvent(
    HANDLE hEventLog,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
    return LWIWriteEventLog(
                   hEventLog,
                   ERROR_EVENT_TYPE,
                   pszCategory,
                   pszDescription,
                   pszData);
}

DWORD
LsaSrvLogSuccessAuditEvent(
    HANDLE hEventLog,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
    return LWIWriteEventLog(
                   hEventLog,
                   SUCCESS_AUDIT_EVENT_TYPE,
                   pszCategory,
                   pszDescription,
                   pszData);
}

DWORD
LsaSrvLogFailureAuditEvent(
    HANDLE hEventLog,
    PCSTR  pszCategory,
    PCSTR  pszDescription,
    PCSTR  pszData
    )
{
    return LWIWriteEventLog(
                   hEventLog,
                   FAILURE_AUDIT_EVENT_TYPE,
                   pszCategory,
                   pszDescription,
                   pszData);
}

VOID
LsaSrvLogServiceSuccessEvent(
    PCSTR pszEventCategory,
    PCSTR pszDescription,
    PCSTR pszData
    )
{
    DWORD dwError = 0;
    HANDLE hEventLog = (HANDLE)NULL;
    
    dwError = LsaSrvOpenEventLog(
                  "System",
                  &hEventLog);       
    BAIL_ON_LSA_ERROR(dwError);    
    
    dwError = LsaSrvLogInformationEvent(            
                  hEventLog,
                  pszEventCategory,
                  pszDescription,
                  pszData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaSrvCloseEventLog(hEventLog);

    return;

error:

    LSA_LOG_ERROR("Failed to post service success event.");
    LSA_LOG_ERROR("Error code: [%d]", dwError);

    goto cleanup;
}

VOID
LsaSrvLogServiceWarningEvent(
    PCSTR pszEventCategory, 
    PCSTR pszDescription,
    PCSTR pszData
    )
{
    DWORD dwError = 0;
    HANDLE hEventLog = (HANDLE)NULL;
    
    dwError = LsaSrvOpenEventLog(
                  "System",
                  &hEventLog);       
    BAIL_ON_LSA_ERROR(dwError);    
    
    dwError = LsaSrvLogWarningEvent(            
                  hEventLog,
                  pszEventCategory,
                  pszDescription,
                  pszData);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:
    
    LsaSrvCloseEventLog(hEventLog);
    
    return;

error:

    LSA_LOG_ERROR("Failed to post service warning event.");
    LSA_LOG_ERROR("Error code: [%d]", dwError);

    goto cleanup;
}


VOID
LsaSrvLogServiceFailureEvent(
    PCSTR pszEventCategory,
    PCSTR pszDescription,
    PCSTR pszData
    )
{
    DWORD dwError = 0;
    HANDLE hEventLog = (HANDLE)NULL;
    
    dwError = LsaSrvOpenEventLog(
                  "System",
                  &hEventLog);       
    BAIL_ON_LSA_ERROR(dwError);    
    
    dwError = LsaSrvLogErrorEvent(            
                  hEventLog,
                  pszEventCategory,
                  pszDescription,
                  pszData);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:
    
    LsaSrvCloseEventLog(hEventLog);
    
    return;

error:

    LSA_LOG_ERROR("Failed to post service failure event.");
    LSA_LOG_ERROR("Error code: [%d]", dwError);

    goto cleanup;
}

VOID
LsaSrvLogUserPWChangeSuccessEvent(
    PCSTR pszLoginId,
    PCSTR pszUserType
    )
{
    DWORD dwError = 0;
    PSTR pszChangePWSuccessDescription = NULL;
    
    dwError = LsaAllocateStringPrintf(
               &pszChangePWSuccessDescription,
               "The password for %s user '%s' was successfully changed.",
               LSA_SAFE_LOG_STRING(pszUserType),
               LSA_SAFE_LOG_STRING(pszLoginId));
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            GENERAL_EVENT_CATEGORY,
            pszChangePWSuccessDescription,
            "<null>");
cleanup:
    
    LSA_SAFE_FREE_STRING(pszChangePWSuccessDescription);
    
    return;
    
error:

    goto cleanup;
}

VOID
LsaSrvLogUserPWChangeFailureEvent(
    PCSTR pszLoginId,
    PCSTR pszUserType,
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszChangePWFailureDescription = NULL;
    PSTR pszData = NULL;
    
    dwError = LsaAllocateStringPrintf(
               &pszChangePWFailureDescription,
               "The password change attempt for %s user '%s' was failed. Error code : [%d].",
               LSA_SAFE_LOG_STRING(pszUserType),
               LSA_SAFE_LOG_STRING(pszLoginId),
               dwErrCode);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaGetErrorMessageForLoggingEvent(
                     dwErrCode,
                     &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceFailureEvent(
            GENERAL_EVENT_CATEGORY,
            pszChangePWFailureDescription,
            pszData);
cleanup:
    
    LSA_SAFE_FREE_STRING(pszChangePWFailureDescription);
    LSA_SAFE_FREE_STRING(pszData);
    
    return;
    
error:

    goto cleanup;
}

VOID
LsaSrvLogUserIDConflictEvent(
    uid_t uid,
    PCSTR pszProviderName,
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszUserIDConflictDescription = NULL;
    PSTR pszData = NULL;

    dwError = LsaAllocateStringPrintf(
                 &pszUserIDConflictDescription,
                 "While querying for user with a uid value '%d', '%s' found duplicate entries. Error code : [%d].",
                 uid,
                 pszProviderName,
                 dwErrCode);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);

    LsaSrvLogServiceWarningEvent(
            GENERAL_EVENT_CATEGORY,
            pszUserIDConflictDescription,
            pszData);

cleanup:

    LSA_SAFE_FREE_STRING(pszUserIDConflictDescription);
    LSA_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;

}

VOID
LsaSrvLogUserAliasConflictEvent(
    PCSTR pszAlias,
    PCSTR pszProviderName,
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszUserAliasConflictDescription = NULL;
    PSTR pszData = NULL;

    dwError = LsaAllocateStringPrintf(
                 &pszUserAliasConflictDescription,
                 "While querying for user with alias '%s', '%s' found duplicate entries. Error code : [%d].",
                 pszAlias,
                 pszProviderName,
                 dwErrCode);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);

    LsaSrvLogServiceWarningEvent(
            GENERAL_EVENT_CATEGORY,
            pszUserAliasConflictDescription,
            pszData);

cleanup:

    LSA_SAFE_FREE_STRING(pszUserAliasConflictDescription);
    LSA_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;

}

VOID
LsaSrvLogDuplicateObjectFoundEvent(
    PCSTR pszName1,
    PCSTR pszName2,
    PCSTR pszProviderName,
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszObjectDuplicateDescription = NULL;
    PSTR pszData = NULL;

    dwError = LsaAllocateStringPrintf(
                 &pszObjectDuplicateDescription,
                 "Found duplicate entries: '%s' and '%s'. Error code : [%d].",
                 pszName1,
                 pszName2,
                 pszProviderName,
                 dwErrCode);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);

    LsaSrvLogServiceWarningEvent(
            GENERAL_EVENT_CATEGORY,
            pszObjectDuplicateDescription,
            pszData);

cleanup:

    LSA_SAFE_FREE_STRING(pszObjectDuplicateDescription);
    LSA_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;

}

