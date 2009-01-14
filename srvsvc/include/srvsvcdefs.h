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

#ifndef _SRVSVCDEFS_H_
#define _SRVSVCDEFS_H_

#include <types.h>
#include <security.h>

/* ERRORS */
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

#define SRVSVC_SAFE_FREE_MEMORY(mem) \
        do {                      \
           if (mem) {             \
              SRVSVCFreeMemory(mem); \
              (mem) = NULL;       \
           }                      \
        } while(0);

#define IsNullOrEmptyString(pszStr)     \
    (pszStr == NULL || *pszStr == '\0')

#define SRVSVC_SAFE_FREE_STRING(str) \
    do {                          \
        if (str) {                \
            SRVSVCFreeString(str);   \
            (str) = NULL;         \
        }                         \
    } while(0);

#define SRVSVC_LOG_ALWAYS(szFmt...)                     \
    SRVSVCLogMessage(LOG_LEVEL_ALWAYS, ## szFmt);

#define SRVSVC_LOG_ERROR(szFmt...)                         \
    if (gSrvSvcLogInfo.dwLogLevel >= LOG_LEVEL_ERROR) {    \
        SRVSVCLogMessage(LOG_LEVEL_ERROR, ## szFmt);       \
    }

#define SRVSVC_LOG_WARNING(szFmt...)                       \
    if (gSrvSvcLogInfo.dwLogLevel >= LOG_LEVEL_WARNING) {  \
        SRVSVCLogMessage(LOG_LEVEL_WARNING, ## szFmt);     \
    }

#define SRVSVC_LOG_INFO(szFmt...)                          \
    if (gSrvSvcLogInfo.dwLogLevel >= LOG_LEVEL_INFO)    {  \
        SRVSVCLogMessage(LOG_LEVEL_INFO, ## szFmt);        \
    }

#define SRVSVC_LOG_VERBOSE(szFmt...)                       \
    if (gSrvSvcLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {  \
        SRVSVCLogMessage(LOG_LEVEL_VERBOSE, ## szFmt);     \
    }

#define SRVSVC_LOG_DEBUG(szFmt...)                         \
    if (gSrvSvcLogInfo.dwLogLevel >= LOG_LEVEL_VERBOSE) {  \
        SRVSVCLogMessage(LOG_LEVEL_VERBOSE, ## szFmt);     \
    }

#define BAIL_ON_SRVSVC_ERROR(dwError) \
    if (dwError) {                 \
        SRVSVC_LOG_DEBUG("Error at %s:%d. Error [code:%d]", __FILE__, __LINE__, dwError); \
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
            SRVSVC_LOG_ERROR("DCE Error [0x%8x] Reason [%s]",              \
                          (rpcstatus), errstr);                         \
        }                                                               \
        else                                                            \
        {                                                               \
            SRVSVC_LOG_ERROR("DCE Error [0x%8x]", (rpcstatus));            \
        }                                                               \
                                                                        \
        switch ((rpcstatus)) {                                          \
        case RPC_S_INVALID_STRING_BINDING:                              \
            (dwError) = SRVSVC_ERROR_RPC_EXCEPTION_UPON_RPC_BINDING;       \
            break;                                                      \
                                                                        \
        default:                                                        \
            (dwError) = SRVSVC_ERROR_RPC_EXCEPTION;                        \
        }                                                               \
                                                                        \
        goto error;                                                     \
    }

#ifndef NET_API_STATUS_DEFINED
#define NET_API_STATUS_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef NET_API_STATUS_DEFINED")
cpp_quote("#define NET_API_STATUS_DEFINED 1")
#endif

typedef uint32 NET_API_STATUS;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef CONNECTION_INFO_0_DEFINED
#define CONNECTION_INFO_0_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef CONNECTION_INFO_0_DEFINED")
cpp_quote("#define CONNECTION_INFO_0_DEFINED 1")
#endif

typedef struct _CONNECTION_INFO_0 {
    uint32 coni0_id;
} CONNECTION_INFO_0, *PCONNECTION_INFO_0, *LPCONNECTION_INFO_0;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef CONNECTION_INFO_1_DEFINED
#define CONNECTION_INFO_1_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef CONNECTION_INFO_1_DEFINED")
cpp_quote("#define CONNECTION_INFO_1_DEFINED 1")
#endif

typedef struct _CONNECTION_INFO_1 {
    uint32 coni1_id;
    uint32 coni1_type;
    uint32 coni1_num_open;
    uint32 coni1_num_users;
    uint32 coni1_time;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *coni1_username;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *coni1_netname;
} CONNECTION_INFO_1, *PCONNECTION_INFO_1, *LPCONNECTION_INFO_1;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef FILE_INFO_2_DEFINED
#define FILE_INFO_2_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef FILE_INFO_2_DEFINED")
cpp_quote("#define FILE_INFO_2_DEFINED 1")
#endif

typedef struct _FILE_INFO_2 {
    uint32 fi2_id;
} FILE_INFO_2, *PFILE_INFO_2, *LPFILE_INFO_2;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef FILE_INFO_3_DEFINED
#define FILE_INFO_3_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef FILE_INFO_3_DEFINED")
cpp_quote("#define FILE_INFO_3_DEFINED 1")
#endif

typedef struct _FILE_INFO_3 {
    uint32 fi3_idd;
    uint32 fi3_permissions;
    uint32 fi3_num_locks;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *fi3_path_name;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *fi3_username;
} FILE_INFO_3, *PFILE_INFO_3, *LPFILE_INFO_3;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SESSION_INFO_0_DEFINED
#define SESSION_INFO_0_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SESSION_INFO_0_DEFINED")
cpp_quote("#define SESSION_INFO_0_DEFINED 1")
#endif

typedef struct _SESSION_INFO_0 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi0_cname;
} SESSION_INFO_0, *PSESSION_INFO_0, *LPSESSION_INFO_0;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SESSION_INFO_1_DEFINED
#define SESSION_INFO_1_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SESSION_INFO_1_DEFINED")
cpp_quote("#define SESSION_INFO_1_DEFINED 1")
#endif

typedef struct _SESSION_INFO_1 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi1_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi1_username;
    uint32 sesi1_num_opens;
    uint32 sesi1_time;
    uint32 sesi1_idle_time;
    uint32 sesi1_user_flags;
} SESSION_INFO_1, *PSESSION_INFO_1, *LPSESSION_INFO_1;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SESSION_INFO_2_DEFINED
#define SESSION_INFO_2_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SESSION_INFO_2_DEFINED")
cpp_quote("#define SESSION_INFO_2_DEFINED 1")
#endif

typedef struct _SESSION_INFO_2 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi2_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi2_username;
    uint32 sesi2_num_opens;
    uint32 sesi2_time;
    uint32 sesi2_idle_time;
    uint32 sesi2_user_flags;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi2_cltype_name;
} SESSION_INFO_2, *PSESSION_INFO_2, *LPSESSION_INFO_2;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SESSION_INFO_10_DEFINED
#define SESSION_INFO_10_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SESSION_INFO_10_DEFINED")
cpp_quote("#define SESSION_INFO_10_DEFINED 1")
#endif

typedef struct _SESSION_INFO_10 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi10_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi10_username;
    uint32 sesi10_time;
    uint32 sesi10_idle_time;
} SESSION_INFO_10, *PSESSION_INFO_10, *LPSESSION_INFO_10;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SESSION_INFO_502_DEFINED
#define SESSION_INFO_502_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SESSION_INFO_502_DEFINED")
cpp_quote("#define SESSION_INFO_502_DEFINED 1")
#endif

typedef struct _SESSION_INFO_502 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_cname;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_username;
    uint32 sesi502_num_opens;
    uint32 sesi502_time;
    uint32 sesi502_idle_time;
    uint32 sesi502_user_flags;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_cltype_name;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sesi502_transport;
} SESSION_INFO_502, *PSESSION_INFO_502, *LPSESSION_INFO_502;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SHARE_INFO_0_DEFINED
#define SHARE_INFO_0_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_0_DEFINED")
cpp_quote("#define SHARE_INFO_0_DEFINED 1")
#endif

typedef struct _SHARE_INFO_0 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *shi0_netname;
} SHARE_INFO_0, *PSHARE_INFO_0, *LPSHARE_INFO_0;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SHARE_INFO_1_DEFINED
#define SHARE_INFO_1_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1_DEFINED")
cpp_quote("#define SHARE_INFO_1_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *shi1_netname;
    uint32 shi1_type;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *shi1_remark;
} SHARE_INFO_1, *PSHARE_INFO_1, *LPSHARE_INFO_1;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SHARE_INFO_2_DEFINED
#define SHARE_INFO_2_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_2_DEFINED")
cpp_quote("#define SHARE_INFO_2_DEFINED 1")
#endif

