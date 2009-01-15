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

#ifndef _PH_DEF_H_
#define _PH_DEF_H_

/*
 * This header is separate from other definitions because it should
 * be possible to include it in idl files when generating dcerpc stubs.
 */


typedef struct guid {
	uint32 time_low;
	uint16 time_mid;
	uint16 time_hi_and_version;
	uint8 clock_seq[2];
	uint8 node[6];
} Guid;


typedef struct policy_handle {
	uint32 handle_type;
	Guid guid;
} PolicyHandle;


#endif /* _PH_DEF_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
