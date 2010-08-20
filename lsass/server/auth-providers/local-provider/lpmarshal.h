/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lpmarshal.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider (Defines)
 *
 *        Marshal from DIRECTORY structures to lsass info levels
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __LP_MARSHAL_H__
#define __LP_MARSHAL_H__

DWORD
LocalMarshalAttrToInteger(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PDWORD           pdwValue
    );

DWORD
LocalMarshalAttrToLargeInteger(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PLONG64          pllValue
    );

DWORD
LocalMarshalAttrToANSIString(
    PDIRECTORY_ENTRY pAttr,
    PWSTR            pwszAttrName,
    PSTR*            ppszValue
    );

DWORD
LocalMarshalAttrToUnicodeString(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PWSTR*           ppwszValue
    );

DWORD
LocalMarshalAttrToANSIFromUnicodeString(
    PDIRECTORY_ENTRY pAttr,
    PWSTR            pwszAttrName,
    PSTR*            ppszValue
    );

DWORD
LocalMarshalAttrToOctetStream(
    PDIRECTORY_ENTRY pAttr,
    PWSTR            pwszAttrName,
    PBYTE*           ppData,
    PDWORD           pdwDataLen
    );

DWORD
LocalMarshalAttrToBOOLEAN(
    PDIRECTORY_ENTRY pAttr,
    PWSTR            pwszAttrName,
    PBOOLEAN         pbValue
    );

DWORD
LocalMarshalAttrToSid(
    PDIRECTORY_ENTRY  pEntry,
    PWSTR             pwszAttrName,
    PSID             *ppSid
    );

DWORD
LocalMarshalEntryToSecurityObject(
    PDIRECTORY_ENTRY pEntry,
    PLSA_SECURITY_OBJECT* ppObject
    );

#endif /* __LP_MARSHAL_H__ */


