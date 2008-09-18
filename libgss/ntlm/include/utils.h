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
 *        utils.h
 *
 * Abstract:
 *
 *       common utilities for gssntlm
 *
 * Author: Todd Stecher (v-todds@likewisesoftware.com)
 *
 */
#ifndef _UTILS_H_
#define _UTILS_H_

#define NTLM_MAX_WORKSTATION_NAME       255

/* helpful error macros */
#define BAIL_ON_NTLM_ERROR(_e_)         do { BAIL_ON_LSA_ERROR(_e_); } while (0)
#define BAIL_WITH_NTLM_ERROR(_e_)       do { dwError = _e_; BAIL_ON_NTLM_ERROR(_e_); } while (0)

/* memory allocation routines */
PVOID 
NTLMAllocateMemory(DWORD dwSize);

PVOID 
NTLMReallocMemory(PVOID  pMemory, DWORD dwSize);

void 
NTLMFreeMemory(PVOID pMemory);

void 
NTLMSafeFreeMemory( PVOID *ppMemory);

#define NTLM_SAFE_FREE(mem)     \
    do {                        \
        if (mem) {              \
            NTLMFreeMemory(mem);\
        }                       \
    } while(0);

PSEC_BUFFER
NTLMCopySecBuffer(PSEC_BUFFER src);

DWORD
NTLMAllocTransferSecBuffer(
    PSEC_BUFFER *dst,
    PSEC_BUFFER src
    );

void
NTLMFreeSecBuffer(
    PSEC_BUFFER freeme
    );

void 
NTLMFreeSecBufferBase(
    PSEC_BUFFER *freeme
    );



/* name helper functions */
DWORD
NTLMGetWorkstationName(PLSA_STRING workstationName);

DWORD
NTLMGetDNSWorkstationName(PLSA_STRING workstationDNSName);

DWORD
NTLMGetDNSDomainName(PLSA_STRING dnsDomainName);

DWORD
NTLMGetNBDomainName(PLSA_STRING nbDomainName);

/* validation / sanity check routines */
bool 
NTLMValidateMarshalledSecBuffer(
    PBYTE base,
    ULONG length,
    PSEC_BUFFER secBuf
    );

bool
NTLMValidateMarshalledLsaString(
    PBYTE base,
    ULONG length,
    PLSA_STRING string
    );

DWORD
NTLMAllocCopySecBuffer(
    PSEC_BUFFER dst,
    PSEC_BUFFER src
    );

/* supplemental cred builder */
DWORD
NTLMBuildSupplementalCredentials(
    char *username,
    char *domain,
    char *password,
    PSEC_BUFFER credBlob
    );

void
NTLMUpperCase(
    PLSA_STRING s
);

/* initialize these routines */
DWORD
NTLMInitUtilityFunctions();


#endif




