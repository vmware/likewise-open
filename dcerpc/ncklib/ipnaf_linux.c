/*
 *
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
**  Copyright (c) 1989 by
**      Hewlett-Packard Company, Palo Alto, Ca. &
**      Digital Equipment Corporation, Maynard, Mass.
**
**
**  NAME
**
**      ipnaf_linux
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  This module contains routines specific to the Internet Protocol,
**  the Internet Network Address Family extension service, and the
**  Linux.
**
**  - taken from ipnaf_bsd.c
**
**
**
*/

#include <commonp.h>
#include <com.h>
#include <comnaf.h>
#include <comsoc.h>
#include <ipnaf.h>

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

/***********************************************************************
 *
 *  Internal prototypes and typedefs.
 */

INTERNAL boolean get_addr(
        rpc_socket_t          /*sock*/,
        rpc_addr_p_t          /*ip_addr*/,
        rpc_addr_p_t          /*netmask_addr*/,
        rpc_addr_p_t          /*broadcast_addr*/
    
    );

INTERNAL boolean get_broadcast_addr(
        rpc_socket_t          /*sock*/,
        rpc_addr_p_t          /*ip_addr*/,
        rpc_addr_p_t          /*netmask_addr*/,
        rpc_addr_p_t          /*broadcast_addr*/
    
    );

#ifndef NO_SPRINTF
#  define RPC__IP_NETWORK_SPRINTF   sprintf
#else
#  define RPC__IP_NETWORK_SPRINTF   rpc__ip_network_sprintf
#endif

typedef struct
{
    unsigned32  num_elt;
    struct
    {
        struct sockaddr_in6  addr;
        struct sockaddr_in6  netmask;
    } elt[1];
} rpc_ip_s_addr_vector_t, *rpc_ip_s_addr_vector_p_t;

INTERNAL rpc_ip_s_addr_vector_p_t local_ip_addr_vec = NULL;

