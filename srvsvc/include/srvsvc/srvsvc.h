/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        srvsvc.h
 *
 * Abstract:
 *
 *        Likewise Server Service (srvsvc) RPC client and server
 *
 *        Client API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _SRVSVC_H_
#define _SRVSVC_H_

#include <lwrpc/types.h>
#include <srvsvc/srvsvcbinding.h>
#include <srvsvc/srvsvcdefs.h>


/*
 * Error codes
 */
#define SRVSVC_ERROR_SUCCESS                   0x0000
#define SRVSVC_ERROR_INVALID_CONFIG_PATH       0x9400 // 37888
#define SRVSVC_ERROR_INVALID_PREFIX_PATH       0x9401 // 37889
#define SRVSVC_ERROR_INSUFFICIENT_BUFFER       0x9402 // 37890
#define SRVSVC_ERROR_OUT_OF_MEMORY             0x9403 // 37891
#define SRVSVC_ERROR_INVALID_MESSAGE           0x9404 // 37892
#define SRVSVC_ERROR_UNEXPECTED_MESSAGE        0x9405 // 37893
#define SRVSVC_ERROR_NO_SUCH_USER              0x9406 // 37894
#define SRVSVC_ERROR_DATA_ERROR                0x9407 // 37895
#define SRVSVC_ERROR_NOT_IMPLEMENTED           0x9408 // 37896
#define SRVSVC_ERROR_NO_CONTEXT_ITEM           0x9409 // 37897
#define SRVSVC_ERROR_NO_SUCH_GROUP             0x940A // 37898
#define SRVSVC_ERROR_REGEX_COMPILE_FAILED      0x940B // 37899
#define SRVSVC_ERROR_NSS_EDIT_FAILED           0x940C // 37900
#define SRVSVC_ERROR_NO_HANDLER                0x940D // 37901
#define SRVSVC_ERROR_INTERNAL                  0x940E // 37902
#define SRVSVC_ERROR_NOT_HANDLED               0x940F // 37903
#define SRVSVC_ERROR_UNEXPECTED_DB_RESULT      0x9410 // 37904
#define SRVSVC_ERROR_INVALID_PARAMETER         0x9411 // 37905
#define SRVSVC_ERROR_LOAD_LIBRARY_FAILED       0x9412 // 37906
#define SRVSVC_ERROR_LOOKUP_SYMBOL_FAILED      0x9413 // 37907
#define SRVSVC_ERROR_INVALID_EVENTLOG          0x9414 // 37908
#define SRVSVC_ERROR_INVALID_CONFIG            0x9415 // 37909
#define SRVSVC_ERROR_STRING_CONV_FAILED        0x9416 // 37910
#define SRVSVC_ERROR_INVALID_DB_HANDLE         0x9417 // 37911
#define SRVSVC_ERROR_FAILED_CONVERT_TIME       0x9418 // 37912
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_RPC_BINDING 0x9419 // 37913
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_OPEN   0x941A // 37914
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_CLOSE  0x941B // 37915
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_COUNT  0x941C // 37916
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_READ   0x941D // 37917
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_WRITE  0x941E // 37918
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_CLEAR  0x941F // 37919
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_DELETE 0x9420 // 37920
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_REGISTER 0x9421 // 37921
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_UNREGISTER 0x9422 // 37922
#define SRVSVC_ERROR_RPC_EXCEPTION_UPON_LISTEN 0x9423 // 37923
#define SRVSVC_ERROR_RPC_EXCEPTION             0x9424 // 37924
#define SRVSVC_ERROR_ACCESS_DENIED             0x9425 // 37925
#define SRVSVC_ERROR_SENTINEL                  0x9426 // 37926


/*
 * Error check macros
 */

#define BAIL_ON_SRVSVC_ERROR(dwError)                                   \
    if (dwError) {                                                      \
        SRVSVC_LOG_DEBUG("Error at %s:%d. Error [code:%d]",             \
                         __FILE__, __LINE__, dwError);                  \
        goto error;                                                     \
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
            SRVSVC_LOG_ERROR("DCE Error [0x%8lx] Reason [%s]",          \
                             (unsigned long)(rpcstatus), errstr);       \
        }                                                               \
        else                                                            \
        {                                                               \
            SRVSVC_LOG_ERROR("DCE Error [0x%8lx]",                      \
                             (unsigned long)(rpcstatus));               \
        }                                                               \
                                                                        \
        switch ((rpcstatus)) {                                          \
        case RPC_S_INVALID_STRING_BINDING:                              \
            (dwError) = SRVSVC_ERROR_RPC_EXCEPTION_UPON_RPC_BINDING;    \
            break;                                                      \
                                                                        \
        default:                                                        \
            (dwError) = SRVSVC_ERROR_RPC_EXCEPTION;                     \
        }                                                               \
                                                                        \
        goto error;                                                     \
    }


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

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif


