/* ex: set shiftwidth=4 expandtab: */
/*
 * Copyright (c) 2007, Novell, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Novell, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
 */
/*
**
**  NAME
**
**      npnaf.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  This module contains routines specific to the Internet Protocol
**  and the Internet Network Address Family extension service.
**  An initialization routine is provided to be called at RPC
**  initialization time provided, the Internet Protocol is supported
**  on the local host platform.  The remaining routines are entered
**  through an Entry Point Vector specific to the Internet Protocol.
**
**
*/

#include <commonp.h>
#include <com.h>
#include <comnaf.h>
#include <npnaf.h>
#include <comsoc_smb.h>
#include <comsoc_bsd.h>
#include <ctype.h>
#include <sys/param.h>
#include <syslog.h>
#include <stddef.h>


/***********************************************************************
 *
 *  Macros for sprint/scanf substitutes.
 */

#ifndef NO_SSCANF
#  define RPC__NP_ENDPOINT_SSCANF   sscanf
#else
#  define RPC__NP_ENDPOINT_SSCANF   rpc__np_endpoint_sscanf
#endif

#ifndef NO_SPRINTF
#  define RPC__NP_ENDPOINT_SPRINTF  sprintf
#  define RPC__NP_NETWORK_SPRINTF   sprintf
#else
#  define RPC__NP_ENDPOINT_SPRINTF  rpc__np_endpoint_sprintf
#  define RPC__NP_NETWORK_SPRINTF   rpc__np_network_sprintf
#endif


/***********************************************************************
 *
 *  Routine Prototypes for the Internet Extension service routines.
 */

INTERNAL void addr_alloc _DCE_PROTOTYPE_ ((
        rpc_protseq_id_t             /*rpc_protseq_id*/,
        rpc_naf_id_t                 /*naf_id*/,
        unsigned_char_p_t            /*endpoint*/,
        unsigned_char_p_t            /*netaddr*/,
        unsigned_char_p_t            /*network_options*/,
        rpc_addr_p_t                * /*rpc_addr*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void addr_copy _DCE_PROTOTYPE_ ((
        rpc_addr_p_t                 /*srpc_addr*/,
        rpc_addr_p_t                * /*drpc_addr*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void addr_free _DCE_PROTOTYPE_ ((
        rpc_addr_p_t                * /*rpc_addr*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void addr_set_endpoint _DCE_PROTOTYPE_ ((
        unsigned_char_p_t            /*endpoint*/,
        rpc_addr_p_t                * /*rpc_addr*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void addr_inq_endpoint _DCE_PROTOTYPE_ ((
        rpc_addr_p_t                 /*rpc_addr*/,
        unsigned_char_t             ** /*endpoint*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void addr_set_netaddr _DCE_PROTOTYPE_ ((
        unsigned_char_p_t            /*netaddr*/,
        rpc_addr_p_t                * /*rpc_addr*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void addr_inq_netaddr _DCE_PROTOTYPE_ ((
        rpc_addr_p_t                 /*rpc_addr*/,
        unsigned_char_t             ** /*netaddr*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void addr_set_options _DCE_PROTOTYPE_ ((
        unsigned_char_p_t            /*network_options*/,
        rpc_addr_p_t                * /*rpc_addr*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void addr_inq_options _DCE_PROTOTYPE_ ((
        rpc_addr_p_t                 /*rpc_addr*/,
        unsigned_char_t             ** /*network_options*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void desc_inq_network _DCE_PROTOTYPE_ ((
        rpc_socket_t                 /*desc*/,
        rpc_network_if_id_t         * /*socket_type*/,
        rpc_network_protocol_id_t   * /*protocol_id*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void inq_max_tsdu _DCE_PROTOTYPE_ ((
        rpc_naf_id_t                 /*naf_id*/,
        rpc_network_if_id_t          /*iftype*/,
        rpc_network_protocol_id_t    /*protocol*/,
        unsigned32                  * /*max_tsdu*/,
        unsigned32                  * /*status*/
    ));

INTERNAL boolean addr_compare _DCE_PROTOTYPE_ ((
        rpc_addr_p_t                 /*addr1*/,
        rpc_addr_p_t                 /*addr2*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void inq_max_pth_unfrag_tpdu _DCE_PROTOTYPE_ ((
        rpc_addr_p_t                 /*rpc_addr*/,
        rpc_network_if_id_t          /*iftype*/,
        rpc_network_protocol_id_t    /*protocol*/,
        unsigned32                  * /*max_tpdu*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void inq_max_loc_unfrag_tpdu _DCE_PROTOTYPE_ ((
        rpc_naf_id_t                 /*naf_id*/,
        rpc_network_if_id_t          /*iftype*/,
        rpc_network_protocol_id_t    /*protocol*/,
        unsigned32                  * /*max_tpdu*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void set_pkt_nodelay _DCE_PROTOTYPE_ ((
        rpc_socket_t                 /*desc*/,
        unsigned32                  * /*status*/
    ));

INTERNAL boolean is_connect_closed _DCE_PROTOTYPE_ ((
        rpc_socket_t                 /*desc*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void tower_flrs_from_addr _DCE_PROTOTYPE_ ((
        rpc_addr_p_t                 /*rpc_addr*/,
        twr_p_t                     * /*lower_flrs*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void tower_flrs_to_addr _DCE_PROTOTYPE_ ((
        byte_p_t                     /*tower_octet_string*/,
        rpc_addr_p_t                * /*rpc_addr*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void desc_inq_peer_addr _DCE_PROTOTYPE_ ((
        rpc_protseq_id_t             /*protseq_id*/,
        rpc_socket_t                 /*desc*/,
        rpc_addr_p_t                * /*rpc_addr*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void set_port_restriction _DCE_PROTOTYPE_ ((
        rpc_protseq_id_t             /*protseq_id*/,
        unsigned32                   /*n_elements*/,
        unsigned_char_p_t           * /*first_port_name_list*/,
        unsigned_char_p_t           * /*last_port_name_list*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void get_next_restricted_port _DCE_PROTOTYPE_ ((
        rpc_protseq_id_t             /*protseq_id*/,
        unsigned_char_p_t           * /*port_name*/,
        unsigned32                  * /*status*/
    ));

INTERNAL void inq_max_frag_size _DCE_PROTOTYPE_ ((
        rpc_addr_p_t                 /*rpc_addr*/,
        unsigned32                  * /*max_frag_size*/,
        unsigned32                  * /*status*/
    ));


