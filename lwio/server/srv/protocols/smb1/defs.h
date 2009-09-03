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
 *        defs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Defines
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __DEFS_H__
#define __DEFS_H__

#define LWIO_SRV_DEFAULT_TIMEOUT_MSECS (30 * 60 * 1000)

#define COM_CREATE_DIRECTORY_DESC       "SMB1_CREATE_DIRECTORY"
#define COM_DELETE_DIRECTORY_DESC       "SMB1_DELETE_DIRECTORY"
#define COM_OPEN_DESC                   "SMB1_OPEN"
#define COM_CREATE_DESC                 "SMB1_CREATE"
#define COM_CLOSE_DESC                  "SMB1_CLOSE"
#define COM_FLUSH_DESC                  "SMB1_FLUSH"
#define COM_DELETE_DESC                 "SMB1_DELETE"
#define COM_RENAME_DESC                 "SMB1_RENAME"
#define COM_QUERY_INFORMATION_DESC      "SMB1_QUERY_INFORMATION"
#define COM_SET_INFORMATION_DESC        "SMB1_SET_INFORMATION"
#define COM_READ_DESC                   "SMB1_READ"
#define COM_WRITE_DESC                  "SMB1_WRITE"
#define COM_LOCK_BYTE_RANGE_DESC        "SMB1_LOCK_BYTE_RANGE"
#define COM_UNLOCK_BYTE_RANGE_DESC      "SMB1_UNLOCK_BYTE_RANGE"
#define COM_CREATE_TEMPORARY_DESC       "SMB1_CREATE_TEMPORARY"
#define COM_CREATE_NEW_DESC             "SMB1_CREATE_NEW"
#define COM_CHECK_DIRECTORY_DESC        "SMB1_CHECK_DIRECTORY"
#define COM_PROCESS_EXIT_DESC           "SMB1_PROCESS_EXIT"
#define COM_SEEK_DESC                   "SMB1_SEEK"
#define COM_LOCK_AND_READ_DESC          "SMB1_LOCK_AND_READ"
#define COM_WRITE_AND_UNLOCK_DESC       "SMB1_WRITE_AND_UNLOCK"
#define COM_READ_RAW_DESC               "SMB1_READ_RAW"
#define COM_READ_MPX_DESC               "SMB1_READ_MPX"
#define COM_READ_MPX_SECONDARY_DESC     "SMB1_READ_MPX_SECONDARY"
#define COM_WRITE_RAW_DESC              "SMB1_WRITE_RAW"
#define COM_WRITE_MPX_DESC              "SMB1_WRITE_MPX"
#define COM_WRITE_MPX_SECONDARY_DESC    "SMB1_WRITE_MPX_SECONDARY"
#define COM_WRITE_COMPLETE_DESC         "SMB1_WRITE_COMPLETE"
#define COM_QUERY_SERVER_DESC           "SMB1_QUERY_SERVER"
#define COM_SET_INFORMATION2_DESC       "SMB1_SET_INFORMATION2"
#define COM_QUERY_INFORMATION2_DESC     "SMB1_QUERY_INFORMATION2"
#define COM_LOCKING_ANDX_DESC           "SMB1_LOCKING_ANDX"
#define COM_TRANSACTION_DESC            "SMB1_TRANSACTION"
#define COM_TRANSACTION_SECONDARY_DESC  "SMB1_TRANSACTION_SECONDARY"
#define COM_IOCTL_DESC                  "SMB1_IOCTL"
#define COM_IOCTL_SECONDARY_DESC        "SMB1_IOCTL_SECONDARY"
#define COM_COPY_DESC                   "SMB1_COPY"
#define COM_MOVE_DESC                   "SMB1_MOVE"
#define COM_ECHO_DESC                   "SMB1_ECHO"
#define COM_WRITE_AND_CLOSE_DESC        "SMB1_WRITE_AND_CLOSE"
#define COM_OPEN_ANDX_DESC              "SMB1_OPEN_ANDX"
#define COM_READ_ANDX_DESC              "SMB1_READ_ANDX"
#define COM_WRITE_ANDX_DESC             "SMB1_WRITE_ANDX"
#define COM_NEW_FILE_SIZE_DESC          "SMB1_NEW_FILE_SIZE"
#define COM_CLOSE_AND_TREE_DISC_DESC    "SMB1_CLOSE_AND_TREE_DISCONNECT"
#define COM_TRANSACTION2_DESC           "SMB1_TRANSACTION_2"
#define COM_TRANSACTION2_SECONDARY_DESC "SMB1_TRANSACTION_2_SECONDARY"
#define COM_FIND_CLOSE2_DESC            "SMB1_FIND_CLOSE_2"
#define COM_FIND_NOTIFY_CLOSE_DESC      "SMB1_FIND_NOTIFY_CLOSE"
#define COM_TREE_CONNECT_DESC           "SMB1_TREE_CONNECT"
#define COM_TREE_DISCONNECT_DESC        "SMB1_TREE_DISCONNECT"
#define COM_NEGOTIATE_DESC              "SMB1_NEGOTIATE"
#define COM_SESSION_SETUP_ANDX_DESC     "SMB1_SESSION_SETUP"
#define COM_LOGOFF_ANDX_DESC            "SMB1_LOGOFF_ANDX"
#define COM_TREE_CONNECT_ANDX_DESC      "SMB1_TREE_CONNECT_ANDX"
#define COM_QUERY_INFORMATION_DISK_DESC "SMB1_QUERY_INFORMATION_DISK"
#define COM_SEARCH_DESC                 "SMB1_SEARCH"
#define COM_FIND_DESC                   "SMB1_FIND"
#define COM_FIND_UNIQUE_DESC            "SMB1_FIND_UNIQUE"
#define COM_FIND_CLOSE_DESC             "SMB1_FIND_CLOSE"
#define COM_NT_TRANSACT_DESC            "SMB1_NT_TRANSACT"
#define COM_NT_TRANSACT_SECONDARY_DESC  "SMB1_NT_TRANSACT_SECONDARY"
#define COM_NT_CREATE_ANDX_DESC         "SMB1_NT_CREATE_ANDX"
#define COM_NT_CANCEL_DESC              "SMB1_NT_CANCEL"
#define COM_NT_RENAME_DESC              "SMB1_NT_RENAME"
#define COM_OPEN_PRINT_FILE_DESC        "SMB1_OPEN_PRINT_FILE"
#define COM_WRITE_PRINT_FILE_DESC       "SMB1_WRITE_PRINT_FILE"
#define COM_CLOSE_PRINT_FILE_DESC       "SMB1_CLOSE_PRINT_FILE"
#define COM_GET_PRINT_QUEUE_DESC        "SMB1_GET_PRINT_QUEUE"
#define COM_READ_BULK_DESC              "SMB1_READ_BULK"
#define COM_WRITE_BULK_DESC             "SMB1_WRITE_BULK"
#define COM_WRITE_BULK_DATA_DESC        "SMB1_WRITE_BULK_DATA"

typedef UCHAR SMB_OPLOCK_REQUEST;

#define SMB_OPLOCK_REQUEST_EXCLUSIVE 0x02
#define SMB_OPLOCK_REQUEST_BATCH     0x04

#endif /* __DEFS_H__ */
