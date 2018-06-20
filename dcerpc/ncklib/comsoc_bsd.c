/*
 *
 * Copyright (C) 2013 VMware, Inc. All rights reserved.
 * (c) Copyright 1989 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1989 DIGITAL EQUIPMENT CORPORATION
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 *                 permission to use, copy, modify, and distribute this
 * file for any purpose is hereby granted without fee, provided that
 * the above copyright notices and this notice appears in all source
 * code copies, and that none of the names of Open Software
 * Foundation, Inc., Hewlett-Packard Company, or Digital Equipment
 * Corporation be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Neither Open Software Foundation, Inc., Hewlett-
 * Packard Company, nor Digital Equipment Corporation makes any
 * representations about the suitability of this software for any
 * purpose.
 *
 */
/*
 * Copyright (c) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* ex: set shiftwidth=4 expandtab: */
/*
**
**  NAME:
**
**      comsoc.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  Veneer over the BSD socket abstraction not provided by the old sock_
**  or new rpc_{tower,addr}_ components.
**
**
*/

#include <config.h>
#include <commonp.h>
#include <com.h>
#include <comprot.h>
#include <comnaf.h>
#include <comp.h>
#include <comsoc_bsd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cnp.h>

#if !defined(_WIN32)
/* ---------------------------- *inux code here ------------------------ */
#include <dce/lrpc.h>
#include <sys/un.h>
#include <sys/param.h>
#include <sys/socket.h>

/*
 * TBD: Adam:  Requires Likewise lwbase to compile
 * rpc__bsd_socket_transport_inq_access_token. Need to use Windows
 * SD APIs vs Likewise. Ignoring for now.
 */

#include <lw/base.h>
#include <ifaddrs.h>

/* ncalrpc supported only on UNIX by Unix Domain Sockets */
#define HAS_NCAL_RPC 1


#define RPC_SOCK_SOL_IPV6 SOL_IPV6
#define CLOSESOCKET(s) close(s)

#else
/* ---------------------------- Windows code here ------------------------ */
#include <io.h>
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <Iphlpapi.h>

#define SHUT_RDWR SD_BOTH
#define unlink _unlink
#ifndef inline
#define inline __inline
#endif


#define RPC_SOCK_SOL_IPV6 SOL_SOCKET
#define CLOSESOCKET(s) closesocket(s)


#endif  /* ------------------ system deps ---------------------------- */

#if defined(__hpux)
#include "xnet-private.h"
#endif


/* Bizarre hack for HP-UX ia64 where a system header
 * makes reference to a kernel-only data structure
 */
#if defined(__hpux) && defined(__ia64) && !defined(_DEFINED_MPINFOU)
union mpinfou {};
#endif
#if !defined(_WIN32)
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif
/* Hack to ensure we actually get a definition of ioctl on AIX */
#if defined(_AIX) && defined(_BSD)
int ioctl(int d, int request, ...);
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h> /* Not just Linux */
#endif

/*#include <dce/cma_ux_wrappers.h>*/

INTERNAL boolean
_rpc__bsd_protoseq_filter_cb(
    rpc_socket_t sock,
    rpc_addr_p_t ip_addr,
    rpc_addr_p_t netmask_addr,
    rpc_addr_p_t broadcast_addr);

INTERNAL void
_rpc__bsd_free_if_entry(
    rpc_addr_p_t  *ip_addr,
    rpc_addr_p_t  *netmask_addr,
    rpc_addr_p_t  *broadcast_addr);

/* ======================================================================== */

/*
 * What we think a socket's buffering is in case rpc__socket_set_bufs()
 * fails miserably.  The #ifndef is here so that these values can be
 * overridden in a per-system file.
 */

#ifndef RPC_C_SOCKET_GUESSED_RCVBUF
#  define RPC_C_SOCKET_GUESSED_RCVBUF    (4 * 1024)
#endif

#ifndef RPC_C_SOCKET_GUESSED_SNDBUF
#  define RPC_C_SOCKET_GUESSED_SNDBUF    (4 * 1024)
#endif

/*
 * Maximum send and receive buffer sizes.  The #ifndef is here so that
 * these values can be overridden in a per-system file.
 */

#ifndef RPC_C_SOCKET_MAX_RCVBUF
#  define RPC_C_SOCKET_MAX_RCVBUF (60 * 1024)
#endif

#ifndef RPC_C_SOCKET_MAX_SNDBUF
#  define RPC_C_SOCKET_MAX_SNDBUF (60 * 1024)
#endif

/*
 * The RPC_SOCKET_DISABLE_CANCEL/RPC_SOCKET_RESTORE_CANCEL macros
 * are used to disable cancellation before entering library calls
 * which were non-cancelable under CMA threads but are generally
 * cancelable on modern POSIX systems.
 */
#define RPC_SOCKET_DISABLE_CANCEL	{ int __cs = dcethread_enableinterrupt_throw(0);
#define RPC_SOCKET_RESTORE_CANCEL	dcethread_enableinterrupt_throw(__cs); }

/*
 * Macros to paper over the difference between the 4.4bsd and 4.3bsd
 * socket API.
 *
 * The layout of a 4.4 struct sockaddr includes a 1 byte "length" field
 * which used to be one of the bytes of the "family" field.  (The "family"
 * field is now 1 byte instead of 2 bytes.)  4.4 provides binary
 * compatibility with applications compiled with a 4.3 sockaddr definition
 * by inferring a default length when the supplied length is zero.  Source
 * compatibility is blown however (if _SOCKADDR_LEN is #define'd) --
 * applications that assign only to the "family" field will leave the
 * "length" field possibly non-zero.
 *
 * Note that RPC's "sockaddr_t" is always defined to contains only a
 * family.  (We defined "rpc_addr_t" to be a struct that contains a length
 * and a sockaddr rather than mucking with the sockaddr itself.)  We
 * assumed that "sockaddr_t" and "struct sockaddr" are the same.  At
 * 4.4, this assumption caused problems.  We use RPC_SOCKET_FIX_ADDRLEN
 * at various opportunities to make sure sockaddrs' length is zero and
 * that makes the problems go away.
 *
 * ADDENDUM:
 *    This only makes the problem go away on little-endian systems
 *    where the length field on the 4.4 struct occupies the same position
 *    as the high byte of the family field on the 4.3 struct.  This is
 *    no good for i386 FreeBSD, so we have actually adapted sockaddr_t
 *    to match the system struct sockaddr.
 *                                           -- Brian Koropoff, Likewise
 *
 * RPC_SOCKET_FIX_ADDRLEN takes an "rpc_addr_p_t" (or "rpc_ip_addr_p_t")
 * as input.  The complicated casting (as opposed to simply setting
 * ".sa_len" to zero) is to ensure that the right thing happens regardless
 * of the integer endian-ness of the system).
 *
 * RPC_SOCKET_INIT_MGRHDR deals with the differences in the field names of
 * the "struct msghdr" data type between 4.3 and 4.4.
 */

#ifdef BSD_4_4_SOCKET
#define RPC_SOCKET_INIT_MSGHDR(msgp) ( \
    (msgp)->msg_control         = NULL, \
    (msgp)->msg_controllen      = 0, \
    (msgp)->msg_flags           = 0 \
)
#else
#define RPC_SOCKET_INIT_MSGHDR(msgp) ( \
    (msgp)->msg_accrights       = NULL, \
    (msgp)->msg_accrightslen    = 0 \
)
#endif /* BSD_4_4_SOCKET */

/*#if defined(_SOCKADDR_LEN)
#define RPC_SOCKET_FIX_ADDRLEN(addrp) ( \
      ((struct osockaddr *) &(addrp)->sa)->sa_family = \
              ((struct sockaddr *) &(addrp)->sa)->sa_family \
  )
  #else*/
#define RPC_SOCKET_FIX_ADDRLEN(addrp) do { } while (0)
/*#endif*/


#ifndef CMSG_ALIGN
#if defined(_CMSG_DATA_ALIGN)
#define CMSG_ALIGN _CMSG_DATA_ALIGN

#elif defined(_CMSG_ALIGN)
#define CMSG_ALIGN _CMSG_ALIGN

#elif defined(__DARWIN_ALIGN32)
#define CMSG_ALIGN __DARWIN_ALIGN32

#elif defined(ALIGN)
#define CMSG_ALIGN ALIGN
#endif
#endif /* CMSG_ALIGN */

#ifndef CMSG_SPACE
#define CMSG_SPACE(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + CMSG_ALIGN(len))
#endif

#ifndef CMSG_LEN
#define CMSG_LEN(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#endif

#ifdef LW_BUILD_ESX
#ifndef PF_VMKUNIX
#define PF_VMKUNIX 26
#endif
#endif

void
rpc_lrpc_transport_info_free(
    rpc_transport_info_handle_t info
    );
/*
 * BSD socket transport layer info structures
 */
typedef struct rpc_bsd_transport_info_s
{
    struct
    {
        unsigned16 length;
        unsigned char* data;
    } session_key;
    uid_t peer_uid;
    gid_t peer_gid;
} rpc_bsd_transport_info_t, *rpc_bsd_transport_info_p_t;

typedef struct rpc_bsd_socket_s
{
    int fd;
    rpc_bsd_transport_info_t info;
} rpc_bsd_socket_t, *rpc_bsd_socket_p_t;

/*
 * Macros for performance critical operations.
 */

#ifdef _WIN32

/* Only Winsock2 implementation */
INTERNAL int
_bsd__socket_sendmsg(
    int s,
    struct msghdr *msg,
    int flags)
{
    WSABUF *iovec = NULL;
    int i = 0;
    int sts = 0;
    size_t iovec_len = 0;
    DWORD io_bytes = 0;

    iovec_len = msg->msg_iovlen;
    if (iovec_len == 1)
    {
        /* Degenerate case, iovector is 1, so just use send() */
        sts = send(s, msg->msg_iov[0].iov_base, (int) msg->msg_iov[0].iov_len, 0);
        return sts;
    }

    iovec = calloc(iovec_len, sizeof(WSABUF));
    if (!iovec)
    {
        WSASetLastError(WSA_NOT_ENOUGH_MEMORY);
        return -1;
    }

    if (iovec)
    {
        for (i=0; i<iovec_len; i++)
        {
            iovec[i].len = (ULONG) msg->msg_iov[i].iov_len;
            iovec[i].buf = msg->msg_iov[i].iov_base;
        }
    }
    sts = WSASend(s, iovec, (DWORD) iovec_len, &io_bytes, 0, 0, 0);
    free(iovec);

    return sts == -1 ? sts : io_bytes;
}

INTERNAL int
_bsd__socket_recvmsg(
    int s,
    struct msghdr *msg,
    int flags)
{
    WSABUF *iovec = NULL;
    int i = 0;
    int sts = 0;
    size_t iovec_len = 0;
    DWORD io_bytes = 0;

    iovec_len = msg->msg_iovlen;
    if (iovec_len == 1)
    {
        /* Degenerate case, iovector is 1, so just use recv() */
        sts = recv(s, msg->msg_iov[0].iov_base, (int) msg->msg_iov[0].iov_len, 0);
        return sts;
    }

    iovec = calloc(iovec_len, sizeof(WSABUF));
    if (!iovec)
    {
        WSASetLastError(WSA_NOT_ENOUGH_MEMORY);
        return -1;
    }

    if (iovec)
    {
        for (i=0; i<iovec_len; i++)
        {
            iovec[i].len = (ULONG) msg->msg_iov[i].iov_len;
            iovec[i].buf = msg->msg_iov[i].iov_base;
        }
    }
    sts = WSARecv(s, iovec, (DWORD) iovec_len, &io_bytes, 0, 0, 0);
    free(iovec);

    return sts == -1 ? sts : io_bytes;
}

#else

/*
 * Pure UNIX implementation, avoiding libdcethread abstractions.
 * UNIX implementation for sendmsg().
 */
INTERNAL int
_bsd__socket_sendmsg(
    int s,
    struct msghdr *msg,
    int flags)
{
    return sendmsg(s, msg, flags);
}

/*
 * Pure UNIX implementation, avoiding libdcethread abstractions.
 * UNIX implementation for recvmsg().
 */
INTERNAL int
_bsd__socket_recvmsg(
    int s,
    struct msghdr *msg,
    int flags)
{
    return recvmsg(s, msg, flags);
}
#endif


inline static void RPC_SOCKET_SENDMSG(
	rpc_socket_t sock,
	rpc_socket_iovec_p_t iovp,
	int iovlen,
	rpc_addr_p_t addrp,
	volatile int *ccp,
	volatile rpc_socket_error_t *serrp
		)
{
	struct msghdr msg;
	rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;
sendmsg_again:
	memset(&msg, 0, sizeof(msg));
	RPC_LOG_SOCKET_SENDMSG_NTR;
	RPC_SOCKET_INIT_MSGHDR(&msg);
#if !defined(_WIN32)
	if ((addrp) != NULL)
	{
		RPC_SOCKET_FIX_ADDRLEN(addrp);
		msg.msg_name = (caddr_t) &(addrp)->sa;
		msg.msg_namelen = (addrp)->len;
	}
	else
	{
		msg.msg_name = (caddr_t) NULL;
	}
#endif
	msg.msg_iov = (struct iovec *) iovp;
	msg.msg_iovlen = iovlen;
	*(ccp) = _bsd__socket_sendmsg(lrpc->fd, (struct msghdr *) &msg, 0);
	*(serrp) = (*(ccp) == -1) ? SocketErrno : RPC_C_SOCKET_OK;
	RPC_LOG_SOCKET_SENDMSG_XIT;
	if (*(serrp) == EINTR)
	{
		goto sendmsg_again;
	}
}