typedef struct _SHARE_INFO_2 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *shi2_netname;
    uint32 shi2_type;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *shi2_remark;
    uint32 shi2_permissions;
    uint32 shi2_max_uses;
    uint32 shi2_current_uses;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *shi2_path;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *shi2_password;
} SHARE_INFO_2, *PSHARE_INFO_2, *LPSHARE_INFO_2;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SHARE_INFO_501_DEFINED
#define SHARE_INFO_501_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_501_DEFINED")
cpp_quote("#define SHARE_INFO_501_DEFINED 1")
#endif

typedef struct _SHARE_INFO_501 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *shi501_netname;
    uint32 shi501_type;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *shi501_remark;
    uint32 shi502_flags;
} SHARE_INFO_501, *PSHARE_INFO_501, *LPSHARE_INFO_501;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SHARE_INFO_502_DEFINED
#define SHARE_INFO_502_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_502_DEFINED")
cpp_quote("#define SHARE_INFO_502_DEFINED 1")
#endif

typedef struct _SHARE_INFO_502 {
    wchar16_t *shi502_netname;
    uint32 shi502_type;
    wchar16_t *shi502_remark;
    uint32 shi502_permissions;
    uint32 shi502_max_uses;
    uint32 shi502_current_uses;
    wchar16_t *shi502_path;
    wchar16_t *shi502_password;
    uint32 shi502_reserved;
    PSECURITY_DESCRIPTOR shi502_security_descriptor;
} SHARE_INFO_502, *PSHARE_INFO_502, *LPSHARE_INFO_502;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SHARE_INFO_1004_DEFINED
#define SHARE_INFO_1004_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1004_DEFINED")
cpp_quote("#define SHARE_INFO_1004_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1004 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *shi1004_remark;
} SHARE_INFO_1004, *PSHARE_INFO_1004, *LPSHARE_INFO_1004;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SHARE_INFO_1005_DEFINED
#define SHARE_INFO_1005_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1005_DEFINED")
cpp_quote("#define SHARE_INFO_1005_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1005 {
    uint32 shi1005_flags;
} SHARE_INFO_1005, *PSHARE_INFO_1005, *LPSHARE_INFO_1005;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SHARE_INFO_1006_DEFINED
#define SHARE_INFO_1006_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1006_DEFINED")
cpp_quote("#define SHARE_INFO_1006_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1006 {
    uint32 shi1006_max_uses;
} SHARE_INFO_1006, *PSHARE_INFO_1006, *LPSHARE_INFO_1006;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SHARE_INFO_1501_DEFINED
#define SHARE_INFO_1501_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1501_DEFINED")
cpp_quote("#define SHARE_INFO_1501_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1501 {
    uint32 shi1501_reserved;
    PSECURITY_DESCRIPTOR shi1501_security_descriptor;
} SHARE_INFO_1501, *PSHARE_INFO_1501, *LPSHARE_INFO_1501;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_100_DEFINED
#define SERVER_INFO_100_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_100_DEFINED")
cpp_quote("#define SERVER_INFO_100_DEFINED 1")
#endif

