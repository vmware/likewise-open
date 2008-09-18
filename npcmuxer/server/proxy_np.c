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

/* ex: set shiftwidth=4 expandtab softtabstop=4: */

#ifdef HAVE_CONFIG_H
#    include <config.h>
#endif

#include "proxy_np.h"
#include <sys/time.h>
#include <libsmbclient.h>
#include <ctlogger.h>
#ifdef HAVE_STRING_H
#    include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#    include <strings.h>
#endif
#include <ctgoto.h>
#include <stdio.h>
#include <ctmemory.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctstring.h>
#include <cttypes.h>
#include <ctserver.h>
#include <time.h>

#ifdef __APPLE__
/* Darwin is stubborn about providing extensions */
int strncasecmp(const char *s1, const char *s2, size_t len);
#endif

#ifdef WORDS_BIGENDIAN
#define NATIVE_BYTE_ORDER 0
#else
#define NATIVE_BYTE_ORDER 1
#endif

#define PACKET_BYTE_ORDER(pkt) (((pkt)->drep[0] >> 4) & 1)

/* 3 gives some, 10 gives packet-level details */
#define PROXY_NP_SMB_DEBUG_LEVEL 3

/* Disable anonymous logons (at least for now) */
#define PROXY_NP_SMB_CTX_FLAGS_DEFAULT ( \
    SMB_CTX_FLAG_USE_KERBEROS | \
    SMB_CTX_FLAG_FALLBACK_AFTER_KERBEROS | \
    0 )
    /* SMBCCTX_FLAG_NO_AUTO_ANONYMOUS_LOGON | \ */

/***************************************************************************/
/* SMB Defines */
/***************************************************************************/

#define GENERIC_WRITE_ACCESS   0x40000000 /* (1<<30) */
#define GENERIC_READ_ACCESS    ((unsigned)0x80000000) /* (((unsigned)1)<<31) */

#define FILE_SHARE_READ 1
#define FILE_SHARE_WRITE 2

#define OPENX_FILE_EXISTS_OPEN 1
#define OPEN_EXISTING OPENX_FILE_EXISTS_OPEN

#define FILE_ATTRIBUTE_NORMAL   0x080L


/***************************************************************************/
/* Proxy State */
/***************************************************************************/

typedef struct {
    /* Allocated by state: */
    SMBCCTX* SmbContext;
    SMBCFILE* File;
    /* Not owned by state: */
    int Fd;
    PROXY_CONNECTION_HANDLE Connection;
} NP_PROXY_STATE;

/***************************************************************************/
/* Authentication Support */
/***************************************************************************/

#define GAD_ZERO(String, Size)                  \
    do {                                        \
        if ((Size) > 0)                         \
        {                                       \
            *(String) = 0;                      \
        }                                       \
    } while (0)

#define GAD_COPY(Dest, Size, Source)            \
    do {                                        \
        GAD_ZERO(Dest, Size);                   \
        if ((Source) && (Size) > 0)             \
        {                                       \
            strncpy(Dest, Source, (Size)-1);    \
            (Dest)[Size-1] = 0;                 \
        }                                       \
    } while (0)

static
void
NppGetAuthDataWithCcName(
    IN SMBCCTX* Context,
    IN const char* Server,
    IN const char* Share,
    OUT char* Workgroup,
    IN int WorkgroupSize,
    OUT char* Username,
    IN int UsernameSize,
    OUT char* Password,
    IN int PasswordSize,
    OUT char* CcName,
    IN int CcNameSize
    )
{
    NP_PROXY_STATE* state = smbc_option_get(Context, "user_data");

    CT_LOG_TRACE("called for '%s' '%s'", Server, Share);

    GAD_ZERO(Workgroup, WorkgroupSize);
    GAD_ZERO(Username, UsernameSize);
    GAD_ZERO(Password, PasswordSize);
    GAD_ZERO(CcName, CcNameSize);

    if (state)
    {
        CT_STATUS status;
        const char* username = NULL;
        const char* password = NULL;
        const char* credCache = NULL;

        status = ProxyConnectionGetAuthInfo(state->Connection,
                                            NULL,
                                            &username,
                                            &password,
                                            &credCache);

        CT_LOG_TRACE("Server = '%s', status = 0x%08x, Username = '%s', "
                     "Password = '%s', CcName = '%s', Context = '%p'", CT_LOG_WRAP_STRING(Server),
                     status, CT_LOG_WRAP_STRING(username),
                     password ? ((*password) ? "*" : "") : "(null)",
                     credCache, Context);

        if (CT_STATUS_IS_OK(status))
        {
            if (!username && (Context->flags & SMB_CTX_FLAG_USE_KERBEROS))
            {
                /* Samba will do anonymous auth if no username is provided and
                   skip Kerberos altogether.  We do not want that. */
                username = " ";
            }

            GAD_COPY(Username, UsernameSize, username);
            GAD_COPY(Password, PasswordSize, password);
            GAD_COPY(CcName, CcNameSize, credCache);
        }
    }
}

