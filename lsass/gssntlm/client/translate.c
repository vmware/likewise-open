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
 *       translate.c 
 *
 * Abstract:
 *
 *       GSS - lsa translation routines
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "client.h"

OM_uint32
NTLMTranslateMajorStatus(
    DWORD dwMajor 
)
{

    switch (dwMajor)
    {
        case LSA_ERROR_SUCCESS:
            return GSS_S_COMPLETE;
        case LSA_WARNING_CONTINUE_NEEDED:
            return GSS_S_CONTINUE_NEEDED;
        case LSA_ERROR_BAD_MECH:
            return GSS_S_BAD_MECH;
        case LSA_ERROR_BAD_NAMETYPE:
            return GSS_S_BAD_NAMETYPE;
        case LSA_ERROR_INVALID_CONTEXT:
        case LSA_ERROR_NO_CONTEXT:
            return GSS_S_NO_CONTEXT;
        case LSA_ERROR_INVALID_CREDENTIAL:
        case LSA_ERROR_NO_CRED:
            return GSS_S_NO_CRED;
        case LSA_ERROR_INSUFFICIENT_BUFFER:
        case LSA_ERROR_INVALID_TOKEN:
        case LSA_ERROR_UNEXPECTED_TOKEN:
            return GSS_S_DEFECTIVE_TOKEN;
        case LSA_ERROR_UNSUPPORTED_SUBPROTO:
            return GSS_S_UNAVAILABLE;
        case LSA_ERROR_NO_SUCH_USER:
        case LSA_ERROR_INVALID_PASSWORD:
        case LSA_ERROR_ACCOUNT_LOCKED:
        case LSA_ERROR_ACCOUNT_DISABLED:
            return GSS_S_UNAUTHORIZED;
        case LSA_ERROR_NOT_IMPLEMENTED:
            return GSS_S_UNAVAILABLE;
        case LSA_ERROR_OUT_OF_MEMORY:
        case LSA_ERROR_INTERNAL:
        default:
            return GSS_S_FAILURE;
    }
            
    return GSS_S_FAILURE;

}

OM_uint32
NTLMTranslateMinorStatus(
    DWORD dwMinor
)
{
    /*@todo - think about this more */
    return NTLMTranslateMajorStatus(dwMinor);
}


DWORD
NTLMTranslateGSSName(
    gss_name_t  gssName,
    PLSA_STRING lsaName
)
{
    DWORD dwError;

    /* other name types to support? */
    /* right now, we only support NT name (char*) */
    dwError = LsaInitializeLsaStringA((char*)gssName, lsaName);
    return dwError;
}