typedef struct _SERVER_INFO_100 {
    uint32 sv100_platform_id;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv100_name;
} SERVER_INFO_100, *PSERVER_INFO_100, *LPSERVER_INFO_100;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_101_DEFINED
#define SERVER_INFO_101_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_101_DEFINED")
cpp_quote("#define SERVER_INFO_101_DEFINED 1")
#endif

typedef struct _SERVER_INFO_101 {
    uint32 sv101_platform_id;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv101_name;
    uint32 sv101_version_major;
    uint32 sv101_version_minor;
    uint32 sv101_type;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv101_comment;
} SERVER_INFO_101, *PSERVER_INFO_101, *LPSERVER_INFO_101;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_102_DEFINED
#define SERVER_INFO_102_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_102_DEFINED")
cpp_quote("#define SERVER_INFO_102_DEFINED 1")
#endif

typedef struct _SERVER_INFO_102 {
    uint32 sv102_platform_id;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv102_name;
    uint32 sv102_version_major;
    uint32 sv102_version_minor;
    uint32 sv102_type;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv102_comment;
    uint32 sv102_users;
    uint32 sv102_disc;
    uint32 sv102_hidden;
    uint32 sv102_announce;
    uint32 sv102_anndelta;
    uint32 sv102_licenses;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv102_userpath;
} SERVER_INFO_102, *PSERVER_INFO_102, *LPSERVER_INFO_102;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_402_DEFINED
#define SERVER_INFO_402_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_402_DEFINED")
cpp_quote("#define SERVER_INFO_402_DEFINED 1")
#endif

typedef struct _SERVER_INFO_402 {
    uint32 sv402_ulist_mtime;
    uint32 sv402_glist_mtime;
    uint32 sv402_alist_mtime;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv402_alerts;
    uint32 sv402_security;
    uint32 sv402_numadmin;
    uint32 sv402_lanmask;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv402_guestacct;
    uint32 sv402_chdevs;
    uint32 sv402_chdevq;
    uint32 sv402_chdevjobs;
    uint32 sv402_connections;
    uint32 sv402_shares;
    uint32 sv402_openfiles;
    uint32 sv402_sessopens;
    uint32 sv402_sesssvcs;
    uint32 sv402_sessreqs;
    uint32 sv402_opensearch;
    uint32 sv402_activelocks;
    uint32 sv402_numreqbuf;
    uint32 sv402_sizreqbuf;
    uint32 sv402_numbigbuf;
    uint32 sv402_numfiletasks;
    uint32 sv402_alertsched;
    uint32 sv402_erroralert;
    uint32 sv402_logonalert;
    uint32 sv402_accessalert;
    uint32 sv402_diskalert;
    uint32 sv402_netioalert;
    uint32 sv402_maxaudits;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv402_srvheuristics;
} SERVER_INFO_402, *PSERVER_INFO_402, *LPSERVER_INFO_402;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_403_DEFINED
#define SERVER_INFO_403_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_403_DEFINED")
cpp_quote("#define SERVER_INFO_403_DEFINED 1")
#endif