/***************************************************************************/
/* SMB Context Management */
/***************************************************************************/

void
NppDestroySmbContext(
    IN SMBCCTX* SmbContext
    )
{
    if (SmbContext)
    {
        int hadError = smbc_free_context(SmbContext, 0);
        if (hadError)
        {
            CT_LOG_WARN("Shutting down SMB context aggressively");
            smbc_free_context(SmbContext, 1);
        }
    }
}

CT_STATUS
NppCreateSmbContext(
    OUT SMBCCTX** SmbContext,
    IN int Flags,
    IN void* UserData
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    SMBCCTX* smbContext = NULL;

    smbContext = smbc_new_context();
    if (!smbContext)
    {
        status = CT_STATUS_OUT_OF_MEMORY;
        GOTO_CLEANUP();
    }

    smbContext->debug = PROXY_NP_SMB_DEBUG_LEVEL;
    smbContext->flags = Flags;

    smbc_option_set(smbContext, "auth_ccname_function", NppGetAuthDataWithCcName);
    smbc_option_set(smbContext, "user_data", UserData);

    if (!smbc_init_context(smbContext))
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP();
    }

cleanup:
    if (status)
    {
        if (smbContext)
        {
            NppDestroySmbContext(smbContext);
            smbContext = NULL;
        }
    }

    *SmbContext = smbContext;

    return status;
}


/***************************************************************************/
/* RPC Packet Support */
/***************************************************************************/

/* GCC doesn't like #pragma pack on solaris */
#ifndef __GNUC__
#pragma pack(push,1)
#endif
typedef struct
{
    uint8_t rpc_vers;
    uint8_t rpc_vers_minor;
    uint8_t ptype;
    uint8_t flags;
    uint8_t drep[4];
    unsigned short frag_len;
    unsigned short auth_len;
    uint32_t call_id;
} 
#ifdef __GNUC__
__attribute__((__packed__))
#endif
rpc_cn_common_hdr_t;
#ifndef __GNUC__
#pragma pack(pop)
#endif

#define RPC_C_CN_FLAGS_LAST_FRAG        0x02    /* Last fragment */

static size_t GetPacketSize(char *_Packet, size_t InputLen)
{
    unsigned short result;
    rpc_cn_common_hdr_t* packet = (rpc_cn_common_hdr_t*) _Packet;
    
    if (InputLen < sizeof(rpc_cn_common_hdr_t))
    {
        return sizeof(rpc_cn_common_hdr_t);
    }
    
    if (PACKET_BYTE_ORDER(packet) != NATIVE_BYTE_ORDER)
    {
        swab(&packet->frag_len, &result, 2);
    }
    else
    {
        result = packet->frag_len;
    }
    
    return result;
}

static bool IsLastFrag(char *Packet)
{
    return (((rpc_cn_common_hdr_t *)Packet)->flags &
            RPC_C_CN_FLAGS_LAST_FRAG) == RPC_C_CN_FLAGS_LAST_FRAG;
}


/***************************************************************************/
/* Main Proxy Code */
/***************************************************************************/

static int SmbWriteAll(NP_PROXY_STATE *Proxy, char *data, size_t len)
{
    size_t remaining = len;
    while (remaining > 0)
    {
        ssize_t written = Proxy->SmbContext->write(Proxy->SmbContext,
                                                   Proxy->File,
                                                   data,
                                                   remaining);
        if (written == 0)
        {
            return 0;
        }
        else if (written < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                continue;
            }
            return written;
        }
        else if (written > remaining)
        {
            CT_LOG_VERBOSE("Too many bytes written: wrote %d of %d.\n",
                           written, remaining);
            written = remaining;
        }
        remaining -= written;
        data += written;
    }
    return len;
}

static int FdWriteAll(NP_PROXY_STATE *Proxy, char *data, size_t len)
{
    size_t remaining = len;
    while (remaining > 0)
    {
        ssize_t written = write(Proxy->Fd, data, remaining);
        if (written == -1)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                continue;
            }
            return 0;
        }
        remaining -= written;
        data += written;
    }
    return len;
}