inline static void RPC_SOCKET_RECVFROM
(
    rpc_socket_t        sock,
    byte_p_t            buf,        /* buf for rcvd data */
    int                 buflen,        /* len of above buf */
    rpc_addr_p_t        from,       /* addr of sender */
    volatile int                 *ccp,        /* returned number of bytes actually rcvd */
	 volatile rpc_socket_error_t *serrp
)
{
	rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;
recvfrom_again:
	if ((from) != NULL) RPC_SOCKET_FIX_ADDRLEN(from);
	RPC_LOG_SOCKET_RECVFROM_NTR;
	*(ccp) = (int) dcethread_recvfrom (lrpc->fd, (char *) buf, (int) buflen, (int) 0,
			(struct sockaddr *) (&(from)->sa), (&(from)->len));
	*(serrp) = (*(ccp) == -1) ? SocketErrno : RPC_C_SOCKET_OK;
	RPC_LOG_SOCKET_RECVFROM_XIT;
	RPC_SOCKET_FIX_ADDRLEN(from);
	if (*(serrp) == EINTR)
	{
		goto recvfrom_again;
	}

}

inline static void RPC_SOCKET_RECVMSG
(
    rpc_socket_t        sock,
    rpc_socket_iovec_p_t iovp,       /* array of bufs for rcvd data */
    int                 iovlen,    /* number of bufs */
    rpc_addr_p_t        addrp,       /* addr of sender */
    volatile int                 *ccp,        /* returned number of bytes actually rcvd */
	 volatile rpc_socket_error_t *serrp
)
{
	rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;
	struct msghdr msg;
recvmsg_again:
	memset(&msg, 0, sizeof(msg));
	RPC_LOG_SOCKET_RECVMSG_NTR;
	RPC_SOCKET_INIT_MSGHDR(&msg);
#if !defined(_WIN32)
	if ((addrp) != NULL)
	{
		RPC_SOCKET_FIX_ADDRLEN(addrp);
		msg.msg_name = (caddr_t) &(addrp)->sa;
		msg.msg_namelen = (addrp)->len;
	}
	else
	{
		msg.msg_name = (caddr_t) NULL;
	}
#endif
	msg.msg_iov = (struct iovec *) iovp;
	msg.msg_iovlen = iovlen;
	*(ccp) = _bsd__socket_recvmsg (lrpc->fd, (struct msghdr *) &msg, 0);
	if ((addrp) != NULL)
	{
		(addrp)->len = msg.msg_namelen;
	}
	*(serrp) = (*(ccp) == -1) ? SocketErrno : RPC_C_SOCKET_OK;
	RPC_LOG_SOCKET_RECVMSG_XIT;
	if (*(serrp) == EINTR)
	{
		goto recvmsg_again;
	}
}

INTERNAL rpc_socket_error_t rpc__bsd_socket_destruct
(
    rpc_socket_t        sock
);


/* ======================================================================== */

/*
 * R P C _ _ S O C K E T _ C O N S T R U C T
 *
 * Create a new socket for the specified Protocol Sequence.
 * The new socket has blocking IO semantics.
 *
 * (see BSD UNIX socket(2)).
 */

INTERNAL rpc_socket_error_t
rpc__bsd_socket_construct(
    rpc_socket_t sock,
    rpc_protseq_id_t    pseq_id,
    rpc_transport_info_handle_t info ATTRIBUTE_UNUSED
    )
{
    rpc_socket_error_t  serr = RPC_C_SOCKET_OK;
    rpc_bsd_socket_p_t lrpc = NULL;

    lrpc = calloc(1, sizeof(*lrpc));

    if (!lrpc)
    {
        serr = ENOMEM;
        goto error;
    }

    sock->data.pointer = (void*) lrpc;

    lrpc->fd                      = -1;
    lrpc->info.peer_uid           = -1;
    lrpc->info.peer_gid           = -1;
    lrpc->info.session_key.data   = NULL;
    lrpc->info.session_key.length = 0;

    RPC_SOCKET_DISABLE_CANCEL;
    lrpc->fd = (int) socket(
        (int) RPC_PROTSEQ_INQ_NAF_ID(pseq_id),
        (int) RPC_PROTSEQ_INQ_NET_IF_ID(pseq_id),
        0 /*(int) RPC_PROTSEQ_INQ_NET_PROT_ID(pseq_id)*/);
    serr = ((lrpc->fd == -1) ? SocketErrno : RPC_C_SOCKET_OK);
    RPC_SOCKET_RESTORE_CANCEL;

    if (serr)
    {
        goto error;
    }

done:

    return serr;

error:

    rpc__bsd_socket_destruct(sock);

    goto done;
}

/*
 * R P C _ _ S O C K E T _ O P E N _ B A S I C
 *
 * A special version of socket_open that is used *only* by
 * the low level initialization code when it is trying to
 * determine what network services are supported by the host OS.
 */

PRIVATE rpc_socket_error_t
rpc__bsd_socket_open_basic(
    rpc_naf_id_t        naf,
    rpc_network_if_id_t net_if,
    rpc_network_protocol_id_t net_prot ATTRIBUTE_UNUSED,
    rpc_socket_basic_t        *sock
    )
{
    rpc_socket_error_t  serr;

    /*
     * Always pass zero as socket protocol to compensate for
     * overloading the protocol field for named pipes
     */
    RPC_SOCKET_DISABLE_CANCEL;
    *sock = (int) socket((int) naf, (int) net_if, 0);
    serr = ((*sock == -1) ? SocketErrno : RPC_C_SOCKET_OK);
    RPC_SOCKET_RESTORE_CANCEL;

    return serr;
}

PRIVATE rpc_socket_error_t
rpc__bsd_socket_close_basic(
    rpc_socket_basic_t        sock
    )
{
    rpc_socket_error_t  serr;

    RPC_LOG_SOCKET_CLOSE_NTR;
    RPC_SOCKET_DISABLE_CANCEL;
    serr = (CLOSESOCKET(sock) == -1) ? SocketErrno : RPC_C_SOCKET_OK;
    RPC_SOCKET_RESTORE_CANCEL;
    RPC_LOG_SOCKET_CLOSE_XIT;

    return (serr);
}


/*
 * R P C _ _ S O C K E T _ C L O S E
 *
 * Close (destroy) a socket.
 *
 * (see BSD UNIX close(2)).
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_destruct
(
    rpc_socket_t        sock
)
{
    rpc_socket_error_t  serr = RPC_C_SOCKET_OK;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;

    if (lrpc)
    {
        if (lrpc->fd > 0)
        {
            RPC_LOG_SOCKET_CLOSE_NTR;
            RPC_SOCKET_DISABLE_CANCEL;
            serr = (CLOSESOCKET(lrpc->fd) == -1) ? SocketErrno : RPC_C_SOCKET_OK;
            RPC_SOCKET_RESTORE_CANCEL;
            RPC_LOG_SOCKET_CLOSE_XIT;
        }

        if (lrpc->info.session_key.data)
        {
            free(lrpc->info.session_key.data);
            lrpc->info.session_key.length = 0;
        }

        free(lrpc);
    }

    sock->data.pointer = NULL;

    return serr;
}

/*
 * R P C _ _ S O C K E T _ B I N D
 *
 * Bind a socket to a specified local address.
 *
 * (see BSD UNIX bind(2)).
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_bind
(
    rpc_socket_t        sock,
    rpc_addr_p_t        addr
)
{
    rpc_socket_error_t  serr = EINVAL;
    unsigned32 status;
    rpc_addr_p_t temp_addr = NULL;
    boolean has_endpoint = false;
    int setsock_val = 1;
    int ncalrpc;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;

    RPC_LOG_SOCKET_BIND_NTR;

    ncalrpc = addr->rpc_protseq_id == RPC_C_PROTSEQ_ID_NCALRPC;

    /*
     * Check if the address has a well-known endpoint.
     */
    if (addr->rpc_protseq_id == RPC_C_PROTSEQ_ID_NCACN_IP_TCP ||
        addr->rpc_protseq_id == RPC_C_PROTSEQ_ID_NCACN_IP6_TCP || ncalrpc)
    {
        unsigned_char_t *endpoint;

        rpc__naf_addr_inq_endpoint (addr, &endpoint, &status);

        if (status == rpc_s_ok && endpoint != NULL)
        {
            if (endpoint[0] != '\0')    /* test for null string */
                has_endpoint = true;

            rpc_string_free (&endpoint, &status);
        }
        status = rpc_s_ok;
    }

    /*
     * If there is no port restriction in this address family, then do a
     * simple bind.
     */

    if (! RPC_PROTSEQ_TEST_PORT_RESTRICTION (addr -> rpc_protseq_id))
    {
        if (!has_endpoint && ncalrpc)
        {
            serr = 0;
        }
        else
        {
#if defined(SOL_SOCKET) && defined(SO_REUSEADDR) && !defined(_WIN32)
	    setsockopt(lrpc->fd, SOL_SOCKET, SO_REUSEADDR,
		       (const void *) &setsock_val, sizeof(setsock_val));
#endif
            if (addr->sa.family == AF_UNIX && addr->sa.data[0] != '\0')
            {
                // This function is going to bind a named socket. First, try
                // to delete the path incase a previous instance of a program
                // left it behind.
                //
                // Ignore any errors from this function.
                unlink((const char*)addr->sa.data);
            }
#if defined(SOL_IPV6) && defined(IPV6_V6ONLY)
            if (((struct sockaddr_in6 *) &addr->sa)->sin6_family == AF_INET6)
            {
                int one = 1;
                setsockopt(lrpc->fd, SOL_IPV6, IPV6_V6ONLY, (void *) &one, sizeof(one));
            }
#endif
            serr =
                (bind(lrpc->fd, (struct sockaddr *)&addr->sa, addr->len) == -1) ?
                      SocketErrno : RPC_C_SOCKET_OK;
        }
    }                                   /* no port restriction */

#if !defined(_WIN32) || defined(HAS_NCAL_RPC)
    else
    {
        /*
         * Port restriction is in place.  If the address has a well-known
         * endpoint, then do a simple bind.
         */

        if (has_endpoint)
        {
#if defined(SOL_SOCKET) && defined(SO_REUSEADDR) && !defined(_WIN32)
	    setsockopt(lrpc->fd, SOL_SOCKET, SO_REUSEADDR,
		       &setsock_val, sizeof(setsock_val));
#endif
            serr = (bind(lrpc->fd, (struct sockaddr *)&addr->sa, addr->len) == -1)?
                SocketErrno : RPC_C_SOCKET_OK;
        }                               /* well-known endpoint */
        else
	{

	    unsigned_char_t *endpoint;
	    unsigned char c;

	    rpc__naf_addr_inq_endpoint (addr, &endpoint, &status);

	    c = endpoint[0];               /* grab first char */
	    rpc_string_free (&endpoint, &status);

	    if (c != '\0')       /* test for null string */
	    {
	        serr = (bind(lrpc->fd, (struct sockaddr *)&addr->sa, addr->len) == -1)?
		    SocketErrno : RPC_C_SOCKET_OK;
	    }                               /* well-known endpoint */

	    else
	    {
	        /*
	         * Port restriction is in place and the address doesn't have a
	         * well-known endpoint.  Try to bind until we hit a good port,
	         * or exhaust the retry count.
	         *
	         * Make a copy of the address to work in; if we hardwire an
	         * endpoint into our caller's address, later logic could infer
	         * that it is a well-known endpoint.
	         */
	
	        unsigned32 i;
	        boolean found;
	
	        for (i = 0, found = false;
		     (i < RPC_PORT_RESTRICTION_INQ_N_TRIES (addr->rpc_protseq_id))
		     && !found;
		     i++)
	        {
		    unsigned_char_p_t port_name;

		    rpc__naf_addr_overcopy (addr, &temp_addr, &status);

		    if (status != rpc_s_ok)
		    {
		        serr = RPC_C_SOCKET_EIO;
		        break;
		    }

		    rpc__naf_get_next_restricted_port (temp_addr -> rpc_protseq_id,
						   &port_name, &status);

		    if (status != rpc_s_ok)
		    {
		        serr = RPC_C_SOCKET_EIO;
		        break;
		    }

		    rpc__naf_addr_set_endpoint (port_name, &temp_addr, &status);

		    if (status != rpc_s_ok)
		    {
		        serr = RPC_C_SOCKET_EIO;
		        rpc_string_free (&port_name, &status);
		        break;
		    }

		    if (bind(lrpc->fd, (struct sockaddr *)&temp_addr->sa, temp_addr->len) == 0)
		    {
		        found = true;
		        serr = RPC_C_SOCKET_OK;
		    }
		    else
		        serr = RPC_C_SOCKET_EIO;

		    rpc_string_free (&port_name, &status);
	        }                           /* for i */

	        if (!found)
	        {
		    serr = RPC_C_SOCKET_EADDRINUSE;
	        }
	    }                               /* no well-known endpoint */
        }				/* has endpoint */
    }                                   /* port restriction is in place */

    if (serr == RPC_C_SOCKET_OK && ncalrpc && has_endpoint)
    {
	struct sockaddr_un *skun = (struct sockaddr_un *)&addr->sa;

	serr = chmod(skun->sun_path,
		     S_IRUSR | S_IWUSR | S_IXUSR |
                     S_IRGRP | S_IWGRP | S_IXGRP |
                     S_IROTH | S_IWOTH | S_IXOTH) == -1 ? SocketErrno : RPC_C_SOCKET_OK;
    }
#endif /* defined(HAS_NCAL_RPC) */

    if (temp_addr != NULL)
        rpc__naf_addr_free (&temp_addr, &status);

    RPC_LOG_SOCKET_BIND_XIT;
    return (serr);
}

INTERNAL rpc_socket_error_t rpc__bsd_socket_getpeereid
(
    rpc_socket_t        sock,
    uid_t		*euid,
    gid_t		*egid
);


#if !defined(SO_PEERCRED) && !(defined(HAVE_GETPEEREID) && HAVE_DECL_GETPEEREID) && !defined(_WIN32)


INTERNAL rpc_socket_error_t rpc__bsd_socket_sendpeereid
(
    rpc_socket_t        sock,
    rpc_addr_p_t        addr
);


INTERNAL rpc_socket_error_t rpc__bsd_socket_recvpeereid
(
    rpc_socket_t        sock,
    uid_t		*euid,
    gid_t		*egid
);

#endif



