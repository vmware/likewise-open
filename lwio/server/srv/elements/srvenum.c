/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        srvenum.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Generic Object Enumeration
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "includes.h"


//
// Forward Declarations
//

static
VOID
SrvEnumCollection(
    PLWRTL_RB_TREE pCollection,
    PSRV_OBJECT_CALLBACK_INFO pCallbackInfo,
    PVOID pContext
    );

static
NTSTATUS
SrvEnumCollectionTreeVisit(
    PVOID pKey,
    PVOID pData,
    PVOID pUserData,
    PBOOLEAN pbContinue
    );

//
// Implementations
//

VOID
SrvEnumSessionCollection(
    PLWRTL_RB_TREE pCollection,
    PFN_SRV_SESSION_ENUM_CALLBACK pfnEnumCallback,
    PVOID pContext
    )
{
    SRV_OBJECT_CALLBACK_INFO callbackInfo = { 0 };

    callbackInfo.objectType = SRV_OBJECT_TYPE_SESSION;
    callbackInfo.callback.pfnSession = pfnEnumCallback;

    SrvEnumCollection(pCollection, &callbackInfo, pContext);
}

VOID
SrvEnumSession2Collection(
    PLWRTL_RB_TREE pCollection,
    PFN_SRV_SESSION_2_ENUM_CALLBACK pfnEnumCallback,
    PVOID pContext
    )
{
    SRV_OBJECT_CALLBACK_INFO callbackInfo = { 0 };

    callbackInfo.objectType = SRV_OBJECT_TYPE_SESSION_2;
    callbackInfo.callback.pfnSession2 = pfnEnumCallback;

    SrvEnumCollection(pCollection, &callbackInfo, pContext);
}

VOID
SrvEnumTreeCollection(
    PLWRTL_RB_TREE pCollection,
    PFN_SRV_TREE_ENUM_CALLBACK pfnEnumCallback,
    PVOID pContext
    )
{
    SRV_OBJECT_CALLBACK_INFO callbackInfo = { 0 };

    callbackInfo.objectType = SRV_OBJECT_TYPE_TREE;
    callbackInfo.callback.pfnTree = pfnEnumCallback;

    SrvEnumCollection(pCollection, &callbackInfo, pContext);
}

VOID
SrvEnumTree2Collection(
    PLWRTL_RB_TREE pCollection,
    PFN_SRV_TREE_2_ENUM_CALLBACK pfnEnumCallback,
    PVOID pContext
    )
{
    SRV_OBJECT_CALLBACK_INFO callbackInfo = { 0 };

    callbackInfo.objectType = SRV_OBJECT_TYPE_TREE_2;
    callbackInfo.callback.pfnTree2 = pfnEnumCallback;

    SrvEnumCollection(pCollection, &callbackInfo, pContext);
}

VOID
SrvEnumFileCollection(
    PLWRTL_RB_TREE pCollection,
    PFN_SRV_FILE_ENUM_CALLBACK pfnEnumCallback,
    PVOID pContext
    )
{
    SRV_OBJECT_CALLBACK_INFO callbackInfo = { 0 };

    callbackInfo.objectType = SRV_OBJECT_TYPE_FILE;
    callbackInfo.callback.pfnFile = pfnEnumCallback;

    SrvEnumCollection(pCollection, &callbackInfo, pContext);
}

VOID
SrvEnumFile2Collection(
    PLWRTL_RB_TREE pCollection,
    PFN_SRV_FILE_2_ENUM_CALLBACK pfnEnumCallback,
    PVOID pContext
    )
{
    SRV_OBJECT_CALLBACK_INFO callbackInfo = { 0 };

    callbackInfo.objectType = SRV_OBJECT_TYPE_FILE_2;
    callbackInfo.callback.pfnFile2 = pfnEnumCallback;

    SrvEnumCollection(pCollection, &callbackInfo, pContext);
}

