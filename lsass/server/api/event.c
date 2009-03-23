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
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"


VOID
LsaSrvWriteLoginSuccessEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId,
    DWORD  dwErrCode
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PSTR pszData = NULL;
    PSTR pszDescription = NULL;

    if (pServerState->hEventLog == (HANDLE)NULL)
    {
        dwError = LsaSrvOpenEventLog(
                      "Security",
                      &pServerState->hEventLog);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Successful Logon:\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     User Name:               %s",
                 pszProvider,
                 pszLoginId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                     dwErrCode,
                     &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvLogSuccessAuditEvent(
                     pServerState->hEventLog,
                     LSASS_EVENT_SUCCESSFUL_LOGON,
                     pszLoginId,
                     LOGIN_LOGOFF_EVENT_CATEGORY,
                     pszDescription,
                     pszData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_STRING(pszData);
    LSA_SAFE_FREE_STRING(pszDescription);

    return;

error:

    LSA_LOG_ERROR("Failed to post login success event for [%s]", LSA_SAFE_LOG_STRING(pszLoginId));
    LSA_LOG_ERROR("Error code: [%d]", dwError);

    goto cleanup;
}

VOID
LsaSrvWriteLoginFailedEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId,
    DWORD  dwErrCode
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    char  szReason[256] = {0};
    PSTR  pszData = NULL;
    DWORD dwEventID = 0;
    PSTR  pszDescription = NULL;

    if (pServerState->hEventLog == (HANDLE)NULL)
    {
        dwError = LsaSrvOpenEventLog(
                      "Security",
                      &pServerState->hEventLog);
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch(dwErrCode)
    {
        case LSA_ERROR_NO_SUCH_USER:
        case LSA_ERROR_INVALID_PASSWORD:
        case LSA_ERROR_PASSWORD_MISMATCH:
            dwEventID = LSASS_EVENT_FAILED_LOGON_UNKNOWN_USERNAME_OR_BAD_PASSWORD;
            strcpy(szReason, "Unknown username or bad password");
            break;

/* Not yet supported, MIT Kerberos needs to return a specific error for these conditions ...
        case LSA_ERROR_XYZ:
            dwEventID = LSASS_EVENT_FAILED_LOGON_TIME_RESTRICTION_VIOLATION;
            strcpy(szReason, "Account logon time restriction violation");
            break;
*/

        case LSA_ERROR_ACCOUNT_DISABLED:
            dwEventID = LSASS_EVENT_FAILED_LOGON_ACCOUNT_DISABLED;
            strcpy(szReason, "Account currently disabled");
            break;

        case LSA_ERROR_ACCOUNT_EXPIRED:
            dwEventID = LSASS_EVENT_FAILED_LOGON_ACCOUNT_EXPIRED;
            strcpy(szReason, "The specified user account has expired");
            break;

/* Not yet supported, MIT Kerberos needs to return a specific error for these conditions ...
        case LSA_ERROR_XYZ:
            dwEventID = LSASS_EVENT_FAILED_LOGON_MACHINE_RESTRICTION_VIOLATION;
            strcpy(szReason, "User not allowed to logon at this computer");
            break;

        case LSA_ERROR_XYZ:
            dwEventID = LSASS_EVENT_FAILED_LOGON_TYPE_OF_LOGON_NOT_GRANTED;
            strcpy(szReason, "The user has not been granted the requested logon type at this machine");
            break;
*/

        case LSA_ERROR_PASSWORD_EXPIRED:
            dwEventID = LSASS_EVENT_FAILED_LOGON_PASSWORD_EXPIRED;
            strcpy(szReason, "The specified account's password has expired");
            break;

        case LSA_ERROR_INVALID_NETLOGON_RESPONSE:
        case LSA_ERROR_RPC_NETLOGON_FAILED:
            dwEventID = LSASS_EVENT_FAILED_LOGON_NETLOGON_FAILED;
            strcpy(szReason, "The NetLogon component is not active");
            break;

        case LSA_ERROR_ACCOUNT_LOCKED:
            dwEventID = LSASS_EVENT_FAILED_LOGON_ACCOUNT_LOCKED;
            strcpy(szReason, "Account locked out");
            break;

        default:
            dwEventID = LSASS_EVENT_FAILED_LOGON_UNEXPECTED_ERROR;
            strcpy(szReason, "An unexpected error occurred");
    }

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Logon Failure:\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     Reason:                  %s\r\n" \
                 "     User Name:               %s",
                 pszProvider,
                 szReason,
                 pszLoginId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                     dwErrCode,
                     &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvLogFailureAuditEvent(
                     pServerState->hEventLog,
                     dwEventID,
                     pszLoginId,
                     LOGIN_LOGOFF_EVENT_CATEGORY,
                     pszDescription,
                     pszData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_STRING(pszData);
    LSA_SAFE_FREE_STRING(pszDescription);

    return;

error:

    LSA_LOG_ERROR("Failed to post login failure event for [%s]", LSA_SAFE_LOG_STRING(pszLoginId));
    LSA_LOG_ERROR("Error code: [%d]", dwError);

    goto cleanup;
}

VOID
LsaSrvWriteLogoutSuccessEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PSTR pszDescription = NULL;

    if (pServerState->hEventLog == (HANDLE)NULL)
    {
        dwError = LsaSrvOpenEventLog(
                      "Security",
                      &pServerState->hEventLog);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "User Logoff:\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     User Name:               %s",
                 pszProvider,
                 pszLoginId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvLogSuccessAuditEvent(
                     pServerState->hEventLog,
                     LSASS_EVENT_SUCCESSFUL_LOGOFF,
                     pszLoginId,
                     LOGIN_LOGOFF_EVENT_CATEGORY,
                     pszDescription,
                     NULL);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);

    return;

error:

    LSA_LOG_ERROR("Failed to post logout success event for [%s]", LSA_SAFE_LOG_STRING(pszLoginId));
    LSA_LOG_ERROR("Error code: [%d]", dwError);

    goto cleanup;
}

VOID
LsaSrvWriteUserPWChangeSuccessEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PSTR pszDescription = NULL;

    if (pServerState->hEventLog == (HANDLE)NULL)
    {
        dwError = LsaSrvOpenEventLog(
                      "Security",
                      &pServerState->hEventLog);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Change Password Attempt:\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     Target Account Name:     %s",
                 pszProvider,
                 LSA_SAFE_LOG_STRING(pszLoginId));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvLogSuccessAuditEvent(
                  pServerState->hEventLog,
                  LSASS_EVENT_SUCCESSFUL_PASSWORD_CHANGE,
                  pszLoginId,
                  PASSWORD_EVENT_CATEGORY,
                  pszDescription,
                  NULL);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);

    return;

error:

    LSA_LOG_ERROR("Failed to post user password change success event.");
    LSA_LOG_ERROR("Error code: [%d]", dwError);

    goto cleanup;
}

VOID
LsaSrvWriteUserPWChangeFailureEvent(
    HANDLE hServer,
    PCSTR  pszProvider,
    PCSTR  pszLoginId,
    DWORD  dwErrCode
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;

    if (pServerState->hEventLog == (HANDLE)NULL)
    {
        dwError = LsaSrvOpenEventLog(
                      "Security",
                      &pServerState->hEventLog);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Change Password Attempt:\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     Target Account Name:     %s",
                 pszProvider,
                 LSA_SAFE_LOG_STRING(pszLoginId));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                     dwErrCode,
                     &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvLogFailureAuditEvent(
                  pServerState->hEventLog,
                  LSASS_EVENT_FAILED_PASSWORD_CHANGE,
                  pszLoginId,
                  PASSWORD_EVENT_CATEGORY,
                  pszDescription,
                  pszData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);
    LSA_SAFE_FREE_STRING(pszData);

    return;

error:

    LSA_LOG_ERROR("Failed to post user password change failed event.");
    LSA_LOG_ERROR("Error code: [%d]", dwError);

    goto cleanup;
}