INTERNAL rpc_socket_error_t rpc__bsd_socket_createsessionkey
(
    unsigned char **session_key,
    unsigned16     *session_key_len
);


#if !defined(_WIN32) || defined(HAS_NCAL_RPC)
INTERNAL rpc_socket_error_t rpc__bsd_socket_sendsessionkey
(
    rpc_socket_t        sock,
    unsigned char      *session_key,
    unsigned16          session_key_len
);


INTERNAL rpc_socket_error_t rpc__bsd_socket_recvsession_key
(
    rpc_socket_t        sock,
    unsigned char     **session_key,
    unsigned16         *session_key_len
);
#endif /* defined(HAS_NCAL_RPC) */



/*
 * R P C _ _ S O C K E T _ C O N N E C T
 *
 * Connect a socket to a specified peer's address.
 * This is used only by Connection oriented Protocol Services.
 *
 * (see BSD UNIX connect(2)).
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_connect
(
    rpc_socket_t        sock,
    rpc_addr_p_t        addr,
    rpc_cn_assoc_t      *assoc ATTRIBUTE_UNUSED
)
{
    rpc_socket_error_t  serr;
    //rpc_binding_rep_t *binding_rep;
    unsigned_char_t *netaddr, *endpoint;
    unsigned32      dbg_status;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;
    unsigned char *session_key = NULL;
    unsigned16 session_key_len = 0;

    rpc__naf_addr_inq_netaddr (addr,
                               &netaddr,
                               &dbg_status);
    rpc__naf_addr_inq_endpoint (addr,
                               &endpoint,
                               &dbg_status);

    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
        ("CN: connection request initiated to %s[%s]\n",
         netaddr,
         endpoint));

connect_again:
    RPC_LOG_SOCKET_CONNECT_NTR;
    serr = (connect (
                (int) lrpc->fd,
                (struct sockaddr *) (&addr->sa),
                (int) (addr->len))
            == -1) ? SocketErrno : RPC_C_SOCKET_OK;
    RPC_LOG_SOCKET_CONNECT_XIT;
    if (serr == EINTR)
    {
        goto connect_again;
    }
    else if (serr != RPC_C_SOCKET_OK)
    {
        goto error;
    }

#if !defined(SO_PEERCRED) && !(defined(HAVE_GETPEEREID) && HAVE_DECL_GETPEEREID) && !defined(_WIN32)
    serr = rpc__bsd_socket_sendpeereid(sock, addr);
    if (serr)
    {
        goto error;
    }
#endif


#if !defined(_WIN32) || defined(HAS_NCAL_RPC)
    if (sock->pseq_id == rpc_c_protseq_id_ncalrpc)
    {
        serr = rpc__bsd_socket_recvsession_key(sock,
                                               &session_key,
                                               &session_key_len);
        if (serr)
        {
            goto error;
        }

        lrpc->info.session_key.data   = session_key;
        lrpc->info.session_key.length = session_key_len;
    }
#endif /* defined(HAS_NCAL_RPC) */

cleanup:
    rpc_string_free (&netaddr, &dbg_status);
    rpc_string_free (&endpoint, &dbg_status);

    return serr;

error:
    goto cleanup;
}



/*
 * R P C _ _ S O C K E T _ A C C E P T
 *
 * Accept a connection on a socket, creating a new socket for the new
 * connection.  A rpc_addr_t appropriate for the NAF corresponding to
 * this socket must be provided.  addr.len must set to the actual size
 * of addr.sa.  This operation fills in addr.sa and sets addr.len to
 * the new size of the field.  This is used only by Connection oriented
 * Protocol Services.
 *
 * (see BSD UNIX accept(2)).
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_accept
(
    rpc_socket_t        sock,
    rpc_addr_p_t        addr,
    rpc_socket_t        *newsock
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;
    rpc_bsd_socket_p_t newlrpc = NULL;
    uid_t euid = -1;
    gid_t egid = -1;
    unsigned char *session_key = NULL;
    unsigned16 session_key_len = 0;

    *newsock = malloc(sizeof (**newsock));

    if (!*newsock)
    {
        return ENOMEM;
    }

    (*newsock)->vtbl = sock->vtbl;
    (*newsock)->pseq_id = sock->pseq_id;

    newlrpc = malloc(sizeof(*newlrpc));
    if (!newlrpc)
    {
        return ENOMEM;
    }

    newlrpc->info.peer_uid = -1;
    newlrpc->info.peer_gid = -1;

    (*newsock)->data.pointer = newlrpc;

accept_again:
    RPC_LOG_SOCKET_ACCEPT_NTR;
    if (addr == NULL)
    {
        socklen_t addrlen;
        addrlen = 0;

        /*
         * Not legal on Windows to pass NULL sockaddr but a pointer
         * to addrlen, even if the value of addrlen = 0.
         */
        newlrpc->fd = (int) accept
            ((int) lrpc->fd, (struct sockaddr *) NULL, NULL);
    }
    else
    {
        newlrpc->fd = (int) accept
            ((int) lrpc->fd, (struct sockaddr *) (&addr->sa), (&addr->len));
    }
    serr = (newlrpc->fd == -1) ? SocketErrno : RPC_C_SOCKET_OK;
    RPC_LOG_SOCKET_ACCEPT_XIT;

    if (serr == EINTR)
    {
        goto accept_again;
    }
    else if (serr)
    {
        goto cleanup;
    }

    if (sock->pseq_id == RPC_C_PROTSEQ_ID_NCALRPC)
    {
        serr = rpc__bsd_socket_getpeereid((*newsock), &euid, &egid);
        if (serr)
        {
            goto cleanup;
        }

#if !defined(_WIN32) || defined(HAS_NCAL_RPC)
        serr = rpc__bsd_socket_createsessionkey(&session_key,
                                                &session_key_len);
        if (serr)
        {
            goto cleanup;
        }

        serr = rpc__bsd_socket_sendsessionkey((*newsock),
                                              session_key,
                                              session_key_len);
        if (serr)
        {
            goto cleanup;
        }
#endif /* defined(HAS_NCAL_RPC) */
    }

    newlrpc->info.peer_uid           = euid;
    newlrpc->info.peer_gid           = egid;
    newlrpc->info.session_key.data   = session_key;
    newlrpc->info.session_key.length = session_key_len;

cleanup:
    if (serr && newlrpc)
    {
        free(newlrpc);
    }

    if (serr && *newsock)
    {
        free(*newsock);
    }

    return serr;
}

/*
 * R P C _ _ S O C K E T _ L I S T E N
 *
 * Listen for a connection on a socket.
 * This is used only by Connection oriented Protocol Services.
 *
 * (see BSD UNIX listen(2)).
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_listen
(
    rpc_socket_t        sock,
    int                 backlog
)
{
    rpc_socket_error_t  serr;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;

    RPC_LOG_SOCKET_LISTEN_NTR;
    RPC_SOCKET_DISABLE_CANCEL;
    serr = (listen(lrpc->fd, backlog) == -1) ? SocketErrno : RPC_C_SOCKET_OK;
    RPC_SOCKET_RESTORE_CANCEL;
    RPC_LOG_SOCKET_LISTEN_XIT;
    return (serr);
}

/*
 * R P C _ _ S O C K E T _ S E N D M S G
 *
 * Send a message over a given socket.  An error code as well as the
 * actual number of bytes sent are returned.
 *
 * (see BSD UNIX sendmsg(2)).
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_sendmsg
(
    rpc_socket_t        sock,
    rpc_socket_iovec_p_t iov,       /* array of bufs of data to send */
    int                 iov_len,    /* number of bufs */
    rpc_addr_p_t        addr,       /* addr of receiver */
    int                 *cc        /* returned number of bytes actually sent */
)
{
    rpc_socket_error_t serr;

    RPC_SOCKET_SENDMSG(sock, iov, iov_len, addr, cc, &serr);
    return (serr);
}

/*
 * R P C _ _ S O C K E T _ R E C V F R O M
 *
 * Recieve the next buffer worth of information from a socket.  A
 * rpc_addr_t appropriate for the NAF corresponding to this socket must
 * be provided.  addr.len must set to the actual size of addr.sa.  This
 * operation fills in addr.sa and sets addr.len to the new size of the
 * field.  An error status as well as the actual number of bytes received
 * are also returned.
 *
 * (see BSD UNIX recvfrom(2)).
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_recvfrom
(
    rpc_socket_t        sock,
    byte_p_t            buf,        /* buf for rcvd data */
    int                 len,        /* len of above buf */
    rpc_addr_p_t        from,       /* addr of sender */
    int                 *cc        /* returned number of bytes actually rcvd */
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_SOCKET_RECVFROM (sock, buf, len, from, cc, &serr);
    return serr;
}

/*
 * R P C _ _ S O C K E T _ R E C V M S G
 *
 * Receive a message over a given socket.  A rpc_addr_t appropriate for
 * the NAF corresponding to this socket must be provided.  addr.len must
 * set to the actual size of addr.sa.  This operation fills in addr.sa
 * and sets addr.len to the new size of the field.  An error code as
 * well as the actual number of bytes received are also returned.
 *
 * (see BSD UNIX recvmsg(2)).
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_recvmsg
(
    rpc_socket_t        sock,
    rpc_socket_iovec_p_t iov,       /* array of bufs for rcvd data */
    int                 iov_len,    /* number of bufs */
    rpc_addr_p_t        addr,       /* addr of sender */
    int                 *cc        /* returned number of bytes actually rcvd */
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;

    RPC_SOCKET_RECVMSG(sock, iov, iov_len, addr, cc, &serr);
    return serr;
}

/*
 * R P C _ _ S O C K E T _ I N Q _ A D D R
 *
 * Return the local address associated with a socket.  A rpc_addr_t
 * appropriate for the NAF corresponding to this socket must be provided.
 * addr.len must set to the actual size of addr.sa.  This operation fills
 * in addr.sa and sets addr.len to the new size of the field.
 *
 * !!! NOTE: You should use rpc__naf_desc_inq_addr() !!!
 *
 * This routine is indended for use only by the internal routine:
 * rpc__naf_desc_inq_addr().  rpc__bsd_socket_inq_endpoint() only has the
 * functionality of BSD UNIX getsockname() which doesn't (at least not
 * on all systems) return the local network portion of a socket's address.
 * rpc__naf_desc_inq_addr() returns the complete address for a socket.
 *
 * (see BSD UNIX getsockname(2)).
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_inq_endpoint
(
    rpc_socket_t        sock,
    rpc_addr_p_t        addr
)
{
    rpc_socket_error_t  serr = 0;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;
    struct sockaddr_in6 saddr;
    struct sockaddr *psaddr = NULL;
    socklen_t saddr_len = 0;

    memset(&saddr, 0, sizeof(saddr));
    RPC_LOG_SOCKET_INQ_EP_NTR;
    RPC_SOCKET_FIX_ADDRLEN(addr);
    RPC_SOCKET_DISABLE_CANCEL;

    /*
     * Not legal on Windows to pass an address length that is smaller
     * than required. Strategy here is to use a local sockaddr_in6
     * buffer when the request length is shorter than sizeof(struct sockaddr_in6),
     * get the data from getsockname(), then copy as much data from
     * the local buffer into the buffer passed by the caller.
     */
    psaddr = (struct sockaddr *) &addr->sa;
    saddr_len = addr->len;
    if (addr->len < sizeof(saddr))
    {
        psaddr = (struct sockaddr*) &saddr;
        saddr_len = sizeof(saddr);
    }
    serr = (getsockname(lrpc->fd, (void*)psaddr, &saddr_len) == -1) ? SocketErrno : RPC_C_SOCKET_OK;
    if (serr != RPC_C_SOCKET_OK)
    {
        goto error;
    }

#ifdef LW_BUILD_ESX
    /*
     * ESX workaround for getsockname() returning PF_VMKUNIX vs PF_UNIX for
     * a UDS socket.
     */
    if (psaddr->sa_family == PF_VMKUNIX)
    {
        psaddr->sa_family = PF_UNIX;
    }
#endif

    if (psaddr != (struct sockaddr *) &addr->sa)
    {
        memcpy(&addr->sa, psaddr, addr->len);
    }

    RPC_SOCKET_FIX_ADDRLEN(addr);
error:
    RPC_SOCKET_RESTORE_CANCEL;
    RPC_LOG_SOCKET_INQ_EP_XIT;
    return (serr);
}

