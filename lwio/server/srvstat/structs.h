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

typedef struct
{
    PSTR    pszIdentifier;
    BOOLEAN bOpened;
    ULONG   ulFacility;
    ULONG   ulOptions;
} SRV_STAT_HANDLER_SYS_LOG, *PSRV_STAT_HANDLER_SYS_LOG;

typedef struct
{
    PSTR  pszFilePath;
    FILE* fp;
} SRV_STAT_HANDLER_FILE_LOG, *PSRV_STAT_HANDLER_FILE_LOG;

typedef struct _SRV_STAT_REQUEST_CONTEXT
{
    SRV_STAT_SMB_VERSION     protocolVersion;

    SRV_STAT_CONNECTION_INFO connInfo;

    LONG64                   requestStartTime;
    LONG64                   requestEndTime;

} SRV_STAT_REQUEST_CONTEXT, *PSRV_STAT_REQUEST_CONTEXT;

typedef struct _SRV_STAT_HANDLER_CONFIG
{
    SRV_STAT_LOG_TARGET_TYPE logTargetType;

    PSTR                     pszPath;

} SRV_STAT_HANDLER_CONFIG, *PSRV_STAT_HANDLER_CONFIG;

typedef struct _SRV_STAT_HANDLER_GLOBALS
{
    SRV_STAT_HANDLER_CONFIG               config;
    LWIO_SRV_STAT_PROVIDER_FUNCTION_TABLE fnTable;

    union
    {
        PSRV_STAT_HANDLER_SYS_LOG  pSysLog;
        PSRV_STAT_HANDLER_FILE_LOG pFileLog;
    };

} SRV_STAT_HANDLER_GLOBALS, *PSRV_STAT_HANDLER_GLOBALS;
