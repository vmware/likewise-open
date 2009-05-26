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
 *        srvdefs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Definitions
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __SRV_DEFS_H__
#define __SRV_DEFS_H__

#define LWIO_SRV_FILE_SYSTEM_PREFIX_A "C:\\"
#define LWIO_SRV_FILE_SYSTEM_PREFIX_W { 'C', ':', '\\', 0 }

#define LWIO_SRV_DEFAULT_SHARE_PATH_A "\\lwtest"
#define LWIO_SRV_DEFAULT_SHARE_PATH_W { '\\', 'l', 'w', 't', 'e', 's', 't', 0 }

#define LWIO_SRV_FILE_SYSTEM_ROOT_A   "\\pvfs"
#define LWIO_SRV_FILE_SYSTEM_ROOT_W   { '\\', 'p', 'v', 'f', 's', 0 }

#define LWIO_SRV_PIPE_SYSTEM_ROOT_A   "\\npvfs"
#define LWIO_SRV_PIPE_SYSTEM_ROOT_W   { '\\', 'n', 'p', 'v', 'f', 's', 0 }

#define SRV_SAFE_FREE_MEMORY(pMemory) \
	if (pMemory) { SrvFreeMemory(pMemory); }

#define SRV_SAFE_FREE_MEMORY_AND_RESET(pMemory) \
	if (pMemory) { SrvFreeMemory(pMemory); (pMemory) = NULL; }


#endif /* __SRV_DEFS_H__ */
