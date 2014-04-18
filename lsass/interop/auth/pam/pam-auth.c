/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        pam-auth.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Authentication API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "pam-lsass.h"

#ifndef PAM_BAD_ITEM
#define PAM_BAD_ITEM PAM_SERVICE_ERR
#endif

static int
pam_need_check_smart_card(
    PLSA_PAM_CONFIG pConfig,
    PSTR pszPamSource
    )
{
    int i;

    if (!pszPamSource)
    {
        LSA_LOG_PAM_DEBUG("Empty pszPamSource, not checking for smartcard.");
        return FALSE;
    }

    for (i = 0; i < pConfig->dwNumSmartCardServices; ++i)
    {
        if (strcmp(pConfig->ppszSmartCardServices[i], pszPamSource) == 0)
        {
            return TRUE;
        }
    }

    LSA_LOG_PAM_DEBUG(
        "Service '%s' is not on the SmartCardServices list; "
        "not checking for smartcard.",
        pszPamSource);

    return FALSE;
}

static DWORD
pam_check_smart_card_user_matches(
    PPAMCONTEXT pPamContext,
    HANDLE hLsaConnection,
    PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwUserInfoLevel = 0;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    DWORD dwError;

    if (pPamContext->pszLoginName == NULL ||
        pPamContext->pszLoginName[0] == '\0')
    {
        dwError = LW_ERROR_SUCCESS;
        goto cleanup;
    }

    /*
     * Verify that the passed-in user name is the same as the smart card
     * user. Go to AD to make sure we get the canonicalized name.
     */
    dwError = LsaFindUserByName(
                    hLsaConnection,
                    pPamContext->pszLoginName,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    if (dwError != LW_ERROR_SUCCESS)
    {
        dwError = LW_ERROR_SUCCESS;
        goto cleanup;
    }

    if (strcmp(pUserInfo->pszName,
               pObject->userInfo.pszUnixName) != 0)
    {
        LSA_LOG_PAM_DEBUG(
            "Pam user '%s' does not match smartcard user",
            pPamContext->pszLoginName);
        dwError = LW_ERROR_NOT_HANDLED;
        goto cleanup;
    }

cleanup:
    if (pUserInfo != NULL)
    {
        LsaFreeUserInfo(dwUserInfoLevel, (PVOID)pUserInfo);
    }

    return dwError;
}

static int
pam_check_smart_card(
    PPAMCONTEXT pPamContext,
    PLSA_SECURITY_OBJECT* ppObject,
    PSTR* ppszSmartCardReader
    )
{
    DWORD dwError;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PSTR pszSmartCardReader = NULL;

    LSA_LOG_PAM_DEBUG("Checking for smartcard");

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetSmartCardUserObject(
                    hLsaConnection,
                    &pObject,
                    &pszSmartCardReader);
    if (dwError != LW_ERROR_SUCCESS)
    {
        LSA_LOG_PAM_DEBUG("No smartcard user found");
        goto error;
    }

    LSA_LOG_PAM_DEBUG(
        "Found smartcard for user '%s' in reader '%s'",
        pObject->userInfo.pszUnixName,
        pszSmartCardReader);

    dwError = pam_check_smart_card_user_matches(
                  pPamContext,
                  hLsaConnection,
                  pObject);
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaShouldIgnoreUser(pObject->userInfo.pszUnixName))
    {
        LSA_LOG_PAM_DEBUG("User %s is in lsassd ignore list",
                          pObject->userInfo.pszUnixName);
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;
    *ppszSmartCardReader = pszSmartCardReader;

cleanup:
    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    return dwError;

error:
    if (pObject)
    {
        LsaFreeSecurityObject(pObject);
    }
    *ppObject = NULL;

    LW_SAFE_FREE_STRING(pszSmartCardReader);
    *ppszSmartCardReader = NULL;

    goto cleanup;
}

static int
pam_do_authenticate(
    pam_handle_t* pamh,
    PPAMCONTEXT pPamContext,
    PLSA_PAM_CONFIG pConfig,
    PSTR pszPamSource,
    PSTR pszLoginId,
    PSTR pszPassword,
    BOOL bPin
    )
{
    DWORD dwError;
    HANDLE hLsaConnection = (HANDLE)NULL;
    LSA_AUTH_USER_PAM_PARAMS params = { 0 };
    PLSA_AUTH_USER_PAM_INFO pInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    int iPamError;

    LSA_LOG_PAM_DEBUG("%s::begin", __FUNCTION__);

    params.pszPamSource = pszPamSource;
    params.dwFlags = LSA_AUTH_USER_PAM_FLAG_RETURN_MESSAGE;
    params.pszLoginName = pszLoginId;
    params.pszPassword = pszPassword;
    if (bPin)
    {
        params.dwFlags |= LSA_AUTH_USER_PAM_FLAG_SMART_CARD;
    }

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAuthenticateUserPam(
                    hLsaConnection,
                    &params,
                    &pInfo);
    if (pInfo && pInfo->pszMessage)
    {
        LsaPamConverse(pamh,
                       pInfo->pszMessage,
                       PAM_TEXT_INFO,
                       NULL);
    }

    if (dwError == LW_ERROR_PASSWORD_EXPIRED)
    {
        // deal with this error in the
        // next call to pam_sm_acct_mgmt
        pPamContext->bPasswordExpired = TRUE;
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (pInfo && !pInfo->bOnlineLogon)
    {
        pPamContext->bOnlineLogon = FALSE;
    }
    else
    {
        // Assume online if no pInfo returned
        pPamContext->bOnlineLogon = TRUE;
    }

    dwError = LsaCheckUserInList(
                    hLsaConnection,
                    pszLoginId,
                    NULL);
    if (dwError)
    {
        if (!LW_IS_NULL_OR_EMPTY_STR(pConfig->pszAccessDeniedMessage))
        {
            LsaPamConverse(pamh,
                           pConfig->pszAccessDeniedMessage,
                           PAM_TEXT_INFO,
                           NULL);
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* On Centos, the pam_console module will grab the username from pam
     * and create a file based off that name. The filename must match the
     * user's canonical name. To fix this, pam_lsass will canonicalize
     * the username before pam_console gets it.
     *
     * We know that the username can be found in AD, and that
     * their password matches the AD user's password. At this point, it
     * is very unlikely that we will mangle a local username.
     */

    dwError = LsaFindUserByName(
                    hLsaConnection,
                    pszLoginId,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (strcmp(pszLoginId, pUserInfo->pszName))
    {
        LSA_LOG_PAM_INFO("Canonicalizing pam username from '%s' to '%s'\n",
                pszLoginId, pUserInfo->pszName);
        iPamError = pam_set_item(
                pamh,
                PAM_USER,
                pUserInfo->pszName);
        dwError = LsaPamUnmapErrorCode(iPamError);
        BAIL_ON_LSA_ERROR(dwError);
    }

#if defined(__LWI_SOLARIS__) || defined(__LWI_HP_UX__)
    /*
     * DTLOGIN tests for home directory existence before
     * pam_sm_open_session() is called, and puts up a failure dialog
     * to create the home directory. Unfortunately, afterwards, the user is
     * logged into a fail-safe session. These LsaOpenSession() /
     * LsaCloseSession() calls force the creation of the home directory after
     * successful authentication, making DTLOGIN happy.
     */
    dwError = LsaOpenSession(hLsaConnection,
                             pszLoginId);
    BAIL_ON_LSA_ERROR(dwError);
    dwError = LsaCloseSession(hLsaConnection,
                              pszLoginId);
    BAIL_ON_LSA_ERROR(dwError);

    /* On Solaris, we must save the user's password in
       a custom location so that we can pull it out later
       for password changes */
    dwError = LsaPamSetDataString(
        pamh,
        PAM_LSASS_OLDAUTHTOK,
        pszPassword);
    BAIL_ON_LSA_ERROR(dwError);
#endif

cleanup:
    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    if (pInfo)
    {
        LsaFreeAuthUserPamInfo(pInfo);
    }

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, (PVOID)pUserInfo);
    }

    LSA_LOG_PAM_DEBUG("%s::end", __FUNCTION__);
    return dwError;

error:
    if (dwError == LW_ERROR_NO_SUCH_USER || dwError == LW_ERROR_NOT_HANDLED)
    {
        LSA_LOG_PAM_WARNING("%s: error [login:%s][error code:%u]",
                          __FUNCTION__,
                          LSA_SAFE_LOG_STRING(pszLoginId),
                          dwError);
    }
    else
    {
        LSA_LOG_PAM_ERROR("%s: error [login:%s][error code:%u]",
                          __FUNCTION__,
                          LSA_SAFE_LOG_STRING(pszLoginId),
                          dwError);
    }

    goto cleanup;
}

static int
pam_authenticate_smart_card(
    pam_handle_t* pamh,
    PPAMCONTEXT pPamContext,
    PLSA_PAM_CONFIG pConfig,
    PSTR pszPamSource,
    PLSA_SECURITY_OBJECT pObject,
    PSTR pszSmartCardReader
    )
{
    DWORD dwError;
    PSTR pszPINPrompt = NULL;
    PSTR pszPIN = NULL;
    int bPromptGecos = FALSE;
    int iPamError;
    int i;

    LSA_LOG_PAM_DEBUG("%s::begin", __FUNCTION__);

    for (i = 0; i < pConfig->dwNumSmartCardPromptGecos; ++i)
    {
        if (strcmp(pConfig->ppszSmartCardPromptGecos[i],
                   pszPamSource) == 0)
        {
            bPromptGecos = TRUE;
            break;
        }
    }

    dwError = LwAllocateStringPrintf(
        &pszPINPrompt,
        "Enter PIN for %s: ",
        pObject->userInfo.pszUnixName);
    BAIL_ON_LSA_ERROR(dwError);

    if (bPromptGecos && pObject->userInfo.pszGecos)
    {
        LSA_PAM_CONVERSE_MESSAGE messages[2];

        messages[0].messageStyle = PAM_TEXT_INFO;
        messages[0].pszMessage = pObject->userInfo.pszGecos;
        messages[0].ppszResponse = NULL;
        messages[1].messageStyle = PAM_PROMPT_ECHO_OFF;
        messages[1].pszMessage = pszPINPrompt;
        messages[1].ppszResponse = &pszPIN;

        dwError = LsaPamConverseMulti(
            pamh,
            2,
            messages);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaPamConverse(
            pamh,
            pszPINPrompt,
            PAM_PROMPT_ECHO_OFF,
            &pszPIN);
        BAIL_ON_LSA_ERROR(dwError);
    }

    iPamError = pam_set_item(
                    pamh,
                    PAM_USER,
                    pObject->userInfo.pszUnixName);
    dwError = LsaPamUnmapErrorCode(iPamError);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Set a bogus password, so that later modules like
     * pam_unix will be guaranteed to fail, even if there's
     * a local user with the same name and the card's PIN
     * as their password.  Put the PIN into a separate data
     * item.
     */
    iPamError = pam_set_item(
                    pamh,
                    PAM_AUTHTOK,
                    "\x01\xFFPIN ENTERED");
    dwError = LsaPamUnmapErrorCode(iPamError);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Now try to authenticate.
     */
    dwError = pam_do_authenticate(
                pamh,
                pPamContext,
                pConfig,
                pszPamSource,
                pObject->userInfo.pszUnixName,
                pszPIN,
                TRUE);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pszPINPrompt);
    LW_SAFE_FREE_STRING(pszPIN);

    LSA_LOG_PAM_DEBUG("%s::end", __FUNCTION__);
    return dwError;

error:
    goto cleanup;
}

static int
pam_authenticate_password(
    pam_handle_t* pamh,
    PPAMCONTEXT pPamContext,
    PLSA_PAM_CONFIG pConfig,
    PSTR pszPamSource
    )
{
    DWORD dwError;
    PSTR pszLoginId = NULL;
    PSTR pszPassword = NULL;

    LSA_LOG_PAM_DEBUG("%s::begin", __FUNCTION__);

    dwError = LsaPamGetLoginId(
        pamh,
        pPamContext,
        &pszLoginId,
        TRUE);
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaShouldIgnoreUser(pszLoginId))
    {
        LSA_LOG_PAM_DEBUG("By-passing lsassd for local account");
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaPamGetCurrentPassword(
        pamh,
        pPamContext,
        &pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Now try to authenticate.
     */
    dwError = pam_do_authenticate(
                pamh,
                pPamContext,
                pConfig,
                pszPamSource,
                pszLoginId,
                pszPassword,
                FALSE);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pszLoginId);
    LW_SAFE_FREE_STRING(pszPassword);

    LSA_LOG_PAM_DEBUG("%s::end", __FUNCTION__);
    return dwError;

error:
    goto cleanup;
}

int
pam_sm_authenticate(
    pam_handle_t* pamh,
    int           flags,
    int           argc,
    const char**  argv
    )
{
    DWORD dwError;
    PPAMCONTEXT pPamContext = NULL;
    PLSA_PAM_CONFIG pConfig = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PSTR pszSmartCardReader = NULL;
    PSTR pszPamSource;
    int iPamError;

    LSA_LOG_PAM_DEBUG("%s::begin", __FUNCTION__);

    dwError = LsaPamGetConfig(&pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPamSetLogLevel(pConfig->dwLogLevel);

    dwError = LsaPamGetContext(
                    pamh,
                    flags,
                    argc,
                    argv,
                    &pPamContext);

    BAIL_ON_LSA_ERROR(dwError);

    /* If we are just overriding the default repository
       (Solaris), only do that */
    if (pPamContext->pamOptions.bSetDefaultRepository)
    {
#ifdef HAVE_STRUCT_PAM_REPOSITORY
        struct pam_repository *currentRepository = NULL;
        pam_get_item(
                pamh,
                PAM_REPOSITORY,
                (PAM_GET_ITEM_TYPE)&currentRepository);
        if (currentRepository == NULL)
        {
            struct pam_repository files = { "files", NULL, 0 };
            iPamError = pam_set_item(pamh, PAM_REPOSITORY, &files);
            dwError = LsaPamUnmapErrorCode(iPamError);
            if (dwError)
            {
                LSA_LOG_PAM_WARNING(
                    "pam_sm_authenticate: "
                    "warning unable to set pam repository [error code: %u]. "
                    "This will cause password changes on login to fail, "
                    "and it may cause password changes in general to fail.",
                    dwError);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        /* This gets mapped to PAM_IGNORE */
        dwError = LW_ERROR_NOT_HANDLED;
        goto cleanup;
#else
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
#endif
    }

    iPamError = pam_get_item(
                    pamh,
                    PAM_SERVICE,
                    (PAM_GET_ITEM_TYPE)&pszPamSource);
    if (iPamError == PAM_BAD_ITEM)
    {
        iPamError = 0;
        pszPamSource = NULL;
    }
    dwError = LsaPamUnmapErrorCode(iPamError);
    BAIL_ON_LSA_ERROR(dwError);

    if (pPamContext->pamOptions.bSmartCardPrompt)
    {
        if (pam_need_check_smart_card(pConfig, pszPamSource))
        {
            dwError = pam_check_smart_card(
                            pPamContext,
                            &pObject,
                            &pszSmartCardReader);
            if (dwError == LW_ERROR_SUCCESS)
            {
                dwError = pam_authenticate_smart_card(
                            pamh,
                            pPamContext,
                            pConfig,
                            pszPamSource,
                            pObject,
                            pszSmartCardReader);
                BAIL_ON_LSA_ERROR(dwError);

                goto cleanup;
            }
        }

        /*
         * SmartCard user was not found (or was not even looked for).
         * Prompt for username and password as usual.
         */
    }

    dwError = pam_authenticate_password(
                pamh,
                pPamContext,
                pConfig,
                pszPamSource);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pObject)
    {
        LsaFreeSecurityObject(pObject);
    }

    LW_SAFE_FREE_STRING(pszSmartCardReader);

    if (pConfig)
    {
        LsaPamFreeConfig(pConfig);
    }

    LSA_LOG_PAM_DEBUG("%s::end", __FUNCTION__);

    return LsaPamOpenPamFilterAuthenticate(
                            LsaPamMapErrorCode(dwError, pPamContext));

error:
    LSA_LOG_PAM_ERROR("%s: failed [error code:%u]", __FUNCTION__, dwError);
    goto cleanup;
}

int
pam_sm_setcred(
    pam_handle_t* pamh,
    int           flags,
    int           argc,
    const char*   argv[]
    )
{
    HANDLE hLsaConnection = (HANDLE)NULL;
    DWORD dwError = 0;
    PLSA_PAM_CONFIG pConfig = NULL;
    PSTR pszLoginId = NULL;
    PPAMCONTEXT pPamContext = NULL;
    DWORD dwUserInfoLevel = 0;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    int iPamError = 0;

    LSA_LOG_PAM_DEBUG("%s::begin", __FUNCTION__);

    dwError = LsaPamGetConfig(&pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPamSetLogLevel(pConfig->dwLogLevel);

    dwError = LsaPamGetContext(
                    pamh,
                    flags,
                    argc,
                    argv,
                    &pPamContext);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaPamGetLoginId(
        pamh,
        pPamContext,
        &pszLoginId,
        TRUE);
    BAIL_ON_LSA_ERROR(dwError);

    if (pPamContext->pamOptions.bSetDefaultRepository)
    {
#ifdef HAVE_STRUCT_PAM_REPOSITORY
        /* This gets mapped to PAM_IGNORE */
        dwError = LW_ERROR_NOT_HANDLED;
#else
        dwError = LW_ERROR_INTERNAL;
#endif
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (LsaShouldIgnoreUser(pszLoginId))
    {
        LSA_LOG_PAM_DEBUG("By passing lsassd for local account");
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindUserByName(
                    hLsaConnection,
                    pszLoginId,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pConfig)
    {
        LsaPamFreeConfig(pConfig);
    }

    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    LW_SAFE_FREE_STRING(pszLoginId);

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, (PVOID)pUserInfo);
    }

    LSA_LOG_PAM_DEBUG("%s::end", __FUNCTION__);

    iPamError = LsaPamOpenPamFilterSetCred(
                                LsaPamMapErrorCode(dwError, pPamContext));
#ifdef __LWI_SOLARIS__
    if (iPamError == PAM_SUCCESS)
    {
        /* Typically on Solaris a pam module would need to call setproject
         * here. It is rather complicated to determine what to set the project
         * to. Instead, if PAM_IGNORE is returned, pam_unix_cred will set the
         * project.
        */
        iPamError = PAM_IGNORE;
    }
#endif
    return iPamError;

error:
    if (dwError == LW_ERROR_NO_SUCH_USER || dwError == LW_ERROR_NOT_HANDLED)
    {
        LSA_LOG_PAM_WARNING("pam_sm_setcred error [login:%s][error code:%u]",
                          LSA_SAFE_LOG_STRING(pszLoginId),
                          dwError);
    }
    else
    {
        LSA_LOG_PAM_ERROR("pam_sm_setcred error [login:%s][error code:%u]",
                          LSA_SAFE_LOG_STRING(pszLoginId),
                          dwError);
    }
    goto cleanup;
}
