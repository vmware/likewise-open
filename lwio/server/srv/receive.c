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

#include "includes.h"

NTSTATUS
SMBSrvProcessRequest_V1(
    PLWIO_SRV_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = 0;

    switch (pContext->pRequest->pSMBHeader->command)
    {
        case COM_NEGOTIATE:

                ntStatus = SrvProcessNegotiate(pContext);

                break;

        case COM_SESSION_SETUP_ANDX:

            ntStatus = SrvProcessSessionSetup(pContext);

            break;

        case COM_LOGOFF_ANDX:

            ntStatus = SrvProcessLogoffAndX(pContext);

            break;

        case COM_TREE_CONNECT_ANDX:

            ntStatus = SrvProcessTreeConnectAndX(pContext);

            break;

        case COM_TREE_DISCONNECT:

            ntStatus = SrvProcessTreeDisconnectAndX(pContext);

            break;

#if 0
        case COM_TRANS2_QUERY_FS_INFORMATION:

            ntStatus = SmbProcessTrans2QueryFSInformation(
                            pSmbRequest,
                            pSmbResponse);

            break;
#endif

        case COM_ECHO:

            ntStatus = SrvProcessEchoAndX(pContext);

            break;

#if 0
        case SMB_NT_CANCEL:

            ntStatus = SmbNTCancel(
                            pSmbRequest,
                            pSmbResponse);

            break;
#endif

        case COM_NT_CREATE_ANDX:

            ntStatus = SrvProcessNTCreateAndX(pContext);

            break;

#if 0
        case SMB_NT_TRANSACT_CREATE:

            ntStatus = SmbNTTransactCreate(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_CREATE_TEMPORARY:

            ntStatus = SmbCreateTemporary(
                            pSmbRequest,
                            pSmbResponse);

            break;
#endif

        case COM_READ_ANDX:

            ntStatus = SrvProcessReadAndX(pContext);

            break;

        case COM_WRITE_ANDX:

            ntStatus = SrvProcessWriteAndX(pContext);

            break;

#if 0
        case SMB_LOCKING_ANDX:

            ntStatus = SmbProcessLockingAndX(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_SEEK:

            ntStatus = SmbProcessSeek(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_FLUSH:

            ntStatus = SmbProcessFlush(
                            pSmbRequest,
                            pSmbResponse);

            break;

#endif

        case COM_CLOSE:

            ntStatus = SrvProcessCloseAndX(pContext);

            break;

#if 0
        case SMB_CLOSE_AND_TREE_DISCONNECT:

            ntStatus = SmbProcessCloseAndTreeDisconnect(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_DELETE:

            ntStatus = SmbProcessDelete(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_RENAME:

            ntStatus = SmbProcessRename(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_NT_RENAME:

            ntStatus = SmbProcessNTRename(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_MOVE:

            ntStatus = SmbProcessCopy(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_COPY:

            ntStatus = SmbProcessCopy(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_TRANS2_QUERY_PATH_INFORMATION:

            ntStatus = SmbProcessTrans2QueryPathInformation(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_TRANS2_QUERY_SET_PATH_INFORMATION:

            ntStatus = SmbProcessTrans2QuerySetPathInformation(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_TRANS2_SET_FILE_INFORMATION:

            ntStatus = SmbProcessTrans2SetFileInformation(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case  SMB_TRANS2_CREATE_DIRECTORY:

            ntStatus = SmbProcessTrans2CreateDirectory(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_DELETE_DIRECTORY:

            ntStatus = SmbProcessDeleteDirectory(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_CHECK_DIRECTORY:

            ntStatus = SmbProcessCheckDirectory(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_TRANS2_FIND_FIRST2:

            ntStatus = SmbProcessTrans2FindFirst2(
                            pSmbRequest,
                            pSmbResponse);
            break;
#endif

    }

    return ntStatus;
}
