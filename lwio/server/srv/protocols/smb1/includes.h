/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        includes.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include <config.h>
#include <lwiosys.h>

#include <uuid/uuid.h>

#include <openssl/rand.h>

#include <lwio/lwio.h>
#include <lwio/lwiosrvstatprovider.h>

#include <lwiodef.h>
#include <lwioutils.h>
#include <lwiolog_r.h>
#include <lwnet.h>

#include <lw/ntstatus.h>
#include <lw/winerror.h>
#include <lw/mapsecurity.h>

#include <lwio/lmshare.h>
#include <lwio/lwshareinfo.h>

#include <iodriver.h>
#include <ioapi.h>
#include <lwiofsctl.h>

#include <smbwire.h>
#include <srvecp.h>

#include <shareapi.h>
#include <srvoem.h>
#include <srvutils.h>
#include <statisticsapi.h>
#include <elementsapi.h>
#include <transportapi.h>
#include <protocolapi.h>

#include <protocolapi_p.h>
#include <smb1.h>

#include "defs.h"
#include "structs.h"
#include "prototypes.h"

#include "externs.h"