/*
 * R P C _ _ S O C K E T _ S E T _ B R O A D C A S T
 *
 * Enable broadcasting for the socket (as best it can).
 * Used only by Datagram based Protocol Services.
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_set_broadcast
(
    rpc_socket_t        sock
)
{
#ifdef SO_BROADCAST
    int                 setsock_val = 1;
    rpc_socket_error_t  serr;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;

    RPC_SOCKET_DISABLE_CANCEL;
    serr = (setsockopt(lrpc->fd, SOL_SOCKET, SO_BROADCAST,
            (const void *) &setsock_val, sizeof(setsock_val)) == -1) ? SocketErrno : RPC_C_SOCKET_OK;
    RPC_SOCKET_RESTORE_CANCEL;
    if (serr)
    {
        RPC_DBG_GPRINTF(("(rpc__bsd_socket_set_broadcast) error=%d\n", serr));
    }

    return(serr);
#else
    return(RPC_C_SOCKET_OK);
#endif
}

/*
 * R P C _ _ S O C K E T _ S E T _ B U F S
 *
 * Set the socket's send and receive buffer sizes and return the new
 * values.  Note that the sizes are min'd with
 * "rpc_c_socket_max_{snd,rcv}buf" because systems tend to fail the
 * operation rather than give the max buffering if the max is exceeded.
 *
 * If for some reason your system is screwed up and defines SOL_SOCKET
 * and SO_SNDBUF, but doesn't actually support the SO_SNDBUF and SO_RCVBUF
 * operations AND using them would result in nasty behaviour (i.e. they
 * don't just return some error code), define NO_SO_SNDBUF.
 *
 * If the buffer sizes provided are 0, then we use the operating
 * system default (i.e. we don't set anything at all).
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_set_bufs

(
    rpc_socket_t        sock,
    unsigned32          txsize,
    unsigned32          rxsize,
    unsigned32          *ntxsize,
    unsigned32          *nrxsize
)
{
    socklen_t sizelen;
    int e;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;

    RPC_SOCKET_DISABLE_CANCEL;

#if (defined (SOL_SOCKET) && defined(SO_SNDBUF)) && !defined(NO_SO_SNDBUF)

    /*
     * Set the new sizes.
     */

    txsize = MIN(txsize, RPC_C_SOCKET_MAX_SNDBUF);
    if (txsize != 0)
    {
        e = setsockopt(lrpc->fd, SOL_SOCKET, SO_SNDBUF, (void *) &txsize, sizeof(txsize));
        if (e == -1)
        {
            RPC_DBG_GPRINTF
(("(rpc__bsd_socket_set_bufs) WARNING: set sndbuf (%d) failed - error = %d\n",
                txsize, SocketErrno));
        }
    }

    rxsize = MIN(rxsize, RPC_C_SOCKET_MAX_RCVBUF);
    if (rxsize != 0)
    {
        e = setsockopt(lrpc->fd, SOL_SOCKET, SO_RCVBUF, (const void *) &rxsize, sizeof(rxsize));
        if (e == -1)
        {
            RPC_DBG_GPRINTF
(("(rpc__bsd_socket_set_bufs) WARNING: set rcvbuf (%d) failed - error = %d\n",
                rxsize, SocketErrno));
        }
    }

    /*
     * Get the new sizes.  If this fails, just return some guessed sizes.
     */
    *ntxsize = 0;
    sizelen = sizeof *ntxsize;
    e = getsockopt(lrpc->fd, SOL_SOCKET, SO_SNDBUF, (void *) ntxsize, &sizelen);
    if (e == -1)
    {
        RPC_DBG_GPRINTF
(("(rpc__bsd_socket_set_bufs) WARNING: get sndbuf failed - error = %d\n", SocketErrno));
        *ntxsize = RPC_C_SOCKET_GUESSED_SNDBUF;
    }

    *nrxsize = 0;
    sizelen = sizeof *nrxsize;
    e = getsockopt(lrpc->fd, SOL_SOCKET, SO_RCVBUF, (void *) nrxsize, &sizelen);
    if (e == -1)
    {
        RPC_DBG_GPRINTF
(("(rpc__bsd_socket_set_bufs) WARNING: get rcvbuf failed - error = %d\n", SocketErrno));
        *nrxsize = RPC_C_SOCKET_GUESSED_RCVBUF;
    }

#  ifdef apollo
    /*
     * On Apollo, modifying the socket buffering doesn't actually do
     * anything on IP sockets, but the calls succeed anyway.  We can
     * detect this by the fact that the new buffer length returned is
     * 0. Return what we think the actually length is.
     */
    if (rxsize != 0 && *nrxsize == 0)
    {
        *nrxsize = (8 * 1024);
    }
    if (txsize != 0 && *ntxsize == 0)
    {
        *ntxsize = (8 * 1024);
    }
#  endif

#else

    *ntxsize = RPC_C_SOCKET_GUESSED_SNDBUF;
    *nrxsize = RPC_C_SOCKET_GUESSED_RCVBUF;

#endif

    RPC_SOCKET_RESTORE_CANCEL;

    return (RPC_C_SOCKET_OK);
}

/*
 * R P C _ _ S O C K E T _ S E T _ N B I O
 *
 * Set a socket to non-blocking mode.
 *
 * Return RPC_C_SOCKET_OK on success, otherwise an error value.
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_set_nbio
(
    rpc_socket_t        sock
)
{
#if !defined(vms) && !defined(_WIN32)

    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;

    RPC_SOCKET_DISABLE_CANCEL;
    serr = ((fcntl(lrpc->fd, F_SETFL, O_NDELAY) == -1) ? SocketErrno : RPC_C_SOCKET_OK);
    RPC_SOCKET_RESTORE_CANCEL;
    if (serr)
    {
        RPC_DBG_GPRINTF(("(rpc__bsd_socket_set_nbio) error=%d\n", serr));
    }

    return (serr);

#else

#ifdef DUMMY
/*
 * Note: This call to select non-blocking I/O is not implemented
 * by UCX on VMS. If this routine is really needed to work in the future
 * on VMS this will have to be done via QIO's.
 */
    int flag = true;

    ioctl(sock, FIONBIO, &flag);
#endif
    return (RPC_C_SOCKET_OK);

#endif
}

/*
 * R P C _ _ S O C K E T _ S E T _ C L O S E _ O N _ E X E C
 *
 *
 * Set a socket to a mode whereby it is not inherited by a spawned process
 * executing some new image. This is possibly a no-op on some systems.
 *
 * Return RPC_C_SOCKET_OK on success, otherwise an error value.
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_set_close_on_exec
(
    rpc_socket_t        sock
)
{
#if !defined(vms) && !defined(_WIN32)
    rpc_socket_error_t  serr = RPC_C_SOCKET_OK;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;

    RPC_SOCKET_DISABLE_CANCEL;
    serr = ((fcntl(lrpc->fd, F_SETFD, 1) == -1) ? SocketErrno : RPC_C_SOCKET_OK);
    RPC_SOCKET_RESTORE_CANCEL;
    if (serr)
    {
        RPC_DBG_GPRINTF(("(rpc__bsd_socket_set_close_on_exec) error=%d\n", serr));
    }
    return (serr);
#else
    return (RPC_C_SOCKET_OK);
#endif
}

/*
 * R P C _ _ S O C K E T _ G E T P E E R N A M E
 *
 * Get name of connected peer.
 * This is used only by Connection oriented Protocol Services.
 *
 * (see BSD UNIX getpeername(2)).
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_getpeername
(
    rpc_socket_t sock,
    rpc_addr_p_t addr
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;
    struct sockaddr_in6 saddr;
    struct sockaddr *psaddr = NULL;
    socklen_t saddr_len = 0;

    memset(&saddr, 0, sizeof(saddr));
    RPC_SOCKET_FIX_ADDRLEN(addr);
    RPC_SOCKET_DISABLE_CANCEL;

    /*
     * Not legal on Windows to pass an address length that is smaller
     * than required. Strategy here is to use a local sockaddr_in6
     * buffer when the request length is shorter than sizeof(struct sockaddr_in6),
     * get the data from getsockname(), then copy as much data from
     * the local buffer into the buffer passed by the caller.
     */
    psaddr = (struct sockaddr *) &addr->sa;
    saddr_len = addr->len;
    if (addr->len < sizeof(saddr))
    {
        psaddr = (struct sockaddr*) &saddr;
        saddr_len = sizeof(saddr);
    }
    serr = (getpeername(lrpc->fd, (void *)psaddr, &saddr_len) == -1) ? SocketErrno : RPC_C_SOCKET_OK;
    if (serr == 0 && psaddr->sa_family == AF_UNIX && saddr_len == sizeof(psaddr->sa_family))
    {
        /*
         * The point of this code is to obtain an address name for who has
         * contacted the server, so that name will appear in the RPC binding string.
         * The issue: calling getpeername() on an AF_UNIX socket will only
         * return the sa_family, but *not* the endpoint name. For only AF_UNIX
         * call getsockname() when just the sa_family value is returned by
         * getpeername().
         */
        saddr_len = addr->len;
        serr = (getsockname(lrpc->fd, (void *)psaddr, &saddr_len) == -1) ? SocketErrno : RPC_C_SOCKET_OK;
    }
    if (serr == 0 && psaddr != (struct sockaddr *) &addr->sa)
    {
        memcpy(&addr->sa, psaddr, addr->len);
    }
    RPC_SOCKET_RESTORE_CANCEL;
    RPC_SOCKET_FIX_ADDRLEN(addr);

    return (serr);
}

/*
 * R P C _ _ S O C K E T _ G E T _ I F _ I D
 *
 * Get socket network interface id (socket type).
 *
 * (see BSD UNIX getsockopt(2)).
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_get_if_id
(
    rpc_socket_t        sock,
    rpc_network_if_id_t *network_if_id
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    socklen_t optlen = 0;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;

    optlen = sizeof(rpc_network_if_id_t);

    RPC_SOCKET_DISABLE_CANCEL;
    serr = (getsockopt (lrpc->fd,
                        SOL_SOCKET,
                        SO_TYPE,
                        (char *) network_if_id,
                        &optlen) == -1  ? SocketErrno : RPC_C_SOCKET_OK);
    RPC_SOCKET_RESTORE_CANCEL;
    return serr;
}

/*
 * R P C _ _ S O C K E T _ S E T _ K E E P A L I V E
 *
 * Enable periodic transmissions on a connected socket, when no
 * other data is being exchanged. If the other end does not respond to
 * these messages, the connection is considered broken and the
 * so_error variable is set to ETIMEDOUT.
 * Used only by Connection based Protocol Services.
 *
 * (see BSD UNIX setsockopt(2)).
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_set_keepalive
(
    rpc_socket_t        sock
)
{
#ifdef SO_KEEPALIVE
    int                 setsock_val = 1;
    rpc_socket_error_t  serr = RPC_C_SOCKET_OK;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;

    RPC_SOCKET_DISABLE_CANCEL;
    serr = ((setsockopt(lrpc->fd, SOL_SOCKET, SO_KEEPALIVE,
       (void *) &setsock_val, sizeof(setsock_val)) == -1) ? SocketErrno : RPC_C_SOCKET_OK);
    RPC_SOCKET_RESTORE_CANCEL;
    if (serr)
    {
        RPC_DBG_GPRINTF(("(rpc__bsd_socket_set_keepalive) error=%d\n", serr));
    }

    return(serr);
#else
    return(RPC_C_SOCKET_OK);
#endif
}


/*
 * R P C _ _ S O C K E T _ N O W R I T E B L O C K _ W A I T
 *
 * Wait until the a write on the socket should succede without
 * blocking.  If tmo is NULL, the wait is unbounded, otherwise
 * tmo specifies the max time to wait. RPC_C_SOCKET_ETIMEDOUT
 * if a timeout occurs.  This operation in not cancellable.
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_nowriteblock_wait
(
    rpc_socket_t sock,
    struct timeval *tmo
)
{
    fd_set  write_fds;
    int     nfds, num_found;
    rpc_socket_error_t  serr = RPC_C_SOCKET_OK;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;

    FD_ZERO (&write_fds);
    FD_SET (lrpc->fd, &write_fds);
    nfds = lrpc->fd + 1;

    RPC_SOCKET_DISABLE_CANCEL;
    num_found = dcethread_select(nfds, NULL, (void *)&write_fds, NULL, tmo);
    serr = ((num_found < 0) ? SocketErrno : RPC_C_SOCKET_OK);
    RPC_SOCKET_RESTORE_CANCEL;

    if (serr)
    {
        RPC_DBG_GPRINTF(("(rpc__bsd_socket_nowriteblock_wait) error=%d\n", serr));
        return serr;
    }

    if (num_found == 0)
    {
        RPC_DBG_GPRINTF(("(rpc__bsd_socket_nowriteblock_wait) timeout\n"));
        return RPC_C_SOCKET_ETIMEDOUT;
    }

    return RPC_C_SOCKET_OK;
}


/*
 * R P C _ _ S O C K E T _ S E T _ R C V T I M E O
 *
 * Set receive timeout on a socket
 * Used only by Connection based Protocol Services.
 *
 * (see BSD UNIX setsockopt(2)).
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_set_rcvtimeo
(
    rpc_socket_t        sock,
    struct timeval      *tmo
)
{
#ifdef SO_RCVTIMEO
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;

#ifdef _WIN32
    DWORD dwTmo = tmo->tv_sec * 1000;
    void *ptmo = &dwTmo;
    int tmoSize = sizeof(DWORD);
#else
    void *ptmo = tmo;
    int tmoSize = sizeof(*tmo);
#endif



    RPC_SOCKET_DISABLE_CANCEL;
    serr = setsockopt(lrpc->fd, SOL_SOCKET, SO_RCVTIMEO, ptmo, tmoSize);
    serr = (serr == -1) ? SocketErrno : RPC_C_SOCKET_OK;
    RPC_SOCKET_RESTORE_CANCEL;
    if (serr)
    {
        RPC_DBG_GPRINTF(("(rpc__bsd_socket_set_rcvtimeo) error=%d\n", serr));
    }

    return(serr);
#else
    return(RPC_C_SOCKET_OK);
#endif
}

/*
 * R P C _ _ S O C K E T _ G E T _ P E E R E I D
 *
 * Get UNIX domain socket peer credentials
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_getpeereid
(
    rpc_socket_t        sock,
    uid_t		*euid,
    gid_t		*egid
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
#if defined(SO_PEERCRED)
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;
    struct ucred peercred = {0};
    socklen_t peercredlen = sizeof(peercred);

    RPC_SOCKET_DISABLE_CANCEL;
    serr = ((getsockopt(lrpc->fd, SOL_SOCKET, SO_PEERCRED,
	&peercred, &peercredlen) == -1) ? SocketErrno : RPC_C_SOCKET_OK);
    RPC_SOCKET_RESTORE_CANCEL;
    if (serr == RPC_C_SOCKET_OK)
    {
	*euid = peercred.uid;
	*egid = peercred.gid;
    }
    else
    {
        RPC_DBG_GPRINTF(("(rpc__bsd_socket_getpeereid) error=%d\n", serr));
    }
#elif (defined(HAVE_GETPEEREID) && HAVE_DECL_GETPEEREID)
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;
    uid_t uid = -1;
    gid_t gid = -1;

    RPC_SOCKET_DISABLE_CANCEL;
    serr = (getpeereid(lrpc->fd, &uid, &gid) == -1) ? SocketErrno : RPC_C_SOCKET_OK;
    RPC_SOCKET_RESTORE_CANCEL;
    if (serr == RPC_C_SOCKET_OK)
    {
        *euid = uid;
	*egid = gid;
    }
    else
    {
        RPC_DBG_GPRINTF(("(rpc__bsd_socket_getpeereid) error=%d\n", serr));
    }
#elif !defined(_WIN32)
    serr = rpc__bsd_socket_recvpeereid(sock, euid, egid);
#endif

    return serr;
}


#if !defined(SO_PEERCRED) && !(defined(HAVE_GETPEEREID) && HAVE_DECL_GETPEEREID) && !defined(_WIN32)

INTERNAL rpc_socket_error_t rpc__bsd_socket_sendpeereid
(
    rpc_socket_t        sock,
    rpc_addr_p_t        addr
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;
    struct sockaddr_un *endpoint_addr = NULL;
    struct stat endpoint_stat = {0};
    uid_t ep_uid = -1;
    int pipefd[2] = {-1, -1};
    char empty_buf[] = {'\0'};
    rpc_socket_iovec_t iovec = {0};
    union
    {
        /* Using union ensures correct alignment on some platforms */
        struct cmsghdr cm;
        char buf[CMSG_SPACE(sizeof(pipefd[0]))];
    } cm_un;
    struct msghdr msg = {0};
    struct cmsghdr *cmsg = NULL;
    int bytes_sent = 0;

    endpoint_addr = (struct sockaddr_un *)(&addr->sa);

    if (stat(endpoint_addr->sun_path, &endpoint_stat))
    {
        serr = errno;
	goto error;
    }

    ep_uid = endpoint_stat.st_uid;
    if (ep_uid == 0 || ep_uid == getuid())
    {
        if (pipe(pipefd) != 0)
	{
            serr = errno;
            goto error;
	}
    }

    iovec.iov_base     = &empty_buf;
    iovec.iov_len      = sizeof(empty_buf);

    msg.msg_iov        = &iovec;
    msg.msg_iovlen     = 1;
    msg.msg_control    = cm_un.buf;
    msg.msg_controllen = sizeof(cm_un.buf);
    msg.msg_flags      = 0;

    memset(&cm_un, 0, sizeof(cm_un));

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_RIGHTS;
    cmsg->cmsg_len   = CMSG_LEN(sizeof(pipefd[0]));

    memcpy(CMSG_DATA(cmsg), &pipefd[0], sizeof(pipefd[0]));

    RPC_SOCKET_DISABLE_CANCEL;
    bytes_sent = sendmsg(lrpc->fd, &msg, 0);
    RPC_SOCKET_RESTORE_CANCEL;
    if (bytes_sent == -1)
    {
        serr = SocketErrno;
        goto error;
    }

