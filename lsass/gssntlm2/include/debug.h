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

/*
 * debug.h
 *
 * Debug helpers
 *
 * Copyright (c) 2007 Likewise software
 *
 * Author: Todd Stecher
 */
#ifndef _DEBUG_H_
#define _DEBUG_H_

extern DWORD db_level;

#define D_CRITICAL  0x0001
#define D_ERROR     0x0002
#define D_WARN      0x0004
#define D_TRACE     0x0008
#define D_CTXT      0x0010
#define D_CRED      0x0020

/* client only - gss helpers & such */
#define D_GSS       0x0100

/* dump bytes for routine */
#define D_BYTES     0x1000

/*@todo - autoconfigure this #ifdef DEBUG_SUPPORT*/
#define DBG(_l_,_m_) do { if (_l_ & db_level) printf _m_; } while (0)

/*
 * Helpful debug routines
 *
 */
void
DBGDumpSecBuffer(
    DWORD lvl,
    char *msg,
    PSEC_BUFFER secBuf
    );

void
DBGDumpGSSBuffer(
    DWORD lvl,
    char *msg,
    gss_buffer_t gssbuf
    );
void
DBGDumpSecBufferS(
    DWORD lvl,
    char *msg,
    PSEC_BUFFER_S secBuf
    );

void
NTLMDumpNegotiateFlags(
    DWORD lvl,
    char* msg,
    ULONG flags
    );

#endif /* _DEBUG_H_ */
