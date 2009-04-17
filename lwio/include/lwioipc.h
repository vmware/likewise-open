/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#ifndef __LWIO_IPC_H__
#define __LWIO_IPC_H__

#include <config.h>

#define LWIO_SERVER_FILENAME CACHEDIR "/.lwiod"

#define MAP_LWMSG_STATUS(status) \
    SMBIPCMapLWMsgStatus(status);

#if defined(WORDS_BIGENDIAN)
#  define UCS2_NATIVE "UCS-2BE"
#else
#  define UCS2_NATIVE "UCS-2LE"
#endif

#define LWMSG_MEMBER_PWSTR(_type, _field)           \
    LWMSG_MEMBER_POINTER_BEGIN(_type, _field),      \
    LWMSG_UINT16(wchar16_t),                        \
    LWMSG_POINTER_END,                              \
    LWMSG_ATTR_ZERO_TERMINATED,                     \
    LWMSG_ATTR_ENCODING(UCS2_NATIVE)

#define LWMSG_MEMBER_PSECTOKEN(_type, _field)       \
    LWMSG_MEMBER_POINTER_BEGIN(_type, _field),      \
    LWMSG_TYPESPEC(gSecurityTokenRepSpec),          \
    LWMSG_POINTER_END

/*
 *  Protocol message enumeration
 */
typedef enum SMBIpcMessageType
{
    SMB_REFRESH_CONFIG,                   // SMB_REQUEST
    SMB_REFRESH_CONFIG_SUCCESS,           // SMB_STATUS_REPLY
    SMB_REFRESH_CONFIG_FAILED,            // SMB_STATUS_REPLY
    SMB_SET_LOG_INFO,                     // LWIO_LOG_INFO
    SMB_SET_LOG_INFO_SUCCESS,             // SMB_STATUS_REPLY
    SMB_SET_LOG_INFO_FAILED,              // SMB_STATUS_REPLY
    SMB_GET_LOG_INFO,                     // LWIO_LOG_INFO
    SMB_GET_LOG_INFO_SUCCESS,             // SMB_STATUS_REPLY
    SMB_GET_LOG_INFO_FAILED,              // SMB_STATUS_REPLY
} SMBIpcMessageType;

typedef struct _SMB_REQUEST
{
    DWORD dwCurTime;
} SMB_REQUEST, *PSMB_REQUEST;

/*
 *  Generic status code reply
 */
typedef struct _SMB_STATUS_REPLY
{
    DWORD dwError;
} SMB_STATUS_REPLY, *PSMB_STATUS_REPLY;

NTSTATUS
LwIoDaemonIpcAddProtocolSpec(
    IN OUT LWMsgProtocol* pProtocol
    );

NTSTATUS
LwIoDaemonIpcAddProtocolSpecEx(
    IN OUT LWMsgProtocol* pProtocol,
    OUT OPTIONAL PCSTR* ppszError
    );

DWORD
SMBIPCMapLWMsgStatus(
    LWMsgStatus status
    );

#endif /* __LWIO_IPC_H__ */
