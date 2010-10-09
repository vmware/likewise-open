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
 *        eventdefs.h
 *
 * Abstract:
 *
 *        Likewise Eventlog Service (LWEVT)
 *
 *        Definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __EVENTDEFS_H__
#define __EVENTDEFS_H__

/*
 * Log levels
 */
#define LOG_LEVEL_ALWAYS  0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_VERBOSE 4
#define LOG_LEVEL_DEBUG   5

/*
 * Logging targets
 */
#define LOG_DISABLED   0
#define LOG_TO_SYSLOG  1
#define LOG_TO_FILE    2
#define LOG_TO_CONSOLE 3

#define TRY DCETHREAD_TRY
#define CATCH_ALL DCETHREAD_CATCH_ALL(THIS_CATCH)
#define CATCH(x) DCETHREAD_CATCH(x)
#define ENDTRY DCETHREAD_ENDTRY

#ifdef _WIN32

void __RPC_FAR * __RPC_USER midl_user_allocate(size_t cBytes);

void __RPC_USER midl_user_free(void __RPC_FAR * p);

#define RPC_SS_ALLOCATE(dwSize) RpcSsAllocate(dwSize)

#define RPC_SS_FREE(node)       RpcSsFree(node)

#else

#define RPC_SS_ALLOCATE(dwSize) rpc_ss_allocate(dwSize)

#define RPC_SS_FREE(node)       rpc_ss_free(node)

#endif

#ifndef RPC_CSTR
#define RPC_CSTR UCHAR*
#endif

#define RPC_STRING_BINDING_COMPOSE(protocol, hostname, endpoint, pBindingString, pStatus) \
    *pStatus = RpcStringBindingCompose(NULL, (RPC_CSTR)(protocol), (RPC_CSTR)(hostname), (RPC_CSTR)endpoint, NULL, (RPC_CSTR*)(pBindingString))

#define RPC_BINDING_FROM_STRING_BINDING(bindingString, pBindingHandle, pStatus) \
    *pStatus = RpcBindingFromStringBinding((RPC_CSTR)(bindingString), (pBindingHandle))

#define RPC_STRING_FREE(pString, pStatus) \
    *pStatus = RpcStringFree((RPC_CSTR*)pString)

#define RPC_BINDING_FREE(pBindingHandle, pStatus) \
    *pStatus = RpcBindingFree(pBindingHandle)

#define EVT_SAFE_FREE_MEMORY(mem) \
        do {                      \
           if (mem) {             \
              EVTFreeMemory(mem); \
              (mem) = NULL;       \
           }                      \
        } while(0);

#define IsNullOrEmptyString(pszStr)     \
    (pszStr == NULL || *pszStr == '\0')

#define EVT_SAFE_FREE_STRING(str) \
    do {                          \
        if (str) {                \
            EVTFreeString(str);   \
            (str) = NULL;         \
        }                         \
    } while(0);

#define EVT_LOG_ALWAYS(szFmt...)                     \
    EVTLogMessage(LOG_LEVEL_ALWAYS, ## szFmt);

#define EVT_LOG_ERROR(szFmt...)                         \
    if (gEvtLogInfo.dwLogLevel >= LOG_LEVEL_ERROR) {    \
        EVTLogMessage(LOG_LEVEL_ERROR, ## szFmt);       \
    }

#define EVT_LOG_WARNING(szFmt...)                       \
    if (gEvtLogInfo.dwLogLevel >= LOG_LEVEL_WARNING) {  \
        EVTLogMessage(LOG_LEVEL_WARNING, ## szFmt);     \
    }

#define EVT_LOG_INFO(szFmt...)                          \
    if (gEvtLogInfo.dwLogLevel >= LOG_LEVEL_INFO)    {  \
        EVTLogMessage(LOG_LEVEL_INFO, ## szFmt);        \
    }

#define EVT_LOG_VERBOSE(szFmt...)                       \
    if (gEvtLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {  \
        EVTLogMessage(LOG_LEVEL_VERBOSE, ## szFmt);     \
    }

#define EVT_LOG_DEBUG(szFmt...)                         \
    if (gEvtLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {  \
        EVTLogMessage(LOG_LEVEL_VERBOSE, ## szFmt);     \
    }

#define BAIL_ON_EVT_ERROR(dwError) \
    if (dwError) {                 \
        EVT_LOG_DEBUG("Error at %s:%d. Error [code:%d]", __FILE__, __LINE__, dwError); \
        goto error;                \
    }

#define BAIL_ON_DCE_ERROR(dwError, rpcstatus)                           \
    if ((rpcstatus) != RPC_S_OK)                                        \
    {                                                                   \
        dce_error_string_t errstr;                                      \
        int error_status;                                               \
        dce_error_inq_text((rpcstatus), (unsigned char*)errstr,         \
                           &error_status);                              \
        if (error_status == error_status_ok)                            \
        {                                                               \
            EVT_LOG_ERROR("DCE Error [0x%8x] Reason [%s]",              \
                          (rpcstatus), errstr);                         \
        }                                                               \
        else                                                            \
        {                                                               \
            EVT_LOG_ERROR("DCE Error [0x%8x]", (rpcstatus));            \
        }                                                               \
                                                                        \
        (dwError) = LwNtStatusToWin32Error(                             \
                LwRpcStatusToNtStatus((rpcstatus)));                    \
                                                                        \
        goto error;                                                     \
    }

#endif /* __EVENTDEFS_H__ */