/*
**++
**
**  ROUTINE NAME:       get_addr
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  This function is called from "rpc__ip_desc_inq_addr" via
**  "enumerate_interfaces".  See comments in "enumerate_interfaces" for
**  details.
**
**
**  INPUTS:             none
**
**      desc            Socket being used for ioctl's.
**
**      ifr             Structure describing the interface.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      ip_addr
**
**      netmask_addr    netmask address
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      result          true => we generated up an address for this interface
**                      false => we didn't.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL boolean get_addr
(
    rpc_socket_t         sock ATTRIBUTE_UNUSED,
    rpc_addr_p_t         ip_addr ATTRIBUTE_UNUSED,
    rpc_addr_p_t         netmask_addr ATTRIBUTE_UNUSED,
    rpc_addr_p_t         broadcast_addr ATTRIBUTE_UNUSED
)
{
    return true;
}

INTERNAL boolean get_addr_noloop
(
    rpc_socket_t         sock ATTRIBUTE_UNUSED,
    rpc_addr_p_t         _ip_addr,
    rpc_addr_p_t         netmask_addr ATTRIBUTE_UNUSED,
    rpc_addr_p_t         broadcast_addr ATTRIBUTE_UNUSED
)
{
    rpc_ip_addr_p_t ip_addr = (rpc_ip_addr_p_t) _ip_addr;

    if (((unsigned char*) &ip_addr->sa.sin_addr)[0] == 127)
    {
        return false;
    }

    return true;
}


/*
**++
**
**  ROUTINE NAME:       rpc__ip_desc_inq_addr
**
**  SCOPE:              PRIVATE - declared in ipnaf.h
**
**  DESCRIPTION:
**
**  Receive a socket descriptor which is queried to obtain family, endpoint
**  and network address.  If this information appears valid for an IP
**  address,  space is allocated for an RPC address which is initialized
**  with the information obtained from the socket.  The address indicating
**  the created RPC address is returned in rpc_addr.
**
**  INPUTS:
**
**      protseq_id      Protocol Sequence ID representing a particular
**                      Network Address Family, its Transport Protocol,
**                      and type.
**
**      desc            Descriptor, indicating a socket that has been
**                      created on the local operating platform.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      rpc_addr_vec
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok               The call was successful.
**
**          rpc_s_no_memory         Call to malloc failed to allocate memory.
**
**          rpc_s_cant_inq_socket  Attempt to get info about socket failed.
**
**          Any of the RPC Protocol Service status codes.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__ip_desc_inq_addr
(
    rpc_protseq_id_t        protseq_id,
    rpc_socket_t            sock,
    rpc_addr_vector_p_t     *rpc_addr_vec,
    unsigned32              *status
)
{
    rpc_ip_addr_p_t         ip_addr;
    rpc_ip_addr_t           loc_ip_addr;
    unsigned16              i;
    socklen_t               _slen;
    int err = 0;

    CODING_ERROR (status);

    /*
     * Do a "getsockname" into a local IP RPC address.  If the network
     * address part of the result is non-zero, then the socket must be
     * bound to a particular IP address and we can just return a RPC
     * address vector with that one address (and endpoint) in it.
     * Otherwise, we have to enumerate over all the local network
     * interfaces the local host has and construct an RPC address for
     * each one of them.
     */
    _slen = (socklen_t) sizeof (rpc_ip_addr_t);

    loc_ip_addr.len = sizeof(loc_ip_addr.sa);
    err = rpc__socket_inq_endpoint (sock, (rpc_addr_p_t) &loc_ip_addr);
    if (err)
    {
        *status = -1;   /* !!! */
        return;
    }

    if (loc_ip_addr.sa.sin_addr.s_addr == 0)
    {
        err = rpc__socket_enum_ifaces(sock, get_addr_noloop, rpc_addr_vec, NULL, NULL);

        if (err != RPC_C_SOCKET_OK)
        {
            *status = -1;
            return;
        }
        for (i = 0; i < (*rpc_addr_vec)->len; i++)
        {
            /* Ignore addresses with AF_INET version mismatch (e.g. IPv4 != IPV6) */
            if (protseq_id != (*rpc_addr_vec)->addrs[i]->rpc_protseq_id)
            {
                continue;
            }
            ((rpc_ip_addr_p_t) (*rpc_addr_vec)->addrs[i])->sa.sin_port = loc_ip_addr.sa.sin_port;
        }

        *status = rpc_s_ok;
        return;
    }
    else
    {
        RPC_MEM_ALLOC (
            ip_addr,
            rpc_ip_addr_p_t,
            sizeof (rpc_ip_addr_t),
            RPC_C_MEM_RPC_ADDR,
            RPC_C_MEM_WAITOK);

        if (ip_addr == NULL)
        {
            *status = rpc_s_no_memory;
            return;
        }

        RPC_MEM_ALLOC (
            *rpc_addr_vec,
            rpc_addr_vector_p_t,
            sizeof **rpc_addr_vec,
            RPC_C_MEM_RPC_ADDR_VEC,
            RPC_C_MEM_WAITOK);

        if (*rpc_addr_vec == NULL)
        {
            RPC_MEM_FREE (ip_addr, RPC_C_MEM_RPC_ADDR);
            *status = rpc_s_no_memory;
            return;
        }

        ip_addr->rpc_protseq_id = protseq_id;
        ip_addr->len            = sizeof (struct sockaddr_in);
        ip_addr->sa             = loc_ip_addr.sa;

        (*rpc_addr_vec)->len = 1;
        (*rpc_addr_vec)->addrs[0] = (rpc_addr_p_t) ip_addr;

        *status = rpc_s_ok;
        return;
    }
}

