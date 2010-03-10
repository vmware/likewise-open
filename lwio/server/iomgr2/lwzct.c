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
 *     lwzct.c
 *
 * @brief
 *
 *     ZCT (Zero Copy Transfer) API Implementation
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

// From Configure Tests
#undef HAVE_SPLICE
#undef HAVE_SENDFILE
#undef HAVE_SENDFILEV

// From Additional Determinations
#undef HAVE_SENDFILE_HEADER_TRAILER
#undef HAVE_SENDFILE_ANY

#include "lwzct.h"
#include <lw/rtlmemory.h>
#include <lw/rtlgoto.h>
#include <lw/safeint.h>
#include "config.h"
#include <lw/errno.h>
#include <assert.h>

//
// Notes about syscall interfaces:
//
// readv/writev - can do mem <-> socket/file/pipe
// splice - can do pipe <-> socket/file/pipe
// sendfilev - can do mem/file -> socket/file(/pipe?)
// sendfile (w/ht) - can do header + file + trailer -> socket
// sendfile - can do file -> socket
//

// For iovec support
#include <sys/uio.h>
#ifdef HAVE_SPLICE
// For splice support
#include <fcntl.h>
#endif
// For sendfile support
#ifdef HAVE_SYS_SENDFILE_H
#include <sys/sendfile.h>
#endif

// Enable to pull in test definitions to help
// catch compilation issues for splice/sendfilev/etc.
// #include "fake-syscalls.h"

#ifdef HAVE_SENDFILE
#if defined(__FreeBSD__)
#define HAVE_SENDFILE_HEADER_TRAILER 1
#endif
#endif

#define IO_ZCT_ENTRY_CAPACITY_MINIMUM   4
#define IO_ZCT_ENTRY_CAPACITY_INCREMENT 2

typedef ULONG IO_ZCT_CURSOR_TYPE, *PIO_ZCT_CURSOR_TYPE;

#define IO_ZCT_CURSOR_TYPE_IOVEC                      1
#define IO_ZCT_CURSOR_TYPE_SPLICE                     2
#define IO_ZCT_CURSOR_TYPE_SENDFILE                   3

typedef struct _IO_ZCT_CURSOR_IOVEC {
    // Next starting Vector location is modified after
    // each transfer as needed.
    struct iovec* Vector;
    // Pass (Count - Index) into syscall.
    int Count;
    // Points to starting Vector location.  Updated
    // after each transfer as needed.
    int Index;
} IO_ZCT_CURSOR_IOVEC, *PIO_ZCT_CURSOR_IOVEC;

typedef struct _IO_ZCT_CURSOR_SPLICE {
    int FileDescriptor;
    // Length is updated after each transfer as needed.
    size_t Length;
} IO_ZCT_CURSOR_SPLICE, *PIO_ZCT_CURSOR_SPLICE;

//
// The ordering for the have sendfile checks is important here and
// elsewhere in the code:
//
// 1) sendfilev is preferred
// 2) sendfile w/ header/trailer is next
// 3) sendfile w/o header/trailer is last
//
#if defined(HAVE_SENDFILEV)
#define HAVE_SENDFILE_ANY 1
typedef struct _IO_ZCT_CURSOR_SENDFILE {
    // Next starting Vector location is modified after
    // each transfer as needed.
    sendfilevec_t* Vector;
    // Pass (Count - Index) into syscall.
    int Count;
    // Points to starting Vector location.  Updated
    // after each transfer as needed.
    int Index;
} IO_ZCT_CURSOR_SENDFILE, *PIO_ZCT_CURSOR_SENDFILE;
#elif defined(HAVE_SENDFILE_HEADER_TRAILER)
#define HAVE_SENDFILE_ANY 1
typedef struct _IO_ZCT_CURSOR_SENDFILE {
    IO_ZCT_CURSOR_IOVEC Header;
    IO_ZCT_CURSOR_IOVEC Trailer;
    int FileDescriptor;
    off_t Offset;
    size_t Length;
} IO_ZCT_CURSOR_SENDFILE, *PIO_ZCT_CURSOR_SENDFILE;
#elif defined(HAVE_SENDFILE)
#define HAVE_SENDFILE_ANY 1
typedef struct _IO_ZCT_CURSOR_SENDFILE {
    int FileDescriptor;
    // Offset and Length are updated after each transfer as needed.
    off_t Offset;
    size_t Length;
} IO_ZCT_CURSOR_SENDFILE, *PIO_ZCT_CURSOR_SENDFILE;
#endif

///
/// ZCT Cursor Entry
///
/// A cursor entry is updated as its data is transferred
/// such that the next transfer can continue from there
/// the previous transfer left off.  Depending on the entry
/// type, pointers, offsets, and sizes are updated.  For
/// entry types that include arrays, the index of where to
/// start in the array is updated.
///
typedef struct _IO_ZCT_CURSOR_ENTRY {
    /// This represent the location(s) in the Entries member
    /// of IO_ZCT that are encompassed by this cursor entry.
    /// This is strictly for debugging.
    struct {
        /// Index into the Entries member of IO_ZCT.
        ULONG Index;
        /// How many entries the current cursor entry contains
        /// starting from Index.  This must be >= 1.
        ULONG Count;
    } DebugExtent;
    /// Current type info.
    IO_ZCT_CURSOR_TYPE Type;
    union {
        IO_ZCT_CURSOR_IOVEC IoVec;
        IO_ZCT_CURSOR_SPLICE Splice;
        IO_ZCT_CURSOR_SENDFILE SendFile;
    } Data;
} IO_ZCT_CURSOR_ENTRY, *PIO_ZCT_CURSOR_ENTRY;

