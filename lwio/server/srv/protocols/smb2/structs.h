/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct __SMB2_NEGOTIATE_HEADER
{
    USHORT  usLength;
    BYTE    ucFlags;
    BYTE    ucPad;
    USHORT  usDialect;
    USHORT  usPad2;
    BYTE    serverGUID[16];
    ULONG   ulCapabilities;
    ULONG   ulMaxTxSize;
    ULONG   ulMaxReadSize;
    ULONG   ulMaxWriteSize;
    ULONG64 ullCurrentTime;
    ULONG64 ullBootTime;
    USHORT  usBlobOffset;
    USHORT  usBlobLength;

    /* GSS Blob follows immediately */

} __attribute__((__packed__)) SMB2_NEGOTIATE_HEADER, *PSMB2_NEGOTIATE_HEADER;

typedef struct __SMB2_SESSION_SETUP_REQUEST_HEADER
{
    USHORT  usLength;
    BYTE    ucVCNumber;
    BYTE    ucSecurityMode;
    ULONG   ulCapabilities;
    ULONG   ulChannel;
    USHORT  usBlobOffset;
    USHORT  usBlobLength;
    ULONG64 ullPrevSessionId;

    /* GSS Blob follows immediately */

} __attribute__((__packed__)) SMB2_SESSION_SETUP_REQUEST_HEADER;

typedef SMB2_SESSION_SETUP_REQUEST_HEADER *PSMB2_SESSION_SETUP_REQUEST_HEADER;

typedef struct __SMB2_SESSION_SETUP_RESPONSE_HEADER
{
    USHORT  usLength;
    USHORT  usSessionFlags;
    USHORT  usBlobOffset;
    USHORT  usBlobLength;

    /* GSS Blob follows immediately */

} __attribute__((__packed__)) SMB2_SESSION_SETUP_RESPONSE_HEADER,
                             *PSMB2_SESSION_SETUP_RESPONSE_HEADER;

typedef struct __SMB2_TREE_CONNECT_REQUEST_HEADER
{
    USHORT usLength;
    USHORT usPad;
    USHORT usPathOffset;
    USHORT usPathLength;
} __attribute__((__packed__)) SMB2_TREE_CONNECT_REQUEST_HEADER,
                             *PSMB2_TREE_CONNECT_REQUEST_HEADER;

typedef struct __SMB2_TREE_CONNECT_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usShareType;
    ULONG  ulShareFlags;
    ULONG  ulShareCapabilities;
    ULONG  ulShareAccessMask;
} __attribute__((__packed__)) SMB2_TREE_CONNECT_RESPONSE_HEADER,
                             *PSMB2_TREE_CONNECT_RESPONSE_HEADER;

typedef struct __SMB2_CREATE_REQUEST_HEADER
{
    USHORT  usLength;
    UCHAR   ucSecurityFlags;
    UCHAR   ucOplockLevel;
    ULONG   ulImpersonationLevel;
    ULONG64 ullCreateFlags;
    ULONG64 ullReserved;
    ULONG   ulDesiredAccess;
    ULONG   ulFileAttributes;
    ULONG   ulShareAccess;
    ULONG   ulCreateDisposition;
    ULONG   ulCreateOptions;
    USHORT  usNameOffset;
    USHORT  usNameLength;
    ULONG   ulCreateContextOffset;
    ULONG   ulCreateContextLength;
} __attribute__((__packed__)) SMB2_CREATE_REQUEST_HEADER,
                             *PSMB2_CREATE_REQUEST_HEADER;

typedef struct __SMB2_FID
{
    ULONG64 ullPersistentId;
    ULONG64 ullVolatileId;
} __attribute__((__packed__)) SMB2_FID, *PSMB2_FID;

typedef struct __SMB2_CREATE_RESPONSE_HEADER
{
    USHORT   usLength;
    UCHAR    ucOplockLevel;
    UCHAR    ucReserved;
    ULONG    ulCreateAction;
    ULONG64  ullCreationTime;
    ULONG64  ullLastAccessTime;
    ULONG64  ullLastWriteTime;
    ULONG64  ullLastChangeTime;
    ULONG64  ullAllocationSize;
    ULONG64  ullEndOfFile;
    ULONG    ulFileAttributes;
    ULONG    ulReserved2;
    SMB2_FID fid;
    ULONG    ulCreateContextOffset;
    ULONG    ulCreateContextLength;
} __attribute__((__packed__)) SMB2_CREATE_RESPONSE_HEADER,
                             *PSMB2_CREATE_RESPONSE_HEADER;

#endif /* __STRUCTS_H__ */
