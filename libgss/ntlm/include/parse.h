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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        parse.h
 *
 * Abstract:
 *
 *        NTLM parsing primitives
 *
 * Author: Todd Stecher (2007)
 *
 */

#ifndef _PARSE_H_
#define _PARSE_H_

#define ALIGN_UP(_x_,_y_) ((((_x_) + ((_y_) - 1)) / (_y_)) * (_y_))
#define ALIGN_UP_8(_x_) (ALIGN_UP(_x_, sizeof(DWORD)))

#define NEED_SIZE(_x_) do { if (_x_ > pBuf->maxLength) return false;} while(0)

#define PARSE_TEST(_x_) do { if (!(_x_)) {dwError = LSA_ERROR_INVALID_MESSAGE; BAIL_ON_NTLM_ERROR(dwError);} } while(0)


bool
ParseBytes(
    PSEC_BUFFER pBuf, 
    DWORD cBytes, 
    PBYTE *pOutbuf
    );

bool
ParseUINT32(
    PSEC_BUFFER pBuf, 
    UINT32 *pUINT32
    );

bool
ParseUINT64(
    PSEC_BUFFER pBuf, 
    UINT64 *pUINT64
    );


bool
ParseLsaString(
    PSEC_BUFFER pBase,
    PSEC_BUFFER pBuf,
    PLSA_STRING pLsaString
    );

bool
ComposeBytes(
    PSEC_BUFFER pBuf, 
    DWORD cBytes, 
    PBYTE *pOutbuf
    );

bool
ComposeUINT32(
    PSEC_BUFFER pBuf, 
    UINT32 *pUINT32
    );

bool
ComposeUINT64(
    PSEC_BUFFER pBuf, 
    UINT64 *pUINT64
    );


bool
ComposeLsaString(
    PSEC_BUFFER pBase,
    PSEC_BUFFER pBuf,
    PLSA_STRING pLsaString
    );


#endif
