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

DWORD
SMBSrvProcessRequest_V1(
    PSMB_PACKET pSmbRequest,
    PSMB_PACKET pSmbResponse
    )
{
    DWORD dwError = 0;

#if 0
    switch (pRequest->pSMBHeader->command)
    {
        case COM_NEGOTIATE:

            dwError = SmbMkDirRequest(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case COM_SESSION_SETUP_ANDX:

            dwError = SmbProcessSessionSetup(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case COM_LOGOFF_ANDX:

            dwError = SmbProcessLogoffAndX(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case COM_TREE_CONNECT_ANDX:

            dwError = SmbProcessTreeConnectAndX(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case COM_TREE_DISCONNECT:

            dwError = SmbTreeDisconnect(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case COM_TRANS2_QUERY_FS_INFORMATION:

            dwError = SmbProcessTrans2QueryFSInformation(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_ECHO:

            dwError = SmbProcessEcho(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_NT_CANCEL:

            dwError = SmbNTCancel(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_NT_CREATE_ANDX:

            dwError = SmbNTCreateAndX(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_NT_TRANSACT_CREATE:

            dwError = SmbNTTransactCreate(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_CREATE_TEMPORARY:

            dwError = SmbCreateTemporary(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_READ_ANDX:

            dwError = SmbProcessReadAndX(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_WRITE_ANDX:

            dwError = SmbProcessWriteAndX(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_LOCKING_ANDX:

            dwError = SmbProcessLockingAndX(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_SEEK:

            dwError = SmbProcessSeek(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_FLUSH:

            dwError = SmbProcessFlush(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_CLOSE:

            dwError = SmbClose(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_CLOSE_AND_TREE_DISCONNECT:

            dwError = SmbProcessCloseAndTreeDisconnect(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_DELETE:

            dwError = SmbProcessDelete(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_RENAME:

            dwError = SmbProcessRename(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_NT_RENAME:

            dwError = SmbProcessNTRename(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_MOVE:

            dwError = SmbProcessCopy(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_COPY:

            dwError = SmbProcessCopy(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_TRANS2_QUERY_PATH_INFORMATION:

            dwError = SmbProcessTrans2QueryPathInformation(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_TRANS2_QUERY_SET_PATH_INFORMATION:

            dwError = SmbProcessTrans2QuerySetPathInformation(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_TRANS2_SET_FILE_INFORMATION:

            dwError = SmbProcessTrans2SetFileInformation(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case  SMB_TRANS2_CREATE_DIRECTORY:

            dwError = SmbProcessTrans2CreateDirectory(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_DELETE_DIRECTORY:

            dwError = SmbProcessDeleteDirectory(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_CHECK_DIRECTORY:

            dwError = SmbProcessCheckDirectory(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_TRANS2_FIND_FIRST2:

            dwError = SmbProcessTrans2FindFirst2(
                            pSmbRequest,
                            pSmbResponse);
            break;
    }
#endif

    return dwError;
}