/*
**++
**
**  ROUTINE NAME:       rpc__np_init
**
**  SCOPE:              PRIVATE - EPV declared in npnaf.h
**
**  DESCRIPTION:
**
**  Named Pipe Family Initialization routine, rpc__np_init, is
**  calld only once, by the Communications Service initialization
**  procedure,  at the time RPC is initialized.  If the Communications
**  Service initialization determines that the Internet protocol
**  family is supported on the local host platform it will call this
**  initialization routine.  It is responsible for all Internet
**  specific initialization for the current RPC.  It will place in
**  Network Address Family Table, a pointer to the Internet family Entry
**  Point Vector.  Afterward all calls to the IP extension service
**  routines will be vectored through this EPV.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:
**
**      naf_epv         The address of a pointer in the Network Address Family
**                      Table whre the pointer to the Entry Point Vectorto
**                      the NP service routines is inserted by this routine.
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
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

#if !NAF_NP_STATIC
#include <comp.h>
PRIVATE void rpc__np_naf_init_func(void)
{
	static rpc_naf_id_elt_t naf[1] = {
		{
			rpc__np_init,
			RPC_C_NAF_ID_UXD,
			RPC_C_NETWORK_IF_ID_STREAM,
			NULL
		}
	};
        /*
         * Unfortunately we can only have one NAF per module, so
         * we need to handle ncacn_np and ncalrpc within the same
         * code. The only difference server-side is the path naming
         * semantics, the protocol towers, and the passing of security
         * descriptors.
         */
	static rpc_tower_prot_ids_t prot_ids[2] = {
        { RPC_C_PROTSEQ_ID_NCACN_NP,   3,
          { {0x0B,   { 0, 0, 0, 0, 0, {0} }}, /* Connection-oriented */
            {0x0F,   { 0, 0, 0, 0, 0, {0} }}, /* SMB Named Pipes */
            {0x11,   { 0, 0, 0, 0, 0, {0} }}, /* NetBIOS host */
            {0x00,   { 0, 0, 0, 0, 0, {0} }} } },
        { RPC_C_PROTSEQ_ID_NCALRPC,   2,
          { {0x0B,   { 0, 0, 0, 0, 0, {0} }}, /* Connection-oriented */
            {0x20,   { 0, 0, 0, 0, 0, {0} }}, /* socket pathname */
            {0x00,   { 0, 0, 0, 0, 0, {0} }} } }
	};
	static rpc_protseq_id_elt_t seq_ids[2] = {
    {                                   /* Connection-RPC / NP / NB */
        0,
        0, /* Does not use endpoint mapper */
        RPC_C_PROTSEQ_ID_NCACN_NP,
        RPC_C_PROTOCOL_ID_NCACN,
        RPC_C_NAF_ID_UXD,
        RPC_C_NETWORK_PROTOCOL_ID_NP,
        RPC_C_NETWORK_IF_ID_STREAM,
        RPC_PROTSEQ_NCACN_NP,
        (rpc_port_restriction_list_p_t) NULL,
        &rpc_g_smb_socket_vtbl
    },
    {                                   /* Connection-RPC / UXD */
        0,
        0, /* Does not use endpoint mapper */
        RPC_C_PROTSEQ_ID_NCALRPC,
        RPC_C_PROTOCOL_ID_NCACN,
        RPC_C_NAF_ID_UXD,
        RPC_C_NETWORK_PROTOCOL_ID_UXD,
        RPC_C_NETWORK_IF_ID_STREAM,
        RPC_PROTSEQ_NCALRPC,
        (rpc_port_restriction_list_p_t) NULL,
        &rpc_g_bsd_socket_vtbl
    }

	};
	rpc__register_protseq(seq_ids, 2);
	rpc__register_tower_prot_id(prot_ids, 2);
	rpc__register_naf_id(naf, 1);
}
#endif