///
/// ZCT Cursor
///
/// The ZCT cursor is allocated when the I/O is prepared
/// via IoZctPreprareIo().  It contains all the OS-level
/// structures to perform the entire I/O.
///
/// The allocated size is sufficient to contain all of
/// the data needed by the cursor, including vectors and
/// such.  This makes the allocation more efficient.
///
typedef struct _IO_ZCT_CURSOR {
    /// Total cursor allocation size.  This must be large enough to
    /// encompass all of the cursor entries.
    ULONG Size;
    /// Offset of struct iovec area
    ULONG IoVecOffset;
    /// Offset of free struct iovec area
    ULONG FreeIoVecOffset;
#ifdef HAVE_SENDFILEV
    /// Offset of sendfilevec_t area
    ULONG SendFileVecOffset;
    /// Offset of free sendfilevec_t area
    ULONG FreeSendFileVecOffset;
#endif
    /// Count of entries.
    ULONG Count;
    /// Index into cursor entries.  Whenever an entry is
    /// finished, the index is incremented.
    ULONG Index;
    /// Cursor entries.
    IO_ZCT_CURSOR_ENTRY Entry[1];
} IO_ZCT_CURSOR, *PIO_ZCT_CURSOR;

struct _IO_ZCT {
    IO_ZCT_IO_TYPE IoType;
    IO_ZCT_ENTRY_MASK Mask;
    PIO_ZCT_ENTRY Entries;
    ULONG Count;
    ULONG Capacity;
    /// Total size of transfer.
    ULONG Length;
    /// Track how much has been transferred so far
    /// so that it can be returned at the end of
    /// the I/O.
    ULONG BytesTransferred;
    /// Do not allow I/O to continue after a failure.
    NTSTATUS Status;
    /// When the cursor is allocated, the ZCT can
    /// no longer have entries added.
    PIO_ZCT_CURSOR Cursor;
};

static
NTSTATUS
IopZctReadWriteSocket(
    IN OUT PIO_ZCT pZct,
    IN int SocketFd,
    IN BOOLEAN IsWrite,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    );

static
NTSTATUS
IopZctCursorEntryReadWriteSocket(
    IN int SocketFd,
    IN BOOLEAN IsWrite,
    IN OUT PIO_ZCT_CURSOR_ENTRY pEntry,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDoneEntry
    );

static
NTSTATUS
IopZctIoVecReadWrite(
    IN int FileDescriptor,
    IN BOOLEAN IsWrite,
    IN OUT PIO_ZCT_CURSOR_IOVEC Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    );

#if defined(HAVE_SPLICE)
static
NTSTATUS
IopZctSplice(
    IN int FileDescriptor,
    IN BOOLEAN IsWrite,
    IN OUT PIO_ZCT_CURSOR_SPLICE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    );
#endif

#if defined(HAVE_SENDFILE_ANY)
static
NTSTATUS
IopZctSendFile(
    IN int FileDescriptor,
    IN OUT PIO_ZCT_CURSOR_SENDFILE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    );
#endif

NTSTATUS
IoZctCreate(
    OUT PIO_ZCT* ppZct,
    IN IO_ZCT_IO_TYPE IoType
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PIO_ZCT pZct = NULL;

    if (!IoZctIsValidIoType(IoType))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    status = RTL_ALLOCATE(&pZct, IO_ZCT, sizeof(*pZct));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pZct->Capacity = IO_ZCT_ENTRY_CAPACITY_MINIMUM;

    status = RTL_ALLOCATE(&pZct->Entries, IO_ZCT_ENTRY, sizeof(*pZct->Entries) * pZct->Capacity);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pZct->IoType = IoType;
    pZct->Mask = IoZctGetSystemSupportedMask(IoType);

cleanup:
    IoZctDestroy(&pZct);

    return status;
}

VOID
IoZctDestroy(
    IN OUT PIO_ZCT* ppZct
    )
{
    PIO_ZCT pZct = *ppZct;

    if (pZct)
    {
        RTL_FREE(&pZct->Cursor);
        RTL_FREE(&pZct->Entries);
        RtlMemoryFree(pZct);
        *ppZct = NULL;
    }
}

NTSTATUS
IopZctCheckEntry(
    IN IO_ZCT_ENTRY_MASK Mask,
    IN PIO_ZCT_ENTRY Entry
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;

    if (Entry->Length < 1)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    if (!IsSetFlag(Mask, _IO_ZCT_ENTRY_MASK_FROM_TYPE(Entry->Type)))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    switch (Entry->Type)
    {
        case IO_ZCT_ENTRY_TYPE_MEMORY:
            if (!Entry->Data.Memory.Buffer)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_ON_STATUS_EE(status, EE);
            }
            if ((PVOID)LwRtlOffsetToPointer(Entry->Data.Memory.Buffer, Entry->Length) < Entry->Data.Memory.Buffer)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_ON_STATUS_EE(status, EE);
            }
            break;

        case IO_ZCT_ENTRY_TYPE_FD_FILE:
            if (Entry->Data.FdFile.Fd < 0)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_ON_STATUS_EE(status, EE);
            }
            break;

        case IO_ZCT_ENTRY_TYPE_FD_PIPE:
            if (Entry->Data.FdPipe.Fd < 0)
            {
                status = STATUS_INVALID_PARAMETER;
                GOTO_CLEANUP_ON_STATUS_EE(status, EE);
            }
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    return status;
}

