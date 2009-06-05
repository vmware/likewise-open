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
 *        srvtransport.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Transport API
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __SRV_TRANSPORT_H__
#define __SRV_TRANSPORT_H__

typedef NTSTATUS (*PFN_SRV_TRANSPORT_GET_REQUEST)(
                        OUT PLWIO_SRV_CONNECTION* ppConnection,
                        OUT PSMB_PACKET*          ppRequest
                        );

typedef NTSTATUS (*PFN_SRV_TRANSPORT_SEND_RESPONSE)(
                        IN           PLWIO_SRV_CONNECTION pConnection,
                        IN  OPTIONAL PSMB_PACKET          pRequest,
                        IN           PSMB_PACKET          pResponse
                        );

typedef struct _SRV_TRANSPORT_FUNCTION_TABLE
{

    PFN_SRV_TRANSPORT_GET_REQUEST   pfnTransportGetRequest;
    PFN_SRV_TRANSPORT_SEND_RESPONSE pfnTransportSendResponse;

} SRV_TRANSPORT_FUNCTION_TABLE, *PSRV_TRANSPORT_FUNCTION_TABLE;

typedef NTSTATUS (*PFN_SRV_TRANSPORT_INITIALIZE)(
                    OUT PSRV_TRANSPORT_FUNCTION_TABLE* ppFnTable
                    );

typedef NTSTATUS (*PFN_SRV_TRANSPORT_SHUTDOWN)(
                    IN PSRV_TRANSPORT_FUNCTION_TABLE pFnTable
                    );

#endif /* __SRV_TRANSPORT_H__ */