/*
**++
**
**  ROUTINE NAME:       rpc__ip_desc_inq_addr6
**
**  SCOPE:              PRIVATE - declared in ipnaf.h
**
**  DESCRIPTION:
**
**  Receive a socket descriptor which is queried to obtain family, endpoint
**  and network address.  If this information appears valid for an IP
**  address,  space is allocated for an RPC address which is initialized
**  with the information obtained from the socket.  The address indicating
**  the created RPC address is returned in rpc_addr.
**
**  INPUTS:
**
**      protseq_id      Protocol Sequence ID representing a particular
**                      Network Address Family, its Transport Protocol,
**                      and type.
**
**      desc            Descriptor, indicating a socket that has been
**                      created on the local operating platform.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      rpc_addr_vec
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok               The call was successful.
**
**          rpc_s_no_memory         Call to malloc failed to allocate memory.
**
**          rpc_s_cant_inq_socket  Attempt to get info about socket failed.
**
**          Any of the RPC Protocol Service status codes.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__ip_desc_inq_addr6
(
    rpc_protseq_id_t        protseq_id,
    rpc_socket_t            sock,
    rpc_addr_vector_p_t     *rpc_addr_vec,
    unsigned32              *status
)
{
    rpc_ip6_addr_p_t        ip6_addr;
    rpc_ip6_addr_t          loc_ip6_addr;
    rpc_ip6_addr_t          zero_ip6_addr;
    unsigned16              i;
    int err = 0;

    CODING_ERROR (status);

    /*
     * Do a "getsockname" into a local IP RPC address.  If the network
     * address part of the result is non-zero, then the socket must be
     * bound to a particular IP address and we can just return a RPC
     * address vector with that one address (and endpoint) in it.
     * Otherwise, we have to enumerate over all the local network
     * interfaces the local host has and construct an RPC address for
     * each one of them.
     */
    memset(&zero_ip6_addr, 0, sizeof(zero_ip6_addr));
    loc_ip6_addr.len = sizeof(loc_ip6_addr.sa);
    err = rpc__socket_inq_endpoint (sock, (rpc_addr_p_t) &loc_ip6_addr);
    if (err)
    {
        *status = -1;   /* !!! */
        return;
    }

    if (memcmp(&loc_ip6_addr.sa.sin6_addr.s6_addr,
               &zero_ip6_addr.sa.sin6_addr.s6_addr,
               sizeof(zero_ip6_addr.sa.sin6_addr.s6_addr)) == 0)
    {
        err = rpc__socket_enum_ifaces(sock, get_addr_noloop, rpc_addr_vec, NULL, NULL);

        if (err != RPC_C_SOCKET_OK)
        {
            *status = -1;
            return;
        }
        for (i = 0; i < (*rpc_addr_vec)->len; i++)
        {
            ((rpc_ip6_addr_p_t) (*rpc_addr_vec)->addrs[i])->sa.sin6_port = loc_ip6_addr.sa.sin6_port;
        }

        *status = rpc_s_ok;
        return;
    }
    else
    {
        RPC_MEM_ALLOC (
            ip6_addr,
            rpc_ip6_addr_p_t,
            sizeof (rpc_ip6_addr_t),
            RPC_C_MEM_RPC_ADDR,
            RPC_C_MEM_WAITOK);

        if (ip6_addr == NULL)
        {
            *status = rpc_s_no_memory;
            return;
        }

        RPC_MEM_ALLOC (
            *rpc_addr_vec,
            rpc_addr_vector_p_t,
            sizeof **rpc_addr_vec,
            RPC_C_MEM_RPC_ADDR_VEC,
            RPC_C_MEM_WAITOK);

        if (*rpc_addr_vec == NULL)
        {
            RPC_MEM_FREE (ip6_addr, RPC_C_MEM_RPC_ADDR);
            *status = rpc_s_no_memory;
            return;
        }

        ip6_addr->rpc_protseq_id = protseq_id;
        ip6_addr->len            = sizeof (struct sockaddr_in);
        ip6_addr->sa             = loc_ip6_addr.sa;

        (*rpc_addr_vec)->len = 1;
        (*rpc_addr_vec)->addrs[0] = (rpc_addr_p_t) ip6_addr;

        *status = rpc_s_ok;
        return;
    }
}

/*
**++
**
**  ROUTINE NAME:       get_broadcast_addr
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  This function is called from "rpc__ip_get_broadcast" via
**  "enumerate_interfaces".  See comments in "enumerate_interfaces" for
**  details.
**
**
**  INPUTS:             none
**
**      desc            Socket being used for ioctl's.
**
**      ifr             Structure describing the interface.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      ip_addr
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**      result          true => we generated up an address for this interface
**                      false => we didn't.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL boolean get_broadcast_addr
(
    rpc_socket_t         sock ATTRIBUTE_UNUSED,
    rpc_addr_p_t         _ip_addr ATTRIBUTE_UNUSED,
    rpc_addr_p_t         netmask_addr ATTRIBUTE_UNUSED,
    rpc_addr_p_t         broadcast_addr
)
{
    rpc_ip_addr_p_t ip_addr = (rpc_ip_addr_p_t) _ip_addr;

    if (broadcast_addr == NULL)
    {
        return false;
    }

    /* IPv6 doesn't have the concept of broadcast mask */
    if (ip_addr->sa.sin_family == AF_INET6)
    {
        return false;
    }
#if !defined(BROADCAST_NEEDS_LOOPBACK) && !defined(USE_LOOPBACK)
{

    if (((unsigned char*) &ip_addr->sa.sin_addr)[0] == 127)
    {
        return false;
    }
}
#endif

    return true;
}

