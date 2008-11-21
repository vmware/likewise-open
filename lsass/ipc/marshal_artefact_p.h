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

#ifndef __MARSHAL_NSS_ARTEFACT_P_H__
#define __MARSHAL_NSS_ARTEFACT_P_H__

DWORD
LsaMarshalNSSArtefactInfoList(
    PVOID* ppNSSArtefactInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumNSSArtefacts,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    );

DWORD
LsaComputeNSSArtefactBufferSize(
    PVOID* ppNSSArtefactInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumNSSArtefacts,
    PDWORD pdwBufLen
    );

DWORD
LsaComputeBufferSize_NSSArtefact0(
    PVOID* ppNSSArtefactInfoList,
    DWORD  dwNumNSSArtefacts
    );

DWORD
LsaMarshalNSSArtefact_0_InfoList(
    PVOID* ppNSSArtefactInfoList,
    DWORD  dwNumNSSArtefacts,
    DWORD  dwBeginOffset,
    PSTR   pszBuffer,
    PDWORD pdwDataBytesWritten
    );

DWORD
LsaMarshalNSSArtefact_0(
    PLSA_NSS_ARTEFACT_INFO_0 pNSSArtefactInfo,
    PSTR   pszHeaderBuffer,
    PSTR   pszDataBuffer,
    DWORD  dwDataBeginOffset,
    PDWORD pdwDataBytesWritten
    );

DWORD
LsaUnmarshalNSSArtefactInfoList(
    PCSTR   pszMsgBuf,
    DWORD   dwMsgLen,
    PDWORD  pdwInfoLevel,
    PVOID** pppNSSArtefactInfoList,
    PDWORD  pdwNumNSSArtefacts
    );

DWORD
LsaUnmarshalNSSArtefact_0_InfoList(
    PCSTR pszMsgBuf,
    PCSTR pszHdrBuf,
    PVOID** pppNSSArtefactInfoList,
    DWORD dwNumNSSArtefacts
    );

DWORD
LsaUnmarshalNSSArtefact_0(
    PCSTR pszMsgBuf,
    PLSA_NSS_ARTEFACT_0_RECORD_HEADER pHeader,
    PLSA_NSS_ARTEFACT_INFO_0* ppNSSArtefactInfo
    );

#endif /* __MARSHAL_NSS_ARTEFACT_P_H__ */

