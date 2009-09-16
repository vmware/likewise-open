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

#ifndef __WINERR_CONV_H__
#define __WINERR_CONV_H__

#include <lw/base.h>


DWORD
LwNtStatusToWin32Error(
    NTSTATUS ntStatus
    );


int
LwNtStatusToErrno(
    NTSTATUS ntStatus
    );


NTSTATUS
LwErrnoToNtStatus(
    int uerror
    );


DWORD
LwErrnoToWin32Error(
    int uerror
    );


int
LwWin32ErrorToErrno(
    DWORD winerr
    );


NTSTATUS
LwWin32ErrorToNtStatus(
    DWORD winerr
    );


PCSTR
LwNtStatusToName(
    NTSTATUS ntStatus
    );


PCSTR
LwWin32ErrorToName(
   int err
   );


int
LwErrnoToLdapErr(
   int uerror
   );


DWORD
LwLdapErrToWin32Error(
   int lderr
   );


#ifndef LW_STRICT_NAMESPACE

#define NtStatusToWin32Error             LwNtStatusToWin32Error
#define NtStatusToErrno                  LwNtStatusToErrno
#define ErrnoToNtStatus                  LwErrnoToNtStatus
#define ErrnoToWin32Error                LwErrnoToWin32Error
#define Win32ErrorToErrno                LwWin32ErrorToErrno
#define Win32ErrorToNtStatus             LwWin32ErrorToNtStatus
#define NtStatusToName                   LwNtStatusToName
#define Win32ErrorToName                 LwWin32ErrorToName
#define ErrnoToLdapErr                   LwErrnoToLdapErr
#define LdapErrToWin32Error              LwLdapErrToWin32Error

#endif /* LW_STRICT_NAMESPACE */


#endif /* __WINERR_CONV_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
