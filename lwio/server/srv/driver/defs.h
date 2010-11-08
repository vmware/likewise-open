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

#define LWIO_SRV_WORKERS_CPU_RATIO             4

/* To enable packet allocator, set the LWIO_SRV_DEFAULT_NUM_MAX_PACKETS to
 * a value greater than zero.
 */
// #define LWIO_SRV_DEFAULT_NUM_MAX_PACKETS       128
#define LWIO_SRV_DEFAULT_NUM_MAX_PACKETS       0

#define LWIO_SRV_DEFAULT_NUM_MAX_QUEUE_ITEMS  4096
#define LWIO_SRV_DEFAULT_MONITOR_INTERVAL_MINS   0
#define LWIO_SRV_DEFAULT_MONITOR_INTERVAL_MINS_MIN 0
#define LWIO_SRV_DEFAULT_MONITOR_INTERVAL_MINS_MAX (24 * 60)

typedef ULONG CCB_TYPE;

#define SRV_CCB_DEVICE 1

#define SRV_CONFIG_TAG_DRIVER  "driver:"
#define SRV_CONFIG_DRIVER_NAME "srv"

#define SRV_CONFIG_FILE_PATH   CONFIGDIR "/lwiod.conf"

#endif /* __DEFS_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
