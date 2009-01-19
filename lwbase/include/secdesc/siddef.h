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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _SIDDEF_H_
#define _SIDDEF_H_

/*
 * This header is separate from other definitions because it should
 * be possible to include it in idl files when generating dcerpc stubs.
 */


typedef struct sid {
    uint8 revision;
#ifdef _DCE_IDL_
	[range(0,15)]
#endif
    uint8 subauth_count;
    uint8 authid[6];
#ifdef _DCE_IDL_
	[size_is(subauth_count)]
#endif
    uint32 subauth[];
} SID, *PSID;


/* Helper macro for transition of old code to the new library */
#define DomSid  SID


typedef struct sid_ptr {
	DomSid *sid;
} SidPtr;


typedef struct sid_array {
#ifdef _DCE_IDL_
	[range(0,1000)]
#endif
	uint32 num_sids;
#ifdef _DCE_IDL_
	[size_is(num_sids)]
#endif
	SidPtr *sids;
} SidArray;


#endif /* _SIDDEF_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