/*
**++
**
**  ROUTINE NAME:       rpc__ip_get_broadcast
**
**  SCOPE:              PRIVATE - EPV declared in ipnaf.h
**
**  DESCRIPTION:
**
**  Return a vector of RPC addresses that represent all the address
**  required so that sending on all of them results in broadcasting on
**  all the local network interfaces.
**
**
**  INPUTS:
**
**      naf_id          Network Address Family ID serves
**                      as index into EPV for IP routines.
**
**      rpc_protseq_id
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      rpc_addr_vec
**
**      status          A value indicating the status of the routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__ip_get_broadcast
(
    rpc_naf_id_t            naf_id ATTRIBUTE_UNUSED,
    rpc_protseq_id_t        protseq_id,
    rpc_addr_vector_p_t     *rpc_addr_vec,
    unsigned32              *status
)
{
    rpc_socket_t sock = RPC_SOCKET_INVALID;
    int err = RPC_C_SOCKET_OK;

    CODING_ERROR (status);

    err = rpc__socket_open(protseq_id, NULL, &sock);

    if (err)
    {
        *status = -1;
        goto done;
    }

    err = rpc__socket_enum_ifaces (sock, get_broadcast_addr, NULL, NULL, rpc_addr_vec);
    if (err)
    {
        *status = -1;
        goto done;
    }

done:

    if (sock != RPC_SOCKET_INVALID)
    {
        RPC_SOCKET_CLOSE(sock);
    }

    return;
}

INTERNAL void _rpc__ipx_init_local_addr_vec(
    rpc_socket_t            sock,
    unsigned32 *status)
{
    unsigned32              i;
    rpc_addr_vector_p_t     rpc_addr_vec = NULL;
    rpc_addr_vector_p_t     netmask_addr_vec = NULL;
    int err;
    int                     sin_addr_len = 0; /* IPv4/IPv6 address length */

    err = rpc__socket_enum_ifaces(sock, get_addr, &rpc_addr_vec, &netmask_addr_vec, NULL);
    if (err)
    {
        *status = -1;
        goto error;
    }

    /*
     * Do some sanity check.
     */

    if (rpc_addr_vec == NULL
        || netmask_addr_vec == NULL
        || rpc_addr_vec->len != netmask_addr_vec->len
        || rpc_addr_vec->len == 0)
    {
        RPC_DBG_GPRINTF(("(rpc__ip_init_local_addr_vec) no local address\n"));
        *status = rpc_s_no_addrs;
        goto error;
    }

    if (local_ip_addr_vec != NULL)
    {
        RPC_MEM_FREE(local_ip_addr_vec, RPC_C_MEM_UTIL);
        local_ip_addr_vec = NULL;
    }

    RPC_MEM_ALLOC (
        local_ip_addr_vec,
        rpc_ip_s_addr_vector_p_t,
        (sizeof *local_ip_addr_vec)
            + ((rpc_addr_vec->len - 1) * (sizeof (local_ip_addr_vec->elt[0]))),
        RPC_C_MEM_UTIL,
        RPC_C_MEM_WAITOK);
    if (local_ip_addr_vec == NULL)
    {
        *status = rpc_s_no_memory;
        goto error;
    }

    local_ip_addr_vec->num_elt = rpc_addr_vec->len;

    for (i = 0; i < rpc_addr_vec->len; i++)
    {
        if (((rpc_ip_addr_p_t) rpc_addr_vec->addrs[i])->sa.sin_family == AF_INET6)
        {
            sin_addr_len = sizeof(struct sockaddr_in6);
        }
        else
        {
            sin_addr_len = sizeof(struct sockaddr_in);
        }
        memcpy(&local_ip_addr_vec->elt[i].addr,
               &((rpc_ip_addr_p_t) rpc_addr_vec->addrs[i])->sa,
               sin_addr_len);
        memcpy(&local_ip_addr_vec->elt[i].netmask,
               &((rpc_ip_addr_p_t) netmask_addr_vec->addrs[i])->sa,
               sin_addr_len);


#ifdef DEBUG_FIXME /* TBD:Adam-BUGBUG build break for "DEBUG" in below section */
        if (RPC_DBG2(rpc_e_dbg_general, 10))
        {
            char         buff[128], mbuff[128];

            getnameinfo(&local_ip_addr_vec->elt[i].addr, sin_addr_len,
                        buff, sizeof(buff),
                        NULL, 0, NI_NUMERICHOST);
            getnameinfo(&local_ip_addr_vec->elt[i].netmask, sin_addr_len,
                        mbuff, sizeof(mbuff),
                        NULL, 0, NI_NUMERICHOST);

            RPC_DBG_PRINTF(rpc_e_dbg_general, 10,
            ("(rpc__ip_init_local_addr_vec) local network [%s] netmask [%s]\n",
                            buff, mbuff));
        }
#endif
    }

