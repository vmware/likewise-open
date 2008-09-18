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
 *        ntlmencode.h
 *
 * Abstract:
 *
 *       NTLM endian safe encoding routines
 *
 * Author: Todd Stecher (2007)
 *
 */
#ifndef _NTLMENCODE_H_
#define _NTLMENCODE_H_


void
NTLMPutSecBuffer(
    PSEC_BUFFER sb,
    PBYTE ptr,
    ULONG *bufofs,
    ULONG *ofs
    );

void
NTLMPutSecBufferS(
    PSEC_BUFFER_S sb,
    PBYTE ptr,
    ULONG *bufofs,
    ULONG *ofs
    );

void
NTLMPutLsaString(
    PLSA_STRING str,
    PBYTE ptr,
    ULONG *bufofs,
    ULONG *ofs
    );

ULONG
NTLMGetSecBuffer(
    PSEC_BUFFER dst,
    PSEC_BUFFER src,
    ULONG *ofs
    );

ULONG
NTLMGetLsaString(
    PLSA_STRING dst,
    PSEC_BUFFER src,
    ULONG *ofs
    );







#if 0 
/* #ifdef BIG_ENDIAN */

/* @todo - create BE versions of below - these are ALL WRONG!! */
/* get values macros */
#define GET_USHORT(_ptr_,_ofs_) (*(USHORT*)((PBYTE)(_ptr_) + (_ofs_)))
#define GET_ULONG(_ptr_,_ofs_)  (*(ULONG*)((PBYTE*)(_ptr_) + (_ofs_)))
#define GET_SHORT(_ptr_,_ofs_)  (*(SHORT*)((PBYTE*)(_ptr_) + (_ofs_)))
#define GET_LONG(_ptr_,_ofs_)   (*(LONG*)((PBYTE*)(_ptr_) + (_ofs_)))

/* store values macros */ 
#define PUT_USHORT(_ptr_,_ofs_,_val_)   GET_USHORT(_ptr_,_ofs_) = ((USHORT)(_val_))
#define PUT_ULONG(_ptr_,_ofs_,_val_)    GET_ULONG(_ptr_,_ofs_) = ((ULONG)(_val_))
#define PUT_SHORT(_ptr_,_ofs_,_val_)    GET_SHORT(_ptr_,_ofs_) = ((SHORT)(_val_))
#define PUT_LONG(_ptr_,_ofs_,_val_)     GET_LONG(_ptr_,_ofs_) = ((LONG)(_val_))


#else


/* get values macros*/
#define GET_USHORT(_ptr_,_ofs_) (*(USHORT*)(((PBYTE)(_ptr_)) + (_ofs_)))
#define GET_ULONG(_ptr_,_ofs_)  (*(ULONG*)(((PBYTE)(_ptr_)) + (_ofs_)))
#define GET_SHORT(_ptr_,_ofs_)  (*(SHORT*)(((PBYTE)(_ptr_)) + (_ofs_)))
#define GET_LONG(_ptr_,_ofs_)   (*(LONG*)(((PBYTE)(_ptr_)) + (_ofs_)))

#define GET_UCS2(_ptr_,_ofs_,_buf_,_len_)   memcpy(_ptr_,&(_buf_[_ofs_]),_len_)
#define GET_BYTES(_ptr_,_ofs_,_buf_,_len_)   memcpy(_ptr_,&(_buf_[_ofs_]),_len_)

/* store values macros */ 
#define PUT_USHORT(_ptr_,_ofs_,_val_)   GET_USHORT(_ptr_,_ofs_) = ((USHORT)(_val_))
#define PUT_ULONG(_ptr_,_ofs_,_val_)    GET_ULONG(_ptr_,_ofs_) = ((ULONG)(_val_))
#define PUT_SHORT(_ptr_,_ofs_,_val_)    GET_SHORT(_ptr_,_ofs_) = ((SHORT)(_val_))
#define PUT_LONG(_ptr_,_ofs_,_val_)     GET_LONG(_ptr_,_ofs_) = ((LONG)(_val_))

#define PUT_UCS2(_ptr_,_ofs_,_buf_,_len_)   memcpy(&(_ptr_[_ofs_]),_buf_,_len_)
#define PUT_BYTES(_ptr_,_ofs_,_buf_,_len_)   memcpy(&(_ptr_[_ofs_]),_buf_,_len_)

#endif /*BIG_ENDIAN*/









#endif /* _NTLMENCODE_H_ */