/*
 * Logging macros
 */
#define SRVSVC_LOG_ALWAYS(szFmt...)                     \
    SrvSvcLogMessage(LOG_LEVEL_ALWAYS, ## szFmt);

#define SRVSVC_LOG_ERROR(szFmt...)                         \
    if (gSrvSvcLogInfo.dwLogLevel >= LOG_LEVEL_ERROR) {    \
        SrvSvcLogMessage(LOG_LEVEL_ERROR, ## szFmt);       \
    }

#define SRVSVC_LOG_WARNING(szFmt...)                       \
    if (gSrvSvcLogInfo.dwLogLevel >= LOG_LEVEL_WARNING) {  \
        SrvSvcLogMessage(LOG_LEVEL_WARNING, ## szFmt);     \
    }

#define SRVSVC_LOG_INFO(szFmt...)                          \
    if (gSrvSvcLogInfo.dwLogLevel >= LOG_LEVEL_INFO)    {  \
        SrvSvcLogMessage(LOG_LEVEL_INFO, ## szFmt);        \
    }

#define SRVSVC_LOG_VERBOSE(szFmt...)                       \
    if (gSrvSvcLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {  \
        SrvSvcLogMessage(LOG_LEVEL_VERBOSE, ## szFmt);     \
    }

#define SRVSVC_LOG_DEBUG(szFmt...)                         \
    if (gSrvSvcLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {  \
        SrvSvcLogMessage(LOG_LEVEL_VERBOSE, ## szFmt);     \
    }

#ifndef NET_API_STATUS_DEFINED
typedef WINERR NET_API_STATUS;

#define NET_API_STATUS_DEFINED
#endif


NET_API_STATUS
NetConnectionEnum(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *qualifier,
    uint32 level,
    uint8 **bufptr,
    uint32 prefmaxlen,
    uint32 *entriesread,
    uint32 *totalentries,
    uint32 *resume_handle
    );


NET_API_STATUS
NetFileEnum(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *basepath,
    const wchar16_t *username,
    uint32 level,
    uint8 **bufptr,
    uint32 prefmaxlen,
    uint32 *entriesread,
    uint32 *totalentries,
    uint32 *resume_handle
    );


NET_API_STATUS
NetFileGetInfo(
    handle_t b,
    const wchar16_t *servername,
    uint32 fileid,
    uint32 level,
    uint8 **bufptr
    );


NET_API_STATUS
NetFileClose(
    handle_t b,
    const wchar16_t *servername,
    uint32 fileid
    );


NET_API_STATUS
NetSessionEnum(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *unc_client_name,
    const wchar16_t *username,
    uint32 level,
    uint8 **bufptr,
    uint32 prefmaxlen,
    uint32 *entriesread,
    uint32 *totalentries,
    uint32 *resume_handle
    );


NET_API_STATUS
NetShareAdd(
    handle_t b,
    const wchar16_t *servername,
    uint32 level,
    uint8 *bufptr,
    uint32 *parm_err
    );


NET_API_STATUS
NetShareEnum(
    handle_t b,
    const wchar16_t *servername,
    uint32 level,
    uint8 **bufptr,
    uint32 prefmaxlen,
    uint32 *entriesread,
    uint32 *totalentries,
    uint32 *resume_handle
    );


NET_API_STATUS
NetShareGetInfo(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *netname,
    uint32 level,
    uint8 **bufptr
    );


NET_API_STATUS
NetShareSetInfo(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *netname,
    uint32 level,
    uint8 *bufptr,
    uint32 *parm_err
    );


NET_API_STATUS
NetShareDel(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *netname,
    uint32 reserved
    );


NET_API_STATUS
NetServerGetInfo(
    handle_t b,
    const wchar16_t *servername,
    uint32 level,
    uint8 **bufptr
    );


NET_API_STATUS
NetServerSetInfo(
    handle_t b,
    const wchar16_t *servername,
    uint32 level,
    uint8 *bufptr,
    uint32 *parm_err
    );


NET_API_STATUS
NetRemoteTOD(
    handle_t b,
    const wchar16_t *servername,
    uint8 **bufptr
    );


NET_API_STATUS
SrvSvcInitMemory(
    void
    );


NET_API_STATUS
SrvSvcDestroyMemory(
    void
    );


NET_API_STATUS
SrvSvcFreeMemory(
    void *ptr
    );


#endif /* _SRVSVC_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