error:
    if (rpc_addr_vec != NULL)
    {
        for (i = 0; i < rpc_addr_vec->len; i++)
        {
            RPC_MEM_FREE (rpc_addr_vec->addrs[i], RPC_C_MEM_RPC_ADDR);
        }
        RPC_MEM_FREE (rpc_addr_vec, RPC_C_MEM_RPC_ADDR_VEC);
    }
    if (netmask_addr_vec != NULL)
    {
        for (i = 0; i < netmask_addr_vec->len; i++)
        {
            RPC_MEM_FREE (netmask_addr_vec->addrs[i], RPC_C_MEM_RPC_ADDR);
        }
        RPC_MEM_FREE (netmask_addr_vec, RPC_C_MEM_RPC_ADDR_VEC);
    }
}

/*
**++
**
**  ROUTINE NAME:       rpc__ip_init_local_addr_vec
**
**  SCOPE:              PRIVATE - declared in ipnaf.h
**
**  DESCRIPTION:
**
**  Initialize the local address vectors.
**
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:
**
**      Update local_ip_addr_vec
**
**--
**/


PRIVATE void rpc__ip_init_local_addr_vec
(
    unsigned32 *status
)
{
    rpc_socket_t            sock = RPC_SOCKET_INVALID;
    int err;

    CODING_ERROR (status);

    err = rpc__socket_open(
              RPC_C_PROTSEQ_ID_NCACN_IP_TCP,
              NULL,
              &sock);
    if (err)
    {
        *status = rpc_s_cant_create_socket;
        goto error;
    }

    _rpc__ipx_init_local_addr_vec(sock, status);

error:
    if (sock != RPC_SOCKET_INVALID)
    {
        RPC_SOCKET_CLOSE(sock);
    }

    return;
}

PRIVATE void rpc__ip6_init_local_addr_vec
(
    unsigned32 *status
)
{
    rpc_socket_t            sock = RPC_SOCKET_INVALID;
    int err;


    CODING_ERROR (status);

    err = rpc__socket_open(
              RPC_C_PROTSEQ_ID_NCACN_IP6_TCP,
              NULL,
              &sock);
    if (err)
    {
        *status = rpc_s_cant_create_socket;
        goto error;
    }

    _rpc__ipx_init_local_addr_vec(sock, status);

error:
    if (sock != RPC_SOCKET_INVALID)
    {
        RPC_SOCKET_CLOSE(sock);
    }

    return;
}