static
NTSTATUS
IopZctAdd(
    IN OUT PIO_ZCT pZct,
    IN BOOLEAN bAddToFront,
    IN PIO_ZCT_ENTRY Entries,
    IN ULONG Count
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    ULONG i = 0;
    ULONG newCount = 0;
    PIO_ZCT_ENTRY pTarget = NULL;
    ULONG newLength = pZct->Length;

    if (pZct->Cursor)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    for (i = 0; i < Count; i++)
    {
        status = IopZctCheckEntry(pZct->Mask, &Entries[i]);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        status = LwRtlSafeAddULONG(&newLength, newLength, Entries[i].Length);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    newCount = pZct->Count + Count;
    if (pZct->Capacity < newCount)
    {
        PIO_ZCT_ENTRY pEntries = NULL;
        ULONG newCapacity = newCount + IO_ZCT_ENTRY_CAPACITY_INCREMENT;

        status = RTL_ALLOCATE(&pEntries, IO_ZCT_ENTRY, sizeof(pEntries[0]) * newCapacity);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        pTarget = pEntries;
        if (bAddToFront)
        {
            pTarget += Count;
        }

        LwRtlCopyMemory(pTarget, pZct->Entries, pZct->Count * sizeof(pZct->Entries[0]));

        RTL_FREE(&pZct->Entries);
        pZct->Entries = pEntries;
        pEntries = NULL;
        pZct->Capacity = newCapacity;
    }
    else if (bAddToFront)
    {
        LwRtlMoveMemory(pZct->Entries + Count, pZct->Entries, pZct->Count * sizeof(pZct->Entries[0]));
    }

    pTarget = pZct->Entries;
    if (!bAddToFront)
    {
        pTarget += pZct->Count;
    }
    LwRtlCopyMemory(pTarget, Entries, sizeof(Entries[0]) * Count);
    pZct->Count = newCount;
    pZct->Length = newLength;

cleanup:
    return status;
}

NTSTATUS
IoZctAppend(
    IN OUT PIO_ZCT pZct,
    IN PIO_ZCT_ENTRY Entries,
    IN ULONG Count
    )
{
    return IopZctAdd(pZct, FALSE, Entries, Count);
}

NTSTATUS
IoZctPrepend(
    IN OUT PIO_ZCT pZct,
    IN PIO_ZCT_ENTRY Entries,
    IN ULONG Count
    )
{
    return IopZctAdd(pZct, TRUE, Entries, Count);
}

ULONG
IoZctGetLength(
    IN PIO_ZCT pZct
    )
{
    return pZct->Length;
}

IO_ZCT_ENTRY_MASK
IoZctGetSupportedMask(
    IN PIO_ZCT pZct
    )
{
    return pZct->Mask;
}

IO_ZCT_ENTRY_MASK
IoZctGetSystemSupportedMask(
    IN IO_ZCT_IO_TYPE IoType
    )
{
    IO_ZCT_ENTRY_MASK mask = 0;

    switch (IoType)
    {
        case IO_ZCT_IO_TYPE_READ_SOCKET:
        case IO_ZCT_IO_TYPE_WRITE_SOCKET:
            SetFlag(mask, IO_ZCT_ENTRY_MASK_MEMORY);
#if defined(HAVE_SPLICE)
            SetFlag(mask, IO_ZCT_ENTRY_MASK_FD_PIPE);
#endif
#if defined(HAVE_SENDFILE_ANY)
            if (IO_ZCT_IO_TYPE_WRITE_SOCKET == IoType)
            {
                SetFlag(mask, IO_ZCT_ENTRY_MASK_FD_FILE);
            }
#endif
            break;
    }

    return mask;
}

static
NTSTATUS
IopZctPrepareForSocketIo(
    IN OUT PIO_ZCT pZct
    );

NTSTATUS
IoZctPrepareIo(
    IN OUT PIO_ZCT pZct
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;

    if (pZct->Count < 1)
    {
        assert(FALSE);
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }

    switch (pZct->IoType)
    {
    case IO_ZCT_IO_TYPE_READ_SOCKET:
    case IO_ZCT_IO_TYPE_WRITE_SOCKET:
        status = IopZctPrepareForSocketIo(pZct);
        GOTO_CLEANUP_EE(EE);
        break;
    default:
        assert(FALSE);
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    return status;
}

static
ULONG
IopZctCountRun(
    IN PIO_ZCT_ENTRY Entries,
    IN ULONG Count,
    IN PIO_ZCT_ENTRY_TYPE AllowedEntryTypes,
    IN ULONG AllowedCount,
    IN OPTIONAL PIO_ZCT_ENTRY_TYPE RequiredType
    )
{
    ULONG count = 0;
    ULONG i = 0;
    BOOLEAN foundRequired = FALSE;

    for (i = 0; i < Count; i++)
    {
        PIO_ZCT_ENTRY pEntry = &Entries[i];
        ULONG allowedIndex = 0;
        BOOLEAN foundAllowed = FALSE;

        if (RequiredType &&
            !foundRequired &&
            ((*RequiredType) == pEntry->Type))
        {
            foundRequired = TRUE;
        }

        for (allowedIndex = 0; allowedIndex < AllowedCount; allowedIndex++)
        {
            if (AllowedEntryTypes[allowedIndex] == pEntry->Type)
            {
                foundAllowed = TRUE;
                break;
            }
        }

        if (!foundAllowed)
        {
            break;
        }

        count++;
    }

    if (RequiredType && !foundRequired)
    {
        count = 0;
    }

    return count;
}

static
ULONG
IopZctCountRunMemory(
    IN PIO_ZCT_ENTRY Entries,
    IN ULONG Count
    )
{
    IO_ZCT_ENTRY_TYPE memoryAllowed[] = { IO_ZCT_ENTRY_TYPE_MEMORY };

    return IopZctCountRun(
                Entries,
                Count,
                memoryAllowed,
                LW_ARRAY_SIZE(memoryAllowed),
                NULL);
}

#if defined(HAVE_SENDFILEV)
static
ULONG
IopZctCountRunSendFileV(
    IN PIO_ZCT_ENTRY Entries,
    IN ULONG Count
    )
{
    IO_ZCT_ENTRY_TYPE memoryFileAllowed[] = { IO_ZCT_ENTRY_TYPE_MEMORY, IO_ZCT_ENTRY_TYPE_FD_FILE };
    IO_ZCT_ENTRY_TYPE fileRequired[] = { IO_ZCT_ENTRY_TYPE_FD_FILE };

    return IopZctCountRun(
                Entries,
                Count,
                memoryFileAllowed,
                LW_ARRAY_SIZE(memoryFileAllowed),
                fileRequired);
}
#endif

static
IO_ZCT_CURSOR_TYPE
IopZctCountRangeForSocketIo(
    IN PIO_ZCT pZct,
    IN ULONG StartIndex,
    OUT PULONG Count
    )
{
    IO_ZCT_CURSOR_TYPE cursorType = 0;
    int EE = 0;
    ULONG count = 0;
    PIO_ZCT_ENTRY pEntry = &pZct->Entries[StartIndex];

    if (StartIndex >= pZct->Count)
    {
        assert(FALSE);
        count = 0;
        cursorType = 0;
        GOTO_CLEANUP_EE(EE);
    }

#if defined(HAVE_SENDFILEV)
    count = IopZctCountRunSendFileV(pEntry, pZct->Count - StartIndex);
    if (count > 0)
    {
        cursorType = IO_ZCT_CURSOR_TYPE_SENDFILE;
        GOTO_CLEANUP_EE(EE);
    }
#elif defined(HAVE_SENDFILE_HEADER_TRAILER)
    count = IopZctCountRunMemory(pEntry, pZct->Count - StartIndex);
    if (((StartIndex + count) < pZct->Count) &&
        (IO_ZCT_ENTRY_TYPE_FD_FILE == pEntry[count].Type))
    {
        ULONG headerCount = count;

        count = IopZctCountRunMemory(
                        &pEntry[count],
                        pZct->Count - (StartIndex + count));
        count = headerCount + 1 + count;
        cursorType = IO_ZCT_CURSOR_TYPE_SENDFILE;
        GOTO_CLEANUP_EE(EE);
    }
#elif defined(HAVE_SENDFILE)
    if (IO_ZCT_ENTRY_TYPE_FD_FILE == pEntry->Type)
    {
        count = 1;
        cursorType = IO_ZCT_CURSOR_TYPE_SENDFILE;
        GOTO_CLEANUP_EE(EE);
    }
#endif
    count = IopZctCountRunMemory(pEntry, pZct->Count - StartIndex);
    if (count > 0)
    {
        cursorType = IO_ZCT_CURSOR_TYPE_IOVEC;
        GOTO_CLEANUP_EE(EE);
    }

    if (IO_ZCT_ENTRY_TYPE_FD_PIPE == pEntry->Type)
    {
        count = 1;
        cursorType = IO_ZCT_CURSOR_TYPE_SPLICE;
        GOTO_CLEANUP_EE(EE);
    }

    // Should never get here.
    assert(FALSE);

cleanup:
    *Count = count;

    return cursorType;
}

static
NTSTATUS
IopZctCursorAllocateForSocketIo(
    IN PIO_ZCT pZct,
    OUT PIO_ZCT_CURSOR* ppCursor
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    ULONG i = 0;
    ULONG cursorEntryCount = 0;
    ULONG cursorEntrySize = 0;
    ULONG ioVecCount = 0;
    ULONG ioVecSize = 0;
#if defined(HAVE_SENDFILEV)
    ULONG sendFileVecCount = 0;
    ULONG sendFileVecSize = 0;
#endif
    ULONG size = 0;
    PIO_ZCT_CURSOR pCursor = NULL;

    while (i < pZct->Count)
    {
        IO_ZCT_CURSOR_TYPE cursorType = 0;
        ULONG count = 0;

        cursorType = IopZctCountRangeForSocketIo(
                            pZct,
                            i,
                            &count);
        switch (cursorType)
        {
            case IO_ZCT_CURSOR_TYPE_IOVEC:
                cursorEntryCount++;
                assert(count > 0);
                ioVecCount += count;
                break;
            case IO_ZCT_CURSOR_TYPE_SPLICE:
                cursorEntryCount++;
                assert(1 == count);
                break;
#if defined(HAVE_SENDFILEV)
            case IO_ZCT_CURSOR_TYPE_SENDFILE:
                cursorEntryCount++;
                assert(count > 0);
                sendFileVecCount += count;
                break;
#elif defined(HAVE_SENDFILE_HEADER_TRAILER)
            case IO_ZCT_CURSOR_TYPE_SENDFILE:
                cursorEntryCount++;
                assert(count > 0);
                ioVecCount += count - 1;
                break;
#elif defined(HAVE_SENDFILE)
            case IO_ZCT_CURSOR_TYPE_SENDFILE:
                cursorEntryCount++;
                assert(1 == count);
                break;
#endif
            default:
                assert(FALSE);
                status = STATUS_INTERNAL_ERROR;
                GOTO_CLEANUP_EE(EE);
                break;
        }

        i += count;
    }

    cursorEntrySize = cursorEntryCount * sizeof(IO_ZCT_CURSOR_ENTRY);
    size += cursorEntrySize;

    ioVecSize = ioVecCount * sizeof(struct iovec);
    size += ioVecSize;

#if defined(HAVE_SENDFILEV)
    sendFileVecSize = sendFileVecCount * sizeof(sendfilevec_t);
    size += sendFileVecSize;
#endif

    status = RTL_ALLOCATE(&pCursor, IO_ZCT_CURSOR, size);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pCursor->Size = size;
    pCursor->IoVecOffset = cursorEntrySize;
    pCursor->FreeIoVecOffset = pCursor->IoVecOffset;
#if defined(HAVE_SENDFILEV)
    pCursor->SendFileVecOffset = pCursor->IoVecOffset + ioVecSize;
    pCursor->FreeSendFileVecOffset = pCursor->SendFileVecOffset;
#endif
    pCursor->Count = cursorEntryCount;

cleanup:
    if (status)
    {
        RTL_FREE(&pCursor);
    }

    *ppCursor = pCursor;

    return status;
}

static
struct iovec*
IopZctCursorAllocateIoVec(
    IN OUT PIO_ZCT_CURSOR pCursor,
    IN ULONG Count
    )
{
    struct iovec* pointer = (struct iovec*) LwRtlOffsetToPointer(pCursor, pCursor->FreeIoVecOffset);
    pCursor->FreeIoVecOffset += sizeof(struct iovec) * Count;
#if defined(HAVE_SENDFILEV)
    assert(pCursor->FreeIoVecOffset <= pCursor->SendFileVecOffset);
#else
    assert(pCursor->FreeIoVecOffset <= pCursor->Size);
#endif
    return pointer;
}

#ifdef HAVE_SENDFILEV
static
sendfilevec_t*
IopZctCursorAllocateSendFileVec(
    IN OUT PIO_ZCT_CURSOR pCursor,
    IN ULONG Count
    )
{
    sendfilevec_t* pointer = (sendfilevec_t*) LwRtlOffsetToPointer(pCursor, pCursor->FreeSendFileVecOffset);
    pCursor->FreeSendFileVecOffset += sizeof(sendfilevec_t) * Count;
    assert(pCursor->FreeSendFileVecOffset <= pCursor->Size);
    return pointer;
}
#endif

static
VOID
IopZctCursorInitiazeIoVecCursorEntry(
    IN OUT PIO_ZCT_CURSOR pCursor,
    OUT PIO_ZCT_CURSOR_IOVEC pIoVecCursor,
    IN PIO_ZCT_ENTRY pEntries,
    IN ULONG Count
    )
{
    ULONG i = 0;

    assert(Count > 0);

    pIoVecCursor->Vector = IopZctCursorAllocateIoVec(pCursor, Count);
    pIoVecCursor->Count = Count;

    for (i = 0; i < Count; i++)
    {
        struct iovec* pVector = &pIoVecCursor->Vector[i];
        PIO_ZCT_ENTRY pEntry = &pEntries[i];

        assert(IO_ZCT_ENTRY_TYPE_MEMORY == pEntry->Type);

        pVector->iov_base = pEntry->Data.Memory.Buffer;
        pVector->iov_len = pEntry->Length;
    }
}

static
VOID
IopZctCursorInitiazeSpliceCursorEntry(
    IN OUT PIO_ZCT_CURSOR pCursor,
    OUT PIO_ZCT_CURSOR_SPLICE pSpliceCursor,
    IN PIO_ZCT_ENTRY pEntries,
    IN ULONG Count
    )
{
    PIO_ZCT_ENTRY pEntry = &pEntries[0];

    assert(1 == Count);
    assert(IO_ZCT_ENTRY_TYPE_FD_PIPE == pEntry->Type);

    pSpliceCursor->FileDescriptor = pEntry->Data.FdPipe.Fd;
    pSpliceCursor->Length = pEntry->Length;
}

#if defined(HAVE_SENDFILEV)
static
VOID
IopZctCursorInitiazeSendFileCursorEntry(
    IN OUT PIO_ZCT_CURSOR pCursor,
    OUT PIO_ZCT_CURSOR_SENDFILE pSendFileCursor,
    IN PIO_ZCT_ENTRY pEntries,
    IN ULONG Count
    )
{
    ULONG i = 0;

    assert(Count > 0);

    pSendFileCursor->Vector = IopZctCursorAllocateSendFileVec(pCursor, Count);
    pSendFileCursor->Count = Count;

    for (i = 0; i < Count; i++)
    {
        sendfilevec_t* pVector = &pSendFileCursor->Vector[i];
        PIO_ZCT_ENTRY pEntry = &pEntries[i];

        pVector->sfv_len = pEntry->Length;
        pVector->sfv_flag = 0;

        switch (pEntry->Type)
        {
            case IO_ZCT_ENTRY_TYPE_FD_FILE:
                pVector->sfv_fd = pEntry->Data.FdFile.Fd;
                pVector->sfv_off = pEntry->Data.FdFile.Offset;
                break;
            case IO_ZCT_ENTRY_TYPE_MEMORY:
                pVector->sfv_fd = SFV_FD_SELF;
                pVector->sfv_off = (off_t) pEntry->Data.Memory.Buffer;
                break;
            default:
                assert(FALSE);
                break;
        }
    }
}
#elif defined(HAVE_SENDFILE_HEADER_TRAILER)
static
VOID
IopZctCursorInitiazeSendFileCursorEntry(
    IN OUT PIO_ZCT_CURSOR pCursor,
    OUT PIO_ZCT_CURSOR_SENDFILE pSendFileCursor,
    IN PIO_ZCT_ENTRY pEntries,
    IN ULONG Count
    )
{
    PIO_ZCT_ENTRY pEntry = &pEntries[0];
    ULONG headerCount = 0;
    ULONG trailerCount = 0;

    assert(Count > 0);

    headerCount = IopZctCountRunMemory(pEntry, Count);
    if (headerCount > 0)
    {
        IopZctCursorInitiazeIoVecCursorEntry(
                pCursor,
                &pSendFileCursor->Header,
                pEntry,
                headerCount);
    }

    pEntry += headerCount;

    assert(IO_ZCT_ENTRY_TYPE_FD_FILE == pEntry->Type);

    pSendFileCursor->FileDescriptor = pEntry->Data.FdFile.Fd;
    pSendFileCursor->Offset = pEntry->Data.FdFile.Offset;
    pSendFileCursor->Length = pEntry->Length;

    pEntry += 1;

    trailerCount = Count - headerCount - 1;
    assert(trailerCount < Count);
    if (trailerCount > 0)
    {
        IopZctCursorInitiazeIoVecCursorEntry(
                pCursor,
                &pSendFileCursor->Trailer,
                pEntry,
                trailerCount);
    }
}
#elif defined(HAVE_SENDFILE)
static
VOID
IopZctCursorInitiazeSendFileCursorEntry(
    IN OUT PIO_ZCT_CURSOR pCursor,
    OUT PIO_ZCT_CURSOR_SENDFILE pSendFileCursor,
    IN PIO_ZCT_ENTRY pEntries,
    IN ULONG Count
    )
{
    PIO_ZCT_ENTRY pEntry = &pEntries[0];

    assert(1 == Count);

    pSendFileCursor->FileDescriptor = pEntry->Data.FdFile.Fd;
    pSendFileCursor->Offset = pEntry->Data.FdFile.Offset;
    pSendFileCursor->Length = pEntry->Length;
}
#endif

static
NTSTATUS
IopZctCursorInitializeForSocketIo(
    IN PIO_ZCT pZct,
    IN OUT PIO_ZCT_CURSOR pCursor
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    ULONG i = 0;
    ULONG cursorIndex = 0;

    while (i < pZct->Count)
    {
        PIO_ZCT_CURSOR_ENTRY pCursorEntry = &pCursor->Entry[cursorIndex];
        IO_ZCT_CURSOR_TYPE cursorType = 0;
        ULONG count = 0;

        assert(cursorIndex < pCursor->Count);

        cursorType = IopZctCountRangeForSocketIo(
                            pZct,
                            i,
                            &count);

        pCursorEntry->Type = cursorType;
        pCursorEntry->DebugExtent.Index = i;
        pCursorEntry->DebugExtent.Count = count;

        switch (cursorType)
        {
            case IO_ZCT_CURSOR_TYPE_IOVEC:
                IopZctCursorInitiazeIoVecCursorEntry(
                        pCursor,
                        &pCursorEntry->Data.IoVec,
                        &pZct->Entries[i],
                        count);
                break;

            case IO_ZCT_CURSOR_TYPE_SPLICE:
                IopZctCursorInitiazeSpliceCursorEntry(
                        pCursor,
                        &pCursorEntry->Data.Splice,
                        &pZct->Entries[i],
                        count);
                break;
#if defined(HAVE_SENDFILE_ANY)
            case IO_ZCT_CURSOR_TYPE_SENDFILE:
                IopZctCursorInitiazeSendFileCursorEntry(
                        pCursor,
                        &pCursorEntry->Data.SendFile,
                        &pZct->Entries[i],
                        count);
                break;
#endif
            default:
                assert(FALSE);
                status = STATUS_INTERNAL_ERROR;
                GOTO_CLEANUP_EE(EE);
                break;
        }

        i += count;
        cursorIndex++;
    }

cleanup:
    return status;
}

static
NTSTATUS
IopZctPrepareForSocketIo(
    IN OUT PIO_ZCT pZct
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PIO_ZCT_CURSOR pCursor = NULL;

    status = IopZctCursorAllocateForSocketIo(pZct, &pCursor);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IopZctCursorInitializeForSocketIo(
                    pZct,
                    pCursor);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pZct->Cursor = pCursor;
    pCursor = NULL;

cleanup:
    RTL_FREE(&pCursor);

    return status;
}


NTSTATUS
IoZctReadSocketIo(
    IN OUT PIO_ZCT pZct,
    IN int SocketFd,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    )
{
    return IopZctReadWriteSocket(
                pZct,
                SocketFd,
                FALSE,
                BytesTransferred,
                BytesRemaining);
}

NTSTATUS
IoZctWriteSocketIo(
    IN OUT PIO_ZCT pZct,
    IN int SocketFd,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    )
{
    return IopZctReadWriteSocket(
                pZct,
                SocketFd,
                TRUE,
                BytesTransferred,
                BytesRemaining);
}

static
NTSTATUS
IopZctReadWriteSocket(
    IN OUT PIO_ZCT pZct,
    IN int SocketFd,
    IN BOOLEAN IsWrite,
    OUT OPTIONAL PULONG BytesTransferred,
    OUT OPTIONAL PULONG BytesRemaining
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    ULONG totalBytesTransferred = 0;
    ULONG bytesRemaining = 0;
    IO_ZCT_IO_TYPE ioType = IsWrite ? IO_ZCT_IO_TYPE_WRITE_SOCKET : IO_ZCT_IO_TYPE_READ_SOCKET;

    if (!pZct->Cursor)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    if (!pZct->IoType != ioType)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    if (pZct->Status)
    {
        status = pZct->Status;
        GOTO_CLEANUP_EE(EE);
    }

    while (pZct->Cursor->Index < pZct->Cursor->Count)
    {
        ULONG bytesTransferred = 0;
        BOOLEAN isDoneEntry = FALSE;
        PIO_ZCT_CURSOR_ENTRY pEntry = &pZct->Cursor->Entry[pZct->Cursor->Index];

        status = IopZctCursorEntryReadWriteSocket(
                        SocketFd,
                        IsWrite,
                        pEntry,
                        &bytesTransferred,
                        &isDoneEntry);
        // Handle blocking where we already got some data
        if ((STATUS_MORE_PROCESSING_REQUIRED == status) &&
            (totalBytesTransferred > 0))
        {
            status = STATUS_SUCCESS;
            break;
        }
        GOTO_CLEANUP_EE(EE);

        totalBytesTransferred += bytesTransferred;
        if (isDoneEntry)
        {
            pZct->Cursor->Index++;
        }
    }

cleanup:
    if (status)
    {
        if (STATUS_MORE_PROCESSING_REQUIRED != status)
        {
            // Subsequent I/O should fail
            pZct->Status = status;
        }
        totalBytesTransferred = 0;
        bytesRemaining = 0;
    }
    else
    {
        pZct->BytesTransferred += totalBytesTransferred;
        bytesRemaining = pZct->Length - pZct->BytesTransferred;
    }

    if (BytesTransferred)
    {
        *BytesTransferred = totalBytesTransferred;
    }

    if (BytesRemaining)
    {
        *BytesRemaining = bytesRemaining;
    }

    return status;
}

static
NTSTATUS
IopZctCursorEntryReadWriteSocket(
    IN int SocketFd,
    IN BOOLEAN IsWrite,
    IN OUT PIO_ZCT_CURSOR_ENTRY pEntry,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDoneEntry
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    ULONG bytesTransferred = 0;
    BOOLEAN isDoneEntry = FALSE;

    switch (pEntry->Type)
    {
    case IO_ZCT_CURSOR_TYPE_IOVEC:
    {
        status = IopZctIoVecReadWrite(
                        SocketFd,
                        IsWrite,
                        &pEntry->Data.IoVec,
                        &bytesTransferred,
                        &isDoneEntry);
        break;
    }
#ifdef HAVE_SPLICE
    case IO_ZCT_CURSOR_TYPE_SPLICE:
        status = IopZctSplice(
                        SocketFd,
                        IsWrite,
                        &pEntry->Data.Splice,
                        &bytesTransferred,
                        &isDoneEntry);
        break;
#endif
#ifdef HAVE_SENDFILE_ANY
    case IO_ZCT_CURSOR_TYPE_SENDFILE:
        assert(IsWrite);
        if (!IsWrite)
        {
            status = STATUS_INTERNAL_ERROR;
        }
        else
        {
            status = IopZctSendFile(
                        SocketFd,
                        &pEntry->Data.SendFile,
                        &bytesTransferred,
                        &isDoneEntry);
        }
        break;
#endif
    default:
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (status)
    {
        bytesTransferred = 0;
        isDoneEntry = FALSE;
    }

    *BytesTransferred = bytesTransferred;
    *IsDoneEntry = isDoneEntry;

    return status;
}

NTSTATUS
IopZctIoVecReadWrite(
    IN int FileDescriptor,
    IN BOOLEAN IsWrite,
    IN OUT PIO_ZCT_CURSOR_IOVEC Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    struct iovec* vector = &Cursor->Vector[Cursor->Index];
    int count = Cursor->Count - Cursor->Index;
    ssize_t result = 0;
    ULONG bytesTransferred = 0;
    BOOLEAN isDone = FALSE;
    int i = 0;

    if (IsWrite)
    {
        result = readv(FileDescriptor, vector, count);
    }
    else
    {
        result = writev(FileDescriptor, vector, count);
    }
    if (result < 0)
    {
        int error = errno;
        if ((EAGAIN == error) || (EWOULDBLOCK == error))
        {
            status = STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            status = LwErrnoToNtStatus(error);
        }
        GOTO_CLEANUP_EE(EE);
        // unreachable
        assert(FALSE);
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }

    assert(result <= LW_MAXULONG);

    bytesTransferred = (ULONG) result;

    for (i = 0; i < count; i++)
    {
        if (result >= vector[i].iov_len)
        {
            // Note: Do not need to zero since we are moving on.
            // vector[i].iov_len = 0;
            result -= vector[i].iov_len;
            Cursor->Index++;

            if (0 == result)
            {
                break;
            }
        }
        else
        {
            vector[i].iov_base = LwRtlOffsetToPointer(vector[i].iov_base, result);
            vector[i].iov_len -= result;
            result -= result;
            break;
        }
        assert(result > 0);
    }

    assert(Cursor->Index <= Cursor->Count);

    if (Cursor->Index == Cursor->Count)
    {
        isDone = TRUE;
    }

cleanup:
    if (status)
    {
        bytesTransferred = 0;
        isDone = FALSE;
    }

    *BytesTransferred = bytesTransferred;
    *IsDone = isDone;

    return status;
}

#if defined(HAVE_SPLICE)
static
NTSTATUS
IopZctSplice(
    IN int FileDescriptor,
    IN BOOLEAN IsWrite,
    IN OUT PIO_ZCT_CURSOR_SPLICE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    long result = 0;
    ULONG bytesTransferred = 0;
    BOOLEAN isDone = FALSE;

    if (IsWrite)
    {
        result = splice(Cursor->FileDescriptor,
                        NULL,
                        FileDescriptor,
                        NULL,
                        Cursor->Length,
                        SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
    }
    else
    {
        result = splice(FileDescriptor,
                        NULL,
                        Cursor->FileDescriptor,
                        NULL,
                        Cursor->Length,
                        SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
    }
    if (result < 0)
    {
        int error = errno;
        if ((EAGAIN == error) || (EWOULDBLOCK == error))
        {
            status = STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            status = LwErrnoToNtStatus(error);
        }
        GOTO_CLEANUP_EE(EE);
        // unreachable
        assert(FALSE);
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }
    if (0 == result)
    {
        // TODO: Need to investigate semantics of this case.
    }

    assert(result <= Cursor->Length);

    bytesTransferred = (ULONG) result;

    if (bytesTransferred < Cursor->Length)
    {
        Cursor->Length -= bytesTransferred;
    }
    else
    {
        isDone = TRUE;
    }

cleanup:
    if (status)
    {
        bytesTransferred = 0;
        isDone = FALSE;
    }

    *BytesTransferred = bytesTransferred;
    *IsDone = isDone;

    return status;
}
#endif

#if defined(HAVE_SENDFILEV)
static
NTSTATUS
IopZctSendFile(
    IN int FileDescriptor,
    IN OUT PIO_ZCT_CURSOR_SENDFILE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
#elif defined (HAVE_SENDFILE_HEADER_TRAILER)
static
NTSTATUS
IopZctSendFile(
    IN int FileDescriptor,
    IN OUT PIO_ZCT_CURSOR_SENDFILE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
#elif defined (HAVE_SENDFILE)
static
NTSTATUS
IopZctSendFile(
    IN int FileDescriptor,
    IN OUT PIO_ZCT_CURSOR_SENDFILE Cursor,
    OUT PULONG BytesTransferred,
    OUT PBOOLEAN IsDone
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    ssize_t result = 0;
    ULONG bytesTransferred = 0;
    BOOLEAN isDone = FALSE;

    result = sendfile(
                    FileDescriptor,
                    Cursor->FileDescriptor,
                    &Cursor->Offset,
                    Cursor->Length);
    if (result < 0)
    {
        int error = errno;
        if ((EAGAIN == error) || (EWOULDBLOCK == error))
        {
            status = STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            status = LwErrnoToNtStatus(error);
        }
        GOTO_CLEANUP_EE(EE);
        // unreachable
        assert(FALSE);
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }

    assert(result <= Cursor->Length);

    bytesTransferred = (ULONG) result;

    if (bytesTransferred < Cursor->Length)
    {
        Cursor->Offset += bytesTransferred;
        Cursor->Length -= bytesTransferred;
    }
    else
    {
        isDone = TRUE;
    }

cleanup:
    if (status)
    {
        bytesTransferred = 0;
        isDone = FALSE;
    }

    *BytesTransferred = bytesTransferred;
    *IsDone = isDone;

    return status;
}
#endif

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
