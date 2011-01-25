/*
 * Copyright Likewise Software
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
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        GSSAPI Wrappers
 *
 *        Function prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

// gssapi.c

DWORD
LsaGssCreateContext(
    PLSA_GSS_CONTEXT* ppContext
    );

DWORD
LsaGssAuthenticate(
   PLSA_GSS_CONTEXT pContext,
   PBYTE            pInBytes,
   DWORD            dwInLength,
   PBYTE*           ppOutBytes,
   PDWORD           pdwOutBytes,
   PBOOLEAN         pbContinueNeeded
   );

DWORD
LsaGssGetRoles(
    PLSA_GSS_CONTEXT pContext,
    PSTR**           pppszRoles,
    PDWORD           pdwNumRoles
    );

DWORD
LsaGssGetClientPrincipalName(
    PLSA_GSS_CONTEXT pContext,
    PSTR*            ppszclientName
    );

VOID
LsaGssFreeContext(
    PLSA_GSS_CONTEXT pContext
    );

// libmain.c

DWORD
LsaGssInitialize(
    VOID
    );

VOID
LsaGssShutdown(
    VOID
    );

VOID
LsaGssFreeStringArray(
    PSTR* ppszStringArray,
    DWORD dwNumStrings
    );

VOID
LsaGssFreeMemory(
    PVOID pMemory
    );