cleanup:

    if (pipefd[0] != -1)
    {
        close(pipefd[0]);
    }

    if (pipefd[1] != -1)
    {
        close(pipefd[1]);
    }

    return serr;

error:

    goto cleanup;
}


INTERNAL rpc_socket_error_t rpc__bsd_socket_recvpeereid
(
    rpc_socket_t        sock,
    uid_t		*euid,
    gid_t		*egid
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;
    int fd = -1;
    int bytes_rcvd = 0;
    struct stat pipe_stat = {0};
    char empty_buf[] = {'\0'};
    rpc_socket_iovec_t iovec = {0};
    union
    {
        /* Using union ensures correct alignment on some platforms */
        struct cmsghdr cm;
        char buf[CMSG_SPACE(sizeof(fd))];
    } cm_un;
    struct cmsghdr *cmsg = NULL;
    struct msghdr msg = {0};


    iovec.iov_base = &empty_buf;
    iovec.iov_len  = sizeof(empty_buf);

    memset(&cm_un, 0, sizeof(cm_un));

    msg.msg_iov        = &iovec;
    msg.msg_iovlen     = 1;
    msg.msg_control    = cm_un.buf;
    msg.msg_controllen = sizeof(cm_un.buf);
    msg.msg_flags      = 0;

    RPC_SOCKET_DISABLE_CANCEL;
    bytes_rcvd = recvmsg(lrpc->fd, &msg, 0);
    RPC_SOCKET_RESTORE_CANCEL;
    if (bytes_rcvd == -1)
    {
        serr = SocketErrno;
        goto error;
    }

    if (msg.msg_controllen == 0 ||
        msg.msg_controllen > sizeof(cm_un))
    {
        serr = RPC_C_SOCKET_EACCESS;
        goto error;
    }

    cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg ||
	!(cmsg->cmsg_type == SCM_RIGHTS) ||
        cmsg->cmsg_len - CMSG_ALIGN(sizeof(*cmsg)) != sizeof(fd))
    {
        serr = RPC_C_SOCKET_EACCESS;
        goto error;
    }

    memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));

    if (fstat(fd, &pipe_stat))
    {
        serr = errno;
        goto error;
    }

    if (!S_ISFIFO(pipe_stat.st_mode) ||
        (pipe_stat.st_mode & (S_IRWXO | S_IRWXG)) != 0)
    {
        serr = RPC_C_SOCKET_EACCESS;
        goto error;
    }

    *euid = pipe_stat.st_uid;
    *egid = pipe_stat.st_gid;

cleanup:

    if (fd > 0)
    {
        close(fd);
    }

    return serr;

error:

    *euid = -1;
    *egid = -1;

    goto cleanup;
}

#endif



/*
 * R P C _ _ S O C K E T _ C R E A T E S E S S I O N K E Y
 *
 * Generate new session key
 */

INTERNAL rpc_socket_error_t rpc__bsd_socket_createsessionkey
(
    unsigned char **session_key,
    unsigned16     *session_key_len
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    unsigned16 key_len = 0;
    unsigned char *key = NULL;
#if 0
    unsigned32 seed = 0;
    unsigned32 rand = 0;
    unsigned32 i = 0;
    unsigned32 offset = 0;

    /* Reseed the random number generator. Since this is a local connection
       it doesn't have to come from high-entropy source */
    seed  = (unsigned32)time(NULL);
    seed *= (unsigned32)getpid();

    RPC_RANDOM_INIT(seed);
#endif

    /* Default key length is 16 bytes */
    key_len = 16;

    key = malloc(key_len);
    if (!key)
    {
        serr = ENOMEM;
        goto cleanup;
    }

#if 0
    for (i = 0; i < (key_len / sizeof(rand)); i++)
    {
        rand = RPC_RANDOM_GET(1, 0xffffffff);
	offset = i * sizeof(rand);

	key[0 + offset] = (unsigned char)((rand >> 24) & 0xff);
	key[1 + offset] = (unsigned char)((rand >> 16) & 0xff);
	key[2 + offset] = (unsigned char)((rand >> 8) & 0xff);
	key[3 + offset] = (unsigned char)((rand) & 0xff);
    }
#endif

    /* Since we only generate this session key for ncalrpc connections,
       and UNIX domain sockets are not sniffable by unpriveledged users,
       we can use a well-known session key of all zeros */
    memset(key, 0, key_len);

    *session_key     = key;
    *session_key_len = key_len;

cleanup:
    return serr;
}


#if !defined(_WIN32) || defined(HAS_NCAL_RPC)
INTERNAL rpc_socket_error_t rpc__bsd_socket_sendsessionkey
(
    rpc_socket_t        sock,
    unsigned char      *session_key,
    unsigned16          session_key_len
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;
    rpc_socket_iovec_t iovec = {0};
    struct msghdr msg = {0};
    int bytes_sent = 0;

    iovec.iov_base     = session_key;
    iovec.iov_len      = session_key_len;

    msg.msg_iov        = &iovec;
    msg.msg_iovlen     = 1;
    msg.msg_flags      = 0;

    RPC_SOCKET_DISABLE_CANCEL;
    bytes_sent = sendmsg(lrpc->fd, &msg, 0);
    RPC_SOCKET_RESTORE_CANCEL;
    if (bytes_sent == -1)
    {
        serr = SocketErrno;
        goto error;
    }

cleanup:
    return serr;

error:

    goto cleanup;
}


INTERNAL rpc_socket_error_t rpc__bsd_socket_recvsession_key
(
    rpc_socket_t        sock,
    unsigned char     **session_key,
    unsigned16 	       *session_key_len
)
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;
    int bytes_rcvd = 0;
    unsigned char buffer[512] = {0};
    rpc_socket_iovec_t iovec = {0};
    struct msghdr msg = {0};
    unsigned char *key = NULL;
    unsigned16 key_len = 0;

    iovec.iov_base = buffer;
    iovec.iov_len  = sizeof(buffer);

    msg.msg_iov        = &iovec;
    msg.msg_iovlen     = 1;
    msg.msg_flags      = 0;

    RPC_SOCKET_DISABLE_CANCEL;
    bytes_rcvd = recvmsg(lrpc->fd, &msg, 0);
    RPC_SOCKET_RESTORE_CANCEL;
    if (bytes_rcvd == -1)
    {
        serr = SocketErrno;
        goto error;
    }

    key_len = bytes_rcvd;
    key = malloc(key_len);
    if (!key)
    {
        serr = ENOMEM;
        goto error;
    }

    memcpy(key, buffer, key_len);

    *session_key     = key;
    *session_key_len = key_len;

cleanup:
    return serr;

error:
    *session_key     = NULL;
    *session_key_len = 0;

    goto cleanup;
}
#endif /* defined(HAS_NCAL_RPC) */


INTERNAL
int rpc__bsd_socket_get_select_desc(
    rpc_socket_t sock
    )
{
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;

    return lrpc->fd;
}

#ifdef _WIN32
INTERNAL DWORD
rpc__win64_alloc_ipv4_adapter_table(
    PMIB_IPADDRTABLE *ppIpAddrTable)
{
    PMIB_IPADDRTABLE pIpAddrTable = NULL;
    MIB_IPADDRTABLE ipAddrTable = {0};
    DWORD dwSize = 0;
    DWORD dwError = 0;

    pIpAddrTable = &ipAddrTable;
    dwError = GetIpAddrTable(pIpAddrTable, &dwSize, 0);
    if (dwError == ERROR_INSUFFICIENT_BUFFER)
    {
        pIpAddrTable = calloc(dwSize, sizeof(BYTE));
        if (!pIpAddrTable)
        {
            return EINVAL;
        }
    }
    dwError = GetIpAddrTable(pIpAddrTable, &dwSize, dwSize);
    if (dwError == NO_ERROR)
    {
        *ppIpAddrTable = pIpAddrTable;
    }
    return dwError;
}

INTERNAL DWORD
rpc__win64_alloc_ipv6_adapter_info(PIP_ADAPTER_ADDRESSES *ppAdapter)
{
    PIP_ADAPTER_ADDRESSES pAdapter = NULL;
    ULONG adapterLen = 0;
    ULONG rsts = 0;

    /* Probe for buffer size */
    rsts = GetAdaptersAddresses(
               AF_INET6,
               GAA_FLAG_INCLUDE_PREFIX,
               NULL,
               NULL,
               (PULONG) &adapterLen);
    if (rsts != ERROR_BUFFER_OVERFLOW)
    {
        goto error;
    }

    pAdapter = (PIP_ADAPTER_ADDRESSES) calloc(adapterLen, sizeof(BYTE));
    if (!pAdapter)
    {
        rsts = errno;
        goto error;
    }

    /* This call should not fail, unless there isn't IPv6 support */
    rsts = GetAdaptersAddresses(
               AF_INET6,
               GAA_FLAG_INCLUDE_PREFIX,
               NULL,
               pAdapter,
               (PULONG) &adapterLen);
    if (rsts)
    {
        goto error;
    }
    *ppAdapter = pAdapter;

error:
    if (rsts)
    {
        if (pAdapter)
        {
            free(pAdapter);
        }
    }
    return rsts;
}

INTERNAL int
rpc__winx64_ipaddr_to_rpc_addr(
    unsigned32 ip_addr,
    rpc_addr_p_t *rpc_ip_addr)
{
    rpc_socket_error_t err = -1;
    rpc_ip_addr_p_t ret_rpc_ip_addr = NULL;

    RPC_MEM_ALLOC (
        (rpc_ip_addr_p_t) ret_rpc_ip_addr,
        rpc_ip_addr_p_t,
        sizeof (rpc_ip_addr_t),
        RPC_C_MEM_RPC_ADDR,
        RPC_C_MEM_WAITOK);
    if (!ret_rpc_ip_addr)
    {
        err = ENOMEM;
        goto error;
    }
    memset(ret_rpc_ip_addr, 0, sizeof (rpc_ip_addr_t));

    ret_rpc_ip_addr->rpc_protseq_id = RPC_C_PROTSEQ_ID_NCACN_IP_TCP;
    ret_rpc_ip_addr->len = sizeof(struct sockaddr_in);
    memcpy(&ret_rpc_ip_addr->sa.sin_addr.s_addr, &ip_addr, sizeof(ip_addr));
    ret_rpc_ip_addr->sa.sin_family = AF_INET;

    err = 0;
    *rpc_ip_addr = (rpc_addr_p_t) ret_rpc_ip_addr;

error:
    if (err)
    {
        if (ret_rpc_ip_addr)
        {
            RPC_MEM_FREE(ret_rpc_ip_addr, RPC_C_MEM_RPC_ADDR);
        }
    }

    return err;
}

INTERNAL int
rpc__winx64_s6addr_to_rpc_addr(
    struct sockaddr *sa_addr,
    rpc_addr_p_t *rpc_ip6_addr)
{
    rpc_socket_error_t err = -1;
    rpc_ip6_addr_p_t ret_rpc_ip6_addr = NULL;

    RPC_MEM_ALLOC (
        (rpc_ip_addr_p_t) ret_rpc_ip6_addr,
        rpc_ip_addr_p_t,
        sizeof (rpc_ip6_addr_t),
        RPC_C_MEM_RPC_ADDR,
        RPC_C_MEM_WAITOK);
    if (!ret_rpc_ip6_addr)
    {
        err = ENOMEM;
        goto error;
    }
    memset(ret_rpc_ip6_addr, 0, sizeof (rpc_ip6_addr_t));

    ret_rpc_ip6_addr->rpc_protseq_id = RPC_C_PROTSEQ_ID_NCACN_IP6_TCP;
    ret_rpc_ip6_addr->len = sizeof(struct sockaddr_in6);
    memcpy(&ret_rpc_ip6_addr->sa, sa_addr, sizeof(struct sockaddr_in6));

    err = 0;
    *rpc_ip6_addr = (rpc_addr_p_t) ret_rpc_ip6_addr;

error:
    if (err)
    {
        if (ret_rpc_ip6_addr)
        {
            RPC_MEM_FREE(ret_rpc_ip6_addr, RPC_C_MEM_RPC_ADDR);
        }
    }

    return err;
}


