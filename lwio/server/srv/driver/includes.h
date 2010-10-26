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
 *        Utilities
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include <config.h>
#include <lwiosys.h>

#include <uuid/uuid.h>

#include <lwio/lwio.h>
#include <lwio/lwiosrvstatprovider.h>

#include <reg/lwntreg.h>

#include <lwiodef.h>
#include <lwioutils.h>
#include <lwiolog_r.h>
#include <lwnet.h>

#include <lw/ntstatus.h>

#include <lwio/lmshare.h>
#include <lwio/lwshareinfo.h>
#include <lwio/lmsession.h>
#include <lwio/lwsessioninfo.h>
#include <lwio/lmfile.h>
#include <lwio/lwfileinfo.h>
#include <lwio/lmconnection.h>
#include <lwio/lwconnectioninfo.h>
#include <lwio/lwiodevctl.h>

#include <lwio/iodriver.h>
#include <lwio/ioapi.h>

#include <smbwire.h>

#include <shareapi.h>
#include <srvoem.h>
#include <srvutils.h>
#include <statisticsapi.h>
#include <elementsapi.h>
#include <protocolapi.h>
#include <transportapi.h>

#include "defs.h"
#include "structs.h"
#include "prototypes.h"

#include "externs.h"