PRIVATE void  rpc__np_init
#ifdef _DCE_PROTO_
(
    rpc_naf_epv_p_t         *naf_epv,
    unsigned32              *status
)
#else
(naf_epv, status)
rpc_naf_epv_p_t         *naf_epv;
unsigned32              *status;
#endif
{
    /*
     * The Internal Entry Point Vectors for the Internet Protocol Family
     * Extension service routines.  At RPC startup time, the IP init routine,
     * rpc__np_init, is responsible for inserting  a pointer to this EPV into
     * the  Network Address Family Table.  Afterward,  all calls to the IP
     * Extension  Service are vectored through these  EPVs.
     */

    static rpc_naf_epv_t rpc_np_epv =
    {
        addr_alloc,
        addr_copy,
        addr_free,
        addr_set_endpoint,
        addr_inq_endpoint,
        addr_set_netaddr,
        addr_inq_netaddr,
        addr_set_options,
        addr_inq_options,
        rpc__np_desc_inq_addr,
        desc_inq_network,
        inq_max_tsdu,
        rpc__np_get_broadcast,
        addr_compare,
        inq_max_pth_unfrag_tpdu,
        inq_max_loc_unfrag_tpdu,
        set_pkt_nodelay,
        is_connect_closed,
        tower_flrs_from_addr,
        tower_flrs_to_addr,
        desc_inq_peer_addr,
        set_port_restriction,
        get_next_restricted_port,
        inq_max_frag_size
    };
    unsigned32 lstatus;

    rpc__np_init_local_addr_vec (&lstatus);

    /*
     * place the address of EPV into Network Address Family Table
     */
    *naf_epv = &rpc_np_epv;

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       addr_alloc
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  Create a copy of an RPC address. Allocate memory for a variable
**  length RPC address, for the NP service.  Insert the Internet
**  address and endpoint along with the overall length of the allocated
**  memory, together with any additional parameters required by the IP
**  service.
**
**  INPUTS:
**
**      rpc_protseq_id  Protocol Sequence ID representing an IP Network
**                      Address Family, its Transport Protocol, and type.
**
**      naf_id          Network Address Family ID serves as index into
**                      EPV for IP routines.
**
**      endpoint        String containing endpoint to insert into newly
**                      allocated RPC address.
**
**      netaddr         String containing Internet format network
**                      address to be inserted in RPC addr.
**
**      network_options String containing options to be placed in
**                      RPC address.  - Not used by NP service.
**
**  INPUTS/OUTPUTS:
**
**      rpc_addr        The address of a pointer to an RPC address -
**                      returned with the address of the memory
**                      allocated by this routine.
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok           The call was successful.
**
**          rpc_s_no_memory     Call to malloc failed to allocate memory
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
**
**--
**/

INTERNAL void addr_alloc
#ifdef _DCE_PROTO_
(
    rpc_protseq_id_t        rpc_protseq_id,
    rpc_naf_id_t            naf_id,
    unsigned_char_p_t       endpoint,
    unsigned_char_p_t       netaddr,
    unsigned_char_p_t       network_options ATTRIBUTE_UNUSED,
    rpc_addr_p_t            *rpc_addr,
    unsigned32              *status
)
#else
(rpc_protseq_id, naf_id, endpoint, netaddr, network_options, rpc_addr, status)
rpc_protseq_id_t        rpc_protseq_id;
rpc_naf_id_t            naf_id;
unsigned_char_p_t       endpoint;
unsigned_char_p_t       netaddr;
unsigned_char_p_t       network_options;
rpc_addr_p_t            *rpc_addr;
unsigned32              *status;
#endif
{
    CODING_ERROR (status);

    /*
     * allocate memory for the new RPC address
     */

    RPC_MEM_ALLOC (
        *rpc_addr,
        rpc_addr_p_t,
        sizeof (rpc_np_addr_t),
        RPC_C_MEM_RPC_ADDR,
        RPC_C_MEM_WAITOK);

    if (*rpc_addr == NULL)
    {
        *status = rpc_s_no_memory;
        return;
    }

    /*
     * zero allocated memory
     */
    memset( *rpc_addr, 0, sizeof (rpc_np_addr_t));

    assert(RPC_C_PATH_NP_MAX <= sizeof(((rpc_np_addr_p_t) *rpc_addr)->sa.sun_path));

    /*
     * insert id, length, family into rpc address
     */
    (*rpc_addr)->rpc_protseq_id = rpc_protseq_id;
    (*rpc_addr)->len = sizeof (struct sockaddr_un);
    (*rpc_addr)->sa.family = naf_id;

    /*
     * set the endpoint in the RPC addr
     */
    addr_set_endpoint (endpoint, rpc_addr, status);
    if (*status != rpc_s_ok) return;

    /*
     * set the network address in the RPC addr
     */
    addr_set_netaddr (netaddr, rpc_addr, status);
    if (*status != rpc_s_ok) return;

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       addr_copy
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  Obtain the length from the source RPC address.  Allocate memory for a
**  new, destination  RPC address. Do a byte copy from the surce address
**  to the destination address.
**
**  INPUTS:
**
**     src_rpc_addr     The address of a pointer to an RPC address to be
**                      copied.  It must be the correct format for Internet
**                      Protocol.
**
**  INPUTS/OUTPUTS:
**
**     dst_rpc_addr     The address of a pointer to an RPC address -returned
**                      with the address of the memory allocated by
**                      this routine.
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**      rpc_s_ok            The call was successful.
**
**      rpc_s_no_memory     Call to malloc failed to allocate memory
**
**      rpc_s_invalid_naf_id  Source RPC address appeared invalid
**
**
**  IMPLICIT INPUTS:
**
**        A check is performed on the source RPC address before malloc.  It
**        must be the IP family.
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:
**
**           In the event, the addres of a of memory segment contained in
**           rpc_addr, is not valid or the length isn't as long as is
**           indicated, a memory fault may result.
**
**--
**/

INTERNAL void addr_copy
#ifdef _DCE_PROTO_
(
    rpc_addr_p_t            src_rpc_addr,
    rpc_addr_p_t            *dst_rpc_addr,
    unsigned32              *status
)
#else
(src_rpc_addr, dst_rpc_addr, status)
rpc_addr_p_t            src_rpc_addr;
rpc_addr_p_t            *dst_rpc_addr;
unsigned32              *status;
#endif
{
    CODING_ERROR (status);

    /*
     * if the source RPC address looks valid - IP family ok
     */
    if (src_rpc_addr->sa.family == RPC_C_NAF_ID_UXD)
    {
        /*
         * allocate memory for the new RPC address
         */
        RPC_MEM_ALLOC (
            *dst_rpc_addr,
            rpc_addr_p_t,
            sizeof (rpc_np_addr_t),
            RPC_C_MEM_RPC_ADDR,
            RPC_C_MEM_WAITOK);

        if (*dst_rpc_addr == NULL)
        {
            *status = rpc_s_no_memory;
            return;
        }

        /*
         * Copy source rpc address to destination rpc address
         */
        /* b_c_o_p_y ((unsigned8 *) src_rpc_addr, (unsigned8 *) *dst_rpc_addr,
                sizeof (rpc_np_addr_t));*/

        memmove( *dst_rpc_addr, src_rpc_addr, sizeof (rpc_np_addr_t));


        *status = rpc_s_ok;
        return;
    }

    *status = rpc_s_invalid_naf_id;
}

/*
**++
**
**  ROUTINE NAME:       addr_free
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  Free the memory for the RPC address pointed to by the argument
**  address pointer rpc_addr.  Null the address pointer.  The memory
**  must have been previously allocated by RPC_MEM_ALLC.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:
**
**     rpc_addr         The address of a pointer to an RPC address -returned
**                      with a NULL value.
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok        The call was successful.
**
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:
**           In the event, the segment of memory refered to by pointer
**           rpc_addr, is allocated by means other than RPC_MEM_ALLOC,
**           unpredictable results will occur when this routine is called.
**--
**/

INTERNAL void addr_free
#ifdef _DCE_PROTO_
(
    rpc_addr_p_t            *rpc_addr,
    unsigned32              *status
)
#else
(rpc_addr, status)
rpc_addr_p_t            *rpc_addr;
unsigned32              *status;
#endif
{
    CODING_ERROR (status);

    /*
     * free memory of RPC addr
     */
    RPC_MEM_FREE (*rpc_addr, RPC_C_MEM_RPC_ADDR);

    /*
     * indicate that the rpc_addr is now empty
     */
    *rpc_addr = NULL;

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       addr_set_endpoint
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**    Receive the null terminated ascii character string,  rpc_endpoint
**    and convert to the Internet Protocol byte order format.  Insert
**    into the RPC address, pointed to by argument rpc_addr.  The only
**    acceptible endpoint for IP is numeric asci string, or a NULL string.
**
**  INPUTS:
**
**      endpoint        String containing endpoint to insert into RPC address.
**                      For IP must contain an ASCII numeric value, or NULL.
**
**  INPUTS/OUTPUTS:
**
**      rpc_addr        The address of a pointer to an RPC address where
**                      the endpoint is to be inserted.
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok                 The call was successful.
**
**          rpc_s_invalid_naf_id  Argument, endpoint contains an
**                                  unauthorized pointer value.
**          rpc_s_invalid_endpoint_format Endpoint Argument can not be
**                                  converted (not numeric).
**
**  IMPLICIT INPUTS:
**
**      A NULL, (first byte NULL), endpoint string is an indicator to
**      the routine to delete the endpoint from the RPC address. indicated.
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void addr_set_endpoint
#ifdef _DCE_PROTO_
(
    unsigned_char_p_t       endpoint,
    rpc_addr_p_t            *rpc_addr,
    unsigned32              *status
)
#else
(endpoint, rpc_addr, status)
unsigned_char_p_t       endpoint;
rpc_addr_p_t            *rpc_addr;
unsigned32              *status;
#endif
{
    rpc_np_addr_p_t     np_addr = (rpc_np_addr_p_t) *rpc_addr;
    unsigned_char_p_t   p;
    size_t              req_len;

    CODING_ERROR (status);

    /*
     * check to see if this is a request to remove the endpoint
     */
    if (endpoint == NULL || strlen ((char *) endpoint) == 0)
    {
        np_addr->sa.sun_path[0] = '\0';
        *status = rpc_s_ok;
        return;
    }

    if (np_addr->rpc_protseq_id == RPC_C_PROTSEQ_ID_NCACN_NP)
    {
        if (strncasecmp((char *)endpoint, "\\PIPE\\", 6) != 0)
        {
            *status = rpc_s_invalid_endpoint_format;
            return;
        }
    }

    req_len = strlen((char *)endpoint);
    if (endpoint[0] != '/')
    {
        /* Relative ncalrpc path. */
        req_len += RPC_C_NP_DIR_LEN + 1;
    }

    if (req_len >= RPC_C_ENDPOINT_NP_MAX - 1)
    {
        *status = rpc_s_invalid_endpoint_format;
        return;
    }

    if (endpoint[0] != '/' && endpoint[0] != '\\')
        snprintf(np_addr->sa.sun_path, RPC_C_ENDPOINT_NP_MAX, "%s/%s", RPC_C_NP_DIR, endpoint);
    else
        strncpy(np_addr->sa.sun_path, (char *)endpoint, req_len);

    /*
     * Convert backslashes to forward slashes to conform to
     * UNIX filesystem convention.
     */
    if (np_addr->rpc_protseq_id == RPC_C_PROTSEQ_ID_NCACN_NP)
    {
        for (p = (unsigned char*) &np_addr->sa.sun_path[RPC_C_NP_DIR_LEN]; *p != '\0'; p++) {
            if (*p == '\\')
                *p = '/';
        }
    }

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       addr_inq_endpoint
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**    From the RPC address indicated by arg., rpc_addr, examine the
**    endpoint.  Convert the endopint value to a NULL terminated asci
**    character string to be returned in the memory segment pointed to
**    by arg., endpoint.
**
**  INPUTS:
**
**      rpc_addr        The address of a pointer to an RPC address that
**                      to be inspected.
**
**  INPUTS/OUTPUTS:     none
**
**
**  OUTPUTS:
**
**      endpoint        String pointer indicating where the endpoint
**                      string is to be placed.
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok           The call was successful.
**
**          Any of the RPC Protocol Service status codes.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:
**
**      A zero length string will be returned if the RPC address contains
**      no endpoint.
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:
**
**    CAUTION -- since this routine has no way of knowing the exact
**      length of the endpoint string which will be derived.  It
**      is asumed that the caller has provided, rpc_c_endpoint_max
**      (or at least "enough") space for the endpoint string.
**--
**/

INTERNAL void addr_inq_endpoint
#ifdef _DCE_PROTO_
(
    rpc_addr_p_t            rpc_addr,
    unsigned_char_t         **endpoint,
    unsigned32              *status
)
#else
(rpc_addr, endpoint, status)
rpc_addr_p_t            rpc_addr;
unsigned_char_t         **endpoint;
unsigned32              *status;
#endif
{
    rpc_np_addr_p_t     np_addr = (rpc_np_addr_p_t) rpc_addr;
    char          *sun_path;
    unsigned_char_t *p, *q;

    CODING_ERROR (status);

    sun_path = np_addr->sa.sun_path;

    RPC_DBG_GPRINTF (("(addr_inq_endpoint) sun_path->%s\n", sun_path));

    /*
     * if no endpoint present, return null string. Otherwise,
     * return the endpoint in Internet "dot" notation.
     */
    if (sun_path[0] == 0)
    {
        RPC_MEM_ALLOC(
            *endpoint,
            unsigned_char_p_t,
            sizeof(unsigned32),     /* can't stand to get just 1 byte */
            RPC_C_MEM_STRING,
            RPC_C_MEM_WAITOK);
         (*endpoint)[0] = 0;
    }
    else
    {
        RPC_MEM_ALLOC(
            *endpoint,
            unsigned_char_p_t,
            RPC_C_ENDPOINT_NP_MAX,
            RPC_C_MEM_STRING,
            RPC_C_MEM_WAITOK);
        if (np_addr->rpc_protseq_id == RPC_C_PROTSEQ_ID_NCACN_NP)
        {
            if (strncasecmp(sun_path, "\\PIPE\\", 6) != 0)
            {
                *status = rpc_s_invalid_endpoint_format;
                return;
            }
            for (p = (unsigned char*) sun_path, q = (unsigned char*) *endpoint; *p != '\0'; p++) {
                *q++ = (*p == '/') ? '\\' : *p;
            }
            *q = '\0';
        }
        else
        {
            if (strncmp(sun_path, RPC_C_NP_DIR, RPC_C_NP_DIR_LEN) != 0)
            {
                strcpy((char*) *endpoint, sun_path);
            }
            else
            {
                strcpy((char*) *endpoint, &sun_path[RPC_C_NP_DIR_LEN + 1]);
            }
        }
    }

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       addr_set_netaddr
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**    Receive the null terminated ascii character string, netaddr,
**    and convert to the Internet Protocol Network Address format.  Insert
**    into the RPC address, indicated by argument rpc_addr.
**
**  INPUTS:
**
**      netaddr         String containing network address to insert into
**                      RPC address.  It must contain an ASCII value in the
**                      Internet dot notation, (a.b.c.d), format.
**
**  INPUTS/OUTPUTS:
**
**      rpc_addr        The address of a pointer to an RPC address where
**                      the network address is to be inserted.
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**      rpc_s_ok                  The call was successful.
**      rpc_s_inval_net_addr      Invalid IP network address string passed
**                                in netaddr
**
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

INTERNAL void addr_set_netaddr
#ifdef _DCE_PROTO_
(
    unsigned_char_p_t       netaddr,
    rpc_addr_p_t            *rpc_addr,
    unsigned32              *status
)
#else
(netaddr, rpc_addr, status)
unsigned_char_p_t       netaddr;
rpc_addr_p_t            *rpc_addr;
unsigned32              *status;
#endif
{
    rpc_np_addr_p_t     np_addr = (rpc_np_addr_p_t) *rpc_addr;

    CODING_ERROR (status);

    /*
     * check to see if this is a request to remove the netaddr
     */
    if (netaddr == NULL || strlen ((char *) netaddr) == 0)
    {
        np_addr->remote_host[0] = '\0';
        *status = rpc_s_ok;
        return;
    }

    strncpy((char*) &np_addr->remote_host, (char*) netaddr, sizeof(np_addr->remote_host) - 1);
    np_addr->remote_host[sizeof(np_addr->remote_host) - 1] = '\0';

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       addr_inq_netaddr
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**    From the RPC address indicated by arg., rpc_addr, examine the
**    IP network address.  Convert the network address from its network
**    format  to a NULL terminated ascii character string in IP dot
**    notation format.  The character string to be returned in the
**    memory segment pointed to by arg., netaddr.
**
**  INPUTS:
**
**      rpc_addr        The address of a pointer to an RPC address that
**                      is to be inspected.
**
**  INPUTS/OUTPUTS:
**
**
**  OUTPUTS:
**
**      netaddr         String pointer indicating where the network
**                      address string is to be placed.
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok           The call was successful.
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

INTERNAL void addr_inq_netaddr
#ifdef _DCE_PROTO_
(
    rpc_addr_p_t            rpc_addr,
    unsigned_char_t         **p_netaddr,
    unsigned32              *status
)
#else
(rpc_addr, netaddr, status)
rpc_addr_p_t            rpc_addr;
unsigned_char_t         **p_netaddr;
unsigned32              *status;
#endif
{
    rpc_np_addr_p_t     np_addr = (rpc_np_addr_p_t) rpc_addr;
    unsigned_char_p_t netaddr;
    size_t addr_length = 0;

    CODING_ERROR (status);

    /* Note that we return an empty string if the protocol
       sequence is not ncacn_np */
    if (np_addr->rpc_protseq_id == RPC_C_PROTSEQ_ID_NCACN_NP)
    {
        addr_length = strlen(np_addr->remote_host);
    }

    RPC_MEM_ALLOC(
        *p_netaddr,
        unsigned_char_p_t,
        addr_length + 1,
        RPC_C_MEM_STRING,
        RPC_C_MEM_WAITOK);
    if (*p_netaddr == NULL) {
        *status = rpc_s_no_memory;
        return;
    }
    netaddr = *p_netaddr;

    memcpy(netaddr, np_addr->remote_host, addr_length);
    netaddr[addr_length] = 0;

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       addr_set_options
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**     Receive a NULL terminated network options string and insert
**     into the RPC address indicated by art., rpc_addr.
**
**      NOTE - there are no options used with the NP service this
**             routine is here only to serve as a stub.
**
**  INPUTS:
**
**      options         String containing network options to insert
**                      into  RPC address.
**
**  INPUTS/OUTPUTS:
**
**      rpc_addr        The address of a pointer to an RPC address where
**                      the network options strig is to be inserted.
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok           The call was successful.
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


INTERNAL void addr_set_options
#ifdef _DCE_PROTO_
(
    unsigned_char_p_t       network_options ATTRIBUTE_UNUSED,
    rpc_addr_p_t            *rpc_addr ATTRIBUTE_UNUSED,
    unsigned32              *status
)
#else
(network_options, rpc_addr, status)
unsigned_char_p_t       network_options;
rpc_addr_p_t            *rpc_addr;
unsigned32              *status;
#endif
{
    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       addr_inq_options
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      Extract the network options from the RPC address pointed to
**      by rpc_addr and convert to a NULL terminated string placed
**      in a buffer indicated by the options arg.
**
**      NOTE - there are no options used with the NP service this
**             routine is here only to serve as a stub.
**
**  INPUTS:
**
**      rpc_addr        The address of a pointer to an RPC address that
**                      is to be inspected.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      options         String pointer indicating where the network
**                      options string is to be placed.
**
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok           The call was successful.
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

INTERNAL void addr_inq_options
#ifdef _DCE_PROTO_
(
    rpc_addr_p_t            rpc_addr ATTRIBUTE_UNUSED,
    unsigned_char_t         **network_options,
    unsigned32              *status
)
#else
(rpc_addr, network_options, status)
rpc_addr_p_t            rpc_addr;
unsigned_char_t         **network_options;
unsigned32              *status;
#endif
{
    RPC_MEM_ALLOC(
        *network_options,
        unsigned_char_p_t,
        sizeof(unsigned32),     /* only really need 1 byte */
        RPC_C_MEM_STRING,
        RPC_C_MEM_WAITOK);

    *network_options[0] = 0;
    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       inq_max_tsdu
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  INPUTS:
**
**      naf_id          Network Address Family ID serves
**                      as index into EPV for IP routines.
**
**      iftype          Network interface type ID
**
**      protocol        Network protocol ID
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      max_tsdu
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

INTERNAL void inq_max_tsdu
#ifdef _DCE_PROTO_
(
    rpc_naf_id_t            naf_id ATTRIBUTE_UNUSED,
    rpc_network_if_id_t     iftype,
    rpc_network_protocol_id_t protocol,
    unsigned32              *max_tsdu,
    unsigned32              *status
)
#else
(naf_id, iftype, protocol, max_tsdu, status)
rpc_naf_id_t            naf_id;
rpc_network_if_id_t     iftype;
rpc_network_protocol_id_t protocol;
unsigned32              *max_tsdu;
unsigned32              *status;
#endif
{
    *max_tsdu = RPC_C_NP_MAX_LOCAL_FRAG_SIZE;
	iftype = 0;
	protocol = 0;

#ifdef DEBUG
    if (RPC_DBG (rpc_es_dbg_np_max_tsdu, 1))
    {
        *max_tsdu = ((unsigned32)
            (rpc_g_dbg_switches[(int) rpc_es_dbg_np_max_tsdu])) * 1024;
    }
#endif

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       addr_compare
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  Determine if two address are equal.
**
**  INPUTS:
**
**      addr1
**
**      addr2
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
**      return          Boolean; true if address are the same.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL boolean addr_compare
#ifdef _DCE_PROTO_
(
    rpc_addr_p_t            addr1,
    rpc_addr_p_t            addr2,
    unsigned32              *status ATTRIBUTE_UNUSED
)
#else
(addr1, addr2, status)
rpc_addr_p_t            addr1, addr2;
unsigned32              *status;
#endif
{
    rpc_np_addr_p_t     np_addr1 = (rpc_np_addr_p_t) addr1;
    rpc_np_addr_p_t     np_addr2 = (rpc_np_addr_p_t) addr2;

    if (np_addr1->sa.sun_family == np_addr2->sa.sun_family &&
        strncmp(np_addr1->sa.sun_path,
		np_addr2->sa.sun_path, sizeof(np_addr1->sa.sun_path)) == 0 &&
	strncmp(np_addr1->remote_host,
		np_addr2->remote_host, sizeof(np_addr1->remote_host)) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}


/*
**++
**
**  ROUTINE NAME:       inq_max_pth_unfrag_tpdu
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  INPUTS:
**
**      naf_id          Network Address Family ID serves
**                      as index into EPV for IP routines.
**
**      iftype          Network interface type ID
**
**      protocol        Network protocol ID
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      max_tpdu
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

INTERNAL void inq_max_pth_unfrag_tpdu
#ifdef _DCE_PROTO_
(
    rpc_addr_p_t            rpc_addr ATTRIBUTE_UNUSED,
    rpc_network_if_id_t     iftype,
    rpc_network_protocol_id_t protocol,
    unsigned32              *max_tpdu,
    unsigned32              *status
)
#else
(rpc_addr, iftype, protocol, max_tpdu, status)
rpc_addr_p_t            rpc_addr;
rpc_network_if_id_t     iftype;
rpc_network_protocol_id_t protocol;
unsigned32              *max_tpdu;
unsigned32              *status;
#endif
{
	iftype = 0;
	protocol = 0;
    *max_tpdu = RPC_C_NP_MAX_LOCAL_FRAG_SIZE;

#ifdef DEBUG
    if (RPC_DBG (rpc_es_dbg_np_max_pth_unfrag_tpdu, 1))
    {
        *max_tpdu = ((unsigned32)
            (rpc_g_dbg_switches[(int) rpc_es_dbg_np_max_pth_unfrag_tpdu])) * 32;
    }
#endif

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       inq_max_loc_unfrag_tpdu
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  INPUTS:
**
**      naf_id          Network Address Family ID serves
**                      as index into EPV for IP routines.
**
**      iftype          Network interface type ID
**
**      protocol        Network protocol ID
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      max_tpdu
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

INTERNAL void inq_max_loc_unfrag_tpdu
#ifdef _DCE_PROTO_
(
    rpc_naf_id_t            naf_id ATTRIBUTE_UNUSED,
    rpc_network_if_id_t     iftype,
    rpc_network_protocol_id_t protocol,
    unsigned32              *max_tpdu,
    unsigned32              *status
)
#else
(naf_id, iftype, protocol, max_tpdu, status)
rpc_naf_id_t            naf_id;
rpc_network_if_id_t     iftype;
rpc_network_protocol_id_t protocol;
unsigned32              *max_tpdu;
unsigned32              *status;
#endif
{
	iftype = 0;
	protocol = 0;
    *max_tpdu = RPC_C_NP_MAX_LOCAL_FRAG_SIZE;

#ifdef DEBUG
    if (RPC_DBG (rpc_es_dbg_np_max_loc_unfrag_tpdu, 1))
    {
        *max_tpdu = ((unsigned32)
            (rpc_g_dbg_switches[(int) rpc_es_dbg_np_max_loc_unfrag_tpdu])) * 32;
    }
#endif

    *status = rpc_s_ok;
}

/*
**++
**
**  ROUTINE NAME:       desc_inq_network
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  This routine is responsible for "reverse-engineering" the parameters to
**  the original socket call that was made to create the socket "desc".
**
**  INPUTS:
**
**      desc            socket descriptor to query
**
**  INPUTS/OUTPUTS:
**
**  OUTPUTS:
**
**      socket_type     network interface type id
**
**      protocol_id     network protocol family id
**
**      status          status returned
**                              rpc_s_ok
**                              rpc_s_cant_get_if_id
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

INTERNAL void desc_inq_network
#ifdef _DCE_PROTO_
(
    rpc_socket_t              desc,
    rpc_network_if_id_t       *socket_type,
    rpc_network_protocol_id_t *protocol_id,
    unsigned32                *status
)
#else
(desc, socket_type, protocol_id, status)
rpc_socket_t              desc;
rpc_network_if_id_t       *socket_type;
rpc_network_protocol_id_t *protocol_id;
unsigned32                *status;
#endif
{
    rpc_socket_error_t serr;
    rpc_np_addr_t addr;
    boolean is_np;

    CODING_ERROR (status);

    /*
     * Get the socket type.
     */
    *protocol_id = 0;

    serr = rpc__socket_get_if_id (desc, socket_type);
    if (RPC_SOCKET_IS_ERR (serr))
    {
        RPC_DBG_GPRINTF (("(desc_inq_network) rpc__socket_get_if_id serr->%d\n",serr));
        *status = rpc_s_cant_get_if_id;
        return;
    }

    addr.len = sizeof(addr) - offsetof(rpc_np_addr_t, sa);
    serr = rpc__socket_inq_endpoint (desc, (rpc_addr_p_t)&addr);
    if (RPC_SOCKET_IS_ERR (serr))
    {
       RPC_DBG_GPRINTF (("(desc_inq_network) rpc__socket_inq_endpoint serr->%d\n", serr));
       *status = rpc_s_cant_get_if_id;
       return;
    }

    if (addr.sa.sun_family != RPC_C_NAF_ID_UXD)
    {
       RPC_DBG_GPRINTF (("(desc_inq_network) not RPC_C_NAF_ID_UXD socket\n"));
       *status = rpc_s_cant_get_if_id;
       return;
    }

    /* XXX should check for arbitrary number of delimiters. */
    is_np = (strncmp(addr.sa.sun_path, RPC_C_NP_DIR "//PIPE", RPC_C_NP_DIR_LEN + 6) == 0) ||
            (strncmp(addr.sa.sun_path, RPC_C_NP_DIR "/PIPE", RPC_C_NP_DIR_LEN + 5) == 0);

    /*
     * This is a disgusting hack due to the tight binding between
     * the BSD socket API and the OSF runtime. Domain sockets use
     * the "IP" protocol and named pipes use "ICMP". Yuck.
     */

    switch ((int)(*socket_type))
    {
        case SOCK_STREAM:       *protocol_id = is_np ? RPC_C_NETWORK_PROTOCOL_ID_NP :
                                                       RPC_C_NETWORK_PROTOCOL_ID_UXD;
                                break;

        default:		/*
				 * rpc_m_unk_sock_type
				 * "(%s) Unknown socket type"
				 */
				RPC_DCE_SVC_PRINTF ((
				    DCE_SVC(RPC__SVC_HANDLE, "%s"),
				    rpc_svc_general,
				    svc_c_sev_fatal | svc_c_action_abort,
				    rpc_m_unk_sock_type,
				    "desc_inq_network" ));
				break;
    }

    *status = rpc_s_ok;
}


/*
**++
**
**  ROUTINE NAME:       set_pkt_nodelay
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  INPUTS:
**
**      desc            The network descriptor to apply the nodelay
**                      option to.
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
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void set_pkt_nodelay
#ifdef _DCE_PROTO_
(
    rpc_socket_t            desc,
    unsigned32              *status
)
#else
(desc, status)
rpc_socket_t            desc;
unsigned32              *status;
#endif
{
	desc = 0;
    *status = rpc_s_ok;
}


/*
**++
**
**  ROUTINE NAME:       is_connect_closed
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**      This routine is called when a recv on a sequenced packet
**      socket has returned zero bytes. Since TCP does not support
**      sequenced sockets the routine is a no-op. "true" is returned
**      because zero bytes received on a stream socket does mean the
**      connection is closed.
**
**  INPUTS:
**
**      desc            The network descriptor representing the connection.
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
**      boolean         true if the connection is closed, false otherwise.
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL boolean is_connect_closed
#ifdef _DCE_PROTO_
(
    rpc_socket_t            desc ATTRIBUTE_UNUSED,
    unsigned32              *status
)
#else
(desc, status)
rpc_socket_t            desc;
unsigned32              *status;
#endif
{
    *status = rpc_s_ok;
    return (true);
}


/*
**++
**
**  ROUTINE NAME:       tower_flrs_from_addr
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  Creates the lower tower floors from an RPC addr.
**
**  INPUTS:
**
**      rpc_addr	RPC addr to convert to lower tower floors.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      lower_flrs      The returned lower tower floors.
**
**      status          A value indicating the return status of the routine.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void tower_flrs_from_addr
#ifdef _DCE_PROTO_
(
    rpc_addr_p_t       rpc_addr,
    twr_p_t            *lower_flrs,
    unsigned32         *status
)
#else
(rpc_addr, lower_flrs, status)
rpc_addr_p_t       rpc_addr;
twr_p_t            *lower_flrs;
unsigned32         *status;
#endif
{
    unsigned32    net_prot_id;

    CODING_ERROR (status);

#ifdef RPC_NO_TOWER_SUPPORT

    *status = rpc_s_coding_error;

#else

#if 0
    /*
     * The use of a temporary lower floors twr_t is in anticipation
     * of the twr_* routines belonging to a separate library with
     * their own memory allocation. In that case, twr_* allocates
     * memory for returning the lower towers, and we must copy from
     * twr_* memory into our (rpc) memory. After the copy, we free
     * the twr_* allocated memory.
     *
     * For now, twr_* routines also use RPC_MEM_ALLOC and RPC_MEM_FREE,
     * so we'll skip the extra copy.
     */
    twr_p_t     temp_lower_flrs;
#endif

    /*
     * Get the network protocol id (aka transport layer protocol)
     * for this RPC addr.
     */
    net_prot_id = RPC_PROTSEQ_INQ_NET_PROT_ID(rpc_addr->rpc_protseq_id);

    /*
     * Convert sockaddr to lower tower floors.
     */
    if (rpc_addr->rpc_protseq_id == RPC_C_PROTSEQ_ID_NCACN_NP)
        twr_np_lower_flrs_from_sa ((sockaddr_t *) &(rpc_addr->sa),
#if 0
        &temp_lower_flrs,
#else
        lower_flrs,
#endif
        status);
    else
        twr_uxd_lower_flrs_from_sa ((sockaddr_t *) &(rpc_addr->sa),
#if 0
        &temp_lower_flrs,
#else
        lower_flrs,
#endif
        status);

    if (*status != twr_s_ok)
    {
        return;
    }

#if 0
    /*
     * Allocate a tower structure to hold the wire (and nameservice)
     * representation of the lower tower floors returned from twr_*().
     *
     * The size includes the sizof twr_t + length of the tower floors
     * returned from twr_np_lower_flrs_from_sa - 1 (for tower_octet_string[0].
     */
    RPC_MEM_ALLOC (
        *lower_flrs,
        twr_p_t,
        sizeof(twr_t) + temp_lower_flrs->tower_length - 1,
        RPC_C_MEM_TOWER,
        RPC_C_MEM_WAITOK );

    /*
     * Set the tower length to the length of the tower flrs returnd from
     * twr_np_lower_flrs_from_sa.
     */
    (*lower_flrs)->tower_length = temp_lower_flrs->tower_length;

    /*
     * Copy the lower tower floors to the tower octet string.
     */
    memcpy ((*lower_flrs)->tower_octet_string,
        temp_lower_flrs->tower_octet_string,
        temp_lower_flrs->tower_length);

    /*
     * Free the twr_np_lower_flrs_from_sa allocated memory.
     */
    RPC_MEM_FREE (temp_lower_flrs, RPC_C_MEM_TOWER);
#endif

#endif

    return;
}

/*
**++
**
**  ROUTINE NAME:       tower_flrs_to_addr
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  Creates an RPC addr from a protocol tower.
**
**  INPUTS:
**
**      tower_octet_string
**                      Protocol tower whose floors are used to construct
**                      an RPC addr
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      rpc_addr	RPC addr constructed from a protocol tower.
**
**      status          A value indicating the return status of the routine:
**                          rpc_s_ok
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void tower_flrs_to_addr
#ifdef _DCE_PROTO_
(
    byte_p_t           tower_octet_string,
    rpc_addr_p_t       *rpc_addr,
    unsigned32         *status
)
#else
(tower_octet_string, rpc_addr, status)
byte_p_t           tower_octet_string;
rpc_addr_p_t       *rpc_addr;
unsigned32         *status;
#endif
{
    sockaddr_t    *sa;
    unsigned32    sa_len;

    CODING_ERROR (status);

#ifdef RPC_NO_TOWER_SUPPORT

    *status = rpc_s_coding_error;

#else

    /*
     * Convert the lower floors of a tower to a sockaddr.
     */
    if (tower_octet_string[0] == 0x05) /* 5 floors for NP, 4 for LPC */
        twr_np_lower_flrs_to_sa (
            tower_octet_string,        /* tower octet string (has flr count). */
            &sa,                       /* returned sockaddr     */
            &sa_len,                   /* returned sockaddr len */
            status);
    else
        twr_uxd_lower_flrs_to_sa (
            tower_octet_string,        /* tower octet string (has flr count). */
            &sa,                       /* returned sockaddr     */
            &sa_len,                   /* returned sockaddr len */
            status);

    if (*status != rpc_s_ok)
        return;

    /*
     * Call the common NAF routine to create an RPC addr from a sockaddr.
     * (rpc__naf_addr_from_sa doesn't dispatch to a naf-specific routine.)
     */
    rpc__naf_addr_from_sa (sa, sa_len, rpc_addr, status);

    /*
     * Always free the twr_np_lower_flrs_to_sa allocated memory - regardless
     * of the status from rpc__naf_addr_from_sa.
     */
    RPC_MEM_FREE (sa, RPC_C_MEM_SOCKADDR);

#endif /* RPC_NO_TOWER_SUPPORT */

    /*
     * Return whatever status we had.
     */
    return;
}


/*
**++
**
**  ROUTINE NAME:       desc_inq_peer_addr
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**    Receive a socket descriptor which is queried to obtain family,
**    remote endpoint and remote network address.  If this information appears valid
**    for an DECnet IV address,  space is allocated for an RPC address which
**    is initialized with the information obtained from this socket.  The
**    address indicating the created RPC address is returned in, arg., rpc_addr.
**
**  INPUTS:
**
**      protseq_id             Protocol Sequence ID representing a
**                             particular Network Address Family,
**                             its Transport Protocol, and type.
**
**      desc                   Descriptor, indicating a socket that
**                             has been created on the local operating
**                             platform.
**
**  INPUTS/OUTPUTS:
**
**      rpc_addr        The address of a pointer where the RPC address
**                      created by this routine will be indicated.
**
**  OUTPUTS:
**
**      status          A value indicating the status of the routine.
**
**          rpc_s_ok               The call was successful.
**
**          rpc_s_no_memory         Call to malloc failed to allocate memory.
**
**          rpc_s_cant_get_peername Call to getpeername failed.
**
**          rpc_s_invalid_naf_id   Socket that arg desc refers is not DECnet IV.
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

INTERNAL void desc_inq_peer_addr
#ifdef _DCE_PROTO_
(
    rpc_protseq_id_t        protseq_id,
    rpc_socket_t            desc,
    rpc_addr_p_t            *rpc_addr,
    unsigned32              *status
)
#else
(protseq_id, desc, rpc_addr, status)
rpc_protseq_id_t        protseq_id;
rpc_socket_t            desc;
rpc_addr_p_t            *rpc_addr;
unsigned32              *status;
#endif
{
    rpc_socket_error_t  serr;
    rpc_np_addr_p_t np_addr;

    CODING_ERROR (status);


    /*
     * allocate memory for the new RPC address
     */
    RPC_MEM_ALLOC (np_addr,
                   rpc_np_addr_p_t,
                   sizeof (rpc_np_addr_t),
                   RPC_C_MEM_RPC_ADDR,
                   RPC_C_MEM_WAITOK);

    /*
     * successful malloc
     */
    if (np_addr == NULL)
    {
        *status = rpc_s_no_memory;
        return;
    }
    *rpc_addr = (rpc_addr_p_t) np_addr;

    /*
     * insert individual parameters into RPC address
     */
    np_addr->rpc_protseq_id = protseq_id;
    np_addr->len = sizeof (struct sockaddr_un);

    /*
     * Get the peer address (name).
     *
     * If we encounter an error, free the address structure and return
     * the status from the getpeername() call, not the free() call.
     */

    memset(np_addr->sa.sun_path, 0, sizeof(np_addr->sa.sun_path));
    serr = rpc__socket_getpeername (desc, (rpc_addr_p_t)np_addr);
    if (RPC_SOCKET_IS_ERR (serr))
    {
        RPC_DBG_GPRINTF(("(desc_inq_peer_addr) rpc__socket_getpeername returned %d (socket->%d)\n", serr, desc));
        RPC_MEM_FREE (np_addr, RPC_C_MEM_RPC_ADDR);
        *rpc_addr = (rpc_addr_p_t)NULL;
        *status = rpc_s_cant_getpeername;
    }
    else
    {
        /* Force the address family to a correct value since getpeername can fail
           silently on some platforms for UNIX domain sockets */
        np_addr->sa.sun_family = RPC_C_NAF_ID_UXD;
        RPC_DBG_GPRINTF(("(desc_inq_peer_addr) peer address is %s (socket->%d)\n", np_addr->sa.sun_path, desc));
        *status = rpc_s_ok;
    }
}


/*
**++
**
**  ROUTINE NAME:       set_port_restriction
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**
**  Builds an rpc_port_restriction_list_t and glues it into the
**  rpc_protseq_id_elt_t in the rpc_g_protseq_id[] list.
**
**  INPUTS:
**
**      protseq_id
**                      The protocol sequence id to set port restriction
**                      on.
**      n_elements
**                      The number of port ranges passed in.
**
**      first_port_name_list
**                      An array of pointers to strings containing the
**                      lower bound port names.
**
**      last_port_name_list
**                      An array of pointers to strings containing the
**                      upper bound port names.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      status
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/


INTERNAL void set_port_restriction
#ifdef _DCE_PROTO_
(
     rpc_protseq_id_t            protseq_id,
     unsigned32                  n_elements,
     unsigned_char_p_t           *first_port_name_list,
     unsigned_char_p_t           *last_port_name_list,
     unsigned32                  *status
)
#else
(protseq_id, n_elements, first_port_name_list, last_port_name_list, status)
rpc_protseq_id_t            protseq_id;
unsigned32                  n_elements;
unsigned_char_p_t           *first_port_name_list;
unsigned_char_p_t           *last_port_name_list;
unsigned32                  *status;
#endif
{
    CODING_ERROR (status);

	protseq_id = 0;
	n_elements = 0;
	first_port_name_list = NULL;
	last_port_name_list = NULL;

    *status = rpc_s_invalid_arg;
}                                       /* set_port_restriction */


/*
**++
**
**  ROUTINE NAME:       get_next_restricted_port
**
**  SCOPE:              INTERNAL
**
**  DESCRIPTION:
**
**  Returns the next restricted port in a sequence.  There is no guarantee
**  that the port is available, that is up to bind() to determine.
**
**  INPUTS:
**
**      protseq_id
**                      The protocol sequence id to get the port on.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      port_name
**                      An IP port name as a text string.
**      status
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     void
**
**  SIDE EFFECTS:       none
**
**--
**/

INTERNAL void get_next_restricted_port
#ifdef _DCE_PROTO_
(
    rpc_protseq_id_t           protseq_id,
     unsigned_char_p_t          *port_name,
     unsigned32                 *status
)
#else
(protseq_id, port_name, status)
rpc_protseq_id_t           protseq_id;
unsigned_char_p_t          *port_name;
unsigned32                 *status;
#endif
{
    CODING_ERROR (status);

	protseq_id = 0;
	port_name = NULL;

	/*
	 * Return an error to tell the caller that there is no range
	 * restriction on this protocol sequence.
	 */

	*status = rpc_s_invalid_arg;
	return;
}                                       /* get_next_restricted_port */

/*
**++
**
**  ROUTINE NAME:       inq_max_frag_size
**
**  SCOPE:              INTERNAL - declared locally
**
**  DESCRIPTION:
**
**  INPUTS:
**
**      naf_id          Network Address Family ID serves
**                      as index into EPV for IP routines.
**
**      iftype          Network interface type ID
**
**      protocol        Network protocol ID
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      max_tpdu
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

INTERNAL void inq_max_frag_size
#ifdef _DCE_PROTO_
(
 rpc_addr_p_t rpc_addr,
 unsigned32   *max_frag_size,
 unsigned32   *status
)
#else
(rpc_addr, max_frag_size, status)
rpc_addr_p_t rpc_addr;
unsigned32   *max_frag_size;
unsigned32   *status;
#endif
{
    rpc_addr = 0;

    *status = rpc_s_ok;

    *max_frag_size = RPC_C_NP_MAX_LOCAL_FRAG_SIZE;
    return;

}

