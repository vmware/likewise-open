/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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
 * license@likewisesoftware.com
 */

/*
 * Abstract: RPC client library
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _TYPES_H_
#define _TYPES_H_

#if defined(_DCE_IDL_)

/* Types needed for idl compiler pass */
typedef unsigned small int uint8;
typedef unsigned short int uint16;
typedef unsigned long int uint32;
typedef unsigned hyper int uint64;
typedef small int int8;
typedef short int int16;
typedef long int int32;
typedef hyper int int64;
typedef uint32 NTSTATUS;
typedef uint16 wchar16_t;

#else

/* Types needed for librpc build pass */
#include <inttypes.h>
#include <lw/ntstatus.h>

#ifndef UINT8_DEFINED
typedef uint8_t uint8;
#define UINT8_DEFINED
#endif

#ifndef UINT16_DEFINED
typedef uint16_t uint16;
#define UINT16_DEFINED
#endif

#ifndef UINT32_DEFINED
typedef uint32_t uint32;
#define UINT32_DEFINED
#endif

#ifndef UINT64_DEFINED
typedef uint64_t uint64;
#define UINT64_DEFINED
#endif

#ifndef INT8_DEFINED
typedef int8_t int8;
#define INT8_DEFINED
#endif

#ifndef INT16_DEFINED
typedef int16_t int16;
#define INT16_DEFINED
#endif

#ifndef INT32_DEFINED
typedef int32_t int32;
#define INT32_DEFINED
#endif

#ifndef INT64_DEFINED
#define INT64_DEFINED
typedef int64_t int64;
#endif

#endif /* _DCE_IDL_ */

typedef uint32 RPCSTATUS;
typedef uint32 WINERR;
typedef uint64 NtTime;

/* Don't require DCE/RPC environment when simply building
   a client using rpc library */
#if !defined(_DCE_IDL_)
#if defined(SRVSVC_BUILD)
#include <dce/rpc.h>
#else
typedef void* handle_t;
typedef unsigned long error_status_t;
#endif /* defined(SRVSVC_BUILD) */
#endif /* !defined(_DCE_IDL_) */

#endif /* _TYPES_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
