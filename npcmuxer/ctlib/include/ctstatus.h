/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#ifndef _CT_STATUS_H_
#define _CT_STATUS_H_

#include <ctspec.h>
#include <inttypes.h>
#include <errno.h>

#define CT_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CT_MAX(a, b) (((a) > (b)) ? (a) : (b))

/*
 * Status code definitions.
 *
 * Status format:
 * - 12 bits of component value for status
 * - 20 bits of code value for status
 */

typedef uint32_t CT_STATUS;

#define CT_STATUS_IS_OK(status) ((status) == CT_STATUS_SUCCESS)

#define _CT_STATUS_COMPONENT_MASK 0xFFF00000
#define _CT_STATUS_COMPONENT_SHIFT 20

#define CT_STATUS_COMPONENT(status) \
    (((status) & _CT_STATUS_COMPONENT_MASK) >> _CT_STATUS_COMPONENT_SHIFT)
#define CT_STATUS_CODE(status) \
        ((status) & ~_CT_STATUS_COMPONENT_MASK)

#define _CT_STATUS(component, code) \
    (((component) << _CT_STATUS_COMPONENT_SHIFT) | code)

#define CT_STATUS_COMPONENT_SYSTEM 0x000
#define CT_STATUS_COMPONENT_ERRNO  0xFFF

#define _CT_STATUS_SYSTEM(code) \
    _CT_STATUS(CT_STATUS_COMPONENT_SYSTEM, code)
#define _CT_STATUS_ERRNO(code) \
    _CT_STATUS(CT_STATUS_COMPONENT_ERRNO, code)

#define CT_STATUS_SUCCESS                       0x00000000

#define CT_STATUS_ERRNO_UNMAPPED                _CT_STATUS_ERRNO(0x000FFFFE)
#define CT_STATUS_ERRNO_UNEXPECTED              _CT_STATUS_ERRNO(0x000FFFFF)

#define CT_STATUS_ACCESS_DENIED                 _CT_STATUS_ERRNO(EACCES)
#define CT_STATUS_OUT_OF_MEMORY                 _CT_STATUS_ERRNO(ENOMEM)
#define CT_STATUS_INVALID_PARAMETER             _CT_STATUS_ERRNO(EINVAL)
#define CT_STATUS_NOT_FOUND                     _CT_STATUS_ERRNO(ENOENT)
#define CT_STATUS_NO_SUCH_PROCESS               _CT_STATUS_ERRNO(ESRCH)
#define CT_STATUS_TIMEOUT                       _CT_STATUS_ERRNO(ETIMEDOUT)


CT_STATUS
CtErrnoToStatus(
    IN int Errno
    );

int
CtStatusToErrno(
    IN CT_STATUS Status
    );

#define CT_ERRNO_TO_STATUS(Errno) \
    ((Errno) ? CtErrnoToStatus(Errno) : CT_STATUS_ERRNO_UNEXPECTED)

#endif /* _CT_STATUS_H_ */
