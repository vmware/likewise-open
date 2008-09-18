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

#include "includes.h"

/*
 * push functions/macros transfer data from net userinfo structs to samr userinfo
 * pull functions/macros transfer data to net userinfo structs from samr userinfo
 */

#define PULL_UNICODE_STRING(dst, src) \
	if (src.string != NULL && src.size > 0) { \
		dst = wc16sdup(src.string); \
		dst[src.len / 2] = '\0'; \
	}

#define PUSH_UNICODE_STRING(netinfo_field, samrinfo_field) \
	if (netinfo_field != NULL) { \
		InitUnicodeString(&samrinfo_field, netinfo_field); \
	}


void *PullLocalGroupInfo0(void *buffer, AliasInfo *ai, int i)
{
	LOCALGROUP_INFO_0 *info = (LOCALGROUP_INFO_0*)buffer;
	if (info == NULL) return NULL;

	PULL_UNICODE_STRING(info[i].lgrpi0_name, ai->all.name);

	return buffer;
}


void *PullLocalGroupInfo1(void *buffer, AliasInfo *ai, int i)
{
	LOCALGROUP_INFO_1 *info = (LOCALGROUP_INFO_1*)buffer;
	if (info == NULL) return NULL;

	PULL_UNICODE_STRING(info[i].lgrpi1_name, ai->all.name);
	PULL_UNICODE_STRING(info[i].lgrpi1_comment, ai->all.description);

	return buffer;
}


void* PushLocalGroupInfo0(void *buffer, uint32 *level, LOCALGROUP_INFO_0 *ninfo)
{
	AliasInfo *info = (AliasInfo*)buffer;
	if (info == NULL) return NULL;

	*level = 2;
	
	PUSH_UNICODE_STRING(ninfo->lgrpi0_name, info->name);

	return buffer;
}


void* PushLocalGroupInfo1to2(void *buffer, uint32 *level, LOCALGROUP_INFO_1 *ninfo)
{
	AliasInfo *info = (AliasInfo*)buffer;
	if (info == NULL) return NULL;
	
	*level = 2;
	
	PUSH_UNICODE_STRING(ninfo->lgrpi1_name, info->name);
	
	return buffer;
}


void* PushLocalGroupInfo1to3(void *buffer, uint32 *level, LOCALGROUP_INFO_1 *ninfo)
{
	AliasInfo *info = (AliasInfo*)buffer;
	if (info == NULL) return NULL;
	
	*level = 3;

	PUSH_UNICODE_STRING(ninfo->lgrpi1_comment, info->description);
	
	return buffer;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
