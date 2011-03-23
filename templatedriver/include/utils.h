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
 *        utils.h
 *
 * Abstract:
 *
 *        Likewise I/O (LWIO) - templatedriver
 *
 *        Utilities
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

#ifndef __UTILS_H__
#define __UTILS_H__


// Logging
#define _TEMPLATEDRIVER_LOG_AT(Level, ...)    LW_RTL_LOG_AT_LEVEL(Level, "lwio", __VA_ARGS__)
#define TEMPLATEDRIVER_LOG_ALWAYS(...)        _TEMPLATEDRIVER_LOG_AT(LW_RTL_LOG_LEVEL_ALWAYS, __VA_ARGS__)
#define TEMPLATEDRIVER_LOG_ERROR(...)         _TEMPLATEDRIVER_LOG_AT(LW_RTL_LOG_LEVEL_ERROR, __VA_ARGS__)
#define TEMPLATEDRIVER_LOG_WARNING(...)       _TEMPLATEDRIVER_LOG_AT(LW_RTL_LOG_LEVEL_WARNING, __VA_ARGS__)
#define TEMPLATEDRIVER_LOG_INFO(...)          _TEMPLATEDRIVER_LOG_AT(LW_RTL_LOG_LEVEL_INFO, __VA_ARGS__)
#define TEMPLATEDRIVER_LOG_VERBOSE(...)       _TEMPLATEDRIVER_LOG_AT(LW_RTL_LOG_LEVEL_VERBOSE, __VA_ARGS__)
#define TEMPLATEDRIVER_LOG_DEBUG(...)         _TEMPLATEDRIVER_LOG_AT(LW_RTL_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define TEMPLATEDRIVER_LOG_TRACE(...)         _TEMPLATEDRIVER_LOG_AT(LW_RTL_LOG_LEVEL_TRACE, __VA_ARGS__)


#define BAIL_ON_NT_STATUS(ntStatus)                \
    if ((ntStatus)) {                              \
       TEMPLATEDRIVER_LOG_DEBUG("Error at %s:%d [status: %s = 0x%08X (%d)]", \
                     __FILE__,                     \
                     __LINE__,                     \
                     LwNtStatusToName(ntStatus), \
                     ntStatus, ntStatus);          \
       goto error;                                 \
    }

 
#endif  // __UTILS_H__

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