INTERNAL int
rpc__winx64_s6addr_to_netmask_rpc_addr(
    struct sockaddr *sa_addr,
    rpc_addr_p_t *rpc_addr)
{
    rpc_socket_error_t err = -1;
    rpc_ip6_addr_p_t ret_rpc_ip6_addr = NULL;
    struct sockaddr_in6 *src6_addr = (struct sockaddr_in6 *) sa_addr;
    struct sockaddr_in6 *dst6_addr = NULL;
    unsigned char link_layer_prefix[] = {'\xfe', '\x80'};

    RPC_MEM_ALLOC (
        (rpc_ip_addr_p_t) ret_rpc_ip6_addr,
        rpc_ip_addr_p_t,
        sizeof (rpc_ip6_addr_t),
        RPC_C_MEM_RPC_ADDR,
        RPC_C_MEM_WAITOK);
    if (!ret_rpc_ip6_addr)
    {
        err = ENOMEM;
        goto error;
    }
    memset(ret_rpc_ip6_addr, 0, sizeof (rpc_ip6_addr_t));
    dst6_addr = (struct sockaddr_in6 *) &ret_rpc_ip6_addr->sa;

    ret_rpc_ip6_addr->rpc_protseq_id = RPC_C_PROTSEQ_ID_NCACN_IP6_TCP;
    ret_rpc_ip6_addr->len = sizeof(struct sockaddr_in6);
    memcpy(&ret_rpc_ip6_addr->sa, src6_addr, sizeof(struct sockaddr_in6));
    memset(&dst6_addr->sin6_addr, 0, sizeof(struct in6_addr));

    /*
     * The trick here assumes the netmask is the first 64 bits of the
     * IPv6 address. The exception is for link-layer addresses, which
     * don't have a network mask. Skip addresses with link-layer
     * 0xfe80... prefix.
     */
    if (memcmp(link_layer_prefix,
                &src6_addr->sin6_addr,
                sizeof(link_layer_prefix)) != 0)
    {
        memcpy(&dst6_addr->sin6_addr,
               &src6_addr->sin6_addr,
               sizeof(struct in6_addr)/2);
    }

    err = 0;
    *rpc_addr = (rpc_addr_p_t) ret_rpc_ip6_addr;

error:
    if (err)
    {
        if (ret_rpc_ip6_addr)
        {
            RPC_MEM_FREE(ret_rpc_ip6_addr, RPC_C_MEM_RPC_ADDR);
        }
    }

    return err;
}


INTERNAL rpc_socket_error_t
rpc__winx64_socket_alloc_vectors(
    int count,
    rpc_addr_vector_p_t *rpc_addr_vec,
    rpc_addr_vector_p_t *netmask_addr_vec,
    rpc_addr_vector_p_t *broadcast_addr_vec)
{
    rpc_socket_error_t err = -1;
    rpc_addr_vector_p_t ret_rpc_addr_vec = NULL;
    rpc_addr_vector_p_t ret_netmask_addr_vec = NULL;
    rpc_addr_vector_p_t ret_broadcast_addr_vec = NULL;

    if (rpc_addr_vec)
    {
        RPC_MEM_ALLOC(ret_rpc_addr_vec,
                  rpc_addr_vector_p_t,
                  sizeof(*ret_rpc_addr_vec)+(count*sizeof(rpc_addr_p_t)),
                  RPC_C_MEM_RPC_ADDR_VEC,
                  RPC_C_MEM_WAITOK);
        if (ret_rpc_addr_vec == NULL)
        {
            err = ENOMEM;
            goto error;
        }
        memset(ret_rpc_addr_vec, 0, count*sizeof(ret_rpc_addr_vec));
    }

    if (netmask_addr_vec)
    {
        RPC_MEM_ALLOC(ret_netmask_addr_vec,
                  rpc_addr_vector_p_t,
                  sizeof(*ret_netmask_addr_vec)+(count*sizeof(rpc_addr_p_t)),
                  RPC_C_MEM_RPC_ADDR_VEC,
                  RPC_C_MEM_WAITOK);
        if (ret_netmask_addr_vec == NULL)
        {
            err = ENOMEM;
            goto error;
        }
        memset(ret_netmask_addr_vec, 0, count*sizeof(ret_netmask_addr_vec));
    }

    if (broadcast_addr_vec)
    {
        RPC_MEM_ALLOC(ret_broadcast_addr_vec,
                  rpc_addr_vector_p_t,
                  sizeof(*ret_broadcast_addr_vec)+(count*sizeof(rpc_addr_p_t)),
                  RPC_C_MEM_RPC_ADDR_VEC,
                  RPC_C_MEM_WAITOK);
        if (ret_broadcast_addr_vec == NULL)
        {
            err = ENOMEM;
            goto error;
        }
        memset(ret_broadcast_addr_vec, 0, count*sizeof(ret_broadcast_addr_vec));
    }

    err = 0;
    if (rpc_addr_vec)
    {
        *rpc_addr_vec = ret_rpc_addr_vec;
    }
    if (netmask_addr_vec)
    {
        *netmask_addr_vec = ret_netmask_addr_vec;
    }
    if (broadcast_addr_vec)
    {
        *broadcast_addr_vec = ret_broadcast_addr_vec;
    }

error:
    if (err)
    {
        if (ret_rpc_addr_vec)
        {
            RPC_MEM_FREE (ret_rpc_addr_vec, RPC_C_MEM_RPC_ADDR_VEC);
        }
        if (ret_netmask_addr_vec)
        {
            RPC_MEM_FREE (ret_netmask_addr_vec, RPC_C_MEM_RPC_ADDR_VEC);
        }
        if (ret_broadcast_addr_vec)
        {
            RPC_MEM_FREE (ret_broadcast_addr_vec, RPC_C_MEM_RPC_ADDR_VEC);
        }
    }

    return err;
}

INTERNAL void
rpc__winx64_socket_free_vectors(
    rpc_addr_vector_p_t address_vec,
    rpc_addr_vector_p_t netmask_vec,
    rpc_addr_vector_p_t broadcast_vec)
{
    DWORD i = 0;

    if (address_vec)
    {
        for (i=0; i < address_vec->len; i++)
        {
            RPC_MEM_FREE (address_vec->addrs[i], RPC_C_MEM_RPC_ADDR);
        }
        RPC_MEM_FREE (address_vec, RPC_C_MEM_RPC_ADDR_VEC);
    }
    if (netmask_vec)
    {
        for (i=0; i < netmask_vec->len; i++)
        {
            RPC_MEM_FREE (netmask_vec->addrs[i], RPC_C_MEM_RPC_ADDR);
        }
        RPC_MEM_FREE (netmask_vec, RPC_C_MEM_RPC_ADDR_VEC);
    }
    if (broadcast_vec)
    {
        for (i=0; i < broadcast_vec->len; i++)
        {
            RPC_MEM_FREE (broadcast_vec->addrs[i], RPC_C_MEM_RPC_ADDR);
        }
        RPC_MEM_FREE (broadcast_vec, RPC_C_MEM_RPC_ADDR_VEC);
    }
}

INTERNAL void
_rpc__process_addr_entry(
    rpc_addr_p_t *address,
    rpc_addr_p_t *netmask,
    rpc_addr_p_t *broadcast,
    rpc_addr_vector_p_t address_vec,
    rpc_addr_vector_p_t netmask_vec,
    rpc_addr_vector_p_t broadcast_vec)
{
    /* Populate address vectors provided by caller */
    if (address_vec)
    {
        address_vec->addrs[address_vec->len++] = *address;
        *address = NULL;
    }
    if (netmask_vec)
    {
        netmask_vec->addrs[netmask_vec->len++] = *netmask;
        *netmask = NULL;
    }
    if (broadcast_vec)
    {
        broadcast_vec->addrs[broadcast_vec->len++] = *broadcast;
        *broadcast = NULL;
    }

    /* Free address entries not appended to above vectors */
    _rpc__bsd_free_if_entry(address, netmask, broadcast);
}


INTERNAL rpc_socket_error_t
rpc__winx64_socket_enum_ifaces(
    rpc_socket_t sock,
    rpc_socket_enum_iface_fn_p_t efun,
    rpc_addr_vector_p_t *rpc_addr_vec,
    rpc_addr_vector_p_t *netmask_addr_vec,
    rpc_addr_vector_p_t *broadcast_addr_vec
)
{
    rpc_socket_error_t err = -1;
    int sts = 0;
    DWORD dwError = 0;
    DWORD dwError2 = 0;
    PMIB_IPADDRTABLE pIpV4AddrTable = NULL;
    PIP_ADAPTER_INFO pIpV4Adapter = NULL;
    PIP_ADAPTER_INFO pIpV4AdapterNext = NULL;
    PIP_ADAPTER_ADDRESSES pIpV6Adapter = NULL;
    PIP_ADAPTER_ADDRESSES pIpV6AdapterNext = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pIpV6UnicastNext = NULL;
    PIP_ADDR_STRING  pIpAddrString = NULL;
    int addrCount = 0;
    rpc_addr_vector_p_t ret_rpc_addr_vec = NULL;
    rpc_addr_vector_p_t ret_netmask_addr_vec = NULL;
    rpc_addr_vector_p_t ret_broadcast_addr_vec = NULL;
    char broadcast_addr_str[16] = {0}; // xxx.xxx.xxx.xxx\0
    rpc_socket_enum_iface_fn_p_t protseq_efun = _rpc__bsd_protoseq_filter_cb;
    DWORD i = 0;
    boolean keep_entry = true;

    rpc_addr_p_t ip_addr = NULL;
    rpc_addr_p_t netmask_addr = NULL;
    rpc_addr_p_t broadcast_addr = NULL;
    struct sockaddr_in6 mock_broadcast_addr;
    struct sockaddr *saddr = NULL;

    /*
     * Get information about network interfaces for further
     * processing. Note: pIpV4Adapter->IpAddressList contains
     * all IP addresses for the current adapter, and the subnet
     * mask. Count all adapters and IP addresses for
     * each adapter.
     */
    dwError = rpc__win64_alloc_ipv4_adapter_table(&pIpV4AddrTable);
    dwError2 = rpc__win64_alloc_ipv6_adapter_info(&pIpV6Adapter);
    if (dwError && dwError2)
    {
        /* Fatal error not having both IPv4 or IPv6 interfaces */
        err = dwError;    // TBD: Map to RPC error space
        goto error;
    }
    if (pIpV4AddrTable)
    {
        addrCount += pIpV4AddrTable->dwNumEntries;
    }

    /*
     * Enumerate the number of IPv6 interfaces. This count is used to allocate
     * memory for the return addr, netmask and broadcast vectors.
     */
    for (pIpV6AdapterNext = pIpV6Adapter;
         pIpV6AdapterNext;
         pIpV6AdapterNext = pIpV6AdapterNext->Next)
    {
        for (pIpV6UnicastNext = pIpV6AdapterNext->FirstUnicastAddress;
             pIpV6UnicastNext != NULL;
             pIpV6UnicastNext = pIpV6UnicastNext->Next)
        {
            /*
             * Don't need to work through the unicast address list.
             * The list contents are redundant.
             */
            addrCount++;
        }
    }
    err = rpc__winx64_socket_alloc_vectors(
              addrCount,
              rpc_addr_vec       ? &ret_rpc_addr_vec       : NULL,
              netmask_addr_vec   ? &ret_netmask_addr_vec   : NULL,
              broadcast_addr_vec ? &ret_broadcast_addr_vec : NULL);
    if (err)
    {
        goto error;
    }

    for (i = 0; pIpV4AddrTable && i < pIpV4AddrTable->dwNumEntries; i++)
    {
        keep_entry = true;
        err = rpc__winx64_ipaddr_to_rpc_addr(pIpV4AddrTable->table[i].dwAddr, &ip_addr);
        if (err)
        {
            goto error;
        }

        err = rpc__winx64_ipaddr_to_rpc_addr(pIpV4AddrTable->table[i].dwMask, &netmask_addr);
        if (err)
        {
            goto error;
        }
        err = rpc__winx64_ipaddr_to_rpc_addr(pIpV4AddrTable->table[i].dwBCastAddr, &broadcast_addr);
        if (err)
        {
            goto error;
        }

        /*
         * Default filtering out of IP address that doesn't match the
         * protocol sequence that was requested. This is determined by
         * the sock->pseq_id.
         */
        keep_entry &= protseq_efun(sock,
                         ip_addr,
                         netmask_addr,
                         broadcast_addr);
        if (efun)
        {
            keep_entry &= efun(sock,
                             ip_addr,
                             netmask_addr,
                             broadcast_addr);
        }
        if (!keep_entry)
        {
            _rpc__bsd_free_if_entry(&ip_addr,
                                    &netmask_addr,
                                    &broadcast_addr);
            continue;
        }

        _rpc__process_addr_entry(
            &ip_addr,
            &netmask_addr,
            &broadcast_addr,
            ret_rpc_addr_vec,
            ret_netmask_addr_vec,
            ret_broadcast_addr_vec);
    }

    /*
     * Walk through the IPv6 interfaces, populating the address / netmask
     * fields. Note: There isn't a broadcast mask in IPv6.
     */
    memset(&mock_broadcast_addr, 0, sizeof(mock_broadcast_addr));
    mock_broadcast_addr.sin6_family = AF_INET6;

    for (pIpV6AdapterNext = pIpV6Adapter;
         pIpV6AdapterNext;
         pIpV6AdapterNext = pIpV6AdapterNext->Next)
    {
        keep_entry = true;

        /* Skip interface that isn't Ethernet */
        if (pIpV6AdapterNext->IfType != IF_TYPE_ETHERNET_CSMACD)
        {
            continue;
        }

        /* Skip interface with a "down" status */
        if (pIpV6AdapterNext->OperStatus != 1)
        {
            continue;
        }

        for (pIpV6UnicastNext = pIpV6AdapterNext->FirstUnicastAddress;
             pIpV6UnicastNext != NULL;
             pIpV6UnicastNext = pIpV6UnicastNext->Next)
        {
            saddr = pIpV6UnicastNext->Address.lpSockaddr;
            err = rpc__winx64_s6addr_to_rpc_addr(
                      saddr,
                      &ip_addr);
            if (err)
            {
                goto error;
            }
            err = rpc__winx64_s6addr_to_netmask_rpc_addr(
                      saddr,
                      &netmask_addr);
            if (err)
            {
                goto error;
            }
            err = rpc__winx64_s6addr_to_rpc_addr(
                      (struct sockaddr *) &mock_broadcast_addr,
                      &broadcast_addr);
            if (err)
            {
                goto error;
            }

            /*
             * Default filtering out of IP address that doesn't match the
             * protocol sequence that was requested. This is determined by
             * the sock->pseq_id.
             */
            keep_entry &= protseq_efun(sock,
                             ip_addr,
                             netmask_addr,
                             broadcast_addr);
            if (efun)
            {
                keep_entry &= efun(sock,
                                 ip_addr,
                                 netmask_addr,
                                 broadcast_addr);
            }
            if (!keep_entry)
            {
                _rpc__bsd_free_if_entry(&ip_addr,
                                        &netmask_addr,
                                        &broadcast_addr);
                continue;
            }

            _rpc__process_addr_entry(
                &ip_addr,
                &netmask_addr,
                &broadcast_addr,
                ret_rpc_addr_vec,
                ret_netmask_addr_vec,
                ret_broadcast_addr_vec);
        }
    }

    /* Success. Assign return values to caller */
    err = 0;
    if (rpc_addr_vec)
    {
        *rpc_addr_vec = ret_rpc_addr_vec;
    }
    if (netmask_addr_vec)
    {
        *netmask_addr_vec = ret_netmask_addr_vec;
    }
    if (broadcast_addr_vec)
    {
        *broadcast_addr_vec = ret_broadcast_addr_vec;
    }

error:
    if (err)
    {
        _rpc__bsd_free_if_entry(&ip_addr, &netmask_addr, &broadcast_addr);
        rpc__winx64_socket_free_vectors(
            ret_rpc_addr_vec,
            ret_netmask_addr_vec,
            ret_broadcast_addr_vec);
    }

    if (pIpV4AddrTable)
    {
        free(pIpV4AddrTable);
    }

    if (pIpV6Adapter)
    {
        free(pIpV6Adapter);
    }

    return err;
}


