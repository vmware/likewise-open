/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/**
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * @file
 *
 *     lwzct.h
 *
 * @brief
 *
 *     ZCT (Zero Copy Transfer) API
 *
 * @defails
 *
 *     The ZCT (zero copy transfer) API supports an abstraction for
 *     performing I/O to/from file system drivers (FSDs) such that the
 *     FSD can pass internal buffers and/or file descriptors to the caller
 *     for reading/writing directly into the file system buffers/file
 *     descriptors.
 *
 * @author Danilo Almeida (dalmeida@likewise.com)
 */

#ifndef __LW_ZCT_H__
#define __LW_ZCT_H__

#include <lw/types.h>
#include <lw/attrs.h>

///
/// ZCT (zero copy transfer)
///
/// The ZCT is an opaque type representing buffers and/or file descriptors
/// and tracking the location of the I/O within the transfer.
///
typedef struct _IO_ZCT IO_ZCT, *PIO_ZCT;

///
/// Type of ZCT I/O.
///

typedef UCHAR IO_ZCT_IO_TYPE, *PIO_ZCT_IO_TYPE;

#define IO_ZCT_IO_TYPE_READ_SOCKET    1
#define IO_ZCT_IO_TYPE_WRITE_SOCKET   2

#define IO_ZCT_IO_TYPE_MASK_VALID \
    ( \
        IO_ZCT_IO_TYPE_READ_SOCKET | \
        IO_ZCT_IO_TYPE_WRITE_SOCKET | \
        0 \
    )

#define IoZctIsValidIoType(IoType) \
    (!((IoType) & ~IO_ZCT_IO_TYPE_MASK_VALID))

///
/// Type of ZCT entries
///
/// A ZCT can contain buffers and file descriptors of several types.
/// A single ZCT entry contains just one type:
///
/// - memory (for use with readv/writev)
/// - file descriptor for a file (for use with sendfile)
/// - file descriptor for a pipe (for use with splice)
///

typedef UCHAR IO_ZCT_ENTRY_TYPE, *PIO_ZCT_ENTRY_TYPE;

#define IO_ZCT_ENTRY_TYPE_MEMORY    1
#define IO_ZCT_ENTRY_TYPE_FD_FILE   2
#define IO_ZCT_ENTRY_TYPE_FD_PIPE   3

//
// Mask of allowed ZCT types
//

typedef UCHAR IO_ZCT_ENTRY_MASK, *PIO_ZCT_ENTRY_MASK;

#define _IO_ZCT_ENTRY_MASK_FROM_TYPE(Type)  (1 << ((Type) - 1))

#define IO_ZCT_ENTRY_MASK_MEMORY    _IO_ZCT_ENTRY_MASK_FROM_TYPE(IO_ZCT_ENTRY_TYPE_MEMORY)
#define IO_ZCT_ENTRY_MASK_FD_FILE   _IO_ZCT_ENTRY_MASK_FROM_TYPE(IO_ZCT_ENTRY_TYPE_FD_FILE)
#define IO_ZCT_ENTRY_MASK_FD_PIPE   _IO_ZCT_ENTRY_MASK_FROM_TYPE(IO_ZCT_ENTRY_TYPE_FD_PIPE)

///
/// A ZCT entry
///
/// A ZCT entry represents a memory buffer or file descriptor.
///
typedef struct _IO_ZCT_ENTRY {
    /// Type of ZCT entry
    IO_ZCT_ENTRY_TYPE Type;
    /// Length of data represented by entry in bytes (e.g., size of the
    /// memory buffer or how much to read/write from/to the file
    /// descriptor).
    ULONG Length;
    union {
        /// IO_ZCT_ENTRY_TYPE_MEMORY
        struct {
            PVOID Buffer;
        } Memory;
        /// IO_ZCT_ENTRY_TYPE_FD_FILE
        struct {
            int Fd;
            LONG64 Offset;
        } FdFile;
        /// IO_ZCT_ENTRY_TYPE_FD_PIPE
        struct {
            int Fd;
        } FdPipe;
    } Data;
} IO_ZCT_ENTRY, *PIO_ZCT_ENTRY;

