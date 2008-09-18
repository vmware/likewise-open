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
 * NTLM.H
 *
 * This contains a majority of the interesting bits and structures 
 * for the libgssntlm library.
 *
 * Copyright (c) 2007 Likewise software
 *
 * Author: Todd Stecher
 */
#ifndef _NTLM_H_
#define _NTLM_H_

/* common structures between client and server */
typedef struct _NTLMGSS_SUPPLIED_CREDS {
    LSA_STRING user;
    LSA_STRING domain;
    LSA_STRING password;
    DWORD flags;
} NTLMGSS_SUPPLIED_CREDS, *PNTLMGSS_SUPPLIED_CREDS;

/* static buffer secbufer */
typedef struct _SEC_BUFFER_S {
    USHORT length;
    USHORT maxLength;
    BYTE buffer[24];
} SEC_BUFFER_S, *PSEC_BUFFER_S;

/* Helper Macros */

/* @todo - fix these up for all byte orders */
#define OFFSET_TO_PTR(_b_,_o_) ((PBYTE)((ULONG)(_b_) +(ULONG)(_o_)))
#define PTR_TO_OFFSET(_b_,_p_) ((ULONG)(((PBYTE)(_p_) - (PBYTE)(_b_))))
#define CHECK_FLAG(_b_,_v_) ((_b_ & _v_) != 0)
#define SET_FLAG(_b_,_v_) ((_b_ |= _v_))
#define ZERO_STRUCT(_s_) memset((char*)&(_s_),0,sizeof(_s_))
#define ZERO_STRUCTP(_s_) do{if ((_s_) != NULL) memset((char*)(_s_),0,sizeof(*(_s_)));} while(0)
#define INIT_SEC_BUFFER_S(_s_, _l_) do{(_s_)->length = (_s_)->maxLength = (_l_); memset((_s_)->buffer, 0, 24);} while (0) 
#define SEC_BUFFER_COPY(_d_,_s_) memcpy((_d_)->buffer,(_s_)->buffer,(_s_)->maxLength)



#define NULL_LSASTRING              {0,0,0}       	


#endif // _NTLM_H_
