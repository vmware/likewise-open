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
 *        pam-lsass.h
 *
 * Abstract:
 * 
 *        Pluggable Authentication Module (Likewise LSASS)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __PAM_LSASS_H__
#define __PAM_LSASS_H__

#define MODULE_NAME_SPECIFIC "lsass"
#define MODULE_NAME "pam_" MODULE_NAME_SPECIFIC

#define MOTD_FILE __LW_MOTD_FILE__
#define MOTD_MAX_SIZE __LW_MOTD_MAX_SIZE__

/* Custom data keys */
#define PAM_LSASS_OLDAUTHTOK "PAM_LSASS_OLDAUTHTOK"

/*
 * AIX declares functions like pam_get_data to take void* pointer
 * as the last argument, while Linux and Mac OS takes const void*.
 * This causes build failures when compiling with -Werror enabled.
 * Use macro just like any other cast e.g.: (PAM_GET_ITEM_TYPE)
 */
#include <config.h>
#if defined(__LWI_AIX__) || defined(__LWI_HP_UX__)
#define PAM_GET_ITEM_TYPE             void**
#define PPCHAR_ARG_CAST               char**
#define PAM_MESSAGE_MSG_TYPE          char*
#define PAM_CONV_2ND_ARG_TYPE         struct pam_message**
#elif defined(__LWI_SOLARIS__)
#define PAM_GET_ITEM_TYPE             const void*
#define PPCHAR_ARG_CAST               char**
#define PAM_MESSAGE_MSG_TYPE          char*
#define PAM_CONV_2ND_ARG_TYPE         struct pam_message**
#else
#define PAM_GET_ITEM_TYPE             const void**
#define PPCHAR_ARG_CAST               const char**
#define PAM_MESSAGE_MSG_TYPE          const char*
#define PAM_CONV_2ND_ARG_TYPE         const struct pam_message**
#endif

#include "lsasystem.h"
#include "lsa/lsa.h"
#include "lsadef.h"
#include "lsautils.h"
#include "lsaclient.h"

#include "pam-system.h"

#include "pam-logging.h"
#include "pam-context.h"
#include "pam-error.h"
#include "pam-externs.h"
#include "pam-passwd.h"
#include "pam-session.h"
#include "pam-conv.h"
#include "pam-auth.h"
#include "pam-config.h"

#endif /* __PAM_LSASS_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
