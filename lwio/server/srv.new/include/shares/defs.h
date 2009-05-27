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
 *        shares/defs.h
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        Share API
 *
 *        definitions
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __SRV_SHARES_DEFS_H__
#define __SRV_SHARES_DEFS_H__

#define LWIO_SRV_FILE_SYSTEM_PREFIX_A "C:\\"
#define LWIO_SRV_FILE_SYSTEM_PREFIX_W { 'C', ':', '\\', 0 }

#define LWIO_SRV_DEFAULT_SHARE_PATH_A "\\lwtest"
#define LWIO_SRV_DEFAULT_SHARE_PATH_W { '\\', 'l', 'w', 't', 'e', 's', 't', 0 }

#define LWIO_SRV_FILE_SYSTEM_ROOT_A   "\\pvfs"
#define LWIO_SRV_FILE_SYSTEM_ROOT_W   { '\\', 'p', 'v', 'f', 's', 0 }

#define LWIO_SRV_PIPE_SYSTEM_ROOT_A   "\\npvfs"
#define LWIO_SRV_PIPE_SYSTEM_ROOT_W   { '\\', 'n', 'p', 'v', 'f', 's', 0 }

#define LWIO_SRV_SHARE_STRING_ID_ANY_A "????"
#define LWIO_SRV_SHARE_STRING_ID_ANY_W {'?','?','?','?',0}

#define LWIO_SRV_SHARE_STRING_ID_IPC_A "IPC"
#define LWIO_SRV_SHARE_STRING_ID_IPC_W {'I','P','C',0}

#define LWIO_SRV_SHARE_STRING_ID_COMM_A "COMM"
#define LWIO_SRV_SHARE_STRING_ID_COMM_W {'C','O','M','M',0}

#define LWIO_SRV_SHARE_STRING_ID_PRINTER_A "LPT1:"
#define LWIO_SRV_SHARE_STRING_ID_PRINTER_W {'L','P','T','1',':',0}

#define LWIO_SRV_SHARE_STRING_ID_DISK_A "A:"
#define LWIO_SRV_SHARE_STRING_ID_DISK_W {'A',':',0}

typedef enum
{
    SHARE_SERVICE_DISK_SHARE = 0,
    SHARE_SERVICE_PRINTER,
    SHARE_SERVICE_COMM_DEVICE,
    SHARE_SERVICE_NAMED_PIPE,
    SHARE_SERVICE_ANY,
    SHARE_SERVICE_UNKNOWN

} SHARE_SERVICE;

#endif /* __SRV_SHARES_DEFS_H__ */
