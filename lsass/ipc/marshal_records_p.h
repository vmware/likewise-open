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
 *        marshal_records_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Marshal/Unmarshal API for User/Group Enumeration Messages
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#ifndef __MARSHAL_RECORDS_P_H__
#define __MARSHAL_RECORDS_P_H__

DWORD
LsaMarshalBeginEnumRecordsQuery(
    DWORD  dwInfoLevel,
    DWORD  dwNumMaxRecords,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalBeginEnumRecordsQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PDWORD pdwInfoLevel,
    PDWORD pdwNumMaxRecords
    );

DWORD
LsaMarshalEnumRecordsToken(
    PCSTR  pszGUID,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaUnmarshalEnumRecordsToken(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PSTR*  ppszGUID
    );

#endif /* __MARSHAL_RECORDS_P_H__ */