CT_STATUS
NppGetSmbContextFlags(
    IN PROXY_CONNECTION_HANDLE Connection,
    OUT int* SmbContextFlags
    )
{
    CT_STATUS status;
    int smbContextFlags = 0;
    NPC_AUTH_FLAGS authFlags = 0;

    status = ProxyConnectionGetAuthInfo(Connection, &authFlags, NULL, NULL, NULL);
    GOTO_CLEANUP_ON_STATUS(status);

    if (!(authFlags & NPC_AUTH_FLAG_NO_DEFAULT))
    {
        smbContextFlags = PROXY_NP_SMB_CTX_FLAGS_DEFAULT;
    }

    if (authFlags & NPC_AUTH_FLAG_KERBEROS)
    {
        smbContextFlags |= SMB_CTX_FLAG_USE_KERBEROS;
    }
    if (authFlags & NPC_AUTH_FLAG_FALLBACK)
    {
        smbContextFlags |= SMB_CTX_FLAG_FALLBACK_AFTER_KERBEROS;
    }
    if (authFlags & NPC_AUTH_FLAG_NO_ANONYMOUS)
    {
        smbContextFlags |= SMBCCTX_FLAG_NO_AUTO_ANONYMOUS_LOGON;
    }

cleanup:
    *SmbContextFlags = smbContextFlags;
    return status;
}

