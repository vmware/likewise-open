/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
*/

/*
    Linux Policy Handle library
    Copyright (C) Rafal Szczesniak  2008


    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef _POL_HND_H_
#define _POL_HND_H_


#include "sdsys.h"
#include "phdef.h"


NTSTATUS
PolHndCreate(
    PolicyHandle *pH,
    DWORD dwHandleType
    );


NTSTATUS
PolHndCopy(
    PolicyHandle *pH,
    const PolicyHandle *in
    );


NTSTATUS
PolHndAllocate(
    PolicyHandle **ppH,
    DWORD dwHandleType
    );


void
PolHndFree(
    PolicyHandle *pH
    );


BOOL
PolHndIsEqual(
    PolicyHandle *pH1,
    PolicyHandle *pH2
    );


BOOL
PolHndIsEmpty(
    PolicyHandle *pH
    );


#endif /* _POL_HND_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
