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
 *        Elements API
 *
 *        prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

// config.c

NTSTATUS
SrvElementsConfigSetupInitial(
    VOID
    );

NTSTATUS
SrvElementsConfigRefresh(
    VOID
    );

ULONG
SrvElementsConfigGetGlobalCreditLimit(
    VOID
    );

USHORT
SrvElementsConfigGetClientCreditLimit(
    VOID
    );

VOID
SrvElementsConfigShutdown(
    VOID
    );

// creditor.c

NTSTATUS
SrvCreditorCreate(
    PSRV_CREDITOR* ppCreditor
    );

VOID
SrvCreditorFree(
    PSRV_CREDITOR pCreditor
    );

// resources.c

NTSTATUS
SrvElementsResourcesInit(
    VOID
    );

VOID
SrvElementsResourcesShutdown(
    VOID
    );

// srvenum.c

VOID
SrvEnumSessionCollection(
    PLWRTL_RB_TREE pCollection,
    PFN_SRV_SESSION_ENUM_CALLBACK pfnEnumCallback,
    PVOID pContext
    );

VOID
SrvEnumTreeCollection(
    PLWRTL_RB_TREE pCollection,
    PFN_SRV_TREE_ENUM_CALLBACK pfnEnumCallback,
    PVOID pContext
    );

VOID
SrvEnumFileCollection(
    PLWRTL_RB_TREE pCollection,
    PFN_SRV_FILE_ENUM_CALLBACK pfnEnumCallback,
    PVOID pContext
    );

VOID
SrvEnumSession2Collection(
    PLWRTL_RB_TREE pCollection,
    PFN_SRV_SESSION_2_ENUM_CALLBACK pfnEnumCallback,
    PVOID pContext
    );

VOID
SrvEnumTree2Collection(
    PLWRTL_RB_TREE pCollection,
    PFN_SRV_TREE_2_ENUM_CALLBACK pfnEnumCallback,
    PVOID pContext
    );

VOID
SrvEnumFile2Collection(
    PLWRTL_RB_TREE pCollection,
    PFN_SRV_FILE_2_ENUM_CALLBACK pfnEnumCallback,
    PVOID pContext
    );

VOID
SrvEnumAsyncStateCollection(
    PLWRTL_RB_TREE pCollection,
    PFN_SRV_ASYNC_STATE_ENUM_CALLBACK pfnEnumCallback,
    PVOID pContext
    );

// srvfile.c

BOOLEAN
SrvFileIsInParent(
    PLWIO_SRV_FILE pFile
    );

VOID
SrvFileSetInParent(
    PLWIO_SRV_FILE pFile
    );

VOID
SrvFileClearInParent(
    PLWIO_SRV_FILE pFile
    );

// srvfile2.c

BOOLEAN
SrvFile2IsInParent(
    PLWIO_SRV_FILE_2 pFile
    );

VOID
SrvFile2SetInParent(
    PLWIO_SRV_FILE_2 pFile
    );

VOID
SrvFile2ClearInParent(
    PLWIO_SRV_FILE_2 pFile
    );


// srvsession.c

BOOLEAN
SrvSessionIsInParent(
    PLWIO_SRV_SESSION pSession
    );

VOID
SrvSessionSetInParent(
    PLWIO_SRV_SESSION pSession
    );

VOID
SrvSessionClearInParent(
    PLWIO_SRV_SESSION pSession
    );

// srvsession2.c

BOOLEAN
SrvSession2IsInParent(
    PLWIO_SRV_SESSION_2 pSession
    );

VOID
SrvSession2SetInParent(
    PLWIO_SRV_SESSION_2 pSession
    );

VOID
SrvSession2ClearInParent(
    PLWIO_SRV_SESSION_2 pSession
    );

// srvtimer.c

NTSTATUS
SrvTimerInit(
    IN PSRV_TIMER pTimer
    );

NTSTATUS
SrvTimerPostRequestSpecific(
    IN  PSRV_TIMER             pTimer,
    IN  LONG64                 llExpiry,
    IN  PVOID                  pUserData,
    IN  PFN_SRV_TIMER_CALLBACK pfnTimerExpiredCB,
    OUT PSRV_TIMER_REQUEST*    ppTimerRequest
    );

NTSTATUS
SrvTimerCancelRequestSpecific(
    IN  PSRV_TIMER         pTimer,
    IN  PSRV_TIMER_REQUEST pTimerRequest,
    OUT PVOID*             ppUserData
    );

NTSTATUS
SrvTimerIndicateStop(
    PSRV_TIMER pTimer
    );

VOID
SrvTimerFreeContents(
    PSRV_TIMER pTimer
    );

// srvtree.c

BOOLEAN
SrvTreeIsInParent(
    PLWIO_SRV_TREE pTree
    );

VOID
SrvTreeSetInParent(
    PLWIO_SRV_TREE pTree
    );

VOID
SrvTreeClearInParent(
    PLWIO_SRV_TREE pTree
    );

// srvtree2.c

BOOLEAN
SrvTree2IsInParent(
    PLWIO_SRV_TREE_2 pTree
    );

VOID
SrvTree2SetInParent(
    PLWIO_SRV_TREE_2 pTree
    );

VOID
SrvTree2ClearInParent(
    PLWIO_SRV_TREE_2 pTree
    );