typedef struct _SERVER_INFO_403 {
    uint32 sv403_ulist_mtime;
    uint32 sv403_glist_mtime;
    uint32 sv403_alist_mtime;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv403_alerts;
    uint32 sv403_security;
    uint32 sv403_numadmin;
    uint32 sv403_lanmask;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv403_guestacct;
    uint32 sv403_chdevs;
    uint32 sv403_chdevq;
    uint32 sv403_chdevjobs;
    uint32 sv403_connections;
    uint32 sv403_shares;
    uint32 sv403_openfiles;
    uint32 sv403_sessopens;
    uint32 sv403_sesssvcs;
    uint32 sv403_sessreqs;
    uint32 sv403_opensearch;
    uint32 sv403_activelocks;
    uint32 sv403_numreqbuf;
    uint32 sv403_sizereqbuf;
    uint32 sv403_numbigbuf;
    uint32 sv403_numfiletasks;
    uint32 sv403_alertsched;
    uint32 sv403_erroralert;
    uint32 sv403_logonalert;
    uint32 sv403_accessalert;
    uint32 sv403_diskalert;
    uint32 sv403_netioalert;
    uint32 sv403_maxaudits;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv403_srvheuristics;
    uint32 sv403_auditedevents;
    uint32 sv403_auditprofile;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv403_autopath;
} SERVER_INFO_403, *PSERVER_INFO_403, *LPSERVER_INFO_403;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_502_DEFINED
#define SERVER_INFO_502_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_502_DEFINED")
cpp_quote("#define SERVER_INFO_502_DEFINED 1")
#endif

typedef struct _SERVER_INFO_502 {
    uint32 sv502_sessopens;
    uint32 sv502_sessvcs;
    uint32 sv502_opensearch;
    uint32 sv502_sizreqbuf;
    uint32 sv502_initworkitems;
    uint32 sv502_maxworkitems;
    uint32 sv502_rawworkitems;
    uint32 sv502_irpstacksize;
    uint32 sv502_maxrawbuflen;
    uint32 sv502_sessusers;
    uint32 sv502_sessconns;
    uint32 sv502_maxpagedmemoryusage;
    uint32 sv502_maxnonpagedmemoryusage;
    uint32 sv502_enablesoftcompat;
    uint32 sv502_enableforcedlogoff;
    uint32 sv502_timesource;
    uint32 sv502_acceptdownlevelapis;
    uint32 sv502_lmannounce;
} SERVER_INFO_502, *PSERVER_INFO_502, *LPSERVER_INFO_502;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_503_DEFINED
#define SERVER_INFO_503_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_503_DEFINED")
cpp_quote("#define SERVER_INFO_503_DEFINED 1")
#endif

typedef struct _SERVER_INFO_503 {
    uint32 sv503_sessopens;
    uint32 sv503_sessvcs;
    uint32 sv503_opensearch;
    uint32 sv503_sizreqbuf;
    uint32 sv503_initworkitems;
    uint32 sv503_maxworkitems;
    uint32 sv503_rawworkitems;
    uint32 sv503_irpstacksize;
    uint32 sv503_maxrawbuflen;
    uint32 sv503_sessusers;
    uint32 sv503_sessconns;
    uint32 sv503_maxpagedmemoryusage;
    uint32 sv503_maxnonpagedmemoryusage;
    uint32 sv503_enablesoftcompat;
    uint32 sv503_enableforcedlogoff;
    uint32 sv503_timesource;
    uint32 sv503_acceptdownlevelapis;
    uint32 sv503_lmannounce;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv503_domain;
    uint32 sv503_maxcopyreadlen;
    uint32 sv503_maxcopywritelen;
    uint32 sv503_minkeepsearch;
    uint32 sv503_maxkeepsearch;
    uint32 sv503_minkeepcomplsearch;
    uint32 sv503_maxkeepcomplsearch;
    uint32 sv503_threadcountadd;
    uint32 sv503_numblockthreads;
    uint32 sv503_scavtimeout;
    uint32 sv503_minrcvqueue;
    uint32 sv503_minfreeworkitems;
    uint32 sv503_xactmemsize;
    uint32 sv503_threadpriority;
    uint32 sv503_maxmpxct;
    uint32 sv503_oplockbreakwait;
    uint32 sv503_oplockbreakresponsewait;
    uint32 sv503_enableoplocks;
    uint32 sv503_enableoplockforceclose;
    uint32 sv503_enablefcbopens;
    uint32 sv503_enableraw;
    uint32 sv503_enablesharednetdrives;
    uint32 sv503_minfreeconnections;
    uint32 sv503_maxfreeconnections;
} SERVER_INFO_503, *PSERVER_INFO_503, *LPSERVER_INFO_503;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_599_DEFINED
#define SERVER_INFO_599_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_599_DEFINED")
cpp_quote("#define SERVER_INFO_599_DEFINED 1")
#endif

