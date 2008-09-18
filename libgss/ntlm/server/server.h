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
 *        server.h
 *
 * Abstract:
 *
 *       common includes for gssntlm server
 *
 * Author: Todd Stecher (v-todds@likewisesoftware.com)
 *
 */
#ifndef _SERVER_H_
#define _SERVER_H_

#include "../include/includes.h"

/* some forward defines to make life easy */
#define NTLM_CHALLENGE_LENGTH           8 
#define NTLM_V1_RESPONSE_LENGTH         24

struct _NTLM_CONTEXT;

#include "messages.h"
#include "authapi.h"
#include "credential.h"
#include "context.h"
#include "processmsg.h"
#include "ntlmsrv.h"

/*
 * Main initializaton routines 
 */

#define NTLM_MODE_STANDALONE    1
#define NTLM_MODE_LSASSD        2


DWORD
TeardownGSSNTLMServer(DWORD dwMode);


DWORD
InitializeGSSNTLMServer(DWORD dwMode);



#endif /* _SERVER_H_ */
