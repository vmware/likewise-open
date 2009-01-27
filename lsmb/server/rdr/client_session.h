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


DWORD
SMBSrvClientSessionCreate(
    PSMB_SOCKET   pSocket,
    uchar8_t      *pszPrincipal,
    PSMB_SESSION* ppSession
    );

DWORD
SMBSrvClientSessionIsStale_inlock(
    PSMB_SESSION pSession,
    PBOOLEAN     pbIsStale
    );

DWORD
SMBSrvClientSessionAddTreeById(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    );

DWORD
SMBSrvClientSessionRemoveTreeById(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    );

DWORD
SMBSrvClientSessionAddTreeByPath(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    );

DWORD
SMBSrvClientSessionRemoveTreeByPath(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    );

DWORD
SMBSrvClientSessionRelease(
    PSMB_SESSION pSession
    );