/*
 * Map Winsock2 errval to errno value
 */
PRIVATE
rpc_socket_error_t
rpc__winx64_winerr_to_errno(DWORD WSAErrval)
{
    rpc_socket_error_t err = 0;

    switch (WSAErrval)
    {
      case WSA_INVALID_HANDLE:
        err = EINVAL;
        break;
      case WSA_NOT_ENOUGH_MEMORY:
        err = ENOMEM;
        break;
      case WSA_INVALID_PARAMETER:
        err = EINVAL;
        break;
      case WSAEINTR:
        err = EINTR;
        break;
      case WSAEBADF:
        err = EBADF;
        break;
      case WSAEACCES:
        err = RPC_C_SOCKET_EACCESS;
        break;
      case WSAEFAULT:
        err = EFAULT;
        break;
      case WSAEINVAL:
        err = EINVAL;
        break;
      case WSAEMFILE:
        err = EMFILE;
        break;
      case WSAEWOULDBLOCK:
        err = RPC_C_SOCKET_EWOULDBLOCK;
        break;
      case WSAEINPROGRESS:
        err = RPC_C_SOCKET_EINPROGRESS;
        break;
      case WSAEALREADY:
        err = RPC_C_SOCKET_EALREADY;
        break;
      case WSAENOTSOCK:
        err = RPC_C_SOCKET_ENOTSOCK;
        break;
//      case WSAEDESTADDRREQ:
//        err = RPC_C_SOCKET_EDESTADDRREQ;
//      break;
//    case WSAEMSGSIZE:
//      err = RPC_C_SOCKET_EMSGSIZE;
//      break;
//    case WSAEPROTOTYPE:
//      err = RPC_C_SOCKET_EPROTOTYPE;
//      break;
//    case WSAENOPROTOOPT:
//      err = RPC_C_SOCKET_ENOPROTOOPT;
//      break;
//    case WSAEPROTONOSUPPORT:
//      err = RPC_C_SOCKET_EPROTONOSUPPORT;
//      break;
//    case WSAESOCKTNOSUPPORT:
//      err = RPC_C_SOCKET_ESOCKTNOSUPPORT;
//      break;
//    case WSAEOPNOTSUPP:
//      err = RPC_C_SOCKET_EOPNOTSUPP;
//      break;
//    case WSAEPFNOSUPPORT:
//      err = RPC_C_SOCKET_EPFNOSUPPORT;
//      break;
//    case WSAEAFNOSUPPORT:
//      err = RPC_C_SOCKET_EAFNOSUPPORT;
//      break;
      case WSAEADDRINUSE:
        err = RPC_C_SOCKET_EADDRINUSE;
        break;
//    case WSAEADDRNOTAVAIL:
//      err = RPC_C_SOCKET_EADDRNOTAVAIL;
//      break;
      case WSAENETDOWN:
        err = RPC_C_SOCKET_ENETDOWN;
        break;
      case WSAENETUNREACH:
        err = RPC_C_SOCKET_ENETUNREACH;
        break;
      case WSAENETRESET:
        err = RPC_C_SOCKET_ENETRESET;
        break;
      case WSAECONNABORTED:
        err = RPC_C_SOCKET_ECONNABORTED;
        break;
      case WSAECONNRESET:
        err = RPC_C_SOCKET_ECONNRESET;
        break;
//    case WSAENOBUFS:
//      err = RPC_C_SOCKET_ENOBUFS;
//      break;
      case WSAEISCONN:
        err = RPC_C_SOCKET_EISCONN;
        break;
//    case WSAENOTCONN:
//      err = RPC_C_SOCKET_ENOTCONN;
//      break;
//    case WSAESHUTDOWN:
//      err = RPC_C_SOCKET_ESHUTDOWN;
//      break;
      case WSAETOOMANYREFS:
        err = RPC_C_SOCKET_ETOOMANYREFS;
        break;
      case WSAETIMEDOUT:
        err = ETIMEDOUT;
        break;
      case WSAECONNREFUSED:
        err = RPC_C_SOCKET_ECONNREFUSED;
        break;
//    case WSAELOOP:
//      err = RPC_C_SOCKET_ELOOP;
//      break;
      case WSAENAMETOOLONG:
        err = ENAMETOOLONG;
        break;
      case WSAEHOSTDOWN:
        err = RPC_C_SOCKET_EHOSTDOWN;
        break;
      case WSAEHOSTUNREACH:
        err = RPC_C_SOCKET_EHOSTUNREACH;
        break;
      case WSAENOTEMPTY:
        err = ENOTEMPTY;
        break;
//    case WSAEPROCLIM:
//      err = RPC_C_SOCKET_EPROCLIM;
//      break;
//    case WSAEUSERS:
//      err = RPC_C_SOCKET_EUSERS;
//      break;
//    case WSAEDQUOT:
//      err = RPC_C_SOCKET_EDQUOT;
//      break;
//    case WSAESTALE:
//      err = RPC_C_SOCKET_ESTALE;
//      break;
//    case WSAEREMOTE:
//      err = RPC_C_SOCKET_EREMOTE;
      default:
        err = WSAErrval;
        break;
    }
    return err;
}

/*
 * Convenience routine that maps WSA error value to UNIX error
 * space, and then returns -1. This implements the semantic of
 * a function that fails with -1 return status, and sets errno
 * to indicate the failure.
 */
PRIVATE
int
rpc__winx64_set_errno(DWORD WSAErrval)
{
    errno = rpc__winx64_winerr_to_errno(WSAErrval);
    return -1;
}

#endif /* _WIN32 */

INTERNAL void
_rpc__bsd_free_if_entry(
    rpc_addr_p_t  *ip_addr,
    rpc_addr_p_t  *netmask_addr,
    rpc_addr_p_t  *broadcast_addr)
{
    if (ip_addr && *ip_addr)
    {
        RPC_MEM_FREE((*ip_addr), RPC_C_MEM_RPC_ADDR);
        *ip_addr = NULL;
    }
    if (netmask_addr && *netmask_addr)
    {
        RPC_MEM_FREE((*netmask_addr), RPC_C_MEM_RPC_ADDR);
        *netmask_addr = NULL;
    }
    if (broadcast_addr && *broadcast_addr)
    {
        RPC_MEM_FREE((*broadcast_addr), RPC_C_MEM_RPC_ADDR);
        *broadcast_addr = NULL;
    }
}


INTERNAL boolean
_rpc__bsd_protoseq_filter_cb(
    rpc_socket_t sock,
    rpc_addr_p_t ip_addr,
    rpc_addr_p_t netmask_addr,
    rpc_addr_p_t broadcast_addr
    )
{
    int pseq = -1;

    if (!sock)
    {
        return false;
    }
    pseq = sock->pseq_id;

    if (ip_addr && ip_addr->rpc_protseq_id != pseq)
    {
        return false;
    }
    if (netmask_addr && netmask_addr->rpc_protseq_id != pseq)
    {
        return false;
    }
    if (broadcast_addr && broadcast_addr->rpc_protseq_id != pseq)
    {
        return false;
    }
    return true;
}