///
/// Create a ZCT.
///
/// @param[out] ppZct - Returns created ZCT.
/// @param[in] IoType - Type of I/O for transfer.
///
/// @retval STATUS_SUCCESS on success
/// @retval !NT_SUCCESS on failure
///
NTSTATUS
IoZctCreate(
    OUT PIO_ZCT* ppZct,
    IN IO_ZCT_IO_TYPE IoType
    );

///
/// Destroy a ZCT.
///
/// Free all resources used for tracking the ZCT buffers and file
/// descriptors.
///
/// @paaram[in,out] ppZct ZCT to destroy.  Set to NULL on output.
///
VOID
IoZctDestroy(
    IN OUT PIO_ZCT* ppZct
    );

///
/// Append entries to a ZCT.
///
/// @param[in] pZct - ZCT to modify.
///
/// @param[in] Entries - Array of entries to add.
///
/// @param[in] Count - Count of entries to add.
///
/// @retval STATUS_SUCCESS on success
/// @retval !NT_SUCCESS on failure
///
NTSTATUS
IoZctAppend(
    IN OUT PIO_ZCT pZct,
    IN PIO_ZCT_ENTRY Entries,
    IN ULONG Count
    );

///
/// Prepend entries to a ZCT.
///
/// @param[in] pZct - ZCT to modify.
///
/// @param[in] Entries - Array of entries to add.
///
/// @param[in] Count - Count of entries to add.
///
/// @retval STATUS_SUCCESS on success
/// @retval !NT_SUCCESS on failure
///
NTSTATUS
IoZctPrepend(
    IN OUT PIO_ZCT pZct,
    IN PIO_ZCT_ENTRY Entries,
    IN ULONG Count
    );

///
/// Get the total length represented by the ZCT.
///
/// @param[in] pZct - ZCT to query.
///
/// @return Length in bytes
///
ULONG
IoZctGetLength(
    IN PIO_ZCT pZct
    );

///
/// Get the mask of ZCT entry types supported in a ZCT.
///
/// @param[in] pZct - ZCT to query.
///
/// @return Appropriate #IO_ZCT_ENTRY_MASK
///
IO_ZCT_ENTRY_MASK
IoZctGetSupportedMask(
    IN PIO_ZCT pZct
    );

///
/// Get supported ZCT types based on I/O type.
///
/// Get supported ZCT types based on I/O type.  If an invalid
/// I/O type is specified, a zero mask would be returned.
///
/// @param[in] IoType - Type of I/O
///
/// @return mask of supported ZCT types on the system

IO_ZCT_ENTRY_MASK
IoZctGetSystemSupportedMask(
    IN IO_ZCT_IO_TYPE IoType
    );

///
/// Prepare ZCT for I/O.
///
/// After calling this, the ZCT can no longer be extended.
///
/// @param[in, out] pZct - ZCT to prepare for I/O.
///
NTSTATUS
IoZctPrepareIo(
    IN OUT PIO_ZCT pZct
    );

#if 0
NTSTATUS
IoZctPerform{Socket}Io(
    IN OUT PIO_ZCT pZct,
    IN int {Socket}Fd,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    );
#endif

///
/// Read from socket into ZCT.
///
/// The ZCT must have been prepared with IoZctPrepareIo().
///
/// @param[in out] pZct - ZCT into which to read.
/// @param[in] pSocketFd - Socket from which to read.
/// @param[out] BytesTrasnferred - returns bytes read.
/// @param[out] BytesRemaining - returns bytes remaining to read.
///
/// @retval STATUS_SUCCESS
/// @retval STATUS_MORE_PROCESSING_REQUIRED
/// @retval !NT_SUCCESS
///
NTSTATUS
IoZctReadSocketIo(
    IN OUT PIO_ZCT pZct,
    IN int SocketFd,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    );

///
/// Write into socket from ZCT.
///
/// The ZCT must have been prepared with IoZctPrepareIo().
///
/// @param[in out] pZct - ZCT from which to write.
/// @param[in] pSocketFd - Socket info which to write.
/// @param[out] BytesTrasnferred - returns bytes written.
/// @param[out] BytesRemaining - returns bytes remaining to write.
///
/// @retval STATUS_SUCCESS
/// @retval STATUS_MORE_PROCESSING_REQUIRED
/// @retval !NT_SUCCESS
///
NTSTATUS
IoZctWriteSocketIo(
    IN OUT PIO_ZCT pZct,
    IN int SocketFd,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    );

#endif /* __LW_ZCT_H__ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