CT_STATUS
NpProxyConnect(
    OUT void** Context,
    IN PROXY_CONNECTION_HANDLE Connection,
    IN int Fd,
    IN const char* Address,
    IN const char* Endpoint,
    IN const char* Options,
    OUT size_t* SessionKeyLen,
    OUT unsigned char** SessionKey
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    NP_PROXY_STATE *context = NULL;
    char *uriPath = NULL;
    unsigned char *SessKey = NULL;
    size_t SessKeyLen = 0;
    int smbContextFlags, i;
    int attempt = 0;
    const struct timespec sleepTime = { 0, 10000000 };

    CT_LOG_INFO("ProxyConnect: connecting to %s on endpoint %s.\n", Address, Endpoint);

    if (strncasecmp(Endpoint, "\\pipe\\", strlen("\\pipe\\")))
    {
        status = CT_STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    Endpoint += strlen("\\pipe\\");

    status = CtAllocateStringPrintf(&uriPath, "smb://%s/IPC$/%s", Address, Endpoint);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    for (i = 0; uriPath[i] != '\0'; i++)
    {
        if (uriPath[i] == '\\')
        {
            uriPath[i] = '/';
        }
    }

    status = NppGetSmbContextFlags(Connection, &smbContextFlags);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtAllocateMemory((void **)&context, sizeof(*context));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NppCreateSmbContext(&context->SmbContext, smbContextFlags, context);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    /* Proxy must be set before doing the create so that we can authenticate */
    context->Connection = Connection;

    while (1)
    {
        context->File = smbc_nt_create(context->SmbContext,
                                       uriPath,
                                       0,
                                       GENERIC_READ_ACCESS | GENERIC_WRITE_ACCESS,
                                       0,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL,
                                       0);
        if (context->File == NULL)
        {
            if (errno == ENOSYS)
            {
                /* The DC is throttling our connections by returning
                 * NT_STATUS_PIPE_NOT_AVAILABLE.
                 */
                attempt++;
                if (attempt > 5)
                {
                    CT_LOG_INFO("Giving up on NT_STATUS_PIPE_NOT_AVAILABLE after %d attempts\n", attempt);
                }
                else
                {
                    CT_LOG_INFO("Retrying on NT_STATUS_PIPE_NOT_AVAILABLE after %d attempts\n", attempt);
                    nanosleep(&sleepTime, NULL);
                    continue;
                }
            }
            status = CT_ERRNO_TO_STATUS(errno);
            GOTO_CLEANUP_EE(EE);
        }
        break;
    }

    smbc_get_session_key(context->File, &SessKey, &SessKeyLen);

    context->Fd = Fd;

    if (Fd >= 0)
    {
        /* Do this last so that we do not alter the socket state on failure */
        status = CtSocketSetBlocking(Fd);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    if (status)
    {
        if (context)
        {
            NpProxyClose(context);
            context = NULL;
        }
    }

    CT_SAFE_FREE(uriPath);

    *Context = context;
    *SessionKeyLen = SessKeyLen;
    *SessionKey = SessKey;

    if (status)
    {
        CT_LOG_TRACE("status = 0x%08x (EE = %d)", status, EE);
    }

    return status;
}

void
NpProxyRun(
    IN void* Context
    )
{
    NP_PROXY_STATE *proxy = (NP_PROXY_STATE *)Context;
    char localBuffer[10 * 1024];
    size_t localBufferCount = 0;
    size_t localBufferStart = 0;

    char namedPipeBuffer[10 * 1024];
    size_t namedPipeBufferCount = 0;
    size_t namedPipeBufferStart = 0;

    ssize_t readSize;
    int expectIncoming = 0;
    size_t packetSize;

    for (;;)
    {
        memmove(localBuffer, localBuffer + localBufferStart, localBufferCount);
        localBufferStart = 0;
        CT_LOG_INFO("Reading from unix socket\n");

        while ((readSize = read(proxy->Fd,
                                localBuffer + localBufferCount,
                                sizeof(localBuffer) - localBufferCount)) < 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                continue;
            }
            else
            {
                break;
            }
        }
        CT_LOG_INFO("Read %d bytes\n", readSize);
        if (readSize == 0)
        {
            // This means the local server closed the unix socket
            break;
        }
        else if (readSize < 0)
        {
            CT_LOG_INFO("Error reading from socket: %s\n", strerror(errno));
            return;
        }
        else if (readSize > 0)
        {
            localBufferCount += readSize;
        }

        // Send all of the whole packets from the local unix socket to
        // the named pipe.
        while (localBufferCount > 0)
        {
            packetSize = GetPacketSize(localBuffer + localBufferStart,
                                       localBufferCount);
            CT_LOG_INFO("Packet in local buffer is %d bytes\n", packetSize);
            if (packetSize > localBufferCount)
            {
                break;
            }
            if (IsLastFrag(localBuffer + localBufferStart))
            {
                CT_LOG_INFO("It is the last fragment\n", packetSize);
                // The server should send a reply back for this packet
                expectIncoming++;
            }
            if (!SmbWriteAll(proxy, localBuffer + localBufferStart, packetSize))
            {
                CT_LOG_INFO("Error writing packet to named pipe\n", packetSize);
                // This means the remote server closed the named pipe
                return;
            }
            CT_LOG_INFO("Wrote packet to named pipe '%p'\n", proxy->SmbContext);
            localBufferCount -= packetSize;
            localBufferStart += packetSize;
        }
        while (expectIncoming > 0)
        {
            memmove(namedPipeBuffer, namedPipeBuffer + namedPipeBufferStart,
                    namedPipeBufferCount);
            namedPipeBufferStart = 0;
            CT_LOG_INFO("Reading from named pipe\n");
            readSize = proxy->SmbContext->read(proxy->SmbContext,
                                               proxy->File,
                                               namedPipeBuffer + namedPipeBufferCount,
                                               sizeof(namedPipeBuffer) - namedPipeBufferCount);
            CT_LOG_INFO("Read %d bytes\n", readSize);
            if (readSize < 0)
            {
                // This means the remote server closed the named pipe
                // or we timed out, or an error occurred
                return;
            }
            namedPipeBufferCount += readSize;

            while (namedPipeBufferCount > 0)
            {
                CT_LOG_INFO("%ld bytes are in the named pipe buffer\n", namedPipeBufferCount);
                packetSize = GetPacketSize(namedPipeBuffer +
                                           namedPipeBufferStart,
                                           namedPipeBufferCount);
                CT_LOG_INFO("Packet in named pipe buffer is %ld bytes\n", packetSize);
                if (packetSize > namedPipeBufferCount)
                {
                    break;
                }
                if (IsLastFrag(namedPipeBuffer + namedPipeBufferStart))
                {
                    // This is the server's reply
                    expectIncoming--;
                }
                if (!FdWriteAll(proxy, namedPipeBuffer + namedPipeBufferStart, packetSize))
                {
                    // This means the local server closed the unix socket
                    return;
                }
                namedPipeBufferCount -= packetSize;
                namedPipeBufferStart += packetSize;
            }
        }
    }
}

void
NpProxyClose(
    IN void* Context
    )
{
    NP_PROXY_STATE *proxy = (NP_PROXY_STATE *)Context;
    if (proxy)
    {
        if (proxy->File)
        {
            proxy->SmbContext->close_fn(proxy->SmbContext, proxy->File);
        }
        if (proxy->SmbContext)
        {
            NppDestroySmbContext(proxy->SmbContext);
        }
        CtFreeMemory(proxy);
    }
}