INTERNAL
rpc_socket_error_t
rpc__bsd_socket_enum_ifaces(
    rpc_socket_t sock,
    rpc_socket_enum_iface_fn_p_t efun,
    rpc_addr_vector_p_t *rpc_addr_vec,
    rpc_addr_vector_p_t *netmask_addr_vec,
    rpc_addr_vector_p_t *broadcast_addr_vec
)
{

#if defined(_WIN32)

    rpc_socket_error_t err = -1;
    err = rpc__winx64_socket_enum_ifaces(
              sock,
              efun,
              rpc_addr_vec,
              netmask_addr_vec,
              broadcast_addr_vec);

    return err;

#else
    rpc_addr_vector_p_t ret_rpc_addr_vec = NULL;
    rpc_addr_vector_p_t ret_netmask_addr_vec = NULL;
    rpc_addr_vector_p_t ret_broadcast_addr_vec = NULL;
    rpc_addr_p_t    ip_addr = NULL;
    rpc_addr_p_t    netmask_addr = NULL;
    rpc_addr_p_t    broadcast_addr = NULL;
    rpc_socket_error_t err = 0;
    int                n_ifs = 0;
    int                sts = 0;
    int                rpc_ip_addrlen = 0;
    int                sin_addrlen = 0;
    int                i = 0;
    boolean            is_ipv6 = false;
    struct ifaddrs     *ifaddrs = NULL;
    struct ifaddrs     *ifaddrs_save = NULL;
    struct sockaddr_in *in_addr;
    rpc_socket_enum_iface_fn_p_t protseq_efun = _rpc__bsd_protoseq_filter_cb;

    sts = getifaddrs(&ifaddrs);
    if (sts == -1)
    {
        err = errno;
        goto done;
    }

    /* Save original value, as need to count number of entries initially */
    for (ifaddrs_save = ifaddrs; ifaddrs; ifaddrs = ifaddrs->ifa_next)
    {
        n_ifs++;
    }
    ifaddrs = ifaddrs_save;

    if (rpc_addr_vec)
    {
        RPC_MEM_ALLOC(
            ret_rpc_addr_vec,
            rpc_addr_vector_p_t,
            (sizeof *ret_rpc_addr_vec) + (n_ifs * (sizeof (rpc_addr_p_t))),
            RPC_C_MEM_RPC_ADDR_VEC,
            RPC_C_MEM_WAITOK);

        if (ret_rpc_addr_vec == NULL)
        {
            err = ENOMEM;
            goto done;
        }
        ret_rpc_addr_vec->len = 0;
    }

    if (netmask_addr_vec)
    {
        RPC_MEM_ALLOC(
            ret_netmask_addr_vec,
            rpc_addr_vector_p_t,
         (sizeof *ret_netmask_addr_vec) + (n_ifs * (sizeof (rpc_addr_p_t))),
            RPC_C_MEM_RPC_ADDR_VEC,
            RPC_C_MEM_WAITOK);

        if (ret_netmask_addr_vec == NULL)
        {
            err = ENOMEM;
            RPC_MEM_FREE(ret_netmask_addr_vec, RPC_C_MEM_RPC_ADDR_VEC);
            goto done;
        }

        ret_netmask_addr_vec->len = 0;
    }

    if (broadcast_addr_vec)
    {
        RPC_MEM_ALLOC(
            ret_broadcast_addr_vec,
            rpc_addr_vector_p_t,
         (sizeof *ret_broadcast_addr_vec) + (n_ifs * (sizeof (rpc_addr_p_t))),
            RPC_C_MEM_RPC_ADDR_VEC,
            RPC_C_MEM_WAITOK);

        if (ret_broadcast_addr_vec == NULL)
        {
            err = ENOMEM;
            RPC_MEM_FREE(ret_broadcast_addr_vec, RPC_C_MEM_RPC_ADDR_VEC);
            goto done;
        }

        ret_broadcast_addr_vec->len = 0;
    }

    /*
     * Go through the interfaces and get the info associated with them.
     */
    for (ifaddrs = ifaddrs_save; ifaddrs; ifaddrs = ifaddrs->ifa_next)
    {
        is_ipv6 = false;

        /*
         * Ignore interfaces which are not 'up'.
         *
         * Ignore Point-to-Point interfaces (i.e. SLIP/PPP )
         * *** NOTE:  We need an Environment Variable Evaluation at
         * some point so we can selectively allow RPC servers to
         * some up with/without SLIP/PPP bindings. For Dynamic PPP/SLIP
         * interfaces, this creates problems for now.
         */
        if ((ifaddrs->ifa_flags & IFF_UP) == 0 ||
           (ifaddrs->ifa_flags & IFF_POINTOPOINT))
        {
            continue;
        }

#ifndef USE_LOOPBACK
        /*
         * Ignore the loopback interface
         */
        if (ifaddrs->ifa_flags & IFF_LOOPBACK)
        {
            continue;
        }
#endif

        /*
         * Allow only IPv4/IPv6 addresses past this point
         */
        in_addr = (struct sockaddr_in *) ifaddrs->ifa_addr;
        if (in_addr->sin_family != AF_INET &&
            in_addr->sin_family != AF_INET6)
        {
            continue;
        }

        /*
         * Interface is definitely either IPv4 or IPv6 here
         */
        if (in_addr->sin_family == AF_INET)
        {
            rpc_ip_addrlen = sizeof(rpc_ip_addr_t);
            sin_addrlen = sizeof(struct sockaddr_in);
        }
        else
        {
            is_ipv6 = true;
            rpc_ip_addrlen = sizeof(rpc_ip6_addr_t);
            sin_addrlen = sizeof(struct sockaddr_in6);
        }

        if (ret_rpc_addr_vec)
        {
            /*
             * Allocate IPv4 or IPv6 address structure here.
             */
            RPC_MEM_ALLOC(
                ip_addr,
                rpc_addr_p_t,
                rpc_ip_addrlen,
                RPC_C_MEM_RPC_ADDR,
                RPC_C_MEM_WAITOK);

            if (!ip_addr)
            {
                err = ENOMEM;
                goto FREE_IT;
            }

            if (is_ipv6)
            {
                ip_addr->rpc_protseq_id = RPC_C_PROTSEQ_ID_NCACN_IP6_TCP;
            }
            else
            {
                ip_addr->rpc_protseq_id = RPC_C_PROTSEQ_ID_NCACN_IP_TCP;
            }
            ip_addr->len = sin_addrlen;
            memcpy(&ip_addr->sa, in_addr, sin_addrlen);
        }

        if (ret_netmask_addr_vec && (ifaddrs->ifa_flags & IFF_LOOPBACK) == 0)
        {
            RPC_MEM_ALLOC(
                netmask_addr,
                rpc_addr_p_t,
                rpc_ip_addrlen,
                RPC_C_MEM_RPC_ADDR,
                RPC_C_MEM_WAITOK);

            if (netmask_addr == NULL)
            {
                err = ENOMEM;
                goto FREE_IT;
            }

            if (is_ipv6)
            {
                netmask_addr->rpc_protseq_id = RPC_C_PROTSEQ_ID_NCACN_IP6_TCP;
            }
            else
            {
                netmask_addr->rpc_protseq_id = RPC_C_PROTSEQ_ID_NCACN_IP_TCP;
            }
            netmask_addr->len = rpc_ip_addrlen;
            in_addr = (struct sockaddr_in *) ifaddrs->ifa_netmask;
            memcpy(&netmask_addr->sa, in_addr, sin_addrlen);
        }

        if (ret_broadcast_addr_vec && (ifaddrs->ifa_flags & IFF_BROADCAST))
        {
            RPC_MEM_ALLOC(
                broadcast_addr,
                rpc_addr_p_t,
                rpc_ip_addrlen,
                RPC_C_MEM_RPC_ADDR,
                RPC_C_MEM_WAITOK);

            if (broadcast_addr == NULL)
            {
                err = ENOMEM;
                goto FREE_IT;
            }

            if (is_ipv6)
            {
                broadcast_addr->rpc_protseq_id = RPC_C_PROTSEQ_ID_NCACN_IP6_TCP;
            }
            else
            {
                broadcast_addr->rpc_protseq_id = RPC_C_PROTSEQ_ID_NCACN_IP_TCP;
            }
            broadcast_addr->len = rpc_ip_addrlen;
            in_addr = (struct sockaddr_in *) ifaddrs->ifa_broadaddr;
            memcpy(&broadcast_addr->sa, in_addr, sin_addrlen);
        }

        /*
         * Default filtering out of IP address that doesn't match the protocol
         * sequence that was requested. This is determined by the sock->pseq_id.
         */
        if (protseq_efun(sock,
                         ip_addr,
                         netmask_addr,
                         broadcast_addr) == false)
        {
            _rpc__bsd_free_if_entry(&ip_addr, &netmask_addr, &broadcast_addr);
            continue;
        }

        /*
         * Call out to do any final filtering and get the desired IP address
         * for this interface.  If the callout function returns false, we
         * forget about this interface.
         */
        if (efun(sock,
            ip_addr,
            netmask_addr,
            broadcast_addr) == false)
        {
            _rpc__bsd_free_if_entry(&ip_addr, &netmask_addr, &broadcast_addr);
            continue;
        }

        if (ret_rpc_addr_vec && ip_addr)
        {
            ret_rpc_addr_vec->addrs[ret_rpc_addr_vec->len++] = ip_addr;
            ip_addr = NULL;
        }

        if (ret_netmask_addr_vec && netmask_addr)
        {
            ret_netmask_addr_vec->addrs[ret_netmask_addr_vec->len++]
                = netmask_addr;
            netmask_addr = NULL;
        }
        if (ret_broadcast_addr_vec && broadcast_addr)
        {
            ret_broadcast_addr_vec->addrs[ret_broadcast_addr_vec->len++]
                = broadcast_addr;
            broadcast_addr = NULL;
        }
        _rpc__bsd_free_if_entry(&ip_addr, &netmask_addr, &broadcast_addr);
    }

    if (ret_rpc_addr_vec->len == 0)
    {
        err = EINVAL;   /* !!! */
        goto FREE_IT;
    }

    if (rpc_addr_vec)
    {
        *rpc_addr_vec = ret_rpc_addr_vec;
        ret_rpc_addr_vec = NULL;
    }
    if (netmask_addr_vec)
    {
        *netmask_addr_vec = ret_netmask_addr_vec;
        ret_netmask_addr_vec = NULL;
    }
    if (broadcast_addr_vec)
    {
        *broadcast_addr_vec = ret_broadcast_addr_vec;
        ret_broadcast_addr_vec = NULL;
    }
    err = RPC_C_SOCKET_OK;

done:
    if (ifaddrs_save)
    {
        freeifaddrs(ifaddrs_save);
    }
    if (ret_netmask_addr_vec)
    {
        for (i = 0; i < ret_netmask_addr_vec->len; i++)
        {
            RPC_MEM_FREE(ret_netmask_addr_vec->addrs[i], RPC_C_MEM_RPC_ADDR);
        }
        RPC_MEM_FREE(ret_netmask_addr_vec, RPC_C_MEM_RPC_ADDR_VEC);
        ret_netmask_addr_vec = NULL;
    }
    if (ret_broadcast_addr_vec)
    {
        for (i = 0; i < ret_broadcast_addr_vec->len; i++)
        {
            RPC_MEM_FREE(ret_broadcast_addr_vec->addrs[i], RPC_C_MEM_RPC_ADDR);
        }
        RPC_MEM_FREE(ret_broadcast_addr_vec, RPC_C_MEM_RPC_ADDR_VEC);
        ret_broadcast_addr_vec = NULL;
    }
    return err;

FREE_IT:

    if (ip_addr)
    {
        RPC_MEM_FREE (ip_addr, RPC_C_MEM_RPC_ADDR);
    }
    if (netmask_addr)
    {
        RPC_MEM_FREE (netmask_addr, RPC_C_MEM_RPC_ADDR);
    }
    if (broadcast_addr)
    {
        RPC_MEM_FREE (broadcast_addr, RPC_C_MEM_RPC_ADDR);
    }

    if (ret_rpc_addr_vec)
    {
        for (i = 0; i < ret_rpc_addr_vec->len; i++)
        {
            RPC_MEM_FREE(ret_rpc_addr_vec->addrs[i], RPC_C_MEM_RPC_ADDR);
        }
        RPC_MEM_FREE (ret_rpc_addr_vec, RPC_C_MEM_RPC_ADDR_VEC);
        ret_rpc_addr_vec = NULL;
    }

    goto done;
#endif
}

INTERNAL
rpc_socket_error_t
rpc__bsd_socket_inq_transport_info(
    rpc_socket_t sock ATTRIBUTE_UNUSED,
    rpc_transport_info_handle_t* info
    )
{
    rpc_socket_error_t serr = RPC_C_SOCKET_OK;
    rpc_bsd_socket_p_t lrpc = (rpc_bsd_socket_p_t) sock->data.pointer;
    rpc_bsd_transport_info_p_t lrpc_info = NULL;

    lrpc_info = calloc(1, sizeof(*lrpc_info));

    if (!lrpc_info)
    {
        serr = ENOMEM;
	goto error;
    }

    lrpc_info->peer_uid = lrpc->info.peer_uid;
    lrpc_info->peer_gid = lrpc->info.peer_gid;

    lrpc_info->session_key.data = malloc(lrpc->info.session_key.length);

    if (!lrpc_info->session_key.data)
    {
        serr = ENOMEM;
        goto error;
    }

    memcpy(lrpc_info->session_key.data,
           lrpc->info.session_key.data,
           lrpc->info.session_key.length);

    lrpc_info->session_key.length = lrpc->info.session_key.length;

    *info = (rpc_transport_info_handle_t) lrpc_info;

error:
    if (serr)
    {
        *info = NULL;

	if (lrpc_info)
        {
            rpc_lrpc_transport_info_free((rpc_transport_info_handle_t) lrpc_info);
        }
    }

    return serr;
}

void
rpc_lrpc_transport_info_free(
    rpc_transport_info_handle_t info
    )
{
    rpc_bsd_transport_info_p_t lrpc_info = (rpc_bsd_transport_info_p_t) info;

    if (lrpc_info->session_key.data)
    {
        free(lrpc_info->session_key.data);
    }

    free(lrpc_info);
}

void
rpc_lrpc_transport_info_inq_peer_eid(
    rpc_transport_info_handle_t info,
    unsigned32                  *uid,
    unsigned32                  *gid
    )
{
    rpc_bsd_transport_info_p_t lrpc_info = (rpc_bsd_transport_info_p_t) info;

    if (uid)
    {
        *uid = lrpc_info->peer_uid;
    }

    if (gid)
    {
        *gid = lrpc_info->peer_gid;
    }
}

void
rpc_lrpc_transport_info_inq_session_key(
    rpc_transport_info_handle_t info,
    unsigned char** sess_key,
    unsigned16* sess_key_len
    )
{
    rpc_bsd_transport_info_p_t lrpc_info = (rpc_bsd_transport_info_p_t) info;

    if (sess_key)
    {
        *sess_key = lrpc_info->session_key.data;
    }

    if (sess_key_len)
    {
        *sess_key_len = lrpc_info->session_key.length;
    }
}

INTERNAL
boolean
rpc__bsd_socket_transport_info_equal(
    rpc_transport_info_handle_t info1,
    rpc_transport_info_handle_t info2
    )
{
    rpc_bsd_transport_info_p_t bsd_info1 = (rpc_bsd_transport_info_p_t) info1;
    rpc_bsd_transport_info_p_t bsd_info2 = (rpc_bsd_transport_info_p_t) info2;

    return
        (bsd_info2 == NULL) ||
        (bsd_info2 != NULL &&
         bsd_info1->session_key.length == bsd_info2->session_key.length &&
         !memcmp(bsd_info1->session_key.data, bsd_info2->session_key.data, bsd_info1->session_key.length) &&
         bsd_info1->peer_uid == bsd_info2->peer_uid &&
         bsd_info1->peer_gid == bsd_info2->peer_gid);
}

INTERNAL
rpc_socket_error_t
rpc__bsd_socket_transport_inq_access_token(
    rpc_transport_info_handle_t info,
    rpc_access_token_p_t* token
    )
{
    /* TBD: Adam:  Requires Likewise lwbase to compile */
    /* Skip for now */
#if defined(_WIN32)
    return -1;
#else
    rpc_bsd_transport_info_p_t lrpc_info = (rpc_bsd_transport_info_p_t) info;
    NTSTATUS status = STATUS_SUCCESS;
    PLW_MAP_SECURITY_CONTEXT context = NULL;

    status = LwMapSecurityCreateContext(&context);
    if (status) goto error;

    status = LwMapSecurityCreateAccessTokenFromUidGid(
        context,
        token,
        lrpc_info->peer_uid,
        lrpc_info->peer_gid);
    if (status) goto error;

error:

    LwMapSecurityFreeContext(&context);

    return LwNtStatusToErrno(status);
#endif
}


PRIVATE rpc_socket_vtbl_t rpc_g_bsd_socket_vtbl =
{
    rpc__bsd_socket_construct,
    rpc__bsd_socket_destruct,
    rpc__bsd_socket_bind,
    rpc__bsd_socket_connect,
    rpc__bsd_socket_accept,
    rpc__bsd_socket_listen,
    rpc__bsd_socket_sendmsg,
    rpc__bsd_socket_recvfrom,
    rpc__bsd_socket_recvmsg,
    rpc__bsd_socket_inq_endpoint,
    rpc__bsd_socket_set_broadcast,
    rpc__bsd_socket_set_bufs,
    rpc__bsd_socket_set_nbio,
    rpc__bsd_socket_set_close_on_exec,
    rpc__bsd_socket_getpeername,
    rpc__bsd_socket_get_if_id,
    rpc__bsd_socket_set_keepalive,
    rpc__bsd_socket_nowriteblock_wait,
    rpc__bsd_socket_set_rcvtimeo,
    rpc__bsd_socket_getpeereid,
    rpc__bsd_socket_get_select_desc,
    rpc__bsd_socket_enum_ifaces,
    rpc__bsd_socket_inq_transport_info,
    rpc_lrpc_transport_info_free,
    rpc__bsd_socket_transport_info_equal,
    rpc__bsd_socket_transport_inq_access_token
};
