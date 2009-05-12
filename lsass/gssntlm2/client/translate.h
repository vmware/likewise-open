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
 *      translate.h
 *
 * Abstract:
 *
 *      GSS translation routines
 *
 * Authors: Todd Stecher
 */
 #ifndef _TRANSLATE_H_
 #define _TRANSLATE_H_

#define MAKE_GSS_BUFFER(_g_,_s_)    do {(_g_)->value = (_s_)->buffer; (_g_)->length = (_s_)->length;} while (0)

#define MAKE_SECBUFFER(_s_,_g_)     do {(_s_)->length = (_s_)->maxLength = (_g_)->length; (_s_)->buffer = (_g_)->value; } while (0)

#define BAIL_ON_GSS_ERROR(_s_)  do {if ((_s_) && LSA_ERROR_MASK((_s_))) BAIL_ON_NTLM_ERROR((_s_));} while (0)

OM_uint32
NTLMTranslateMajorStatus(
    DWORD dwMajor
);

OM_uint32
NTLMTranslateMinorStatus(
    DWORD dwMinor
);

DWORD
NTLMTranslateGSSName(
    gss_name_t gssName,
    PLSA_STRING lsaName
);




#endif








