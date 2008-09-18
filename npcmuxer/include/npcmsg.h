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

#ifndef __NPC_MSG_H__
#define __NPC_MSG_H__

#include <npctypes.h>

#define DAEMON_NAME "npcmuxer"

#define PID_DIR "/var/run"

#define SYSLOG_ID DAEMON_NAME
#define PID_FILE PID_DIR "/" "npcmuxd.pid"
#define SERVER_PATH "/tmp/." DAEMON_NAME ".server"
#define CLIENT_PREFIX "/tmp/." DAEMON_NAME ".client"
#define SEC_SOCKET_PREFIX "/tmp/." DAEMON_NAME ".auth"

#define NPC_MSG_VERSION 17

#define NPC_MSG_TYPE_STATUS                  1 /* reply only */
#define NPC_MSG_TYPE_CONNECT_CHECK_CREDS     2 /* CONNECT_CHECK_CREDS -> STATUS */
#define NPC_MSG_TYPE_CONNECT                 3 /* CONNECT -> STATUS */
#define NPC_MSG_TYPE_AUTH_SET                4 /* AUTH_INFO -> STATUS */
#define NPC_MSG_TYPE_AUTH_CLEAR              5 /* STRING -> STATUS */
#define NPC_MSG_TYPE_SESSION_KEY             6 /* reply SESSION_KEY */
#define NPC_MSG_TYPE_SEC_SOCKET_INFO         7 /* SOCKET INFO -> NONCE */
#define NPC_MSG_TYPE_SEC_SOCKET_REP          8 /* reply NONCE */
#define NPC_MSG_TYPE_SEC_SOCKET_NONCE        9 /* confirm NONCE */
#define NPC_MSG_TYPE_CREATE_IMP_TOKEN       10 /* () -> TOKEN */
#define NPC_MSG_TYPE_IMP_TOKEN_REP          11 /* reply TOKEN */
#if 0
#define NPC_MSG_TYPE_AUTH_ENUM_SERVERS       5 /* N/A -> AUTH_ENUM_SERVERS */
#define NPC_MSG_TYPE_AUTH_ENUM               5 /* (optional uid) -> AUTH_ENUM */
#define NPC_MSG_TYPE_CONNECTION_ENUM         5 /* (optional uid) -> CONNECTION_ENUM */
#endif

#define ANY_SIZE 1

typedef struct _NPC_MSG_PAYLOAD_STATUS {
    CT_STATUS Status;
} NPC_MSG_PAYLOAD_STATUS;

typedef struct _NPC_MSG_PAYLOAD_CONNECT_CHECK_CREDS {
    uint32_t ProtocolSize;
    uint32_t AddressSize;
    uint32_t EndpointSize;
    uint32_t OptionsSize;
    uint32_t UsernameSize;
    uint32_t PasswordSize;
    uint32_t CredCacheSize;
    NPC_AUTH_FLAGS AuthFlags;
    char Data[ANY_SIZE];
    // Protocol
    // Address
    // EndPoint
    // Options
    // Username
    // Password
    // CredCache
} NPC_MSG_PAYLOAD_CONNECT_CHECK_CREDS;

#define NPC_IS_SIZE_OK_MSG_PAYLOAD_CONNECT_CHECK_CREDS(Message, Size) \
    (((Size) >= CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_CONNECT_CHECK_CREDS, Data)) && \
     ((Size) >= (CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_CONNECT_CHECK_CREDS, Data) + \
                 (Message)->ProtocolSize + \
                 (Message)->AddressSize + \
                 (Message)->EndpointSize + \
                 (Message)->OptionsSize + \
                 (Message)->UsernameSize + \
                 (Message)->PasswordSize + \
                 (Message)->CredCacheSize)))

typedef struct _NPC_MSG_PAYLOAD_CONNECT {
    uint32_t ProtocolSize;
    uint32_t AddressSize;
    uint32_t EndpointSize;
    uint32_t OptionsSize;
    uint32_t CredCacheSize;
    NPC_TOKEN_ID Token;
    char Data[ANY_SIZE];
    // Protocol
    // Address
    // EndPoint
    // Options
    // CredCache
} NPC_MSG_PAYLOAD_CONNECT;

#define NPC_IS_SIZE_OK_MSG_PAYLOAD_CONNECT(Message, Size) \
    (((Size) >= CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_CONNECT, Data)) && \
     ((Size) >= (CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_CONNECT, Data) + \
                 (Message)->ProtocolSize + \
                 (Message)->AddressSize + \
                 (Message)->EndpointSize + \
                 (Message)->OptionsSize + \
                 (Message)->CredCacheSize)))

typedef struct _NPC_MSG_PAYLOAD_AUTH_CLEAR {
    uint32_t ServerSize;
    NPC_TOKEN_ID Token;
    char Data[ANY_SIZE];
} NPC_MSG_PAYLOAD_AUTH_CLEAR;

#define NPC_IS_SIZE_OK_MSG_PAYLOAD_AUTH_CLEAR(Message, Size) \
    (((Size) >= CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_AUTH_CLEAR, Data)) && \
     ((Size) >= (CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_AUTH_CLEAR, Data) + \
                 (Message)->ServerSize)))

typedef struct _NPC_MSG_PAYLOAD_AUTH_INFO {
    uint32_t ServerSize;
    uint32_t UsernameSize;
    uint32_t PasswordSize;
    uint32_t CredCacheSize;
    NPC_AUTH_FLAGS AuthFlags;
    NPC_TOKEN_ID Token;
    char Data[ANY_SIZE];
    // Server
    // Username
    // Password
    // CredCache
} NPC_MSG_PAYLOAD_AUTH_INFO;

#define NPC_IS_SIZE_OK_MSG_PAYLOAD_AUTH_INFO(Message, Size) \
    (((Size) >= CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_AUTH_INFO, Data)) && \
     ((Size) >= (CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_AUTH_INFO, Data) + \
                 (Message)->ServerSize + \
                 (Message)->UsernameSize + \
                 (Message)->PasswordSize + \
                 (Message)->CredCacheSize)))

typedef struct _NPC_MSG_PAYLOAD_SESSION_KEY {
    CT_STATUS Status;
    uint32_t SessionKeyLen;
    char Data[ANY_SIZE];
    // SessionKey
} NPC_MSG_PAYLOAD_SESSION_KEY;


typedef struct _NPC_MSG_SEC_SOCKET_INFO {
    uint32_t SocketNameSize;
    uint32_t SocketOwnerUid;
    char Data[ANY_SIZE];
} NPC_MSG_PAYLOAD_SEC_SOCKET_INFO;

#define NPC_IS_SIZE_OK_MSG_PAYLOAD_SEC_SOCKET_INFO(Message, Size) \
    (((Size) >= CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_SEC_SOCKET_INFO, Data)) && \
     ((Size) >= (CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_SEC_SOCKET_INFO, Data) + \
	       (Message)->SocketNameSize)))

typedef struct _NPC_MSG_SEC_SOCKET_REP {
    CT_STATUS Status;
    uint32_t Nonce;
} NPC_MSG_PAYLOAD_SEC_SOCKET_REP;

typedef struct _NPC_MSG_IMP_TOKEN_REP {
    CT_STATUS Status;
    NPC_TOKEN_ID Token;
} NPC_MSG_PAYLOAD_IMP_TOKEN_REP;

typedef struct _NPC_MSG_SEC_SOCKET_NONCE {
    uint32_t Nonce;
} NPC_MSG_PAYLOAD_SEC_SOCKET_NONCE;


#endif /* __NPC_MSG_H__ */