/*
**++
**
**  ROUTINE NAME:       rpc__ip_is_local_network
**
**  SCOPE:              PRIVATE - declared in ipnaf.h
**
**  DESCRIPTION:
**
**  Return a boolean value to indicate if the given RPC address is on
**  the same IP subnet.
**
**
**  INPUTS:
**
**      rpc_addr        The address that forms the path of interest
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      result          true => the address is on the same subnet.
**                      false => not.
**
**  SIDE EFFECTS:       none
**
**--
**/
PRIVATE boolean32 rpc__ip_is_local_network
(
    rpc_addr_p_t rpc_addr,
    unsigned32   *status
)
{
    rpc_ip_addr_p_t         ip_addr = (rpc_ip_addr_p_t) rpc_addr;
    unsigned char           *addr1;
    unsigned char           *addr2;
    unsigned char           *netmask;
    unsigned char           and_array1[sizeof(struct in6_addr)];
    unsigned char           and_array2[sizeof(struct in6_addr)];
    unsigned32              i;
    unsigned32              ai;
    unsigned32              sin_addr_len = 0;

    CODING_ERROR (status);

    if (rpc_addr == NULL)
    {
        *status = rpc_s_invalid_arg;
        return false;
    }

    *status = rpc_s_ok;

    if (local_ip_addr_vec == NULL)
    {
        /*
         * We should call rpc__ip_init_local_addr_vec() here. But, it
         * requires the mutex lock for local_ip_addr_vec. For now just return
         * false.
         */
        return false;
    }

    /*
     * Compare addresses. Since they can be either IPv4/IPv6, must byte-by-byte
     * mask the addresses against the netmask. Afterwardds, compare byte-by-byte
     * the resultant masked addresses arrays for equality.
     */
    for (i = 0; i < local_ip_addr_vec->num_elt; i++)
    {
        if (ip_addr->sa.sin_family != AF_INET &&
            ip_addr->sa.sin_family != AF_INET6)
        {
            continue;
        }

        if (ip_addr->sa.sin_family !=
            local_ip_addr_vec->elt[i].addr.sin6_family)
        {
            continue;
        }

        /* Cast address/netmask to neutral type for byte-wise comparison */
        if (ip_addr->sa.sin_family == AF_INET6)
        {
            sin_addr_len = sizeof(struct in6_addr);
            addr1 = (unsigned char *) &((struct sockaddr_in6 *) &ip_addr->sa)->sin6_addr;
            addr2 = (unsigned char *) &local_ip_addr_vec->elt[i].addr;
            netmask = (unsigned char *) &local_ip_addr_vec->elt[i].netmask;
        }
        else
        {
            sin_addr_len = sizeof(struct in_addr);
            addr1 = (unsigned char *) &((struct sockaddr_in *) &ip_addr->sa)->sin_addr;
            addr2 = (unsigned char *) &local_ip_addr_vec->elt[i].addr;
            netmask = (unsigned char *) &local_ip_addr_vec->elt[i].netmask;
        }
        for (ai=0; ai < sin_addr_len; ai++)
        {
           and_array1[ai] = addr1[ai] & netmask[ai];
           and_array2[ai] = addr2[ai] & netmask[ai];
        }

        if (memcmp(and_array1, and_array2, sin_addr_len) == 0)
        {
            return true;
        }
    }

    return false;
}

/*
**++
**
**  ROUTINE NAME:       rpc__ip_is_local_addr
**
**  SCOPE:              PRIVATE - declared in ipnaf.h
**
**  DESCRIPTION:
**
**  Return a boolean value to indicate if the given RPC address is the
**  the local IP address.
**
**
**  INPUTS:
**
**      rpc_addr        The address that forms the path of interest
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:
**
**      result          true => the address is local.
**                      false => not.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE boolean32 rpc__ip_is_local_addr
(
    rpc_addr_p_t rpc_addr,
    unsigned32   *status
)
{
    rpc_ip_addr_p_t         ip_addr;
    unsigned32              i;
    unsigned char *         addr1;
    unsigned char *         addr2;
    int                     sin_addr_len;

    CODING_ERROR (status);

    if (rpc_addr == NULL)
    {
        *status = rpc_s_invalid_arg;
        return false;
    }
    ip_addr = (rpc_ip_addr_p_t) rpc_addr;

    *status = rpc_s_ok;

    if (local_ip_addr_vec == NULL)
    {
        /*
         * We should call rpc__ip_init_local_addr_vec() here. But, it
         * requires the mutex lock for local_ip_addr_vec. For now just return
         * false.
         */
        return false;
    }

    /*
     * Compare addresses.
     */
    for (i = 0; i < local_ip_addr_vec->num_elt; i++)
    {
        if (ip_addr->sa.sin_family != AF_INET &&
            ip_addr->sa.sin_family != AF_INET6)
        {
            continue;
        }
        if (ip_addr->sa.sin_family != local_ip_addr_vec->elt[i].addr.sin6_family)
        {
            continue;
        }

        if (ip_addr->sa.sin_family == AF_INET6)
        {
            sin_addr_len = sizeof(struct in6_addr);
            addr1 = (unsigned char *) &((struct sockaddr_in6 *) &ip_addr->sa)->sin6_addr;
            addr2 = (unsigned char *) &local_ip_addr_vec->elt[i].addr;
        }
        else
        {
            sin_addr_len = sizeof(struct in_addr);
            addr1 = (unsigned char *) &((struct sockaddr_in *) &ip_addr->sa)->sin_addr;
            addr2 = (unsigned char *) &local_ip_addr_vec->elt[i].addr;
        }

        if (memcmp(addr1, addr2, sin_addr_len) == 0)
        {
            return true;
        }
    }

    return false;
}