typedef struct _SERVER_INFO_599 {
    uint32 sv599_sessopens;
    uint32 sv599_sessvcs;
    uint32 sv599_opensearch;
    uint32 sv599_sizreqbuf;
    uint32 sv599_initworkitems;
    uint32 sv599_maxworkitems;
    uint32 sv599_rawworkitems;
    uint32 sv599_irpstacksize;
    uint32 sv599_maxrawbuflen;
    uint32 sv599_sessusers;
    uint32 sv599_sessconns;
    uint32 sv599_maxpagedmemoryusage;
    uint32 sv599_maxnonpagedmemoryusage;
    uint32 sv599_enablesoftcompat;
    uint32 sv599_enableforcedlogoff;
    uint32 sv599_timesource;
    uint32 sv599_acceptdownlevelapis;
    uint32 sv599_lmannounce;
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv599_domain;
    uint32 sv599_maxcopyreadlen;
    uint32 sv599_maxcopywritelen;
    uint32 sv599_minkeepsearch;
    uint32 sv599_maxkeepsearch;
    uint32 sv599_minkeepcomplsearch;
    uint32 sv599_maxkeepcomplsearch;
    uint32 sv599_threadcountadd;
    uint32 sv599_numblockthreads;
    uint32 sv599_scavtimeout;
    uint32 sv599_minrcvqueue;
    uint32 sv599_minfreeworkitems;
    uint32 sv599_xactmemsize;
    uint32 sv599_threadpriority;
    uint32 sv599_maxmpxct;
    uint32 sv599_oplockbreakwait;
    uint32 sv599_oplockbreakresponsewait;
    uint32 sv599_enableoplocks;
    uint32 sv599_enableoplockforceclose;
    uint32 sv599_enablefcbopens;
    uint32 sv599_enableraw;
    uint32 sv599_enablesharednetdrives;
    uint32 sv599_minfreeconnections;
    uint32 sv599_maxfreeconnections;
    uint32 sv599_initsesstable;
    uint32 sv599_initconntable;
    uint32 sv599_initfiletable;
    uint32 sv599_initsearchtable;
    uint32 sv599_alertschedule;
    uint32 sv599_errorthreshold;
    uint32 sv599_networkerrorthreshold;
    uint32 sv599_diskspacethreshold;
    uint32 sv599_reserved;
    uint32 sv599_maxlinkdelay;
    uint32 sv599_minlinkthroughput;
    uint32 sv599_linkinfovalidtime;
    uint32 sv599_scavqosinfoupdatetime;
    uint32 sv599_maxworkitemidletime;
} SERVER_INFO_599, *PSERVER_INFO_599, *LPSERVER_INFO_599;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1005_DEFINED
#define SERVER_INFO_1005_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1005_DEFINED")
cpp_quote("#define SERVER_INFO_1005_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1005 {
#ifdef _DCE_IDL_
    [string]
#endif
    wchar16_t *sv1005_comment;
} SERVER_INFO_1005, *PSERVER_INFO_1005, *LPSERVER_INFO_1005;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1010_DEFINED
#define SERVER_INFO_1010_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1010_DEFINED")
cpp_quote("#define SERVER_INFO_1010_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1010 {
    uint32 sv1010_disc;
} SERVER_INFO_1010, *PSERVER_INFO_1010, *LPSERVER_INFO_1010;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1016_DEFINED
#define SERVER_INFO_1016_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1016_DEFINED")
cpp_quote("#define SERVER_INFO_1016_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1016 {
    uint32 sv1016_hidden;
} SERVER_INFO_1016, *PSERVER_INFO_1016, *LPSERVER_INFO_1016;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1017_DEFINED
#define SERVER_INFO_1017_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1017_DEFINED")
cpp_quote("#define SERVER_INFO_1017_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1017 {
    uint32 sv1017_announce;
} SERVER_INFO_1017, *PSERVER_INFO_1017, *LPSERVER_INFO_1017;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1018_DEFINED
#define SERVER_INFO_1018_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1018_DEFINED")
cpp_quote("#define SERVER_INFO_1018_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1018 {
    uint32 sv1018_anndelta;
} SERVER_INFO_1018, *PSERVER_INFO_1018, *LPSERVER_INFO_1018;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1107_DEFINED
#define SERVER_INFO_1107_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1107_DEFINED")
cpp_quote("#define SERVER_INFO_1107_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1107 {
    uint32 sv1107_users;
} SERVER_INFO_1107, *PSERVER_INFO_1107, *LPSERVER_INFO_1107;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1501_DEFINED
#define SERVER_INFO_1501_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1501_DEFINED")
cpp_quote("#define SERVER_INFO_1501_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1501 {
    uint32 sv1501_sessopens;
} SERVER_INFO_1501, *PSERVER_INFO_1501, *LPSERVER_INFO_1501;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1502_DEFINED
#define SERVER_INFO_1502_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1502_DEFINED")
cpp_quote("#define SERVER_INFO_1502_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1502 {
    uint32 sv1502_sessvcs;
} SERVER_INFO_1502, *PSERVER_INFO_1502, *LPSERVER_INFO_1502;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1503_DEFINED
#define SERVER_INFO_1503_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1503_DEFINED")
cpp_quote("#define SERVER_INFO_1503_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1503 {
    uint32 sv1503_opensearch;
} SERVER_INFO_1503, *PSERVER_INFO_1503, *LPSERVER_INFO_1503;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1506_DEFINED
#define SERVER_INFO_1506_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1506_DEFINED")
cpp_quote("#define SERVER_INFO_1506_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1506 {
    uint32 sv1506_maxworkitems;
} SERVER_INFO_1506, *PSERVER_INFO_1506, *LPSERVER_INFO_1506;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1509_DEFINED
#define SERVER_INFO_1509_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1509_DEFINED")
cpp_quote("#define SERVER_INFO_1509_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1509 {
    uint32 sv1509_maxrawbuflen;
} SERVER_INFO_1509, *PSERVER_INFO_1509, *LPSERVER_INFO_1509;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1510_DEFINED
#define SERVER_INFO_1510_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1510_DEFINED")
cpp_quote("#define SERVER_INFO_1510_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1510 {
    uint32 sv1510_sessusers;
} SERVER_INFO_1510, *PSERVER_INFO_1510, *LPSERVER_INFO_1510;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1511_DEFINED
#define SERVER_INFO_1511_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1511_DEFINED")
cpp_quote("#define SERVER_INFO_1511_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1511 {
    uint32 sv1511_sessconns;
} SERVER_INFO_1511, *PSERVER_INFO_1511, *LPSERVER_INFO_1511;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1512_DEFINED
#define SERVER_INFO_1512_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1512_DEFINED")
cpp_quote("#define SERVER_INFO_1512_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1512 {
    uint32 sv1512_maxnonpagedmemoryusage;
} SERVER_INFO_1512, *PSERVER_INFO_1512, *LPSERVER_INFO_1512;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1513_DEFINED
#define SERVER_INFO_1513_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1513_DEFINED")
cpp_quote("#define SERVER_INFO_1513_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1513 {
    uint32 sv1513_maxpagedmemoryusage;
} SERVER_INFO_1513, *PSERVER_INFO_1513, *LPSERVER_INFO_1513;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1514_DEFINED
#define SERVER_INFO_1514_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1514_DEFINED")
cpp_quote("#define SERVER_INFO_1514_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1514 {
    uint32 sv1514_enablesoftcompat;
} SERVER_INFO_1514, *PSERVER_INFO_1514, *LPSERVER_INFO_1514;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1515_DEFINED
#define SERVER_INFO_1515_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1515_DEFINED")
cpp_quote("#define SERVER_INFO_1515_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1515 {
    uint32 sv1515_enableforcedlogoff;
} SERVER_INFO_1515, *PSERVER_INFO_1515, *LPSERVER_INFO_1515;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1516_DEFINED
#define SERVER_INFO_1516_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1516_DEFINED")
cpp_quote("#define SERVER_INFO_1516_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1516 {
    uint32 sv1516_timesource;
} SERVER_INFO_1516, *PSERVER_INFO_1516, *LPSERVER_INFO_1516;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1518_DEFINED
#define SERVER_INFO_1518_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1518_DEFINED")
cpp_quote("#define SERVER_INFO_1518_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1518 {
    uint32 sv1518_lmannounce;
} SERVER_INFO_1518, *PSERVER_INFO_1518, *LPSERVER_INFO_1518;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1520_DEFINED
#define SERVER_INFO_1520_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1520_DEFINED")
cpp_quote("#define SERVER_INFO_1520_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1520 {
    uint32 sv1520_maxcopyreadlen;
} SERVER_INFO_1520, *PSERVER_INFO_1520, *LPSERVER_INFO_1520;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1521_DEFINED
#define SERVER_INFO_1521_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1521_DEFINED")
cpp_quote("#define SERVER_INFO_1521_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1521 {
    uint32 sv1521_maxcopywritelen;
} SERVER_INFO_1521, *PSERVER_INFO_1521, *LPSERVER_INFO_1521;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1522_DEFINED
#define SERVER_INFO_1522_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1522_DEFINED")
cpp_quote("#define SERVER_INFO_1522_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1522 {
    uint32 sv1522_minkeepsearch;
} SERVER_INFO_1522, *PSERVER_INFO_1522, *LPSERVER_INFO_1522;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1523_DEFINED
#define SERVER_INFO_1523_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1523_DEFINED")
cpp_quote("#define SERVER_INFO_1523_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1523 {
    uint32 sv1523_maxkeepsearch;
} SERVER_INFO_1523, *PSERVER_INFO_1523, *LPSERVER_INFO_1523;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1524_DEFINED
#define SERVER_INFO_1524_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1524_DEFINED")
cpp_quote("#define SERVER_INFO_1524_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1524 {
    uint32 sv1524_minkeepcomplsearch;
} SERVER_INFO_1524, *PSERVER_INFO_1524, *LPSERVER_INFO_1524;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1525_DEFINED
#define SERVER_INFO_1525_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1525_DEFINED")
cpp_quote("#define SERVER_INFO_1525_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1525 {
    uint32 sv1525_maxkeepcomplsearch;
} SERVER_INFO_1525, *PSERVER_INFO_1525, *LPSERVER_INFO_1525;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1528_DEFINED
#define SERVER_INFO_1528_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1528_DEFINED")
cpp_quote("#define SERVER_INFO_1528_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1528 {
    uint32 sv1528_scavtimeout;
} SERVER_INFO_1528, *PSERVER_INFO_1528, *LPSERVER_INFO_1528;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1529_DEFINED
#define SERVER_INFO_1529_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1529_DEFINED")
cpp_quote("#define SERVER_INFO_1529_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1529 {
    uint32 sv1529_minrcvqueue;
} SERVER_INFO_1529, *PSERVER_INFO_1529, *LPSERVER_INFO_1529;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1530_DEFINED
#define SERVER_INFO_1530_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1530_DEFINED")
cpp_quote("#define SERVER_INFO_1530_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1530 {
    uint32 sv1530_minfreeworkitems;
} SERVER_INFO_1530, *PSERVER_INFO_1530, *LPSERVER_INFO_1530;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1533_DEFINED
#define SERVER_INFO_1533_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1533_DEFINED")
cpp_quote("#define SERVER_INFO_1533_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1533 {
    uint32 sv1533_maxmpxct;
} SERVER_INFO_1533, *PSERVER_INFO_1533, *LPSERVER_INFO_1533;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1534_DEFINED
#define SERVER_INFO_1534_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1534_DEFINED")
cpp_quote("#define SERVER_INFO_1534_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1534 {
    uint32 sv1534_oplockbreakwait;
} SERVER_INFO_1534, *PSERVER_INFO_1534, *LPSERVER_INFO_1534;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1535_DEFINED
#define SERVER_INFO_1535_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1535_DEFINED")
cpp_quote("#define SERVER_INFO_1535_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1535 {
    uint32 sv1535_oplockbreakresponsewait;
} SERVER_INFO_1535, *PSERVER_INFO_1535, *LPSERVER_INFO_1535;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1536_DEFINED
#define SERVER_INFO_1536_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1536_DEFINED")
cpp_quote("#define SERVER_INFO_1536_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1536 {
    uint32 sv1536_enableoplocks;
} SERVER_INFO_1536, *PSERVER_INFO_1536, *LPSERVER_INFO_1536;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1537_DEFINED
#define SERVER_INFO_1537_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1537_DEFINED")
cpp_quote("#define SERVER_INFO_1537_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1537 {
    uint32 sv1537_enableoplockforceclose;
} SERVER_INFO_1537, *PSERVER_INFO_1537, *LPSERVER_INFO_1537;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1538_DEFINED
#define SERVER_INFO_1538_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1538_DEFINED")
cpp_quote("#define SERVER_INFO_1538_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1538 {
    uint32 sv1538_enablefcbopens;
} SERVER_INFO_1538, *PSERVER_INFO_1538, *LPSERVER_INFO_1538;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1539_DEFINED
#define SERVER_INFO_1539_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1539_DEFINED")
cpp_quote("#define SERVER_INFO_1539_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1539 {
    uint32 sv1539_enableraw;
} SERVER_INFO_1539, *PSERVER_INFO_1539, *LPSERVER_INFO_1539;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1540_DEFINED
#define SERVER_INFO_1540_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1540_DEFINED")
cpp_quote("#define SERVER_INFO_1540_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1540 {
    uint32 sv1540_enablesharednetdrives;
} SERVER_INFO_1540, *PSERVER_INFO_1540, *LPSERVER_INFO_1540;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1541_DEFINED
#define SERVER_INFO_1541_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1541_DEFINED")
cpp_quote("#define SERVER_INFO_1541_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1541 {
    uint32 sv1541_minfreeconnections;
} SERVER_INFO_1541, *PSERVER_INFO_1541, *LPSERVER_INFO_1541;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1542_DEFINED
#define SERVER_INFO_1542_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1542_DEFINED")
cpp_quote("#define SERVER_INFO_1542_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1542 {
    uint32 sv1542_maxfreeconnections;
} SERVER_INFO_1542, *PSERVER_INFO_1542, *LPSERVER_INFO_1542;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1543_DEFINED
#define SERVER_INFO_1543_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1543_DEFINED")
cpp_quote("#define SERVER_INFO_1543_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1543 {
    uint32 sv1543_initsesstable;
} SERVER_INFO_1543, *PSERVER_INFO_1543, *LPSERVER_INFO_1543;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1544_DEFINED
#define SERVER_INFO_1544_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1544_DEFINED")
cpp_quote("#define SERVER_INFO_1544_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1544 {
    uint32 sv1544_initconntable;
} SERVER_INFO_1544, *PSERVER_INFO_1544, *LPSERVER_INFO_1544;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1545_DEFINED
#define SERVER_INFO_1545_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1545_DEFINED")
cpp_quote("#define SERVER_INFO_1545_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1545 {
    uint32 sv1545_initfiletable;
} SERVER_INFO_1545, *PSERVER_INFO_1545, *LPSERVER_INFO_1545;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1546_DEFINED
#define SERVER_INFO_1546_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1546_DEFINED")
cpp_quote("#define SERVER_INFO_1546_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1546 {
    uint32 sv1546_initsearchtable;
} SERVER_INFO_1546, *PSERVER_INFO_1546, *LPSERVER_INFO_1546;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1547_DEFINED
#define SERVER_INFO_1547_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1547_DEFINED")
cpp_quote("#define SERVER_INFO_1547_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1547 {
    uint32 sv1547_alertsched;
} SERVER_INFO_1547, *PSERVER_INFO_1547, *LPSERVER_INFO_1547;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1548_DEFINED
#define SERVER_INFO_1548_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1548_DEFINED")
cpp_quote("#define SERVER_INFO_1548_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1548 {
    uint32 sv1548_errorthreshold;
} SERVER_INFO_1548, *PSERVER_INFO_1548, *LPSERVER_INFO_1548;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1549_DEFINED
#define SERVER_INFO_1549_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1549_DEFINED")
cpp_quote("#define SERVER_INFO_1549_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1549 {
    uint32 sv1549_networkerrorthreshold;
} SERVER_INFO_1549, *PSERVER_INFO_1549, *LPSERVER_INFO_1549;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1550_DEFINED
#define SERVER_INFO_1550_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1550_DEFINED")
cpp_quote("#define SERVER_INFO_1550_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1550 {
    uint32 sv1550_diskspacethreshold;
} SERVER_INFO_1550, *PSERVER_INFO_1550, *LPSERVER_INFO_1550;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1552_DEFINED
#define SERVER_INFO_1552_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1552_DEFINED")
cpp_quote("#define SERVER_INFO_1552_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1552 {
    uint32 sv1552_maxlinkdelay;
} SERVER_INFO_1552, *PSERVER_INFO_1552, *LPSERVER_INFO_1552;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1553_DEFINED
#define SERVER_INFO_1553_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1553_DEFINED")
cpp_quote("#define SERVER_INFO_1553_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1553 {
    uint32 sv1553_minlinkthroughput;
} SERVER_INFO_1553, *PSERVER_INFO_1553, *LPSERVER_INFO_1553;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1554_DEFINED
#define SERVER_INFO_1554_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1554_DEFINED")
cpp_quote("#define SERVER_INFO_1554_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1554 {
    uint32 sv1554_linkinfovalidtime;
} SERVER_INFO_1554, *PSERVER_INFO_1554, *LPSERVER_INFO_1554;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1555_DEFINED
#define SERVER_INFO_1555_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1555_DEFINED")
cpp_quote("#define SERVER_INFO_1555_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1555 {
    uint32 sv1555_scavqosinfoupdatetime;
} SERVER_INFO_1555, *PSERVER_INFO_1555, *LPSERVER_INFO_1555;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SERVER_INFO_1556_DEFINED
#define SERVER_INFO_1556_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SERVER_INFO_1556_DEFINED")
cpp_quote("#define SERVER_INFO_1556_DEFINED 1")
#endif

