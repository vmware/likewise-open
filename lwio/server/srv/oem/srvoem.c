/*
 * Copyright Likewise Software    2004-2009
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
 *        srvoem.c
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        OEM Utility Functions
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "includes.h"

NTSTATUS
SrvOEMCreateClientConnection(
    struct sockaddr* pClientAddress,                   /* IN     */
    SOCKLEN_T        ulClientAddressLength,            /* IN     */
    struct sockaddr* pServerAddress,                   /* IN     */
    SOCKLEN_T        ulServerAddressLength,            /* IN     */
    ULONG            ulResourceId,                     /* IN     */
    PVOID*           ppOEMConnection,                  /*    OUT */
    PULONG           pulOEMConnectionLength            /* IN OUT */
    )
{
    *ppOEMConnection = NULL;
    *pulOEMConnectionLength = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
SrvOEMCreateSession(
    PVOID                       pOEMConnection,        /* IN     */
    ULONG                       ulOEMConnectionLength, /* IN     */
    PSRV_OEM_SESSION_ID         pSessionId,            /* IN     */
    PIO_CREATE_SECURITY_CONTEXT pSecurityContext,      /* IN     */
    PVOID*                      ppOEMSession,          /*    OUT */
    PULONG                      pulOEMSessionLength    /* IN OUT */
    )
{
    *ppOEMSession = NULL;
    *pulOEMSessionLength = 0;

    return STATUS_SUCCESS;
}

VOID
SrvOEMCloseSession(
    PVOID                       pOEMSession,           /* IN OUT */
    ULONG                       ulOEMSessionLength     /* IN     */
    )
{
}

VOID
SrvOEMCloseClientConnection(
    PVOID            pOEMConnection,                   /* IN OUT */
    ULONG            ulOEMConnectionLength             /* IN     */
    )
{
}