VOID
SrvEnumAsyncStateCollection(
    PLWRTL_RB_TREE pCollection,
    PFN_SRV_ASYNC_STATE_ENUM_CALLBACK pfnEnumCallback,
    PVOID pContext
    )
{
    SRV_OBJECT_CALLBACK_INFO callbackInfo = { 0 };

    callbackInfo.objectType = SRV_OBJECT_TYPE_ASYNC_STATE;
    callbackInfo.callback.pfnAsyncState = pfnEnumCallback;

    SrvEnumCollection(pCollection, &callbackInfo, pContext);
}

static
VOID
SrvEnumCollection(
    PLWRTL_RB_TREE pCollection,
    PSRV_OBJECT_CALLBACK_INFO pCallbackInfo,
    PVOID pContext
    )
{
    NTSTATUS ntStatus = 0;
    SRV_ENUM_CALLBACK_CONTEXT enumContext = { 0 };

    enumContext.pCallbackInfo = pCallbackInfo;
    enumContext.pContext = pContext;

    ntStatus = LwRtlRBTreeTraverse(
                    pCollection,
                    LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    &SrvEnumCollectionTreeVisit,
                    &enumContext);
    if (ntStatus)
    {
        LWIO_LOG_ERROR("Unexpected enumeration error while enumerating "
                       "collection of object type %d, status = 0x%08X (%s)",
                       pCallbackInfo->objectType,
                       ntStatus, LwNtStatusToName(ntStatus));
        LWIO_ASSERT_MSG(FALSE, "Unexpected error");
    }
}

static
NTSTATUS
SrvEnumCollectionTreeVisit(
    PVOID pKey,
    PVOID pData,
    PVOID pUserData,
    PBOOLEAN pbContinue
    )
{
    PSRV_ENUM_CALLBACK_CONTEXT pEnumContext = (PSRV_ENUM_CALLBACK_CONTEXT) pUserData;
    PSRV_OBJECT_CALLBACK_INFO pCallbackInfo = pEnumContext->pCallbackInfo;
    PVOID pContext = pEnumContext->pContext;
    BOOLEAN bContinue = FALSE;

    switch (pCallbackInfo->objectType)
    {
        case SRV_OBJECT_TYPE_CONNECTION:
            bContinue = pCallbackInfo->callback.pfnConnection(
                                (PLWIO_SRV_CONNECTION) pData,
                                pContext);
            break;
        case SRV_OBJECT_TYPE_SESSION:
            bContinue = pCallbackInfo->callback.pfnSession(
                                (PLWIO_SRV_SESSION) pData,
                                pContext);
            break;
        case SRV_OBJECT_TYPE_TREE:
            bContinue = pCallbackInfo->callback.pfnTree(
                                (PLWIO_SRV_TREE) pData,
                                pContext);
            break;
        case SRV_OBJECT_TYPE_FILE:
            bContinue = pCallbackInfo->callback.pfnFile(
                                (PLWIO_SRV_FILE) pData,
                                pContext);
            break;
        case SRV_OBJECT_TYPE_SESSION_2:
            bContinue = pCallbackInfo->callback.pfnSession2(
                                (PLWIO_SRV_SESSION_2) pData,
                                pContext);
            break;
        case SRV_OBJECT_TYPE_TREE_2:
            bContinue = pCallbackInfo->callback.pfnTree2(
                                (PLWIO_SRV_TREE_2) pData,
                                pContext);
            break;
        case SRV_OBJECT_TYPE_FILE_2:
            bContinue = pCallbackInfo->callback.pfnFile2(
                                (PLWIO_SRV_FILE_2) pData,
                                pContext);
            break;
        case SRV_OBJECT_TYPE_ASYNC_STATE:
            bContinue = pCallbackInfo->callback.pfnAsyncState(
                                (PLWIO_ASYNC_STATE) pData,
                                pContext);
            break;
        default:
            LWIO_LOG_ERROR("Invalid object type %d", pCallbackInfo->objectType);
            LWIO_ASSERT_MSG(FALSE, "Invalid object type");
            bContinue = FALSE;
            break;
    }

    *pbContinue = bContinue;

    return STATUS_SUCCESS;
}