typedef struct _SERVER_INFO_1556 {
    uint32 sv1556_maxworkitemidletime;
} SERVER_INFO_1556, *PSERVER_INFO_1556, *LPSERVER_INFO_1556;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef TIME_OF_DAY_INFO_DEFINED
#define TIME_OF_DAY_INFO_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef TIME_OF_DAY_INFO_DEFINED")
cpp_quote("#define TIME_OF_DAY_INFO_DEFINED 1")
#endif

typedef struct _TIME_OF_DAY_INFO {
    uint32 tod_elapsedt; /* time(NULL) */
    uint32 tod_msecs; /* milliseconds till system reboot (uptime) */
    uint32 tod_hours;
    uint32 tod_mins;
    uint32 tod_secs;
    uint32 tod_hunds;
    int32  tod_timezone; /* in minutes */
    uint32 tod_tinterval; /* clock tick interval in 0.0001 second units; 310 on windows */
    uint32 tod_day;
    uint32 tod_month;
    uint32 tod_year;
    uint32 tod_weekday;
} TIME_OF_DAY_INFO, *PTIME_OF_DAY_INFO, *LPTIME_OF_DAY_INFO;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif /* TIME_OF_DAY_INFO_DEFINED */

#endif /* _SRVSVCDEFS_H_ */
