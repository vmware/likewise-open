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

#ifndef __SMBIPC_H__
#define __SMBIPC_H__

#define SMB_SERVER_FILENAME CACHEDIR "/.lsmbd"

#define MAP_LWMSG_STATUS(status) \
    SMBIPCMapLWMsgStatus(status);

#define LWMSG_MEMBER_PWSTR(_type, _field)           \
    LWMSG_MEMBER_POINTER_BEGIN(_type, _field),      \
    LWMSG_UINT16(wchar16_t),                        \
    LWMSG_POINTER_END,                              \
    LWMSG_ATTR_STRING

#define LWMSG_MEMBER_PSECTOKEN(_type, _field)       \
    LWMSG_MEMBER_POINTER_BEGIN(_type, _field),      \
    LWMSG_TYPESPEC(gSecurityTokenRepSpec),          \
    LWMSG_POINTER_END

/*
 *  Protocol message enumeration
 */
typedef enum SMBIpcMessageType
{
    SMB_REFRESH_CONFIG,                   /* SMB_REFRESH_REQUEST       */
    SMB_REFRESH_CONFIG_SUCCESS,           /* SMB_STATUS_REPLY          */
    SMB_REFRESH_CONFIG_FAILED,            /* SMB_STATUS_REPLY          */
    SMB_SET_LOG_LEVEL,                    /* SMB_SET_LOG_LEVEL_REQUEST */
    SMB_SET_LOG_LEVEL_SUCCESS,            /* SMB_STATUS_REPLY          */
    SMB_SET_LOG_LEVEL_FAILED,             /* SMB_STATUS_REPLY          */
    SMB_GET_LOG_INFO,                     /* SMB_GET_LOG_INFO_REQUEST  */
    SMB_GET_LOG_INFO_SUCCESS,             /* SMB_LOG_INFO_RESPONSE     */
    SMB_GET_LOG_INFO_FAILED,              /* SMB_STATUS_REPLY          */
    SMB_CALL_NAMED_PIPE,                  /* SMB_CALL_NP_REQUEST       */
    SMB_CALL_NAMED_PIPE_SUCCESS,          /* SMB_CALL_NP_RESPONSE      */
    SMB_CALL_NAMED_PIPE_FAILED,           /* SMB_STATUS_REPLY          */
    SMB_CREATE_NAMED_PIPE,                /* SMB_CREATE_NP_REQUEST     */
    SMB_CREATE_NAMED_PIPE_SUCCESS,        /* SMB_HANDLE_RESPONSE       */
    SMB_CREATE_NAMED_PIPE_FAILED,         /* SMB_STATUS_REPLY          */
    SMB_GET_NAMED_PIPE_INFO,              /* SMB_FILE_HANDLE           */
    SMB_GET_NAMED_PIPE_INFO_SUCCESS,      /* SMB_NAMED_PIPE_INFO       */
    SMB_GET_NAMED_PIPE_INFO_FAILED,       /* SMB_STATUS_REPLY          */
    SMB_CONNECT_NAMED_PIPE,               /* SMB_FILE_HANDLE           */
    SMB_CONNECT_NAMED_PIPE_SUCCESS,       /* SMB_STATUS_REPLY          */
    SMB_CONNECT_NAMED_PIPE_FAILED,        /* SMB_STATUS_REPLY          */
    SMB_TRANSACT_NAMED_PIPE,              /* SMB_TRASACT_NP_REQUEST    */
    SMB_TRANSACT_NAMED_PIPE_SUCCESS,      /* SMB_CALL_NP_RESPONSE      */
    SMB_TRANSACT_NAMED_PIPE_FAILED,       /* SMB_STATUS_REPLY          */
    SMB_WAIT_NAMED_PIPE,                  /* SMB_WAIT_NP_REQUEST       */
    SMB_WAIT_NAMED_PIPE_SUCCESS,          /* SMB_STATUS_REPLY          */
    SMB_WAIT_NAMED_PIPE_FAILED,           /* SMB_STATUS_REPLY          */
    SMB_GET_CLIENT_COMPUTER_NAME,         /* SMB_GET_CLIENT_COMPUTER_NAME_REQUEST */
    SMB_GET_CLIENT_COMPUTER_NAME_SUCCESS, /* SMB_GET_CLIENT_COMPUTER_NAME_RESPONSE */
    SMB_GET_CLIENT_COMPUTER_NAME_FAILED,  /* SMB_STATUS_REPLY     */
    SMB_GET_CLIENT_PROCESS_ID,            /* SMB_FILE_HANDLE         */
    SMB_GET_CLIENT_PROCESS_ID_SUCCESS,    /* SMB_ID_REPLY            */
    SMB_GET_CLIENT_PROCESS_ID_FAILED,     /* SMB_STATUS_REPLY        */
    SMB_GET_SERVER_PROCESS_ID,            /* SMB_FILE_HANDLE         */
    SMB_GET_SERVER_PROCESS_ID_SUCCESS,    /* SMB_ID_REPLY            */
    SMB_GET_SERVER_PROCESS_ID_FAILED,     /* SMB_STATUS_REPLY        */
    SMB_GET_CLIENT_SESSION_ID,            /* SMB_FILE_HANDLE         */
    SMB_GET_CLIENT_SESSION_ID_SUCCESS,    /* SMB_ID_REPLY            */
    SMB_GET_CLIENT_SESSION_ID_FAILED,     /* SMB_STATUS_REPLY        */
    SMB_PEEK_NAMED_PIPE,                  /* SMB_PEEK_NP_REQUEST     */
    SMB_PEEK_NAMED_PIPE_SUCCESS,          /* SMB_PEEK_NP_RESPONSE    */
    SMB_PEEK_NAMED_PIPE_FAILED,           /* SMB_STATUS_REPLY        */
    SMB_DISCONNECT_NAMED_PIPE,            /* SMB_FILE_HANDLE         */
    SMB_DISCONNECT_NAMED_PIPE_SUCCESS,    /* SMB_STATUS_REPLY        */
    SMB_DISCONNECT_NAMED_PIPE_FAILED,     /* SMB_STATUS_REPLY        */
    SMB_CREATE_FILE,                      /* SMB_CREATE_FILE_REQUEST */
    SMB_CREATE_FILE_SUCCESS,              /* SMB_FILE_HANDLE         */
    SMB_CREATE_FILE_FAILED,               /* SMB_STATUS_REPLY        */
    SMB_SET_NAMED_PIPE_HANDLE_STATE,      /* SMB_SETNAMEDPIPEHANDLESTATE_REQUEST */
    SMB_SET_NAMED_PIPE_HANDLE_STATE_SUCCESS, /* SMB_STATUS_REPLY     */
    SMB_SET_NAMED_PIPE_HANDLE_STATE_FAILED,  /* SMB_STATUS_REPLY     */
    SMB_READ_FILE,                        /* SMB_READ_FILE_REQUEST   */
    SMB_READ_FILE_SUCCESS,                /* SMB_READ_FILE_RESPONSE  */
    SMB_READ_FILE_FAILED,                 /* SMB_STATUS_REPLY        */
    SMB_WRITE_FILE,                       /* SMB_WRITE_FILE_REQUEST  */
    SMB_WRITE_FILE_SUCCESS,               /* SMB_WRITE_FILE_RESPONSE */
    SMB_WRITE_FILE_FAILED,                /* SMB_STATUS_REPLY        */
    SMB_CLOSE_FILE,                       /* SMB_FILE_HANDLE         */
    SMB_CLOSE_FILE_SUCCESS,               /* SMB_STATUS_REPLY        */
    SMB_CLOSE_FILE_FAILED,                /* SMB_STATUS_REPLY        */
    SMB_GET_SESSION_KEY,                  /* SMB_FILE_HANDLE         */
    SMB_GET_SESSION_KEY_SUCCESS,          /* SMB_SESSION_KEY_RESPONSE */
    SMB_GET_SESSION_KEY_FAILED            /* SMB_STATUS_REPLY        */
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

typedef struct _SMB_ID_REPLY
{
    DWORD dwId;
} SMB_ID_REPLY, *PSMB_ID_REPLY;

typedef struct _SMB_CALL_NP_REQUEST
{
    PSMB_SECURITY_TOKEN_REP pSecurityToken;
    LPWSTR   pwszNamedPipeName;
    DWORD    dwInBufferSize;
    PBYTE    pInBuffer;
    DWORD    dwOutBufferSize;
    DWORD    dwTimeout;
} SMB_CALL_NP_REQUEST, *PSMB_CALL_NP_REQUEST;

typedef struct _SMB_CALL_NP_RESPONSE
{
    PBYTE pOutBuffer;
    DWORD dwOutBufferSize;
} SMB_CALL_NP_RESPONSE, *PSMB_CALL_NP_RESPONSE;

typedef struct _SMB_CREATE_NP_REQUEST
{
    PSMB_SECURITY_TOKEN_REP pSecurityToken;
    LPWSTR   pwszName;
    DWORD    dwOpenMode;
    DWORD    dwPipeMode;
    DWORD    dwMaxInstances;
    DWORD    dwOutBufferSize;
    DWORD    dwInBufferSize;
    DWORD    dwDefaultTimeOut;
    PSECURITY_ATTRIBUTES pSecurityAttributes;
} SMB_CREATE_NP_REQUEST, *PSMB_CREATE_NP_REQUEST;

typedef struct _SMB_NP_INFO
{
    DWORD dwFlags;
    DWORD dwOutBufferSize;
    DWORD dwInBufferSize;
    DWORD dwMaxInstances;
} SMB_NP_INFO, *PSMB_NP_INFO;

typedef struct _SMB_TRANSACT_NP_REQUEST
{
    PSMB_FILE_HANDLE hNamedPipe;
    DWORD    dwInBufferSize;
    PBYTE    pInBuffer;
    DWORD    dwOutBufferSize;
} SMB_TRANSACT_NP_REQUEST, *PSMB_TRANSACT_NP_REQUEST;

typedef struct _SMB_WAIT_NP_REQUEST
{
    PSMB_SECURITY_TOKEN_REP pSecurityToken;
    LPWSTR pwszName;
    DWORD    dwTimeout;
} SMB_WAIT_NP_REQUEST, *PSMB_WAIT_NP_REQUEST;

typedef struct _SMB_GET_CLIENT_COMPUTER_NAME_REQUEST
{
    PSMB_FILE_HANDLE hNamedPipe;
    DWORD dwComputerNameMaxSize;
} SMB_GET_CLIENT_COMPUTER_NAME_REQUEST, *PSMB_GET_CLIENT_COMPUTER_NAME_REQUEST;

typedef struct _SMB_GET_CLIENT_COMPUTER_NAME_RESPONSE
{
    LPWSTR pwszName;
    DWORD    dwLength;
} SMB_GET_CLIENT_COMPUTER_NAME_RESPONSE, *PSMB_GET_CLIENT_COMPUTER_NAME_RESPONSE;

typedef struct _SMB_PEEK_NP_REQUEST
{
    PSMB_FILE_HANDLE hNamedPipe;
    DWORD dwInBufferSize;
    PBYTE pInBuffer;
} SMB_PEEK_NP_REQUEST, *PSMB_PEEK_NP_REQUEST;

typedef struct _SMB_PEEK_NP_RESPONSE
{
    DWORD dwBytesRead;
    DWORD dwTotalBytesAvail;
    DWORD dwBytesLeftThisMessage;
} SMB_PEEK_NP_RESPONSE, *PSMB_PEEK_NP_RESPONSE;

typedef struct _SMB_CREATE_FILE_REQUEST
{
    PSMB_SECURITY_TOKEN_REP pSecurityToken;
    LPWSTR   pwszFileName;
    DWORD    dwDesiredAccess;
    DWORD    dwSharedMode;
    DWORD    dwCreationDisposition;
    DWORD    dwFlagsAndAttributes;
    PSECURITY_ATTRIBUTES pSecurityAttributes;
} SMB_CREATE_FILE_REQUEST, *PSMB_CREATE_FILE_REQUEST;

typedef struct _SMB_SETNAMEDPIPEHANDLESTATE_REQUEST
{
    PSMB_FILE_HANDLE hPipe;
    PDWORD             pdwMode;
    PDWORD             pdwCollectionCount;
    PDWORD             pdwTimeout;
} SMB_SETNAMEDPIPEHANDLESTATE_REQUEST, *PSMB_SETNAMEDPIPEHANDLESTATE_REQUEST;

typedef struct _SMB_READ_FILE_REQUEST
{
    PSMB_FILE_HANDLE hFile;
    DWORD            dwBytesToRead;
} SMB_READ_FILE_REQUEST, *PSMB_READ_FILE_REQUEST;

typedef struct _SMB_WRITE_FILE_REQUEST
{
    PSMB_FILE_HANDLE hFile;
    DWORD            dwBytesToWrite;
    PBYTE            pBuffer;
} SMB_WRITE_FILE_REQUEST, *PSMB_WRITE_FILE_REQUEST;

typedef struct _SMB_WRITE_FILE_RESPONSE
{
    DWORD            dwBytesWritten;
} SMB_WRITE_FILE_RESPONSE, *PSMB_WRITE_FILE_RESPONSE;

typedef struct _SMB_READ_FILE_RESPONSE
{
    DWORD dwBytesRead;
    PBYTE pBuffer;
} SMB_READ_FILE_RESPONSE, *PSMB_READ_FILE_RESPONSE;

typedef struct _SMB_GET_SESSION_KEY_RESPONSE
{
    DWORD dwSessionKeyLength;
    PBYTE pSessionKey;
} SMB_GET_SESSION_KEY_RESPONSE, *PSMB_GET_SESSION_KEY_RESPONSE;

DWORD
SMBIPCGetProtocolSpec(
    LWMsgProtocolSpec** ppProtocolSpec
    );

DWORD
SMBIPCMapLWMsgStatus(
    LWMsgStatus status
    );

#endif /* __SMBIPC_H__ */
