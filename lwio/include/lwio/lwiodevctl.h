/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwio/lwiodevctl.h
 *
 * Abstract:
 *
 *        Public Device Control codes and structures
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#ifndef __LW_IO_PUBLIC_DEVICECTL_H__
#define __LW_IO_PUBLIC_DEVICECTL_H__

/* Control Codes */

#define IO_DEVICE_CTL_OPEN_FILE_INFO   0x00000001


/* Device IoControl structures */

typedef struct _IO_OPEN_FILE_INFO_0
{
    ULONG NextEntryOffset;
    ULONG OpenHandleCount;
    ULONG FileNameLength;
    PWSTR pwszFileName[1];

} IO_OPEN_FILE_INFO_0, *PIO_OPEN_FILE_INFO_0;

typedef struct _IO_OPEN_FILE_INFO_INPUT_BUFFER
{
    DWORD Level;

} IO_OPEN_FILE_INFO_INPUT_BUFFER, *PIO_OPEN_FILE_INFO_INPUT_BUFFER;

#endif   /* __LW_IO_PUBLIC_DEVICECTL_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
