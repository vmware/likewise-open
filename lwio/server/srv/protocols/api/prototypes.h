/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocol Handler API
 *
 *        prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __PROTOTYPES_H__
#define __PROTOTYPES_H__


// config.c

NTSTATUS
SrvProtocolReadConfig(
    PSRV_PROTOCOL_CONFIG pConfig
    );

NTSTATUS
SrvProtocolInitConfig(
    PSRV_PROTOCOL_CONFIG pConfig
    );

VOID
SrvProtocolFreeConfigContents(
    PSRV_PROTOCOL_CONFIG pConfig
    );

BOOLEAN
SrvProtocolConfigIsSigningEnabled(
    VOID
    );

BOOLEAN
SrvProtocolConfigIsSigningRequired(
    VOID
    );

BOOLEAN
SrvProtocolConfigIsSmb2Enabled(
    VOID
    );

ULONG
SrvProtocolConfigGetZctReadThreshold(
    VOID
    );

ULONG
SrvProtocolConfigGetZctWriteThreshold(
    VOID
    );

ULONG
SrvProtocolConfigGetIdleTimeout(
    VOID
    );

// negotiate.c

NTSTATUS
SrvProcessNegotiate(
        IN  PLWIO_SRV_CONNECTION pConnection,
        IN  PSMB_PACKET          pSmbRequest,
        OUT PSMB_PACKET*         ppSmbResponse
        );

// transport.c

NTSTATUS
SrvProtocolTransportDriverInit(
    PSRV_PROTOCOL_API_GLOBALS pGlobals
    );

VOID
SrvProtocolTransportDriverShutdown(
    PSRV_PROTOCOL_API_GLOBALS pGlobals
    );

#endif /* __PROTOTYPES_H__ */

