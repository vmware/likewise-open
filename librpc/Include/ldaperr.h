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

#ifndef _LDAPERR_H_
#define _LDAPERR_H_

#include <ldap.h>
#include <lwrpc/types.h>
#include <lwrpc/winerror.h>


struct lderr_winerr {
    int lderr;
    int winerr;
    const char *lderrstr;
};

#define LDERR(lderr, winerr) { lderr, winerr, #lderr }

static const struct lderr_winerr ldaperr_winerr_map[] = {
    LDERR(LDAP_SUCCESS, ERROR_SUCCESS),
    LDERR(LDAP_PARAM_ERROR, ERROR_INVALID_PARAMETER),
    LDERR(LDAP_NO_MEMORY, ERROR_OUTOFMEMORY),
    LDERR(LDAP_NOT_SUPPORTED, ERROR_INVALID_FUNCTION),
    LDERR(LDAP_UNWILLING_TO_PERFORM, ERROR_DS_UNWILLING_TO_PERFORM),
    LDERR(LDAP_REFERRAL, ERROR_DS_REFERRAL),
    
    /* termination  */
    { 0, 0, NULL }
};

#undef LDERR

#endif /* _LDAPERR_H_ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
