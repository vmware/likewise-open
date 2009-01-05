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
    PCSTR  pszLoginId,
    DWORD  dwErrCode
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PSTR pszData = NULL;

    if (pServerState->hEventLog == (HANDLE)NULL)
    {
        dwError = LsaSrvOpenEventLog(
                      "Security",
                      &pServerState->hEventLog);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaGetErrorMessageForLoggingEvent(
                     dwErrCode,
                     &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvLogSuccessAuditEvent(
                     pServerState->hEventLog,
                     LOGIN_EVENT_CATEGORY,
                     pszLoginId,
                     pszData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_STRING(pszData);

    return;

error:

    LSA_LOG_ERROR("Failed to post login success event for [%s]", LSA_SAFE_LOG_STRING(pszLoginId));
    LSA_LOG_ERROR("Error code: [%d]", dwError);

    goto cleanup;
}

VOID
LsaSrvWriteLoginFailedEvent(
    HANDLE hServer,
    PCSTR  pszLoginId,
    DWORD  dwErrCode
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PSTR  pszData = NULL;

    if (pServerState->hEventLog == (HANDLE)NULL)
    {
        dwError = LsaSrvOpenEventLog(
                      "Security",
                      &pServerState->hEventLog);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaGetErrorMessageForLoggingEvent(
                     dwErrCode,
                     &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvLogFailureAuditEvent(
                     pServerState->hEventLog,
                     LOGIN_EVENT_CATEGORY,
                     pszLoginId,
                     pszData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_STRING(pszData);

    return;

error:

    LSA_LOG_ERROR("Failed to post login failure event for [%s]", LSA_SAFE_LOG_STRING(pszLoginId));
    LSA_LOG_ERROR("Error code: [%d]", dwError);

    goto cleanup;
}

VOID
LsaSrvWriteLogoutSuccessEvent(
    HANDLE hServer,
    PCSTR  pszLoginId,
    DWORD  dwErrCode
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PSTR pszData = NULL;

    if (pServerState->hEventLog == (HANDLE)NULL)
    {
        dwError = LsaSrvOpenEventLog(
                      "Security",
                      &pServerState->hEventLog);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaGetErrorMessageForLoggingEvent(
                     dwErrCode,
                     &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvLogSuccessAuditEvent(
                     pServerState->hEventLog,
                     LOGOUT_EVENT_CATEGORY,
                     pszLoginId,
                     pszData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_STRING(pszData);

    return;

error:

    LSA_LOG_ERROR("Failed to post logout success event for [%s]", LSA_SAFE_LOG_STRING(pszLoginId));
    LSA_LOG_ERROR("Error code: [%d]", dwError);

    goto cleanup;
}

VOID
LsaSrvWriteLogoutFailedEvent(
    HANDLE hServer,
    PCSTR  pszLoginId,
    DWORD  dwErrCode
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PSTR pszData = NULL;

    if (pServerState->hEventLog == (HANDLE)NULL)
    {
        dwError = LsaSrvOpenEventLog(
                      "Security",
                      &pServerState->hEventLog);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaGetErrorMessageForLoggingEvent(
                     dwErrCode,
                     &pszData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvLogFailureAuditEvent(
                     pServerState->hEventLog,
                     LOGOUT_EVENT_CATEGORY,
                     pszLoginId,
                     pszData);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_STRING(pszData);

    return;

error:

    LSA_LOG_ERROR("Failed to post logout failure event for [%s]", LSA_SAFE_LOG_STRING(pszLoginId));
    LSA_LOG_ERROR("Error code: [%d]", dwError);

    goto cleanup;
}
