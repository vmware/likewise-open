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
 *        Likewise IO (LWIO)
 *
 *        Listener Definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __DEFS_H__
#define __DEFS_H__

#define SMB_SERVER_PORT      445
#define SMB_LISTEN_Q         5

#define LWIO_SRV_DB_DIR CACHEDIR     "/db"
#define LWIO_SRV_SHARE_DB LWIO_SRV_DB_DIR "/lwio-shares.db"

#define LWIO_SRV_DEFAULT_NUM_READERS          1
#define LWIO_SRV_DEFAULT_NUM_WORKERS          1
#define LWIO_SRV_DEFAULT_NUM_MAX_QUEUE_ITEMS 20
#define LWIO_SRV_DEFAULT_NUM_MAX_PACKETS     10

#define LWIO_SRV_SHARE_STRING_ID_ANY     "????"
#define LWIO_SRV_SHARE_STRING_ID_IPC     "IPC"
#define LWIO_SRV_SHARE_STRING_ID_COMM    "COMM"
#define LWIO_SRV_SHARE_STRING_ID_PRINTER "LPT1:"
#define LWIO_SRV_SHARE_STRING_ID_DISK    "A:"

typedef enum
{
    SMB_SRV_CONN_STATE_INITIAL = 0,
    SMB_SRV_CONN_STATE_READY,
    SMB_SRV_CONN_STATE_CLOSED,
    SMB_SRV_CONN_STATE_INVALID
} SMB_SRV_CONN_STATE;

#define SRV_DEVCTL_ADD_SHARE      1
#define SRV_DEVCTL_DELETE_SHARE   2
#define SRV_DEVCTL_ENUM_SHARE     3
#define SRV_DEVCTL_SET_SHARE_INFO 4
#define SRV_DEVCTL_GET_SHARE_INFO 5

typedef USHORT SMB_SEARCH_FLAG;

#define SMB_FIND_CLOSE_AFTER_REQUEST 0x1
#define SMB_FIND_CLOSE_IF_EOS        0x2
#define SMB_FIND_RETURN_RESUME_KEYS  0x4
#define SMB_FIND_CONTINUE_SEARCH     0x8
#define SMB_FIND_WITH_BACKUP_INTENT  0x10

#endif /* __DEFS_H__ */
