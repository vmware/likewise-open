/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Reference Statistics Logging Module (SRV)
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

typedef struct _SRV_STAT_HANDLER_SYS_LOG
{
    PSTR    pszIdentifier;
    BOOLEAN bOpened;
    ULONG   ulFacility;
    ULONG   ulOptions;
} SRV_STAT_HANDLER_SYS_LOG, *PSRV_STAT_HANDLER_SYS_LOG;

typedef struct _SRV_STAT_HANDLER_FILE_LOG
{
    PSTR  pszFilePath;
    FILE* fp;
} SRV_STAT_HANDLER_FILE_LOG, *PSRV_STAT_HANDLER_FILE_LOG;

typedef struct _SRV_STAT_HANDLER_LOGGER
{
    SRV_STAT_LOG_TARGET_TYPE   logTargetType;
    PSRV_STAT_HANDLER_FILE_LOG pFileLog;

} SRV_STAT_HANDLER_LOGGER, *PSRV_STAT_HANDLER_LOGGER;

typedef struct _SRV_STAT_HANDLER_VALUE
{
    SRV_STAT_HANDLER_VALUE_TYPE valueType;
    SRV_STAT_PRINT_FLAG         ulPrintFlags;

    union
    {
        PULONG           pulValue;
        PLONG            plValue;
        PLONG64          pllValue;
        PSTR             pszValue;
        struct sockaddr* pSockAddr;
    } val;

} SRV_STAT_HANDLER_VALUE, *PSRV_STAT_HANDLER_VALUE;

typedef struct _SRV_STAT_MESSAGE_CONTEXT
{
    ULONG  ulOpcode;
    ULONG  ulSubOpcode;
    ULONG  ulIOCTLcode;

    LONG64 llMsgStartTime;
    LONG64 llMsgEndTime;

    ULONG  ulMessageRequestLength;
    ULONG  ulMessageResponseLength;

    NTSTATUS responseStatus;

    ULONG    ulFlags;

    struct _SRV_STAT_MESSAGE_CONTEXT* pNext;

} SRV_STAT_MESSAGE_CONTEXT, *PSRV_STAT_MESSAGE_CONTEXT;

typedef struct _SRV_STAT_REQUEST_CONTEXT
{
    pthread_mutex_t           mutex;
    pthread_mutex_t*          pMutex;

    SRV_STAT_SMB_VERSION      protocolVersion;

    struct {
        struct sockaddr* pClientAddress;
        size_t           clientAddrLen;
        struct sockaddr* pServerAddress;
        size_t           serverAddrLen;
        ULONG            ulResourceId;
    } connInfo;

    struct
    {
        PWSTR   pwszUserPrincipal;
        ULONG   ulUid;
        ULONG   ulGid;
        ULONG64 ullSessionId;
    } sessionInfo;

    LONG64                    llRequestStartTime;
    LONG64                    llRequestEndTime;

    ULONG                     ulRequestLength;
    ULONG                     ulResponseLength;

    PSRV_STAT_MESSAGE_CONTEXT pMessageStack;
    PSRV_STAT_MESSAGE_CONTEXT pCurrentMessage;

} SRV_STAT_REQUEST_CONTEXT, *PSRV_STAT_REQUEST_CONTEXT;

typedef struct _SRV_STAT_HANDLER_CONFIG
{
    SRV_STAT_LOG_TARGET_TYPE logTargetType;

    PSTR                     pszPath;

} SRV_STAT_HANDLER_CONFIG, *PSRV_STAT_HANDLER_CONFIG;

typedef struct _SRV_STAT_HANDLER_GLOBALS
{
    pthread_mutex_t                       mutex;

    SRV_STAT_HANDLER_CONFIG               config;
    LWIO_SRV_STAT_PROVIDER_FUNCTION_TABLE fnTable;

    PSRV_STAT_HANDLER_LOGGER              pLogger;

} SRV_STAT_HANDLER_GLOBALS, *PSRV_STAT_HANDLER_GLOBALS;
