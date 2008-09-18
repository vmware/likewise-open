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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ntlmdlist.h
 *
 * Abstract:
 *
 *        List functions
 *
 * Author: Todd Stecher (2007)
 *
 */

#ifndef __NTLM_D_LIST_H__
#define __NTLM_D_LIST_H__

typedef struct _NTLM_LIST NTLM_LIST;

struct _NTLM_LIST
{
    NTLM_LIST *next;
    NTLM_LIST *prev;
};


NTLM_LIST*
NTLMListAppend(
    NTLM_LIST *list,
    NTLM_LIST *new_node
    );

NTLM_LIST*
NTLMListPrepend(
    NTLM_LIST *list,
    NTLM_LIST *new_node
    );

NTLM_LIST*
NTLMListFindNode(
    NTLM_LIST *list,
    NTLM_LIST *toFind
    );

NTLM_LIST*
NTLMRemoveLink(
    NTLM_LIST *list,
    NTLM_LIST *link
    );

NTLM_LIST*
NTLMListRemove(
    NTLM_LIST   *list,
    NTLM_LIST   *toRemove    
    );

NTLM_LIST*
NTLMListLast(
    NTLM_LIST *list
    );

NTLM_LIST*
NTLMListFirst (
    NTLM_LIST *list
    );

#endif /* _NTLM_D_LIST_H_ */
