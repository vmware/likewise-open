/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        base.h
 *
 * Abstract:
 *
 *        NT status codes
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#ifndef __LWBASE_NTSTATUS_H__
#define __LWBASE_NTSTATUS_H__

#include <lw/types.h>

//
// NTSTATUS
//
// NTSTATUS is similar to HRESULT in that it is a 32-bit integer with the format:
//
//     <SS|X|R|FFFFFFFFFFFF|CCCCCCCCCCCCCCC>
//              (12 bits)     (16 bits)
//
//   S - severity level (2 bits):
//
//       00 - success
//       01 - informational
//       10 - warning
//       11 - error
//
//   X - customer code bit (1 bit)
//
//   R - reserved bit (1 bit)
//
//   F - facility code (12 bits)
//
//   C - facility's status code (16 bits)
//
// Defined facility codes:
// 0 - FACILITY_SYSTEM
// 2 - FACILITY_RPC_RUNTIME
// 3 - FACILITY_RPC_STUBS
//
// In general, stay away from informational and success codes.
// That being said, some system facilities require the use of
// success codes.  The most important of the success codes is
// STATUS_PENDING.
//

typedef LW_LONG LW_NTSTATUS, *LW_PNTSTATUS;

// Determine whether this is a failure code.  Only warning and
// error codes are considered failure.
#define LW_NT_SUCCESS(status) \
    (((LW_LONG)(status)) >= 0)

// Determines whether NTSTATUS is 0 or !NT_SUCCESS(status).
// This helps check for non-zero success code from APIs.
// TODO--Need a better name for this macro?
#define LW_NT_SUCCESS_OR_NOT(status) \
    (!LW_NT_SUCCESS(status) || (0 == (status)))

//
// NTSTATUS codes (sorted numerically)
//

// Success status codes
#define LW_STATUS_SUCCESS                                         ((LW_NTSTATUS) 0x00000000)
#define LW_STATUS_PENDING                                         ((LW_NTSTATUS) 0x00000103)
#define LW_STATUS_MORE_ENTRIES                                    ((LW_NTSTATUS) 0x00000105)
#define LW_STATUS_SOME_UNMAPPED                                   ((LW_NTSTATUS) 0x00000107)

// Warning status codes
#define LW_STATUS_BUFFER_OVERFLOW                                 ((LW_NTSTATUS) 0x80000005)
#define LW_STATUS_NO_MORE_ENTRIES                                 ((LW_NTSTATUS) 0x8000001a)

// Error status codes
#define LW_STATUS_UNSUCCESSFUL                                    ((LW_NTSTATUS) 0xc0000001)
#define LW_STATUS_NOT_IMPLEMENTED                                 ((LW_NTSTATUS) 0xc0000002)
#define LW_STATUS_INVALID_INFO_CLASS                              ((LW_NTSTATUS) 0xc0000003)
#define LW_STATUS_INFO_LENGTH_MISMATCH                            ((LW_NTSTATUS) 0xc0000004)
#define LW_STATUS_ACCESS_VIOLATION                                ((LW_NTSTATUS) 0xc0000005)
#define LW_STATUS_IN_PAGE_ERROR                                   ((LW_NTSTATUS) 0xc0000006)
#define LW_STATUS_PAGEFILE_QUOTA                                  ((LW_NTSTATUS) 0xc0000007)
#define LW_STATUS_INVALID_HANDLE                                  ((LW_NTSTATUS) 0xc0000008)
#define LW_STATUS_BAD_INITIAL_STACK                               ((LW_NTSTATUS) 0xc0000009)
#define LW_STATUS_BAD_INITIAL_PC                                  ((LW_NTSTATUS) 0xc000000a)
#define LW_STATUS_INVALID_CID                                     ((LW_NTSTATUS) 0xc000000b)
#define LW_STATUS_TIMER_NOT_CANCELED                              ((LW_NTSTATUS) 0xc000000c)
#define LW_STATUS_INVALID_PARAMETER                               ((LW_NTSTATUS) 0xc000000d)
#define LW_STATUS_NO_SUCH_DEVICE                                  ((LW_NTSTATUS) 0xc000000e)
#define LW_STATUS_NO_SUCH_FILE                                    ((LW_NTSTATUS) 0xc000000f)
#define LW_STATUS_INVALID_DEVICE_REQUEST                          ((LW_NTSTATUS) 0xc0000010)
#define LW_STATUS_END_OF_FILE                                     ((LW_NTSTATUS) 0xc0000011)
#define LW_STATUS_WRONG_VOLUME                                    ((LW_NTSTATUS) 0xc0000012)
#define LW_STATUS_NO_MEDIA_IN_DEVICE                              ((LW_NTSTATUS) 0xc0000013)
#define LW_STATUS_UNRECOGNIZED_MEDIA                              ((LW_NTSTATUS) 0xc0000014)
#define LW_STATUS_NONEXISTENT_SECTOR                              ((LW_NTSTATUS) 0xc0000015)
#define LW_STATUS_MORE_PROCESSING_REQUIRED                        ((LW_NTSTATUS) 0xc0000016)
#define LW_STATUS_NO_MEMORY                                       ((LW_NTSTATUS) 0xc0000017)
#define LW_STATUS_CONFLICTING_ADDRESSES                           ((LW_NTSTATUS) 0xc0000018)
#define LW_STATUS_NOT_MAPPED_VIEW                                 ((LW_NTSTATUS) 0xc0000019)
#define LW_STATUS_UNABLE_TO_FREE_VM                               ((LW_NTSTATUS) 0xc000001a)
#define LW_STATUS_UNABLE_TO_DELETE_SECTION                        ((LW_NTSTATUS) 0xc000001b)
#define LW_STATUS_INVALID_SYSTEM_SERVICE                          ((LW_NTSTATUS) 0xc000001c)
#define LW_STATUS_ILLEGAL_INSTRUCTION                             ((LW_NTSTATUS) 0xc000001d)
#define LW_STATUS_INVALID_LOCK_SEQUENCE                           ((LW_NTSTATUS) 0xc000001e)
#define LW_STATUS_INVALID_VIEW_SIZE                               ((LW_NTSTATUS) 0xc000001f)
#define LW_STATUS_INVALID_FILE_FOR_SECTION                        ((LW_NTSTATUS) 0xc0000020)
#define LW_STATUS_ALREADY_COMMITTED                               ((LW_NTSTATUS) 0xc0000021)
#define LW_STATUS_ACCESS_DENIED                                   ((LW_NTSTATUS) 0xc0000022)
#define LW_STATUS_BUFFER_TOO_SMALL                                ((LW_NTSTATUS) 0xc0000023)
#define LW_STATUS_OBJECT_TYPE_MISMATCH                            ((LW_NTSTATUS) 0xc0000024)
#define LW_STATUS_NONCONTINUABLE_EXCEPTION                        ((LW_NTSTATUS) 0xc0000025)
#define LW_STATUS_INVALID_DISPOSITION                             ((LW_NTSTATUS) 0xc0000026)
#define LW_STATUS_UNWIND                                          ((LW_NTSTATUS) 0xc0000027)
#define LW_STATUS_BAD_STACK                                       ((LW_NTSTATUS) 0xc0000028)
#define LW_STATUS_INVALID_UNWIND_TARGET                           ((LW_NTSTATUS) 0xc0000029)
#define LW_STATUS_NOT_LOCKED                                      ((LW_NTSTATUS) 0xc000002a)
#define LW_STATUS_PARITY_ERROR                                    ((LW_NTSTATUS) 0xc000002b)
#define LW_STATUS_UNABLE_TO_DECOMMIT_VM                           ((LW_NTSTATUS) 0xc000002c)
#define LW_STATUS_NOT_COMMITTED                                   ((LW_NTSTATUS) 0xc000002d)
#define LW_STATUS_INVALID_PORT_ATTRIBUTES                         ((LW_NTSTATUS) 0xc000002e)
#define LW_STATUS_PORT_MESSAGE_TOO_LONG                           ((LW_NTSTATUS) 0xc000002f)
#define LW_STATUS_INVALID_PARAMETER_MIX                           ((LW_NTSTATUS) 0xc0000030)
#define LW_STATUS_INVALID_QUOTA_LOWER                             ((LW_NTSTATUS) 0xc0000031)
#define LW_STATUS_DISK_CORRUPT_ERROR                              ((LW_NTSTATUS) 0xc0000032)
#define LW_STATUS_OBJECT_NAME_INVALID                             ((LW_NTSTATUS) 0xc0000033)
#define LW_STATUS_OBJECT_NAME_NOT_FOUND                           ((LW_NTSTATUS) 0xc0000034)
#define LW_STATUS_OBJECT_NAME_COLLISION                           ((LW_NTSTATUS) 0xc0000035)
#define LW_STATUS_PORT_DISCONNECTED                               ((LW_NTSTATUS) 0xc0000037)
#define LW_STATUS_DEVICE_ALREADY_ATTACHED                         ((LW_NTSTATUS) 0xc0000038)
#define LW_STATUS_OBJECT_PATH_INVALID                             ((LW_NTSTATUS) 0xc0000039)
#define LW_STATUS_OBJECT_PATH_NOT_FOUND                           ((LW_NTSTATUS) 0xc000003a)
#define LW_STATUS_OBJECT_PATH_SYNTAX_BAD                          ((LW_NTSTATUS) 0xc000003b)
#define LW_STATUS_DATA_OVERRUN                                    ((LW_NTSTATUS) 0xc000003c)
#define LW_STATUS_DATA_LATE_ERROR                                 ((LW_NTSTATUS) 0xc000003d)
#define LW_STATUS_DATA_ERROR                                      ((LW_NTSTATUS) 0xc000003e)
#define LW_STATUS_CRC_ERROR                                       ((LW_NTSTATUS) 0xc000003f)
#define LW_STATUS_SECTION_TOO_BIG                                 ((LW_NTSTATUS) 0xc0000040)
#define LW_STATUS_PORT_CONNECTION_REFUSED                         ((LW_NTSTATUS) 0xc0000041)
#define LW_STATUS_INVALID_PORT_HANDLE                             ((LW_NTSTATUS) 0xc0000042)
#define LW_STATUS_SHARING_VIOLATION                               ((LW_NTSTATUS) 0xc0000043)
#define LW_STATUS_QUOTA_EXCEEDED                                  ((LW_NTSTATUS) 0xc0000044)
#define LW_STATUS_INVALID_PAGE_PROTECTION                         ((LW_NTSTATUS) 0xc0000045)
#define LW_STATUS_MUTANT_NOT_OWNED                                ((LW_NTSTATUS) 0xc0000046)
#define LW_STATUS_SEMAPHORE_LIMIT_EXCEEDED                        ((LW_NTSTATUS) 0xc0000047)
#define LW_STATUS_PORT_ALREADY_SET                                ((LW_NTSTATUS) 0xc0000048)
#define LW_STATUS_SECTION_NOT_IMAGE                               ((LW_NTSTATUS) 0xc0000049)
#define LW_STATUS_SUSPEND_COUNT_EXCEEDED                          ((LW_NTSTATUS) 0xc000004a)
#define LW_STATUS_THREAD_IS_TERMINATING                           ((LW_NTSTATUS) 0xc000004b)
#define LW_STATUS_BAD_WORKING_SET_LIMIT                           ((LW_NTSTATUS) 0xc000004c)
#define LW_STATUS_INCOMPATIBLE_FILE_MAP                           ((LW_NTSTATUS) 0xc000004d)
#define LW_STATUS_SECTION_PROTECTION                              ((LW_NTSTATUS) 0xc000004e)
#define LW_STATUS_EAS_NOT_SUPPORTED                               ((LW_NTSTATUS) 0xc000004f)
#define LW_STATUS_EA_TOO_LARGE                                    ((LW_NTSTATUS) 0xc0000050)
#define LW_STATUS_NONEXISTENT_EA_ENTRY                            ((LW_NTSTATUS) 0xc0000051)
#define LW_STATUS_NO_EAS_ON_FILE                                  ((LW_NTSTATUS) 0xc0000052)
#define LW_STATUS_EA_CORRUPT_ERROR                                ((LW_NTSTATUS) 0xc0000053)
#define LW_STATUS_FILE_LOCK_CONFLICT                              ((LW_NTSTATUS) 0xc0000054)
#define LW_STATUS_LOCK_NOT_GRANTED                                ((LW_NTSTATUS) 0xc0000055)
#define LW_STATUS_DELETE_PENDING                                  ((LW_NTSTATUS) 0xc0000056)
#define LW_STATUS_CTL_FILE_NOT_SUPPORTED                          ((LW_NTSTATUS) 0xc0000057)
#define LW_STATUS_UNKNOWN_REVISION                                ((LW_NTSTATUS) 0xc0000058)
#define LW_STATUS_REVISION_MISMATCH                               ((LW_NTSTATUS) 0xc0000059)
#define LW_STATUS_INVALID_OWNER                                   ((LW_NTSTATUS) 0xc000005a)
#define LW_STATUS_INVALID_PRIMARY_GROUP                           ((LW_NTSTATUS) 0xc000005b)
#define LW_STATUS_NO_IMPERSONATION_TOKEN                          ((LW_NTSTATUS) 0xc000005c)
#define LW_STATUS_CANT_DISABLE_MANDATORY                          ((LW_NTSTATUS) 0xc000005d)
#define LW_STATUS_NO_LOGON_SERVERS                                ((LW_NTSTATUS) 0xc000005e)
#define LW_STATUS_NO_SUCH_LOGON_SESSION                           ((LW_NTSTATUS) 0xc000005f)
#define LW_STATUS_NO_SUCH_PRIVILEGE                               ((LW_NTSTATUS) 0xc0000060)
#define LW_STATUS_PRIVILEGE_NOT_HELD                              ((LW_NTSTATUS) 0xc0000061)
#define LW_STATUS_INVALID_ACCOUNT_NAME                            ((LW_NTSTATUS) 0xc0000062)
#define LW_STATUS_USER_EXISTS                                     ((LW_NTSTATUS) 0xc0000063)
#define LW_STATUS_NO_SUCH_USER                                    ((LW_NTSTATUS) 0xc0000064)
#define LW_STATUS_GROUP_EXISTS                                    ((LW_NTSTATUS) 0xc0000065)
#define LW_STATUS_NO_SUCH_GROUP                                   ((LW_NTSTATUS) 0xc0000066)
#define LW_STATUS_MEMBER_IN_GROUP                                 ((LW_NTSTATUS) 0xc0000067)
#define LW_STATUS_MEMBER_NOT_IN_GROUP                             ((LW_NTSTATUS) 0xc0000068)
#define LW_STATUS_LAST_ADMIN                                      ((LW_NTSTATUS) 0xc0000069)
#define LW_STATUS_WRONG_PASSWORD                                  ((LW_NTSTATUS) 0xc000006a)
#define LW_STATUS_ILL_FORMED_PASSWORD                             ((LW_NTSTATUS) 0xc000006b)
#define LW_STATUS_PASSWORD_RESTRICTION                            ((LW_NTSTATUS) 0xc000006c)
#define LW_STATUS_LOGON_FAILURE                                   ((LW_NTSTATUS) 0xc000006d)
#define LW_STATUS_ACCOUNT_RESTRICTION                             ((LW_NTSTATUS) 0xc000006e)
#define LW_STATUS_INVALID_LOGON_HOURS                             ((LW_NTSTATUS) 0xc000006f)
#define LW_STATUS_INVALID_WORKSTATION                             ((LW_NTSTATUS) 0xc0000070)
#define LW_STATUS_PASSWORD_EXPIRED                                ((LW_NTSTATUS) 0xc0000071)
#define LW_STATUS_ACCOUNT_DISABLED                                ((LW_NTSTATUS) 0xc0000072)
#define LW_STATUS_NONE_MAPPED                                     ((LW_NTSTATUS) 0xc0000073)
#define LW_STATUS_TOO_MANY_LUIDS_REQUESTED                        ((LW_NTSTATUS) 0xc0000074)
#define LW_STATUS_LUIDS_EXHAUSTED                                 ((LW_NTSTATUS) 0xc0000075)
#define LW_STATUS_INVALID_SUB_AUTHORITY                           ((LW_NTSTATUS) 0xc0000076)
#define LW_STATUS_INVALID_ACL                                     ((LW_NTSTATUS) 0xc0000077)
#define LW_STATUS_INVALID_SID                                     ((LW_NTSTATUS) 0xc0000078)
#define LW_STATUS_INVALID_SECURITY_DESCR                          ((LW_NTSTATUS) 0xc0000079)
#define LW_STATUS_PROCEDURE_NOT_FOUND                             ((LW_NTSTATUS) 0xc000007a)
#define LW_STATUS_INVALID_IMAGE_FORMAT                            ((LW_NTSTATUS) 0xc000007b)
#define LW_STATUS_NO_TOKEN                                        ((LW_NTSTATUS) 0xc000007c)
#define LW_STATUS_BAD_INHERITANCE_ACL                             ((LW_NTSTATUS) 0xc000007d)
#define LW_STATUS_RANGE_NOT_LOCKED                                ((LW_NTSTATUS) 0xc000007e)
#define LW_STATUS_DISK_FULL                                       ((LW_NTSTATUS) 0xc000007f)
#define LW_STATUS_SERVER_DISABLED                                 ((LW_NTSTATUS) 0xc0000080)
#define LW_STATUS_SERVER_NOT_DISABLED                             ((LW_NTSTATUS) 0xc0000081)
#define LW_STATUS_TOO_MANY_GUIDS_REQUESTED                        ((LW_NTSTATUS) 0xc0000082)
#define LW_STATUS_GUIDS_EXHAUSTED                                 ((LW_NTSTATUS) 0xc0000083)
#define LW_STATUS_INVALID_ID_AUTHORITY                            ((LW_NTSTATUS) 0xc0000084)
#define LW_STATUS_AGENTS_EXHAUSTED                                ((LW_NTSTATUS) 0xc0000085)
#define LW_STATUS_INVALID_VOLUME_LABEL                            ((LW_NTSTATUS) 0xc0000086)
#define LW_STATUS_SECTION_NOT_EXTENDED                            ((LW_NTSTATUS) 0xc0000087)
#define LW_STATUS_NOT_MAPPED_DATA                                 ((LW_NTSTATUS) 0xc0000088)
#define LW_STATUS_RESOURCE_DATA_NOT_FOUND                         ((LW_NTSTATUS) 0xc0000089)
#define LW_STATUS_RESOURCE_TYPE_NOT_FOUND                         ((LW_NTSTATUS) 0xc000008a)
#define LW_STATUS_RESOURCE_NAME_NOT_FOUND                         ((LW_NTSTATUS) 0xc000008b)
#define LW_STATUS_ARRAY_BOUNDS_EXCEEDED                           ((LW_NTSTATUS) 0xc000008c)
#define LW_STATUS_FLOAT_DENORMAL_OPERAND                          ((LW_NTSTATUS) 0xc000008d)
#define LW_STATUS_FLOAT_DIVIDE_BY_ZERO                            ((LW_NTSTATUS) 0xc000008e)
#define LW_STATUS_FLOAT_INEXACT_RESULT                            ((LW_NTSTATUS) 0xc000008f)
#define LW_STATUS_FLOAT_INVALID_OPERATION                         ((LW_NTSTATUS) 0xc0000090)
#define LW_STATUS_FLOAT_OVERFLOW                                  ((LW_NTSTATUS) 0xc0000091)
#define LW_STATUS_FLOAT_STACK_CHECK                               ((LW_NTSTATUS) 0xc0000092)
#define LW_STATUS_FLOAT_UNDERFLOW                                 ((LW_NTSTATUS) 0xc0000093)
#define LW_STATUS_INTEGER_DIVIDE_BY_ZERO                          ((LW_NTSTATUS) 0xc0000094)
#define LW_STATUS_INTEGER_OVERFLOW                                ((LW_NTSTATUS) 0xc0000095)
#define LW_STATUS_PRIVILEGED_INSTRUCTION                          ((LW_NTSTATUS) 0xc0000096)
#define LW_STATUS_TOO_MANY_PAGING_FILES                           ((LW_NTSTATUS) 0xc0000097)
#define LW_STATUS_FILE_INVALID                                    ((LW_NTSTATUS) 0xc0000098)
#define LW_STATUS_ALLOTTED_SPACE_EXCEEDED                         ((LW_NTSTATUS) 0xc0000099)
#define LW_STATUS_INSUFFICIENT_RESOURCES                          ((LW_NTSTATUS) 0xc000009a)
#define LW_STATUS_DFS_EXIT_PATH_FOUND                             ((LW_NTSTATUS) 0xc000009b)
#define LW_STATUS_DEVICE_DATA_ERROR                               ((LW_NTSTATUS) 0xc000009c)
#define LW_STATUS_DEVICE_NOT_CONNECTED                            ((LW_NTSTATUS) 0xc000009d)
#define LW_STATUS_DEVICE_POWER_FAILURE                            ((LW_NTSTATUS) 0xc000009e)
#define LW_STATUS_FREE_VM_NOT_AT_BASE                             ((LW_NTSTATUS) 0xc000009f)
#define LW_STATUS_MEMORY_NOT_ALLOCATED                            ((LW_NTSTATUS) 0xc00000a0)
#define LW_STATUS_WORKING_SET_QUOTA                               ((LW_NTSTATUS) 0xc00000a1)
#define LW_STATUS_MEDIA_WRITE_PROTECTED                           ((LW_NTSTATUS) 0xc00000a2)
#define LW_STATUS_DEVICE_NOT_READY                                ((LW_NTSTATUS) 0xc00000a3)
#define LW_STATUS_INVALID_GROUP_ATTRIBUTES                        ((LW_NTSTATUS) 0xc00000a4)
#define LW_STATUS_BAD_IMPERSONATION_LEVEL                         ((LW_NTSTATUS) 0xc00000a5)
#define LW_STATUS_CANT_OPEN_ANONYMOUS                             ((LW_NTSTATUS) 0xc00000a6)
#define LW_STATUS_BAD_VALIDATION_CLASS                            ((LW_NTSTATUS) 0xc00000a7)
#define LW_STATUS_BAD_TOKEN_TYPE                                  ((LW_NTSTATUS) 0xc00000a8)
#define LW_STATUS_BAD_MASTER_BOOT_RECORD                          ((LW_NTSTATUS) 0xc00000a9)
#define LW_STATUS_INSTRUCTION_MISALIGNMENT                        ((LW_NTSTATUS) 0xc00000aa)
#define LW_STATUS_INSTANCE_NOT_AVAILABLE                          ((LW_NTSTATUS) 0xc00000ab)
#define LW_STATUS_PIPE_NOT_AVAILABLE                              ((LW_NTSTATUS) 0xc00000ac)
#define LW_STATUS_INVALID_PIPE_STATE                              ((LW_NTSTATUS) 0xc00000ad)
#define LW_STATUS_PIPE_BUSY                                       ((LW_NTSTATUS) 0xc00000ae)
#define LW_STATUS_ILLEGAL_FUNCTION                                ((LW_NTSTATUS) 0xc00000af)
#define LW_STATUS_PIPE_DISCONNECTED                               ((LW_NTSTATUS) 0xc00000b0)
#define LW_STATUS_PIPE_CLOSING                                    ((LW_NTSTATUS) 0xc00000b1)
#define LW_STATUS_PIPE_CONNECTED                                  ((LW_NTSTATUS) 0xc00000b2)
#define LW_STATUS_PIPE_LISTENING                                  ((LW_NTSTATUS) 0xc00000b3)
#define LW_STATUS_INVALID_READ_MODE                               ((LW_NTSTATUS) 0xc00000b4)
#define LW_STATUS_IO_TIMEOUT                                      ((LW_NTSTATUS) 0xc00000b5)
#define LW_STATUS_FILE_FORCED_CLOSED                              ((LW_NTSTATUS) 0xc00000b6)
#define LW_STATUS_PROFILING_NOT_STARTED                           ((LW_NTSTATUS) 0xc00000b7)
#define LW_STATUS_PROFILING_NOT_STOPPED                           ((LW_NTSTATUS) 0xc00000b8)
#define LW_STATUS_COULD_NOT_INTERPRET                             ((LW_NTSTATUS) 0xc00000b9)
#define LW_STATUS_FILE_IS_A_DIRECTORY                             ((LW_NTSTATUS) 0xc00000ba)
#define LW_STATUS_NOT_SUPPORTED                                   ((LW_NTSTATUS) 0xc00000bb)
#define LW_STATUS_REMOTE_NOT_LISTENING                            ((LW_NTSTATUS) 0xc00000bc)
#define LW_STATUS_DUPLICATE_NAME                                  ((LW_NTSTATUS) 0xc00000bd)
#define LW_STATUS_BAD_NETWORK_PATH                                ((LW_NTSTATUS) 0xc00000be)
#define LW_STATUS_NETWORK_BUSY                                    ((LW_NTSTATUS) 0xc00000bf)
#define LW_STATUS_DEVICE_DOES_NOT_EXIST                           ((LW_NTSTATUS) 0xc00000c0)
#define LW_STATUS_TOO_MANY_COMMANDS                               ((LW_NTSTATUS) 0xc00000c1)
#define LW_STATUS_ADAPTER_HARDWARE_ERROR                          ((LW_NTSTATUS) 0xc00000c2)
#define LW_STATUS_INVALID_NETWORK_RESPONSE                        ((LW_NTSTATUS) 0xc00000c3)
#define LW_STATUS_UNEXPECTED_NETWORK_ERROR                        ((LW_NTSTATUS) 0xc00000c4)
#define LW_STATUS_BAD_REMOTE_ADAPTER                              ((LW_NTSTATUS) 0xc00000c5)
#define LW_STATUS_PRINT_QUEUE_FULL                                ((LW_NTSTATUS) 0xc00000c6)
#define LW_STATUS_NO_SPOOL_SPACE                                  ((LW_NTSTATUS) 0xc00000c7)
#define LW_STATUS_PRINT_CANCELLED                                 ((LW_NTSTATUS) 0xc00000c8)
#define LW_STATUS_NETWORK_ACCESS_DENIED                           ((LW_NTSTATUS) 0xc00000ca)
#define LW_STATUS_BAD_DEVICE_TYPE                                 ((LW_NTSTATUS) 0xc00000cb)
#define LW_STATUS_BAD_NETWORK_NAME                                ((LW_NTSTATUS) 0xc00000cc)
#define LW_STATUS_TOO_MANY_NAMES                                  ((LW_NTSTATUS) 0xc00000cd)
#define LW_STATUS_TOO_MANY_SESSIONS                               ((LW_NTSTATUS) 0xc00000ce)
#define LW_STATUS_SHARING_PAUSED                                  ((LW_NTSTATUS) 0xc00000cf)
#define LW_STATUS_REQUEST_NOT_ACCEPTED                            ((LW_NTSTATUS) 0xc00000d0)
#define LW_STATUS_REDIRECTOR_PAUSED                               ((LW_NTSTATUS) 0xc00000d1)
#define LW_STATUS_NET_WRITE_FAULT                                 ((LW_NTSTATUS) 0xc00000d2)
#define LW_STATUS_PROFILING_AT_LIMIT                              ((LW_NTSTATUS) 0xc00000d3)
#define LW_STATUS_NOT_SAME_DEVICE                                 ((LW_NTSTATUS) 0xc00000d4)
#define LW_STATUS_FILE_RENAMED                                    ((LW_NTSTATUS) 0xc00000d5)
#define LW_STATUS_VIRTUAL_CIRCUIT_CLOSED                          ((LW_NTSTATUS) 0xc00000d6)
#define LW_STATUS_NO_SECURITY_ON_OBJECT                           ((LW_NTSTATUS) 0xc00000d7)
#define LW_STATUS_CANT_WAIT                                       ((LW_NTSTATUS) 0xc00000d8)
#define LW_STATUS_PIPE_EMPTY                                      ((LW_NTSTATUS) 0xc00000d9)
#define LW_STATUS_CANT_ACCESS_DOMAIN_INFO                         ((LW_NTSTATUS) 0xc00000da)
#define LW_STATUS_CANT_TERMINATE_SELF                             ((LW_NTSTATUS) 0xc00000db)
#define LW_STATUS_INVALID_SERVER_STATE                            ((LW_NTSTATUS) 0xc00000dc)
#define LW_STATUS_INVALID_DOMAIN_STATE                            ((LW_NTSTATUS) 0xc00000dd)
#define LW_STATUS_INVALID_DOMAIN_ROLE                             ((LW_NTSTATUS) 0xc00000de)
#define LW_STATUS_NO_SUCH_DOMAIN                                  ((LW_NTSTATUS) 0xc00000df)
#define LW_STATUS_DOMAIN_EXISTS                                   ((LW_NTSTATUS) 0xc00000e0)
#define LW_STATUS_DOMAIN_LIMIT_EXCEEDED                           ((LW_NTSTATUS) 0xc00000e1)
#define LW_STATUS_OPLOCK_NOT_GRANTED                              ((LW_NTSTATUS) 0xc00000e2)
#define LW_STATUS_INVALID_OPLOCK_PROTOCOL                         ((LW_NTSTATUS) 0xc00000e3)
#define LW_STATUS_INTERNAL_DB_CORRUPTION                          ((LW_NTSTATUS) 0xc00000e4)
#define LW_STATUS_INTERNAL_ERROR                                  ((LW_NTSTATUS) 0xc00000e5)
#define LW_STATUS_GENERIC_NOT_MAPPED                              ((LW_NTSTATUS) 0xc00000e6)
#define LW_STATUS_BAD_DESCRIPTOR_FORMAT                           ((LW_NTSTATUS) 0xc00000e7)
#define LW_STATUS_INVALID_USER_BUFFER                             ((LW_NTSTATUS) 0xc00000e8)
#define LW_STATUS_UNEXPECTED_IO_ERROR                             ((LW_NTSTATUS) 0xc00000e9)
#define LW_STATUS_UNEXPECTED_MM_CREATE_ERR                        ((LW_NTSTATUS) 0xc00000ea)
#define LW_STATUS_UNEXPECTED_MM_MAP_ERROR                         ((LW_NTSTATUS) 0xc00000eb)
#define LW_STATUS_UNEXPECTED_MM_EXTEND_ERR                        ((LW_NTSTATUS) 0xc00000ec)
#define LW_STATUS_NOT_LOGON_PROCESS                               ((LW_NTSTATUS) 0xc00000ed)
#define LW_STATUS_LOGON_SESSION_EXISTS                            ((LW_NTSTATUS) 0xc00000ee)
#define LW_STATUS_INVALID_PARAMETER_1                             ((LW_NTSTATUS) 0xc00000ef)
#define LW_STATUS_INVALID_PARAMETER_2                             ((LW_NTSTATUS) 0xc00000f0)
#define LW_STATUS_INVALID_PARAMETER_3                             ((LW_NTSTATUS) 0xc00000f1)
#define LW_STATUS_INVALID_PARAMETER_4                             ((LW_NTSTATUS) 0xc00000f2)
#define LW_STATUS_INVALID_PARAMETER_5                             ((LW_NTSTATUS) 0xc00000f3)
#define LW_STATUS_INVALID_PARAMETER_6                             ((LW_NTSTATUS) 0xc00000f4)
#define LW_STATUS_INVALID_PARAMETER_7                             ((LW_NTSTATUS) 0xc00000f5)
#define LW_STATUS_INVALID_PARAMETER_8                             ((LW_NTSTATUS) 0xc00000f6)
#define LW_STATUS_INVALID_PARAMETER_9                             ((LW_NTSTATUS) 0xc00000f7)
#define LW_STATUS_INVALID_PARAMETER_10                            ((LW_NTSTATUS) 0xc00000f8)
#define LW_STATUS_INVALID_PARAMETER_11                            ((LW_NTSTATUS) 0xc00000f9)
#define LW_STATUS_INVALID_PARAMETER_12                            ((LW_NTSTATUS) 0xc00000fa)
#define LW_STATUS_REDIRECTOR_NOT_STARTED                          ((LW_NTSTATUS) 0xc00000fb)
#define LW_STATUS_REDIRECTOR_STARTED                              ((LW_NTSTATUS) 0xc00000fc)
#define LW_STATUS_STACK_OVERFLOW                                  ((LW_NTSTATUS) 0xc00000fd)
#define LW_STATUS_NO_SUCH_PACKAGE                                 ((LW_NTSTATUS) 0xc00000fe)
#define LW_STATUS_BAD_FUNCTION_TABLE                              ((LW_NTSTATUS) 0xc00000ff)
#define LW_STATUS_VARIABLE_NOT_FOUND                              ((LW_NTSTATUS) 0xc0000100)
#define LW_STATUS_DIRECTORY_NOT_EMPTY                             ((LW_NTSTATUS) 0xc0000101)
#define LW_STATUS_FILE_CORRUPT_ERROR                              ((LW_NTSTATUS) 0xc0000102)
#define LW_STATUS_NOT_A_DIRECTORY                                 ((LW_NTSTATUS) 0xc0000103)
#define LW_STATUS_BAD_LOGON_SESSION_STATE                         ((LW_NTSTATUS) 0xc0000104)
#define LW_STATUS_LOGON_SESSION_COLLISION                         ((LW_NTSTATUS) 0xc0000105)
#define LW_STATUS_NAME_TOO_LONG                                   ((LW_NTSTATUS) 0xc0000106)
#define LW_STATUS_FILES_OPEN                                      ((LW_NTSTATUS) 0xc0000107)
#define LW_STATUS_CONNECTION_IN_USE                               ((LW_NTSTATUS) 0xc0000108)
#define LW_STATUS_MESSAGE_NOT_FOUND                               ((LW_NTSTATUS) 0xc0000109)
#define LW_STATUS_PROCESS_IS_TERMINATING                          ((LW_NTSTATUS) 0xc000010a)
#define LW_STATUS_INVALID_LOGON_TYPE                              ((LW_NTSTATUS) 0xc000010b)
#define LW_STATUS_NO_GUID_TRANSLATION                             ((LW_NTSTATUS) 0xc000010c)
#define LW_STATUS_CANNOT_IMPERSONATE                              ((LW_NTSTATUS) 0xc000010d)
#define LW_STATUS_IMAGE_ALREADY_LOADED                            ((LW_NTSTATUS) 0xc000010e)
#define LW_STATUS_ABIOS_NOT_PRESENT                               ((LW_NTSTATUS) 0xc000010f)
#define LW_STATUS_ABIOS_LID_NOT_EXIST                             ((LW_NTSTATUS) 0xc0000110)
#define LW_STATUS_ABIOS_LID_ALREADY_OWNED                         ((LW_NTSTATUS) 0xc0000111)
#define LW_STATUS_ABIOS_NOT_LID_OWNER                             ((LW_NTSTATUS) 0xc0000112)
#define LW_STATUS_ABIOS_INVALID_COMMAND                           ((LW_NTSTATUS) 0xc0000113)
#define LW_STATUS_ABIOS_INVALID_LID                               ((LW_NTSTATUS) 0xc0000114)
#define LW_STATUS_ABIOS_SELECTOR_NOT_AVAILABLE                    ((LW_NTSTATUS) 0xc0000115)
#define LW_STATUS_ABIOS_INVALID_SELECTOR                          ((LW_NTSTATUS) 0xc0000116)
#define LW_STATUS_NO_LDT                                          ((LW_NTSTATUS) 0xc0000117)
#define LW_STATUS_INVALID_LDT_SIZE                                ((LW_NTSTATUS) 0xc0000118)
#define LW_STATUS_INVALID_LDT_OFFSET                              ((LW_NTSTATUS) 0xc0000119)
#define LW_STATUS_INVALID_LDT_DESCRIPTOR                          ((LW_NTSTATUS) 0xc000011a)
#define LW_STATUS_INVALID_IMAGE_NE_FORMAT                         ((LW_NTSTATUS) 0xc000011b)
#define LW_STATUS_RXACT_INVALID_STATE                             ((LW_NTSTATUS) 0xc000011c)
#define LW_STATUS_RXACT_COMMIT_FAILURE                            ((LW_NTSTATUS) 0xc000011d)
#define LW_STATUS_MAPPED_FILE_SIZE_ZERO                           ((LW_NTSTATUS) 0xc000011e)
#define LW_STATUS_TOO_MANY_OPENED_FILES                           ((LW_NTSTATUS) 0xc000011f)
#define LW_STATUS_CANCELLED                                       ((LW_NTSTATUS) 0xc0000120)
#define LW_STATUS_CANNOT_DELETE                                   ((LW_NTSTATUS) 0xc0000121)
#define LW_STATUS_INVALID_COMPUTER_NAME                           ((LW_NTSTATUS) 0xc0000122)
#define LW_STATUS_FILE_DELETED                                    ((LW_NTSTATUS) 0xc0000123)
#define LW_STATUS_SPECIAL_ACCOUNT                                 ((LW_NTSTATUS) 0xc0000124)
#define LW_STATUS_SPECIAL_GROUP                                   ((LW_NTSTATUS) 0xc0000125)
#define LW_STATUS_SPECIAL_USER                                    ((LW_NTSTATUS) 0xc0000126)
#define LW_STATUS_MEMBERS_PRIMARY_GROUP                           ((LW_NTSTATUS) 0xc0000127)
#define LW_STATUS_FILE_CLOSED                                     ((LW_NTSTATUS) 0xc0000128)
#define LW_STATUS_TOO_MANY_THREADS                                ((LW_NTSTATUS) 0xc0000129)
#define LW_STATUS_THREAD_NOT_IN_PROCESS                           ((LW_NTSTATUS) 0xc000012a)
#define LW_STATUS_TOKEN_ALREADY_IN_USE                            ((LW_NTSTATUS) 0xc000012b)
#define LW_STATUS_PAGEFILE_QUOTA_EXCEEDED                         ((LW_NTSTATUS) 0xc000012c)
#define LW_STATUS_COMMITMENT_LIMIT                                ((LW_NTSTATUS) 0xc000012d)
#define LW_STATUS_INVALID_IMAGE_LE_FORMAT                         ((LW_NTSTATUS) 0xc000012e)
#define LW_STATUS_INVALID_IMAGE_NOT_MZ                            ((LW_NTSTATUS) 0xc000012f)
#define LW_STATUS_INVALID_IMAGE_PROTECT                           ((LW_NTSTATUS) 0xc0000130)
#define LW_STATUS_INVALID_IMAGE_WIN_16                            ((LW_NTSTATUS) 0xc0000131)
#define LW_STATUS_LOGON_SERVER_CONFLICT                           ((LW_NTSTATUS) 0xc0000132)
#define LW_STATUS_TIME_DIFFERENCE_AT_DC                           ((LW_NTSTATUS) 0xc0000133)
#define LW_STATUS_SYNCHRONIZATION_REQUIRED                        ((LW_NTSTATUS) 0xc0000134)
#define LW_STATUS_DLL_NOT_FOUND                                   ((LW_NTSTATUS) 0xc0000135)
#define LW_STATUS_OPEN_FAILED                                     ((LW_NTSTATUS) 0xc0000136)
#define LW_STATUS_IO_PRIVILEGE_FAILED                             ((LW_NTSTATUS) 0xc0000137)
#define LW_STATUS_ORDINAL_NOT_FOUND                               ((LW_NTSTATUS) 0xc0000138)
#define LW_STATUS_ENTRYPOINT_NOT_FOUND                            ((LW_NTSTATUS) 0xc0000139)
#define LW_STATUS_CONTROL_C_EXIT                                  ((LW_NTSTATUS) 0xc000013a)
#define LW_STATUS_LOCAL_DISCONNECT                                ((LW_NTSTATUS) 0xc000013b)
#define LW_STATUS_REMOTE_DISCONNECT                               ((LW_NTSTATUS) 0xc000013c)
#define LW_STATUS_REMOTE_RESOURCES                                ((LW_NTSTATUS) 0xc000013d)
#define LW_STATUS_LINK_FAILED                                     ((LW_NTSTATUS) 0xc000013e)
#define LW_STATUS_LINK_TIMEOUT                                    ((LW_NTSTATUS) 0xc000013f)
#define LW_STATUS_INVALID_CONNECTION                              ((LW_NTSTATUS) 0xc0000140)
#define LW_STATUS_INVALID_ADDRESS                                 ((LW_NTSTATUS) 0xc0000141)
#define LW_STATUS_DLL_INIT_FAILED                                 ((LW_NTSTATUS) 0xc0000142)
#define LW_STATUS_MISSING_SYSTEMFILE                              ((LW_NTSTATUS) 0xc0000143)
#define LW_STATUS_UNHANDLED_EXCEPTION                             ((LW_NTSTATUS) 0xc0000144)
#define LW_STATUS_APP_INIT_FAILURE                                ((LW_NTSTATUS) 0xc0000145)
#define LW_STATUS_PAGEFILE_CREATE_FAILED                          ((LW_NTSTATUS) 0xc0000146)
#define LW_STATUS_NO_PAGEFILE                                     ((LW_NTSTATUS) 0xc0000147)
#define LW_STATUS_INVALID_LEVEL                                   ((LW_NTSTATUS) 0xc0000148)
#define LW_STATUS_WRONG_PASSWORD_CORE                             ((LW_NTSTATUS) 0xc0000149)
#define LW_STATUS_ILLEGAL_FLOAT_CONTEXT                           ((LW_NTSTATUS) 0xc000014a)
#define LW_STATUS_PIPE_BROKEN                                     ((LW_NTSTATUS) 0xc000014b)
#define LW_STATUS_REGISTRY_CORRUPT                                ((LW_NTSTATUS) 0xc000014c)
#define LW_STATUS_REGISTRY_IO_FAILED                              ((LW_NTSTATUS) 0xc000014d)
#define LW_STATUS_NO_EVENT_PAIR                                   ((LW_NTSTATUS) 0xc000014e)
#define LW_STATUS_UNRECOGNIZED_VOLUME                             ((LW_NTSTATUS) 0xc000014f)
#define LW_STATUS_SERIAL_NO_DEVICE_INITED                         ((LW_NTSTATUS) 0xc0000150)
#define LW_STATUS_NO_SUCH_ALIAS                                   ((LW_NTSTATUS) 0xc0000151)
#define LW_STATUS_MEMBER_NOT_IN_ALIAS                             ((LW_NTSTATUS) 0xc0000152)
#define LW_STATUS_MEMBER_IN_ALIAS                                 ((LW_NTSTATUS) 0xc0000153)
#define LW_STATUS_ALIAS_EXISTS                                    ((LW_NTSTATUS) 0xc0000154)
#define LW_STATUS_LOGON_NOT_GRANTED                               ((LW_NTSTATUS) 0xc0000155)
#define LW_STATUS_TOO_MANY_SECRETS                                ((LW_NTSTATUS) 0xc0000156)
#define LW_STATUS_SECRET_TOO_LONG                                 ((LW_NTSTATUS) 0xc0000157)
#define LW_STATUS_INTERNAL_DB_ERROR                               ((LW_NTSTATUS) 0xc0000158)
#define LW_STATUS_FULLSCREEN_MODE                                 ((LW_NTSTATUS) 0xc0000159)
#define LW_STATUS_TOO_MANY_CONTEXT_IDS                            ((LW_NTSTATUS) 0xc000015a)
#define LW_STATUS_LOGON_TYPE_NOT_GRANTED                          ((LW_NTSTATUS) 0xc000015b)
#define LW_STATUS_NOT_REGISTRY_FILE                               ((LW_NTSTATUS) 0xc000015c)
#define LW_STATUS_NT_CROSS_ENCRYPTION_REQUIRED                    ((LW_NTSTATUS) 0xc000015d)
#define LW_STATUS_DOMAIN_CTRLR_CONFIG_ERROR                       ((LW_NTSTATUS) 0xc000015e)
#define LW_STATUS_FT_MISSING_MEMBER                               ((LW_NTSTATUS) 0xc000015f)
#define LW_STATUS_ILL_FORMED_SERVICE_ENTRY                        ((LW_NTSTATUS) 0xc0000160)
#define LW_STATUS_ILLEGAL_CHARACTER                               ((LW_NTSTATUS) 0xc0000161)
#define LW_STATUS_UNMAPPABLE_CHARACTER                            ((LW_NTSTATUS) 0xc0000162)
#define LW_STATUS_UNDEFINED_CHARACTER                             ((LW_NTSTATUS) 0xc0000163)
#define LW_STATUS_FLOPPY_VOLUME                                   ((LW_NTSTATUS) 0xc0000164)
#define LW_STATUS_FLOPPY_ID_MARK_NOT_FOUND                        ((LW_NTSTATUS) 0xc0000165)
#define LW_STATUS_FLOPPY_WRONG_CYLINDER                           ((LW_NTSTATUS) 0xc0000166)
#define LW_STATUS_FLOPPY_UNKNOWN_ERROR                            ((LW_NTSTATUS) 0xc0000167)
#define LW_STATUS_FLOPPY_BAD_REGISTERS                            ((LW_NTSTATUS) 0xc0000168)
#define LW_STATUS_DISK_RECALIBRATE_FAILED                         ((LW_NTSTATUS) 0xc0000169)
#define LW_STATUS_DISK_OPERATION_FAILED                           ((LW_NTSTATUS) 0xc000016a)
#define LW_STATUS_DISK_RESET_FAILED                               ((LW_NTSTATUS) 0xc000016b)
#define LW_STATUS_SHARED_IRQ_BUSY                                 ((LW_NTSTATUS) 0xc000016c)
#define LW_STATUS_FT_ORPHANING                                    ((LW_NTSTATUS) 0xc000016d)
#define LW_STATUS_BIOS_FAILED_TO_CONNECT_INTERRUPT                ((LW_NTSTATUS) 0xc000016e)
#define LW_STATUS_PARTITION_FAILURE                               ((LW_NTSTATUS) 0xc0000172)
#define LW_STATUS_INVALID_BLOCK_LENGTH                            ((LW_NTSTATUS) 0xc0000173)
#define LW_STATUS_DEVICE_NOT_PARTITIONED                          ((LW_NTSTATUS) 0xc0000174)
#define LW_STATUS_UNABLE_TO_LOCK_MEDIA                            ((LW_NTSTATUS) 0xc0000175)
#define LW_STATUS_UNABLE_TO_UNLOAD_MEDIA                          ((LW_NTSTATUS) 0xc0000176)
#define LW_STATUS_EOM_OVERFLOW                                    ((LW_NTSTATUS) 0xc0000177)
#define LW_STATUS_NO_MEDIA                                        ((LW_NTSTATUS) 0xc0000178)
#define LW_STATUS_NO_SUCH_MEMBER                                  ((LW_NTSTATUS) 0xc000017a)
#define LW_STATUS_INVALID_MEMBER                                  ((LW_NTSTATUS) 0xc000017b)
#define LW_STATUS_KEY_DELETED                                     ((LW_NTSTATUS) 0xc000017c)
#define LW_STATUS_NO_LOG_SPACE                                    ((LW_NTSTATUS) 0xc000017d)
#define LW_STATUS_TOO_MANY_SIDS                                   ((LW_NTSTATUS) 0xc000017e)
#define LW_STATUS_LM_CROSS_ENCRYPTION_REQUIRED                    ((LW_NTSTATUS) 0xc000017f)
#define LW_STATUS_KEY_HAS_CHILDREN                                ((LW_NTSTATUS) 0xc0000180)
#define LW_STATUS_CHILD_MUST_BE_VOLATILE                          ((LW_NTSTATUS) 0xc0000181)
#define LW_STATUS_DEVICE_CONFIGURATION_ERROR                      ((LW_NTSTATUS) 0xc0000182)
#define LW_STATUS_DRIVER_INTERNAL_ERROR                           ((LW_NTSTATUS) 0xc0000183)
#define LW_STATUS_INVALID_DEVICE_STATE                            ((LW_NTSTATUS) 0xc0000184)
#define LW_STATUS_IO_DEVICE_ERROR                                 ((LW_NTSTATUS) 0xc0000185)
#define LW_STATUS_DEVICE_PROTOCOL_ERROR                           ((LW_NTSTATUS) 0xc0000186)
#define LW_STATUS_BACKUP_CONTROLLER                               ((LW_NTSTATUS) 0xc0000187)
#define LW_STATUS_LOG_FILE_FULL                                   ((LW_NTSTATUS) 0xc0000188)
#define LW_STATUS_TOO_LATE                                        ((LW_NTSTATUS) 0xc0000189)
#define LW_STATUS_NO_TRUST_LSA_SECRET                             ((LW_NTSTATUS) 0xc000018a)
#define LW_STATUS_NO_TRUST_SAM_ACCOUNT                            ((LW_NTSTATUS) 0xc000018b)
#define LW_STATUS_TRUSTED_DOMAIN_FAILURE                          ((LW_NTSTATUS) 0xc000018c)
#define LW_STATUS_TRUSTED_RELATIONSHIP_FAILURE                    ((LW_NTSTATUS) 0xc000018d)
#define LW_STATUS_EVENTLOG_FILE_CORRUPT                           ((LW_NTSTATUS) 0xc000018e)
#define LW_STATUS_EVENTLOG_CANT_START                             ((LW_NTSTATUS) 0xc000018f)
#define LW_STATUS_TRUST_FAILURE                                   ((LW_NTSTATUS) 0xc0000190)
#define LW_STATUS_MUTANT_LIMIT_EXCEEDED                           ((LW_NTSTATUS) 0xc0000191)
#define LW_STATUS_NETLOGON_NOT_STARTED                            ((LW_NTSTATUS) 0xc0000192)
#define LW_STATUS_ACCOUNT_EXPIRED                                 ((LW_NTSTATUS) 0xc0000193)
#define LW_STATUS_POSSIBLE_DEADLOCK                               ((LW_NTSTATUS) 0xc0000194)
#define LW_STATUS_NETWORK_CREDENTIAL_CONFLICT                     ((LW_NTSTATUS) 0xc0000195)
#define LW_STATUS_REMOTE_SESSION_LIMIT                            ((LW_NTSTATUS) 0xc0000196)
#define LW_STATUS_EVENTLOG_FILE_CHANGED                           ((LW_NTSTATUS) 0xc0000197)
#define LW_STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT               ((LW_NTSTATUS) 0xc0000198)
#define LW_STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT               ((LW_NTSTATUS) 0xc0000199)
#define LW_STATUS_NOLOGON_SERVER_TRUST_ACCOUNT                    ((LW_NTSTATUS) 0xc000019a)
#define LW_STATUS_DOMAIN_TRUST_INCONSISTENT                       ((LW_NTSTATUS) 0xc000019b)
#define LW_STATUS_FS_DRIVER_REQUIRED                              ((LW_NTSTATUS) 0xc000019c)
#define LW_STATUS_NO_USER_SESSION_KEY                             ((LW_NTSTATUS) 0xc0000202)
#define LW_STATUS_USER_SESSION_DELETED                            ((LW_NTSTATUS) 0xc0000203)
#define LW_STATUS_RESOURCE_LANG_NOT_FOUND                         ((LW_NTSTATUS) 0xc0000204)
#define LW_STATUS_INSUFF_SERVER_RESOURCES                         ((LW_NTSTATUS) 0xc0000205)
#define LW_STATUS_INVALID_BUFFER_SIZE                             ((LW_NTSTATUS) 0xc0000206)
#define LW_STATUS_INVALID_ADDRESS_COMPONENT                       ((LW_NTSTATUS) 0xc0000207)
#define LW_STATUS_INVALID_ADDRESS_WILDCARD                        ((LW_NTSTATUS) 0xc0000208)
#define LW_STATUS_TOO_MANY_ADDRESSES                              ((LW_NTSTATUS) 0xc0000209)
#define LW_STATUS_ADDRESS_ALREADY_EXISTS                          ((LW_NTSTATUS) 0xc000020a)
#define LW_STATUS_ADDRESS_CLOSED                                  ((LW_NTSTATUS) 0xc000020b)
#define LW_STATUS_CONNECTION_DISCONNECTED                         ((LW_NTSTATUS) 0xc000020c)
#define LW_STATUS_CONNECTION_RESET                                ((LW_NTSTATUS) 0xc000020d)
#define LW_STATUS_TOO_MANY_NODES                                  ((LW_NTSTATUS) 0xc000020e)
#define LW_STATUS_TRANSACTION_ABORTED                             ((LW_NTSTATUS) 0xc000020f)
#define LW_STATUS_TRANSACTION_TIMED_OUT                           ((LW_NTSTATUS) 0xc0000210)
#define LW_STATUS_TRANSACTION_NO_RELEASE                          ((LW_NTSTATUS) 0xc0000211)
#define LW_STATUS_TRANSACTION_NO_MATCH                            ((LW_NTSTATUS) 0xc0000212)
#define LW_STATUS_TRANSACTION_RESPONDED                           ((LW_NTSTATUS) 0xc0000213)
#define LW_STATUS_TRANSACTION_INVALID_ID                          ((LW_NTSTATUS) 0xc0000214)
#define LW_STATUS_TRANSACTION_INVALID_TYPE                        ((LW_NTSTATUS) 0xc0000215)
#define LW_STATUS_NOT_SERVER_SESSION                              ((LW_NTSTATUS) 0xc0000216)
#define LW_STATUS_NOT_CLIENT_SESSION                              ((LW_NTSTATUS) 0xc0000217)
#define LW_STATUS_CANNOT_LOAD_REGISTRY_FILE                       ((LW_NTSTATUS) 0xc0000218)
#define LW_STATUS_DEBUG_ATTACH_FAILED                             ((LW_NTSTATUS) 0xc0000219)
#define LW_STATUS_SYSTEM_PROCESS_TERMINATED                       ((LW_NTSTATUS) 0xc000021a)
#define LW_STATUS_DATA_NOT_ACCEPTED                               ((LW_NTSTATUS) 0xc000021b)
#define LW_STATUS_NO_BROWSER_SERVERS_FOUND                        ((LW_NTSTATUS) 0xc000021c)
#define LW_STATUS_VDM_HARD_ERROR                                  ((LW_NTSTATUS) 0xc000021d)
#define LW_STATUS_DRIVER_CANCEL_TIMEOUT                           ((LW_NTSTATUS) 0xc000021e)
#define LW_STATUS_REPLY_MESSAGE_MISMATCH                          ((LW_NTSTATUS) 0xc000021f)
#define LW_STATUS_MAPPED_ALIGNMENT                                ((LW_NTSTATUS) 0xc0000220)
#define LW_STATUS_IMAGE_CHECKSUM_MISMATCH                         ((LW_NTSTATUS) 0xc0000221)
#define LW_STATUS_LOST_WRITEBEHIND_DATA                           ((LW_NTSTATUS) 0xc0000222)
#define LW_STATUS_CLIENT_SERVER_PARAMETERS_INVALID                ((LW_NTSTATUS) 0xc0000223)
#define LW_STATUS_PASSWORD_MUST_CHANGE                            ((LW_NTSTATUS) 0xc0000224)
#define LW_STATUS_NOT_FOUND                                       ((LW_NTSTATUS) 0xc0000225)
#define LW_STATUS_NOT_TINY_STREAM                                 ((LW_NTSTATUS) 0xc0000226)
#define LW_STATUS_RECOVERY_FAILURE                                ((LW_NTSTATUS) 0xc0000227)
#define LW_STATUS_STACK_OVERFLOW_READ                             ((LW_NTSTATUS) 0xc0000228)
#define LW_STATUS_FAIL_CHECK                                      ((LW_NTSTATUS) 0xc0000229)
#define LW_STATUS_DUPLICATE_OBJECTID                              ((LW_NTSTATUS) 0xc000022a)
#define LW_STATUS_OBJECTID_EXISTS                                 ((LW_NTSTATUS) 0xc000022b)
#define LW_STATUS_CONVERT_TO_LARGE                                ((LW_NTSTATUS) 0xc000022c)
#define LW_STATUS_RETRY                                           ((LW_NTSTATUS) 0xc000022d)
#define LW_STATUS_FOUND_OUT_OF_SCOPE                              ((LW_NTSTATUS) 0xc000022e)
#define LW_STATUS_ALLOCATE_BUCKET                                 ((LW_NTSTATUS) 0xc000022f)
#define LW_STATUS_PROPSET_NOT_FOUND                               ((LW_NTSTATUS) 0xc0000230)
#define LW_STATUS_MARSHALL_OVERFLOW                               ((LW_NTSTATUS) 0xc0000231)
#define LW_STATUS_INVALID_VARIANT                                 ((LW_NTSTATUS) 0xc0000232)
#define LW_STATUS_DOMAIN_CONTROLLER_NOT_FOUND                     ((LW_NTSTATUS) 0xc0000233)
#define LW_STATUS_ACCOUNT_LOCKED_OUT                              ((LW_NTSTATUS) 0xc0000234)
#define LW_STATUS_HANDLE_NOT_CLOSABLE                             ((LW_NTSTATUS) 0xc0000235)
#define LW_STATUS_CONNECTION_REFUSED                              ((LW_NTSTATUS) 0xc0000236)
#define LW_STATUS_GRACEFUL_DISCONNECT                             ((LW_NTSTATUS) 0xc0000237)
#define LW_STATUS_ADDRESS_ALREADY_ASSOCIATED                      ((LW_NTSTATUS) 0xc0000238)
#define LW_STATUS_ADDRESS_NOT_ASSOCIATED                          ((LW_NTSTATUS) 0xc0000239)
#define LW_STATUS_CONNECTION_INVALID                              ((LW_NTSTATUS) 0xc000023a)
#define LW_STATUS_CONNECTION_ACTIVE                               ((LW_NTSTATUS) 0xc000023b)
#define LW_STATUS_NETWORK_UNREACHABLE                             ((LW_NTSTATUS) 0xc000023c)
#define LW_STATUS_HOST_UNREACHABLE                                ((LW_NTSTATUS) 0xc000023d)
#define LW_STATUS_PROTOCOL_UNREACHABLE                            ((LW_NTSTATUS) 0xc000023e)
#define LW_STATUS_PORT_UNREACHABLE                                ((LW_NTSTATUS) 0xc000023f)
#define LW_STATUS_REQUEST_ABORTED                                 ((LW_NTSTATUS) 0xc0000240)
#define LW_STATUS_CONNECTION_ABORTED                              ((LW_NTSTATUS) 0xc0000241)
#define LW_STATUS_BAD_COMPRESSION_BUFFER                          ((LW_NTSTATUS) 0xc0000242)
#define LW_STATUS_USER_MAPPED_FILE                                ((LW_NTSTATUS) 0xc0000243)
#define LW_STATUS_AUDIT_FAILED                                    ((LW_NTSTATUS) 0xc0000244)
#define LW_STATUS_TIMER_RESOLUTION_NOT_SET                        ((LW_NTSTATUS) 0xc0000245)
#define LW_STATUS_CONNECTION_COUNT_LIMIT                          ((LW_NTSTATUS) 0xc0000246)
#define LW_STATUS_LOGIN_TIME_RESTRICTION                          ((LW_NTSTATUS) 0xc0000247)
#define LW_STATUS_LOGIN_WKSTA_RESTRICTION                         ((LW_NTSTATUS) 0xc0000248)
#define LW_STATUS_IMAGE_MP_UP_MISMATCH                            ((LW_NTSTATUS) 0xc0000249)
#define LW_STATUS_INSUFFICIENT_LOGON_INFO                         ((LW_NTSTATUS) 0xc0000250)
#define LW_STATUS_BAD_DLL_ENTRYPOINT                              ((LW_NTSTATUS) 0xc0000251)
#define LW_STATUS_BAD_SERVICE_ENTRYPOINT                          ((LW_NTSTATUS) 0xc0000252)
#define LW_STATUS_LPC_REPLY_LOST                                  ((LW_NTSTATUS) 0xc0000253)
#define LW_STATUS_IP_ADDRESS_CONFLICT1                            ((LW_NTSTATUS) 0xc0000254)
#define LW_STATUS_IP_ADDRESS_CONFLICT2                            ((LW_NTSTATUS) 0xc0000255)
#define LW_STATUS_REGISTRY_QUOTA_LIMIT                            ((LW_NTSTATUS) 0xc0000256)
#define LW_STATUS_PATH_NOT_COVERED                                ((LW_NTSTATUS) 0xc0000257)
#define LW_STATUS_NO_CALLBACK_ACTIVE                              ((LW_NTSTATUS) 0xc0000258)
#define LW_STATUS_LICENSE_QUOTA_EXCEEDED                          ((LW_NTSTATUS) 0xc0000259)
#define LW_STATUS_PWD_TOO_SHORT                                   ((LW_NTSTATUS) 0xc000025a)
#define LW_STATUS_PWD_TOO_RECENT                                  ((LW_NTSTATUS) 0xc000025b)
#define LW_STATUS_PWD_HISTORY_CONFLICT                            ((LW_NTSTATUS) 0xc000025c)
#define LW_STATUS_PLUGPLAY_NO_DEVICE                              ((LW_NTSTATUS) 0xc000025e)
#define LW_STATUS_UNSUPPORTED_COMPRESSION                         ((LW_NTSTATUS) 0xc000025f)
#define LW_STATUS_INVALID_HW_PROFILE                              ((LW_NTSTATUS) 0xc0000260)
#define LW_STATUS_INVALID_PLUGPLAY_DEVICE_PATH                    ((LW_NTSTATUS) 0xc0000261)
#define LW_STATUS_DRIVER_ORDINAL_NOT_FOUND                        ((LW_NTSTATUS) 0xc0000262)
#define LW_STATUS_DRIVER_ENTRYPOINT_NOT_FOUND                     ((LW_NTSTATUS) 0xc0000263)
#define LW_STATUS_RESOURCE_NOT_OWNED                              ((LW_NTSTATUS) 0xc0000264)
#define LW_STATUS_TOO_MANY_LINKS                                  ((LW_NTSTATUS) 0xc0000265)
#define LW_STATUS_QUOTA_LIST_INCONSISTENT                         ((LW_NTSTATUS) 0xc0000266)
#define LW_STATUS_FILE_IS_OFFLINE                                 ((LW_NTSTATUS) 0xc0000267)
#define LW_STATUS_EVALUATION_EXPIRATION                           ((LW_NTSTATUS) 0xc0000268)
#define LW_STATUS_ILLEGAL_DLL_RELOCATION                          ((LW_NTSTATUS) 0xc0000269)
#define LW_STATUS_LICENSE_VIOLATION                               ((LW_NTSTATUS) 0xc000026a)
#define LW_STATUS_DLL_INIT_FAILED_LOGOFF                          ((LW_NTSTATUS) 0xc000026b)
#define LW_STATUS_DRIVER_UNABLE_TO_LOAD                           ((LW_NTSTATUS) 0xc000026c)
#define LW_STATUS_DFS_UNAVAILABLE                                 ((LW_NTSTATUS) 0xc000026d)
#define LW_STATUS_VOLUME_DISMOUNTED                               ((LW_NTSTATUS) 0xc000026e)
#define LW_STATUS_WX86_INTERNAL_ERROR                             ((LW_NTSTATUS) 0xc000026f)
#define LW_STATUS_WX86_FLOAT_STACK_CHECK                          ((LW_NTSTATUS) 0xc0000270)
#define LW_STATUS_VALIDATE_CONTINUE                               ((LW_NTSTATUS) 0xc0000271)
#define LW_STATUS_NO_MATCH                                        ((LW_NTSTATUS) 0xc0000272)
#define LW_STATUS_NO_MORE_MATCHES                                 ((LW_NTSTATUS) 0xc0000273)
#define LW_STATUS_NOT_A_REPARSE_POINT                             ((LW_NTSTATUS) 0xc0000275)
#define LW_STATUS_IO_REPARSE_TAG_INVALID                          ((LW_NTSTATUS) 0xc0000276)
#define LW_STATUS_IO_REPARSE_TAG_MISMATCH                         ((LW_NTSTATUS) 0xc0000277)
#define LW_STATUS_IO_REPARSE_DATA_INVALID                         ((LW_NTSTATUS) 0xc0000278)
#define LW_STATUS_IO_REPARSE_TAG_NOT_HANDLED                      ((LW_NTSTATUS) 0xc0000279)
#define LW_STATUS_REPARSE_POINT_NOT_RESOLVED                      ((LW_NTSTATUS) 0xc0000280)
#define LW_STATUS_DIRECTORY_IS_A_REPARSE_POINT                    ((LW_NTSTATUS) 0xc0000281)
#define LW_STATUS_RANGE_LIST_CONFLICT                             ((LW_NTSTATUS) 0xc0000282)
#define LW_STATUS_SOURCE_ELEMENT_EMPTY                            ((LW_NTSTATUS) 0xc0000283)
#define LW_STATUS_DESTINATION_ELEMENT_FULL                        ((LW_NTSTATUS) 0xc0000284)
#define LW_STATUS_ILLEGAL_ELEMENT_ADDRESS                         ((LW_NTSTATUS) 0xc0000285)
#define LW_STATUS_MAGAZINE_NOT_PRESENT                            ((LW_NTSTATUS) 0xc0000286)
#define LW_STATUS_REINITIALIZATION_NEEDED                         ((LW_NTSTATUS) 0xc0000287)
#define LW_STATUS_ENCRYPTION_FAILED                               ((LW_NTSTATUS) 0xc000028a)
#define LW_STATUS_DECRYPTION_FAILED                               ((LW_NTSTATUS) 0xc000028b)
#define LW_STATUS_RANGE_NOT_FOUND                                 ((LW_NTSTATUS) 0xc000028c)
#define LW_STATUS_NO_RECOVERY_POLICY                              ((LW_NTSTATUS) 0xc000028d)
#define LW_STATUS_NO_EFS                                          ((LW_NTSTATUS) 0xc000028e)
#define LW_STATUS_WRONG_EFS                                       ((LW_NTSTATUS) 0xc000028f)
#define LW_STATUS_NO_USER_KEYS                                    ((LW_NTSTATUS) 0xc0000290)
#define LW_STATUS_FILE_NOT_ENCRYPTED                              ((LW_NTSTATUS) 0xc0000291)
#define LW_STATUS_NOT_EXPORT_FORMAT                               ((LW_NTSTATUS) 0xc0000292)
#define LW_STATUS_FILE_ENCRYPTED                                  ((LW_NTSTATUS) 0xc0000293)
#define LW_STATUS_WMI_GUID_NOT_FOUND                              ((LW_NTSTATUS) 0xc0000295)
#define LW_STATUS_WMI_INSTANCE_NOT_FOUND                          ((LW_NTSTATUS) 0xc0000296)
#define LW_STATUS_WMI_ITEMID_NOT_FOUND                            ((LW_NTSTATUS) 0xc0000297)
#define LW_STATUS_WMI_TRY_AGAIN                                   ((LW_NTSTATUS) 0xc0000298)
#define LW_STATUS_SHARED_POLICY                                   ((LW_NTSTATUS) 0xc0000299)
#define LW_STATUS_POLICY_OBJECT_NOT_FOUND                         ((LW_NTSTATUS) 0xc000029a)
#define LW_STATUS_POLICY_ONLY_IN_DS                               ((LW_NTSTATUS) 0xc000029b)
#define LW_STATUS_VOLUME_NOT_UPGRADED                             ((LW_NTSTATUS) 0xc000029c)
#define LW_STATUS_REMOTE_STORAGE_NOT_ACTIVE                       ((LW_NTSTATUS) 0xc000029d)
#define LW_STATUS_REMOTE_STORAGE_MEDIA_ERROR                      ((LW_NTSTATUS) 0xc000029e)
#define LW_STATUS_NO_TRACKING_SERVICE                             ((LW_NTSTATUS) 0xc000029f)
#define LW_STATUS_SERVER_SID_MISMATCH                             ((LW_NTSTATUS) 0xc00002a0)
#define LW_STATUS_DS_NO_ATTRIBUTE_OR_VALUE                        ((LW_NTSTATUS) 0xc00002a1)
#define LW_STATUS_DS_INVALID_ATTRIBUTE_SYNTAX                     ((LW_NTSTATUS) 0xc00002a2)
#define LW_STATUS_DS_ATTRIBUTE_TYPE_UNDEFINED                     ((LW_NTSTATUS) 0xc00002a3)
#define LW_STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS                    ((LW_NTSTATUS) 0xc00002a4)
#define LW_STATUS_DS_BUSY                                         ((LW_NTSTATUS) 0xc00002a5)
#define LW_STATUS_DS_UNAVAILABLE                                  ((LW_NTSTATUS) 0xc00002a6)
#define LW_STATUS_DS_NO_RIDS_ALLOCATED                            ((LW_NTSTATUS) 0xc00002a7)
#define LW_STATUS_DS_NO_MORE_RIDS                                 ((LW_NTSTATUS) 0xc00002a8)
#define LW_STATUS_DS_INCORRECT_ROLE_OWNER                         ((LW_NTSTATUS) 0xc00002a9)
#define LW_STATUS_DS_RIDMGR_INIT_ERROR                            ((LW_NTSTATUS) 0xc00002aa)
#define LW_STATUS_DS_OBJ_CLASS_VIOLATION                          ((LW_NTSTATUS) 0xc00002ab)
#define LW_STATUS_DS_CANT_ON_NON_LEAF                             ((LW_NTSTATUS) 0xc00002ac)
#define LW_STATUS_DS_CANT_ON_RDN                                  ((LW_NTSTATUS) 0xc00002ad)
#define LW_STATUS_DS_CANT_MOD_OBJ_CLASS                           ((LW_NTSTATUS) 0xc00002ae)
#define LW_STATUS_DS_CROSS_DOM_MOVE_FAILED                        ((LW_NTSTATUS) 0xc00002af)
#define LW_STATUS_DS_GC_NOT_AVAILABLE                             ((LW_NTSTATUS) 0xc00002b0)
#define LW_STATUS_DIRECTORY_SERVICE_REQUIRED                      ((LW_NTSTATUS) 0xc00002b1)
#define LW_STATUS_REPARSE_ATTRIBUTE_CONFLICT                      ((LW_NTSTATUS) 0xc00002b2)
#define LW_STATUS_CANT_ENABLE_DENY_ONLY                           ((LW_NTSTATUS) 0xc00002b3)
#define LW_STATUS_FLOAT_MULTIPLE_FAULTS                           ((LW_NTSTATUS) 0xc00002b4)
#define LW_STATUS_FLOAT_MULTIPLE_TRAPS                            ((LW_NTSTATUS) 0xc00002b5)
#define LW_STATUS_DEVICE_REMOVED                                  ((LW_NTSTATUS) 0xc00002b6)
#define LW_STATUS_JOURNAL_DELETE_IN_PROGRESS                      ((LW_NTSTATUS) 0xc00002b7)
#define LW_STATUS_JOURNAL_NOT_ACTIVE                              ((LW_NTSTATUS) 0xc00002b8)
#define LW_STATUS_NOINTERFACE                                     ((LW_NTSTATUS) 0xc00002b9)
#define LW_STATUS_DS_ADMIN_LIMIT_EXCEEDED                         ((LW_NTSTATUS) 0xc00002c1)
#define LW_STATUS_DRIVER_FAILED_SLEEP                             ((LW_NTSTATUS) 0xc00002c2)
#define LW_STATUS_MUTUAL_AUTHENTICATION_FAILED                    ((LW_NTSTATUS) 0xc00002c3)
#define LW_STATUS_CORRUPT_SYSTEM_FILE                             ((LW_NTSTATUS) 0xc00002c4)
#define LW_STATUS_DATATYPE_MISALIGNMENT_ERROR                     ((LW_NTSTATUS) 0xc00002c5)
#define LW_STATUS_WMI_READ_ONLY                                   ((LW_NTSTATUS) 0xc00002c6)
#define LW_STATUS_WMI_SET_FAILURE                                 ((LW_NTSTATUS) 0xc00002c7)
#define LW_STATUS_COMMITMENT_MINIMUM                              ((LW_NTSTATUS) 0xc00002c8)
#define LW_STATUS_REG_NAT_CONSUMPTION                             ((LW_NTSTATUS) 0xc00002c9)
#define LW_STATUS_TRANSPORT_FULL                                  ((LW_NTSTATUS) 0xc00002ca)
#define LW_STATUS_DS_SAM_INIT_FAILURE                             ((LW_NTSTATUS) 0xc00002cb)
#define LW_STATUS_ONLY_IF_CONNECTED                               ((LW_NTSTATUS) 0xc00002cc)
#define LW_STATUS_DS_SENSITIVE_GROUP_VIOLATION                    ((LW_NTSTATUS) 0xc00002cd)
#define LW_STATUS_PNP_RESTART_ENUMERATION                         ((LW_NTSTATUS) 0xc00002ce)
#define LW_STATUS_JOURNAL_ENTRY_DELETED                           ((LW_NTSTATUS) 0xc00002cf)
#define LW_STATUS_DS_CANT_MOD_PRIMARYGROUPID                      ((LW_NTSTATUS) 0xc00002d0)
#define LW_STATUS_SYSTEM_IMAGE_BAD_SIGNATURE                      ((LW_NTSTATUS) 0xc00002d1)
#define LW_STATUS_PNP_REBOOT_REQUIRED                             ((LW_NTSTATUS) 0xc00002d2)
#define LW_STATUS_POWER_STATE_INVALID                             ((LW_NTSTATUS) 0xc00002d3)
#define LW_STATUS_DS_INVALID_GROUP_TYPE                           ((LW_NTSTATUS) 0xc00002d4)
#define LW_STATUS_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN           ((LW_NTSTATUS) 0xc00002d5)
#define LW_STATUS_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN            ((LW_NTSTATUS) 0xc00002d6)
#define LW_STATUS_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER                ((LW_NTSTATUS) 0xc00002d7)
#define LW_STATUS_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER            ((LW_NTSTATUS) 0xc00002d8)
#define LW_STATUS_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER             ((LW_NTSTATUS) 0xc00002d9)
#define LW_STATUS_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER          ((LW_NTSTATUS) 0xc00002da)
#define LW_STATUS_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER     ((LW_NTSTATUS) 0xc00002db)
#define LW_STATUS_DS_HAVE_PRIMARY_MEMBERS                         ((LW_NTSTATUS) 0xc00002dc)
#define LW_STATUS_WMI_NOT_SUPPORTED                               ((LW_NTSTATUS) 0xc00002dd)
#define LW_STATUS_INSUFFICIENT_POWER                              ((LW_NTSTATUS) 0xc00002de)
#define LW_STATUS_SAM_NEED_BOOTKEY_PASSWORD                       ((LW_NTSTATUS) 0xc00002df)
#define LW_STATUS_SAM_NEED_BOOTKEY_FLOPPY                         ((LW_NTSTATUS) 0xc00002e0)
#define LW_STATUS_DS_CANT_START                                   ((LW_NTSTATUS) 0xc00002e1)
#define LW_STATUS_DS_INIT_FAILURE                                 ((LW_NTSTATUS) 0xc00002e2)
#define LW_STATUS_SAM_INIT_FAILURE                                ((LW_NTSTATUS) 0xc00002e3)
#define LW_STATUS_DS_GC_REQUIRED                                  ((LW_NTSTATUS) 0xc00002e4)
#define LW_STATUS_DS_LOCAL_MEMBER_OF_LOCAL_ONLY                   ((LW_NTSTATUS) 0xc00002e5)
#define LW_STATUS_DS_NO_FPO_IN_UNIVERSAL_GROUPS                   ((LW_NTSTATUS) 0xc00002e6)
#define LW_STATUS_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED               ((LW_NTSTATUS) 0xc00002e7)
#define LW_STATUS_MULTIPLE_FAULT_VIOLATION                        ((LW_NTSTATUS) 0xc00002e8)
#define LW_STATUS_NOT_SUPPORTED_ON_SBS                            ((LW_NTSTATUS) 0xc0000300)
#define LW_STATUS_ASSERTION_FAILURE                               ((LW_NTSTATUS) 0xc0000420)

#ifndef LW_STRICT_NAMESPACE

typedef LW_NTSTATUS NTSTATUS;
typedef LW_PNTSTATUS PNTSTATUS;

#define NT_SUCCESS(status) LW_NT_SUCCESS(status)
#define NT_SUCCESS_OR_NOT(status) LW_NT_SUCCESS_OR_NOT(status)

// NOTE: STATUS_<XXX> definitions generated using process-ntstatus.sh
#define STATUS_SUCCESS                                     LW_STATUS_SUCCESS
#define STATUS_PENDING                                     LW_STATUS_PENDING
#define STATUS_MORE_ENTRIES                                LW_STATUS_MORE_ENTRIES
#define STATUS_SOME_UNMAPPED                               LW_STATUS_SOME_UNMAPPED
#define STATUS_NO_MORE_ENTRIES                             LW_STATUS_NO_MORE_ENTRIES
#define STATUS_BUFFER_OVERFLOW                             LW_STATUS_BUFFER_OVERFLOW
#define STATUS_UNSUCCESSFUL                                LW_STATUS_UNSUCCESSFUL
#define STATUS_NOT_IMPLEMENTED                             LW_STATUS_NOT_IMPLEMENTED
#define STATUS_INVALID_INFO_CLASS                          LW_STATUS_INVALID_INFO_CLASS
#define STATUS_INFO_LENGTH_MISMATCH                        LW_STATUS_INFO_LENGTH_MISMATCH
#define STATUS_ACCESS_VIOLATION                            LW_STATUS_ACCESS_VIOLATION
#define STATUS_IN_PAGE_ERROR                               LW_STATUS_IN_PAGE_ERROR
#define STATUS_PAGEFILE_QUOTA                              LW_STATUS_PAGEFILE_QUOTA
#define STATUS_INVALID_HANDLE                              LW_STATUS_INVALID_HANDLE
#define STATUS_BAD_INITIAL_STACK                           LW_STATUS_BAD_INITIAL_STACK
#define STATUS_BAD_INITIAL_PC                              LW_STATUS_BAD_INITIAL_PC
#define STATUS_INVALID_CID                                 LW_STATUS_INVALID_CID
#define STATUS_TIMER_NOT_CANCELED                          LW_STATUS_TIMER_NOT_CANCELED
#define STATUS_INVALID_PARAMETER                           LW_STATUS_INVALID_PARAMETER
#define STATUS_NO_SUCH_DEVICE                              LW_STATUS_NO_SUCH_DEVICE
#define STATUS_NO_SUCH_FILE                                LW_STATUS_NO_SUCH_FILE
#define STATUS_INVALID_DEVICE_REQUEST                      LW_STATUS_INVALID_DEVICE_REQUEST
#define STATUS_END_OF_FILE                                 LW_STATUS_END_OF_FILE
#define STATUS_WRONG_VOLUME                                LW_STATUS_WRONG_VOLUME
#define STATUS_NO_MEDIA_IN_DEVICE                          LW_STATUS_NO_MEDIA_IN_DEVICE
#define STATUS_UNRECOGNIZED_MEDIA                          LW_STATUS_UNRECOGNIZED_MEDIA
#define STATUS_NONEXISTENT_SECTOR                          LW_STATUS_NONEXISTENT_SECTOR
#define STATUS_MORE_PROCESSING_REQUIRED                    LW_STATUS_MORE_PROCESSING_REQUIRED
#define STATUS_NO_MEMORY                                   LW_STATUS_NO_MEMORY
#define STATUS_CONFLICTING_ADDRESSES                       LW_STATUS_CONFLICTING_ADDRESSES
#define STATUS_NOT_MAPPED_VIEW                             LW_STATUS_NOT_MAPPED_VIEW
#define STATUS_UNABLE_TO_FREE_VM                           LW_STATUS_UNABLE_TO_FREE_VM
#define STATUS_UNABLE_TO_DELETE_SECTION                    LW_STATUS_UNABLE_TO_DELETE_SECTION
#define STATUS_INVALID_SYSTEM_SERVICE                      LW_STATUS_INVALID_SYSTEM_SERVICE
#define STATUS_ILLEGAL_INSTRUCTION                         LW_STATUS_ILLEGAL_INSTRUCTION
#define STATUS_INVALID_LOCK_SEQUENCE                       LW_STATUS_INVALID_LOCK_SEQUENCE
#define STATUS_INVALID_VIEW_SIZE                           LW_STATUS_INVALID_VIEW_SIZE
#define STATUS_INVALID_FILE_FOR_SECTION                    LW_STATUS_INVALID_FILE_FOR_SECTION
#define STATUS_ALREADY_COMMITTED                           LW_STATUS_ALREADY_COMMITTED
#define STATUS_ACCESS_DENIED                               LW_STATUS_ACCESS_DENIED
#define STATUS_BUFFER_TOO_SMALL                            LW_STATUS_BUFFER_TOO_SMALL
#define STATUS_OBJECT_TYPE_MISMATCH                        LW_STATUS_OBJECT_TYPE_MISMATCH
#define STATUS_NONCONTINUABLE_EXCEPTION                    LW_STATUS_NONCONTINUABLE_EXCEPTION
#define STATUS_INVALID_DISPOSITION                         LW_STATUS_INVALID_DISPOSITION
#define STATUS_UNWIND                                      LW_STATUS_UNWIND
#define STATUS_BAD_STACK                                   LW_STATUS_BAD_STACK
#define STATUS_INVALID_UNWIND_TARGET                       LW_STATUS_INVALID_UNWIND_TARGET
#define STATUS_NOT_LOCKED                                  LW_STATUS_NOT_LOCKED
#define STATUS_PARITY_ERROR                                LW_STATUS_PARITY_ERROR
#define STATUS_UNABLE_TO_DECOMMIT_VM                       LW_STATUS_UNABLE_TO_DECOMMIT_VM
#define STATUS_NOT_COMMITTED                               LW_STATUS_NOT_COMMITTED
#define STATUS_INVALID_PORT_ATTRIBUTES                     LW_STATUS_INVALID_PORT_ATTRIBUTES
#define STATUS_PORT_MESSAGE_TOO_LONG                       LW_STATUS_PORT_MESSAGE_TOO_LONG
#define STATUS_INVALID_PARAMETER_MIX                       LW_STATUS_INVALID_PARAMETER_MIX
#define STATUS_INVALID_QUOTA_LOWER                         LW_STATUS_INVALID_QUOTA_LOWER
#define STATUS_DISK_CORRUPT_ERROR                          LW_STATUS_DISK_CORRUPT_ERROR
#define STATUS_OBJECT_NAME_INVALID                         LW_STATUS_OBJECT_NAME_INVALID
#define STATUS_OBJECT_NAME_NOT_FOUND                       LW_STATUS_OBJECT_NAME_NOT_FOUND
#define STATUS_OBJECT_NAME_COLLISION                       LW_STATUS_OBJECT_NAME_COLLISION
#define STATUS_PORT_DISCONNECTED                           LW_STATUS_PORT_DISCONNECTED
#define STATUS_DEVICE_ALREADY_ATTACHED                     LW_STATUS_DEVICE_ALREADY_ATTACHED
#define STATUS_OBJECT_PATH_INVALID                         LW_STATUS_OBJECT_PATH_INVALID
#define STATUS_OBJECT_PATH_NOT_FOUND                       LW_STATUS_OBJECT_PATH_NOT_FOUND
#define STATUS_OBJECT_PATH_SYNTAX_BAD                      LW_STATUS_OBJECT_PATH_SYNTAX_BAD
#define STATUS_DATA_OVERRUN                                LW_STATUS_DATA_OVERRUN
#define STATUS_DATA_LATE_ERROR                             LW_STATUS_DATA_LATE_ERROR
#define STATUS_DATA_ERROR                                  LW_STATUS_DATA_ERROR
#define STATUS_CRC_ERROR                                   LW_STATUS_CRC_ERROR
#define STATUS_SECTION_TOO_BIG                             LW_STATUS_SECTION_TOO_BIG
#define STATUS_PORT_CONNECTION_REFUSED                     LW_STATUS_PORT_CONNECTION_REFUSED
#define STATUS_INVALID_PORT_HANDLE                         LW_STATUS_INVALID_PORT_HANDLE
#define STATUS_SHARING_VIOLATION                           LW_STATUS_SHARING_VIOLATION
#define STATUS_QUOTA_EXCEEDED                              LW_STATUS_QUOTA_EXCEEDED
#define STATUS_INVALID_PAGE_PROTECTION                     LW_STATUS_INVALID_PAGE_PROTECTION
#define STATUS_MUTANT_NOT_OWNED                            LW_STATUS_MUTANT_NOT_OWNED
#define STATUS_SEMAPHORE_LIMIT_EXCEEDED                    LW_STATUS_SEMAPHORE_LIMIT_EXCEEDED
#define STATUS_PORT_ALREADY_SET                            LW_STATUS_PORT_ALREADY_SET
#define STATUS_SECTION_NOT_IMAGE                           LW_STATUS_SECTION_NOT_IMAGE
#define STATUS_SUSPEND_COUNT_EXCEEDED                      LW_STATUS_SUSPEND_COUNT_EXCEEDED
#define STATUS_THREAD_IS_TERMINATING                       LW_STATUS_THREAD_IS_TERMINATING
#define STATUS_BAD_WORKING_SET_LIMIT                       LW_STATUS_BAD_WORKING_SET_LIMIT
#define STATUS_INCOMPATIBLE_FILE_MAP                       LW_STATUS_INCOMPATIBLE_FILE_MAP
#define STATUS_SECTION_PROTECTION                          LW_STATUS_SECTION_PROTECTION
#define STATUS_EAS_NOT_SUPPORTED                           LW_STATUS_EAS_NOT_SUPPORTED
#define STATUS_EA_TOO_LARGE                                LW_STATUS_EA_TOO_LARGE
#define STATUS_NONEXISTENT_EA_ENTRY                        LW_STATUS_NONEXISTENT_EA_ENTRY
#define STATUS_NO_EAS_ON_FILE                              LW_STATUS_NO_EAS_ON_FILE
#define STATUS_EA_CORRUPT_ERROR                            LW_STATUS_EA_CORRUPT_ERROR
#define STATUS_FILE_LOCK_CONFLICT                          LW_STATUS_FILE_LOCK_CONFLICT
#define STATUS_LOCK_NOT_GRANTED                            LW_STATUS_LOCK_NOT_GRANTED
#define STATUS_DELETE_PENDING                              LW_STATUS_DELETE_PENDING
#define STATUS_CTL_FILE_NOT_SUPPORTED                      LW_STATUS_CTL_FILE_NOT_SUPPORTED
#define STATUS_UNKNOWN_REVISION                            LW_STATUS_UNKNOWN_REVISION
#define STATUS_REVISION_MISMATCH                           LW_STATUS_REVISION_MISMATCH
#define STATUS_INVALID_OWNER                               LW_STATUS_INVALID_OWNER
#define STATUS_INVALID_PRIMARY_GROUP                       LW_STATUS_INVALID_PRIMARY_GROUP
#define STATUS_NO_IMPERSONATION_TOKEN                      LW_STATUS_NO_IMPERSONATION_TOKEN
#define STATUS_CANT_DISABLE_MANDATORY                      LW_STATUS_CANT_DISABLE_MANDATORY
#define STATUS_NO_LOGON_SERVERS                            LW_STATUS_NO_LOGON_SERVERS
#define STATUS_NO_SUCH_LOGON_SESSION                       LW_STATUS_NO_SUCH_LOGON_SESSION
#define STATUS_NO_SUCH_PRIVILEGE                           LW_STATUS_NO_SUCH_PRIVILEGE
#define STATUS_PRIVILEGE_NOT_HELD                          LW_STATUS_PRIVILEGE_NOT_HELD
#define STATUS_INVALID_ACCOUNT_NAME                        LW_STATUS_INVALID_ACCOUNT_NAME
#define STATUS_USER_EXISTS                                 LW_STATUS_USER_EXISTS
#define STATUS_NO_SUCH_USER                                LW_STATUS_NO_SUCH_USER
#define STATUS_GROUP_EXISTS                                LW_STATUS_GROUP_EXISTS
#define STATUS_NO_SUCH_GROUP                               LW_STATUS_NO_SUCH_GROUP
#define STATUS_MEMBER_IN_GROUP                             LW_STATUS_MEMBER_IN_GROUP
#define STATUS_MEMBER_NOT_IN_GROUP                         LW_STATUS_MEMBER_NOT_IN_GROUP
#define STATUS_LAST_ADMIN                                  LW_STATUS_LAST_ADMIN
#define STATUS_WRONG_PASSWORD                              LW_STATUS_WRONG_PASSWORD
#define STATUS_ILL_FORMED_PASSWORD                         LW_STATUS_ILL_FORMED_PASSWORD
#define STATUS_PASSWORD_RESTRICTION                        LW_STATUS_PASSWORD_RESTRICTION
#define STATUS_LOGON_FAILURE                               LW_STATUS_LOGON_FAILURE
#define STATUS_ACCOUNT_RESTRICTION                         LW_STATUS_ACCOUNT_RESTRICTION
#define STATUS_INVALID_LOGON_HOURS                         LW_STATUS_INVALID_LOGON_HOURS
#define STATUS_INVALID_WORKSTATION                         LW_STATUS_INVALID_WORKSTATION
#define STATUS_PASSWORD_EXPIRED                            LW_STATUS_PASSWORD_EXPIRED
#define STATUS_ACCOUNT_DISABLED                            LW_STATUS_ACCOUNT_DISABLED
#define STATUS_NONE_MAPPED                                 LW_STATUS_NONE_MAPPED
#define STATUS_TOO_MANY_LUIDS_REQUESTED                    LW_STATUS_TOO_MANY_LUIDS_REQUESTED
#define STATUS_LUIDS_EXHAUSTED                             LW_STATUS_LUIDS_EXHAUSTED
#define STATUS_INVALID_SUB_AUTHORITY                       LW_STATUS_INVALID_SUB_AUTHORITY
#define STATUS_INVALID_ACL                                 LW_STATUS_INVALID_ACL
#define STATUS_INVALID_SID                                 LW_STATUS_INVALID_SID
#define STATUS_INVALID_SECURITY_DESCR                      LW_STATUS_INVALID_SECURITY_DESCR
#define STATUS_PROCEDURE_NOT_FOUND                         LW_STATUS_PROCEDURE_NOT_FOUND
#define STATUS_INVALID_IMAGE_FORMAT                        LW_STATUS_INVALID_IMAGE_FORMAT
#define STATUS_NO_TOKEN                                    LW_STATUS_NO_TOKEN
#define STATUS_BAD_INHERITANCE_ACL                         LW_STATUS_BAD_INHERITANCE_ACL
#define STATUS_RANGE_NOT_LOCKED                            LW_STATUS_RANGE_NOT_LOCKED
#define STATUS_DISK_FULL                                   LW_STATUS_DISK_FULL
#define STATUS_SERVER_DISABLED                             LW_STATUS_SERVER_DISABLED
#define STATUS_SERVER_NOT_DISABLED                         LW_STATUS_SERVER_NOT_DISABLED
#define STATUS_TOO_MANY_GUIDS_REQUESTED                    LW_STATUS_TOO_MANY_GUIDS_REQUESTED
#define STATUS_GUIDS_EXHAUSTED                             LW_STATUS_GUIDS_EXHAUSTED
#define STATUS_INVALID_ID_AUTHORITY                        LW_STATUS_INVALID_ID_AUTHORITY
#define STATUS_AGENTS_EXHAUSTED                            LW_STATUS_AGENTS_EXHAUSTED
#define STATUS_INVALID_VOLUME_LABEL                        LW_STATUS_INVALID_VOLUME_LABEL
#define STATUS_SECTION_NOT_EXTENDED                        LW_STATUS_SECTION_NOT_EXTENDED
#define STATUS_NOT_MAPPED_DATA                             LW_STATUS_NOT_MAPPED_DATA
#define STATUS_RESOURCE_DATA_NOT_FOUND                     LW_STATUS_RESOURCE_DATA_NOT_FOUND
#define STATUS_RESOURCE_TYPE_NOT_FOUND                     LW_STATUS_RESOURCE_TYPE_NOT_FOUND
#define STATUS_RESOURCE_NAME_NOT_FOUND                     LW_STATUS_RESOURCE_NAME_NOT_FOUND
#define STATUS_ARRAY_BOUNDS_EXCEEDED                       LW_STATUS_ARRAY_BOUNDS_EXCEEDED
#define STATUS_FLOAT_DENORMAL_OPERAND                      LW_STATUS_FLOAT_DENORMAL_OPERAND
#define STATUS_FLOAT_DIVIDE_BY_ZERO                        LW_STATUS_FLOAT_DIVIDE_BY_ZERO
#define STATUS_FLOAT_INEXACT_RESULT                        LW_STATUS_FLOAT_INEXACT_RESULT
#define STATUS_FLOAT_INVALID_OPERATION                     LW_STATUS_FLOAT_INVALID_OPERATION
#define STATUS_FLOAT_OVERFLOW                              LW_STATUS_FLOAT_OVERFLOW
#define STATUS_FLOAT_STACK_CHECK                           LW_STATUS_FLOAT_STACK_CHECK
#define STATUS_FLOAT_UNDERFLOW                             LW_STATUS_FLOAT_UNDERFLOW
#define STATUS_INTEGER_DIVIDE_BY_ZERO                      LW_STATUS_INTEGER_DIVIDE_BY_ZERO
#define STATUS_INTEGER_OVERFLOW                            LW_STATUS_INTEGER_OVERFLOW
#define STATUS_PRIVILEGED_INSTRUCTION                      LW_STATUS_PRIVILEGED_INSTRUCTION
#define STATUS_TOO_MANY_PAGING_FILES                       LW_STATUS_TOO_MANY_PAGING_FILES
#define STATUS_FILE_INVALID                                LW_STATUS_FILE_INVALID
#define STATUS_ALLOTTED_SPACE_EXCEEDED                     LW_STATUS_ALLOTTED_SPACE_EXCEEDED
#define STATUS_INSUFFICIENT_RESOURCES                      LW_STATUS_INSUFFICIENT_RESOURCES
#define STATUS_DFS_EXIT_PATH_FOUND                         LW_STATUS_DFS_EXIT_PATH_FOUND
#define STATUS_DEVICE_DATA_ERROR                           LW_STATUS_DEVICE_DATA_ERROR
#define STATUS_DEVICE_NOT_CONNECTED                        LW_STATUS_DEVICE_NOT_CONNECTED
#define STATUS_DEVICE_POWER_FAILURE                        LW_STATUS_DEVICE_POWER_FAILURE
#define STATUS_FREE_VM_NOT_AT_BASE                         LW_STATUS_FREE_VM_NOT_AT_BASE
#define STATUS_MEMORY_NOT_ALLOCATED                        LW_STATUS_MEMORY_NOT_ALLOCATED
#define STATUS_WORKING_SET_QUOTA                           LW_STATUS_WORKING_SET_QUOTA
#define STATUS_MEDIA_WRITE_PROTECTED                       LW_STATUS_MEDIA_WRITE_PROTECTED
#define STATUS_DEVICE_NOT_READY                            LW_STATUS_DEVICE_NOT_READY
#define STATUS_INVALID_GROUP_ATTRIBUTES                    LW_STATUS_INVALID_GROUP_ATTRIBUTES
#define STATUS_BAD_IMPERSONATION_LEVEL                     LW_STATUS_BAD_IMPERSONATION_LEVEL
#define STATUS_CANT_OPEN_ANONYMOUS                         LW_STATUS_CANT_OPEN_ANONYMOUS
#define STATUS_BAD_VALIDATION_CLASS                        LW_STATUS_BAD_VALIDATION_CLASS
#define STATUS_BAD_TOKEN_TYPE                              LW_STATUS_BAD_TOKEN_TYPE
#define STATUS_BAD_MASTER_BOOT_RECORD                      LW_STATUS_BAD_MASTER_BOOT_RECORD
#define STATUS_INSTRUCTION_MISALIGNMENT                    LW_STATUS_INSTRUCTION_MISALIGNMENT
#define STATUS_INSTANCE_NOT_AVAILABLE                      LW_STATUS_INSTANCE_NOT_AVAILABLE
#define STATUS_PIPE_NOT_AVAILABLE                          LW_STATUS_PIPE_NOT_AVAILABLE
#define STATUS_INVALID_PIPE_STATE                          LW_STATUS_INVALID_PIPE_STATE
#define STATUS_PIPE_BUSY                                   LW_STATUS_PIPE_BUSY
#define STATUS_ILLEGAL_FUNCTION                            LW_STATUS_ILLEGAL_FUNCTION
#define STATUS_PIPE_DISCONNECTED                           LW_STATUS_PIPE_DISCONNECTED
#define STATUS_PIPE_CLOSING                                LW_STATUS_PIPE_CLOSING
#define STATUS_PIPE_CONNECTED                              LW_STATUS_PIPE_CONNECTED
#define STATUS_PIPE_LISTENING                              LW_STATUS_PIPE_LISTENING
#define STATUS_INVALID_READ_MODE                           LW_STATUS_INVALID_READ_MODE
#define STATUS_IO_TIMEOUT                                  LW_STATUS_IO_TIMEOUT
#define STATUS_FILE_FORCED_CLOSED                          LW_STATUS_FILE_FORCED_CLOSED
#define STATUS_PROFILING_NOT_STARTED                       LW_STATUS_PROFILING_NOT_STARTED
#define STATUS_PROFILING_NOT_STOPPED                       LW_STATUS_PROFILING_NOT_STOPPED
#define STATUS_COULD_NOT_INTERPRET                         LW_STATUS_COULD_NOT_INTERPRET
#define STATUS_FILE_IS_A_DIRECTORY                         LW_STATUS_FILE_IS_A_DIRECTORY
#define STATUS_NOT_SUPPORTED                               LW_STATUS_NOT_SUPPORTED
#define STATUS_REMOTE_NOT_LISTENING                        LW_STATUS_REMOTE_NOT_LISTENING
#define STATUS_DUPLICATE_NAME                              LW_STATUS_DUPLICATE_NAME
#define STATUS_BAD_NETWORK_PATH                            LW_STATUS_BAD_NETWORK_PATH
#define STATUS_NETWORK_BUSY                                LW_STATUS_NETWORK_BUSY
#define STATUS_DEVICE_DOES_NOT_EXIST                       LW_STATUS_DEVICE_DOES_NOT_EXIST
#define STATUS_TOO_MANY_COMMANDS                           LW_STATUS_TOO_MANY_COMMANDS
#define STATUS_ADAPTER_HARDWARE_ERROR                      LW_STATUS_ADAPTER_HARDWARE_ERROR
#define STATUS_INVALID_NETWORK_RESPONSE                    LW_STATUS_INVALID_NETWORK_RESPONSE
#define STATUS_UNEXPECTED_NETWORK_ERROR                    LW_STATUS_UNEXPECTED_NETWORK_ERROR
#define STATUS_BAD_REMOTE_ADAPTER                          LW_STATUS_BAD_REMOTE_ADAPTER
#define STATUS_PRINT_QUEUE_FULL                            LW_STATUS_PRINT_QUEUE_FULL
#define STATUS_NO_SPOOL_SPACE                              LW_STATUS_NO_SPOOL_SPACE
#define STATUS_PRINT_CANCELLED                             LW_STATUS_PRINT_CANCELLED
#define STATUS_NETWORK_ACCESS_DENIED                       LW_STATUS_NETWORK_ACCESS_DENIED
#define STATUS_BAD_DEVICE_TYPE                             LW_STATUS_BAD_DEVICE_TYPE
#define STATUS_BAD_NETWORK_NAME                            LW_STATUS_BAD_NETWORK_NAME
#define STATUS_TOO_MANY_NAMES                              LW_STATUS_TOO_MANY_NAMES
#define STATUS_TOO_MANY_SESSIONS                           LW_STATUS_TOO_MANY_SESSIONS
#define STATUS_SHARING_PAUSED                              LW_STATUS_SHARING_PAUSED
#define STATUS_REQUEST_NOT_ACCEPTED                        LW_STATUS_REQUEST_NOT_ACCEPTED
#define STATUS_REDIRECTOR_PAUSED                           LW_STATUS_REDIRECTOR_PAUSED
#define STATUS_NET_WRITE_FAULT                             LW_STATUS_NET_WRITE_FAULT
#define STATUS_PROFILING_AT_LIMIT                          LW_STATUS_PROFILING_AT_LIMIT
#define STATUS_NOT_SAME_DEVICE                             LW_STATUS_NOT_SAME_DEVICE
#define STATUS_FILE_RENAMED                                LW_STATUS_FILE_RENAMED
#define STATUS_VIRTUAL_CIRCUIT_CLOSED                      LW_STATUS_VIRTUAL_CIRCUIT_CLOSED
#define STATUS_NO_SECURITY_ON_OBJECT                       LW_STATUS_NO_SECURITY_ON_OBJECT
#define STATUS_CANT_WAIT                                   LW_STATUS_CANT_WAIT
#define STATUS_PIPE_EMPTY                                  LW_STATUS_PIPE_EMPTY
#define STATUS_CANT_ACCESS_DOMAIN_INFO                     LW_STATUS_CANT_ACCESS_DOMAIN_INFO
#define STATUS_CANT_TERMINATE_SELF                         LW_STATUS_CANT_TERMINATE_SELF
#define STATUS_INVALID_SERVER_STATE                        LW_STATUS_INVALID_SERVER_STATE
#define STATUS_INVALID_DOMAIN_STATE                        LW_STATUS_INVALID_DOMAIN_STATE
#define STATUS_INVALID_DOMAIN_ROLE                         LW_STATUS_INVALID_DOMAIN_ROLE
#define STATUS_NO_SUCH_DOMAIN                              LW_STATUS_NO_SUCH_DOMAIN
#define STATUS_DOMAIN_EXISTS                               LW_STATUS_DOMAIN_EXISTS
#define STATUS_DOMAIN_LIMIT_EXCEEDED                       LW_STATUS_DOMAIN_LIMIT_EXCEEDED
#define STATUS_OPLOCK_NOT_GRANTED                          LW_STATUS_OPLOCK_NOT_GRANTED
#define STATUS_INVALID_OPLOCK_PROTOCOL                     LW_STATUS_INVALID_OPLOCK_PROTOCOL
#define STATUS_INTERNAL_DB_CORRUPTION                      LW_STATUS_INTERNAL_DB_CORRUPTION
#define STATUS_INTERNAL_ERROR                              LW_STATUS_INTERNAL_ERROR
#define STATUS_GENERIC_NOT_MAPPED                          LW_STATUS_GENERIC_NOT_MAPPED
#define STATUS_BAD_DESCRIPTOR_FORMAT                       LW_STATUS_BAD_DESCRIPTOR_FORMAT
#define STATUS_INVALID_USER_BUFFER                         LW_STATUS_INVALID_USER_BUFFER
#define STATUS_UNEXPECTED_IO_ERROR                         LW_STATUS_UNEXPECTED_IO_ERROR
#define STATUS_UNEXPECTED_MM_CREATE_ERR                    LW_STATUS_UNEXPECTED_MM_CREATE_ERR
#define STATUS_UNEXPECTED_MM_MAP_ERROR                     LW_STATUS_UNEXPECTED_MM_MAP_ERROR
#define STATUS_UNEXPECTED_MM_EXTEND_ERR                    LW_STATUS_UNEXPECTED_MM_EXTEND_ERR
#define STATUS_NOT_LOGON_PROCESS                           LW_STATUS_NOT_LOGON_PROCESS
#define STATUS_LOGON_SESSION_EXISTS                        LW_STATUS_LOGON_SESSION_EXISTS
#define STATUS_INVALID_PARAMETER_1                         LW_STATUS_INVALID_PARAMETER_1
#define STATUS_INVALID_PARAMETER_2                         LW_STATUS_INVALID_PARAMETER_2
#define STATUS_INVALID_PARAMETER_3                         LW_STATUS_INVALID_PARAMETER_3
#define STATUS_INVALID_PARAMETER_4                         LW_STATUS_INVALID_PARAMETER_4
#define STATUS_INVALID_PARAMETER_5                         LW_STATUS_INVALID_PARAMETER_5
#define STATUS_INVALID_PARAMETER_6                         LW_STATUS_INVALID_PARAMETER_6
#define STATUS_INVALID_PARAMETER_7                         LW_STATUS_INVALID_PARAMETER_7
#define STATUS_INVALID_PARAMETER_8                         LW_STATUS_INVALID_PARAMETER_8
#define STATUS_INVALID_PARAMETER_9                         LW_STATUS_INVALID_PARAMETER_9
#define STATUS_INVALID_PARAMETER_10                        LW_STATUS_INVALID_PARAMETER_10
#define STATUS_INVALID_PARAMETER_11                        LW_STATUS_INVALID_PARAMETER_11
#define STATUS_INVALID_PARAMETER_12                        LW_STATUS_INVALID_PARAMETER_12
#define STATUS_REDIRECTOR_NOT_STARTED                      LW_STATUS_REDIRECTOR_NOT_STARTED
#define STATUS_REDIRECTOR_STARTED                          LW_STATUS_REDIRECTOR_STARTED
#define STATUS_STACK_OVERFLOW                              LW_STATUS_STACK_OVERFLOW
#define STATUS_NO_SUCH_PACKAGE                             LW_STATUS_NO_SUCH_PACKAGE
#define STATUS_BAD_FUNCTION_TABLE                          LW_STATUS_BAD_FUNCTION_TABLE
#define STATUS_VARIABLE_NOT_FOUND                          LW_STATUS_VARIABLE_NOT_FOUND
#define STATUS_DIRECTORY_NOT_EMPTY                         LW_STATUS_DIRECTORY_NOT_EMPTY
#define STATUS_FILE_CORRUPT_ERROR                          LW_STATUS_FILE_CORRUPT_ERROR
#define STATUS_NOT_A_DIRECTORY                             LW_STATUS_NOT_A_DIRECTORY
#define STATUS_BAD_LOGON_SESSION_STATE                     LW_STATUS_BAD_LOGON_SESSION_STATE
#define STATUS_LOGON_SESSION_COLLISION                     LW_STATUS_LOGON_SESSION_COLLISION
#define STATUS_NAME_TOO_LONG                               LW_STATUS_NAME_TOO_LONG
#define STATUS_FILES_OPEN                                  LW_STATUS_FILES_OPEN
#define STATUS_CONNECTION_IN_USE                           LW_STATUS_CONNECTION_IN_USE
#define STATUS_MESSAGE_NOT_FOUND                           LW_STATUS_MESSAGE_NOT_FOUND
#define STATUS_PROCESS_IS_TERMINATING                      LW_STATUS_PROCESS_IS_TERMINATING
#define STATUS_INVALID_LOGON_TYPE                          LW_STATUS_INVALID_LOGON_TYPE
#define STATUS_NO_GUID_TRANSLATION                         LW_STATUS_NO_GUID_TRANSLATION
#define STATUS_CANNOT_IMPERSONATE                          LW_STATUS_CANNOT_IMPERSONATE
#define STATUS_IMAGE_ALREADY_LOADED                        LW_STATUS_IMAGE_ALREADY_LOADED
#define STATUS_ABIOS_NOT_PRESENT                           LW_STATUS_ABIOS_NOT_PRESENT
#define STATUS_ABIOS_LID_NOT_EXIST                         LW_STATUS_ABIOS_LID_NOT_EXIST
#define STATUS_ABIOS_LID_ALREADY_OWNED                     LW_STATUS_ABIOS_LID_ALREADY_OWNED
#define STATUS_ABIOS_NOT_LID_OWNER                         LW_STATUS_ABIOS_NOT_LID_OWNER
#define STATUS_ABIOS_INVALID_COMMAND                       LW_STATUS_ABIOS_INVALID_COMMAND
#define STATUS_ABIOS_INVALID_LID                           LW_STATUS_ABIOS_INVALID_LID
#define STATUS_ABIOS_SELECTOR_NOT_AVAILABLE                LW_STATUS_ABIOS_SELECTOR_NOT_AVAILABLE
#define STATUS_ABIOS_INVALID_SELECTOR                      LW_STATUS_ABIOS_INVALID_SELECTOR
#define STATUS_NO_LDT                                      LW_STATUS_NO_LDT
#define STATUS_INVALID_LDT_SIZE                            LW_STATUS_INVALID_LDT_SIZE
#define STATUS_INVALID_LDT_OFFSET                          LW_STATUS_INVALID_LDT_OFFSET
#define STATUS_INVALID_LDT_DESCRIPTOR                      LW_STATUS_INVALID_LDT_DESCRIPTOR
#define STATUS_INVALID_IMAGE_NE_FORMAT                     LW_STATUS_INVALID_IMAGE_NE_FORMAT
#define STATUS_RXACT_INVALID_STATE                         LW_STATUS_RXACT_INVALID_STATE
#define STATUS_RXACT_COMMIT_FAILURE                        LW_STATUS_RXACT_COMMIT_FAILURE
#define STATUS_MAPPED_FILE_SIZE_ZERO                       LW_STATUS_MAPPED_FILE_SIZE_ZERO
#define STATUS_TOO_MANY_OPENED_FILES                       LW_STATUS_TOO_MANY_OPENED_FILES
#define STATUS_CANCELLED                                   LW_STATUS_CANCELLED
#define STATUS_CANNOT_DELETE                               LW_STATUS_CANNOT_DELETE
#define STATUS_INVALID_COMPUTER_NAME                       LW_STATUS_INVALID_COMPUTER_NAME
#define STATUS_FILE_DELETED                                LW_STATUS_FILE_DELETED
#define STATUS_SPECIAL_ACCOUNT                             LW_STATUS_SPECIAL_ACCOUNT
#define STATUS_SPECIAL_GROUP                               LW_STATUS_SPECIAL_GROUP
#define STATUS_SPECIAL_USER                                LW_STATUS_SPECIAL_USER
#define STATUS_MEMBERS_PRIMARY_GROUP                       LW_STATUS_MEMBERS_PRIMARY_GROUP
#define STATUS_FILE_CLOSED                                 LW_STATUS_FILE_CLOSED
#define STATUS_TOO_MANY_THREADS                            LW_STATUS_TOO_MANY_THREADS
#define STATUS_THREAD_NOT_IN_PROCESS                       LW_STATUS_THREAD_NOT_IN_PROCESS
#define STATUS_TOKEN_ALREADY_IN_USE                        LW_STATUS_TOKEN_ALREADY_IN_USE
#define STATUS_PAGEFILE_QUOTA_EXCEEDED                     LW_STATUS_PAGEFILE_QUOTA_EXCEEDED
#define STATUS_COMMITMENT_LIMIT                            LW_STATUS_COMMITMENT_LIMIT
#define STATUS_INVALID_IMAGE_LE_FORMAT                     LW_STATUS_INVALID_IMAGE_LE_FORMAT
#define STATUS_INVALID_IMAGE_NOT_MZ                        LW_STATUS_INVALID_IMAGE_NOT_MZ
#define STATUS_INVALID_IMAGE_PROTECT                       LW_STATUS_INVALID_IMAGE_PROTECT
#define STATUS_INVALID_IMAGE_WIN_16                        LW_STATUS_INVALID_IMAGE_WIN_16
#define STATUS_LOGON_SERVER_CONFLICT                       LW_STATUS_LOGON_SERVER_CONFLICT
#define STATUS_TIME_DIFFERENCE_AT_DC                       LW_STATUS_TIME_DIFFERENCE_AT_DC
#define STATUS_SYNCHRONIZATION_REQUIRED                    LW_STATUS_SYNCHRONIZATION_REQUIRED
#define STATUS_DLL_NOT_FOUND                               LW_STATUS_DLL_NOT_FOUND
#define STATUS_OPEN_FAILED                                 LW_STATUS_OPEN_FAILED
#define STATUS_IO_PRIVILEGE_FAILED                         LW_STATUS_IO_PRIVILEGE_FAILED
#define STATUS_ORDINAL_NOT_FOUND                           LW_STATUS_ORDINAL_NOT_FOUND
#define STATUS_ENTRYPOINT_NOT_FOUND                        LW_STATUS_ENTRYPOINT_NOT_FOUND
#define STATUS_CONTROL_C_EXIT                              LW_STATUS_CONTROL_C_EXIT
#define STATUS_LOCAL_DISCONNECT                            LW_STATUS_LOCAL_DISCONNECT
#define STATUS_REMOTE_DISCONNECT                           LW_STATUS_REMOTE_DISCONNECT
#define STATUS_REMOTE_RESOURCES                            LW_STATUS_REMOTE_RESOURCES
#define STATUS_LINK_FAILED                                 LW_STATUS_LINK_FAILED
#define STATUS_LINK_TIMEOUT                                LW_STATUS_LINK_TIMEOUT
#define STATUS_INVALID_CONNECTION                          LW_STATUS_INVALID_CONNECTION
#define STATUS_INVALID_ADDRESS                             LW_STATUS_INVALID_ADDRESS
#define STATUS_DLL_INIT_FAILED                             LW_STATUS_DLL_INIT_FAILED
#define STATUS_MISSING_SYSTEMFILE                          LW_STATUS_MISSING_SYSTEMFILE
#define STATUS_UNHANDLED_EXCEPTION                         LW_STATUS_UNHANDLED_EXCEPTION
#define STATUS_APP_INIT_FAILURE                            LW_STATUS_APP_INIT_FAILURE
#define STATUS_PAGEFILE_CREATE_FAILED                      LW_STATUS_PAGEFILE_CREATE_FAILED
#define STATUS_NO_PAGEFILE                                 LW_STATUS_NO_PAGEFILE
#define STATUS_INVALID_LEVEL                               LW_STATUS_INVALID_LEVEL
#define STATUS_WRONG_PASSWORD_CORE                         LW_STATUS_WRONG_PASSWORD_CORE
#define STATUS_ILLEGAL_FLOAT_CONTEXT                       LW_STATUS_ILLEGAL_FLOAT_CONTEXT
#define STATUS_PIPE_BROKEN                                 LW_STATUS_PIPE_BROKEN
#define STATUS_REGISTRY_CORRUPT                            LW_STATUS_REGISTRY_CORRUPT
#define STATUS_REGISTRY_IO_FAILED                          LW_STATUS_REGISTRY_IO_FAILED
#define STATUS_NO_EVENT_PAIR                               LW_STATUS_NO_EVENT_PAIR
#define STATUS_UNRECOGNIZED_VOLUME                         LW_STATUS_UNRECOGNIZED_VOLUME
#define STATUS_SERIAL_NO_DEVICE_INITED                     LW_STATUS_SERIAL_NO_DEVICE_INITED
#define STATUS_NO_SUCH_ALIAS                               LW_STATUS_NO_SUCH_ALIAS
#define STATUS_MEMBER_NOT_IN_ALIAS                         LW_STATUS_MEMBER_NOT_IN_ALIAS
#define STATUS_MEMBER_IN_ALIAS                             LW_STATUS_MEMBER_IN_ALIAS
#define STATUS_ALIAS_EXISTS                                LW_STATUS_ALIAS_EXISTS
#define STATUS_LOGON_NOT_GRANTED                           LW_STATUS_LOGON_NOT_GRANTED
#define STATUS_TOO_MANY_SECRETS                            LW_STATUS_TOO_MANY_SECRETS
#define STATUS_SECRET_TOO_LONG                             LW_STATUS_SECRET_TOO_LONG
#define STATUS_INTERNAL_DB_ERROR                           LW_STATUS_INTERNAL_DB_ERROR
#define STATUS_FULLSCREEN_MODE                             LW_STATUS_FULLSCREEN_MODE
#define STATUS_TOO_MANY_CONTEXT_IDS                        LW_STATUS_TOO_MANY_CONTEXT_IDS
#define STATUS_LOGON_TYPE_NOT_GRANTED                      LW_STATUS_LOGON_TYPE_NOT_GRANTED
#define STATUS_NOT_REGISTRY_FILE                           LW_STATUS_NOT_REGISTRY_FILE
#define STATUS_NT_CROSS_ENCRYPTION_REQUIRED                LW_STATUS_NT_CROSS_ENCRYPTION_REQUIRED
#define STATUS_DOMAIN_CTRLR_CONFIG_ERROR                   LW_STATUS_DOMAIN_CTRLR_CONFIG_ERROR
#define STATUS_FT_MISSING_MEMBER                           LW_STATUS_FT_MISSING_MEMBER
#define STATUS_ILL_FORMED_SERVICE_ENTRY                    LW_STATUS_ILL_FORMED_SERVICE_ENTRY
#define STATUS_ILLEGAL_CHARACTER                           LW_STATUS_ILLEGAL_CHARACTER
#define STATUS_UNMAPPABLE_CHARACTER                        LW_STATUS_UNMAPPABLE_CHARACTER
#define STATUS_UNDEFINED_CHARACTER                         LW_STATUS_UNDEFINED_CHARACTER
#define STATUS_FLOPPY_VOLUME                               LW_STATUS_FLOPPY_VOLUME
#define STATUS_FLOPPY_ID_MARK_NOT_FOUND                    LW_STATUS_FLOPPY_ID_MARK_NOT_FOUND
#define STATUS_FLOPPY_WRONG_CYLINDER                       LW_STATUS_FLOPPY_WRONG_CYLINDER
#define STATUS_FLOPPY_UNKNOWN_ERROR                        LW_STATUS_FLOPPY_UNKNOWN_ERROR
#define STATUS_FLOPPY_BAD_REGISTERS                        LW_STATUS_FLOPPY_BAD_REGISTERS
#define STATUS_DISK_RECALIBRATE_FAILED                     LW_STATUS_DISK_RECALIBRATE_FAILED
#define STATUS_DISK_OPERATION_FAILED                       LW_STATUS_DISK_OPERATION_FAILED
#define STATUS_DISK_RESET_FAILED                           LW_STATUS_DISK_RESET_FAILED
#define STATUS_SHARED_IRQ_BUSY                             LW_STATUS_SHARED_IRQ_BUSY
#define STATUS_FT_ORPHANING                                LW_STATUS_FT_ORPHANING
#define STATUS_BIOS_FAILED_TO_CONNECT_INTERRUPT            LW_STATUS_BIOS_FAILED_TO_CONNECT_INTERRUPT
#define STATUS_PARTITION_FAILURE                           LW_STATUS_PARTITION_FAILURE
#define STATUS_INVALID_BLOCK_LENGTH                        LW_STATUS_INVALID_BLOCK_LENGTH
#define STATUS_DEVICE_NOT_PARTITIONED                      LW_STATUS_DEVICE_NOT_PARTITIONED
#define STATUS_UNABLE_TO_LOCK_MEDIA                        LW_STATUS_UNABLE_TO_LOCK_MEDIA
#define STATUS_UNABLE_TO_UNLOAD_MEDIA                      LW_STATUS_UNABLE_TO_UNLOAD_MEDIA
#define STATUS_EOM_OVERFLOW                                LW_STATUS_EOM_OVERFLOW
#define STATUS_NO_MEDIA                                    LW_STATUS_NO_MEDIA
#define STATUS_NO_SUCH_MEMBER                              LW_STATUS_NO_SUCH_MEMBER
#define STATUS_INVALID_MEMBER                              LW_STATUS_INVALID_MEMBER
#define STATUS_KEY_DELETED                                 LW_STATUS_KEY_DELETED
#define STATUS_NO_LOG_SPACE                                LW_STATUS_NO_LOG_SPACE
#define STATUS_TOO_MANY_SIDS                               LW_STATUS_TOO_MANY_SIDS
#define STATUS_LM_CROSS_ENCRYPTION_REQUIRED                LW_STATUS_LM_CROSS_ENCRYPTION_REQUIRED
#define STATUS_KEY_HAS_CHILDREN                            LW_STATUS_KEY_HAS_CHILDREN
#define STATUS_CHILD_MUST_BE_VOLATILE                      LW_STATUS_CHILD_MUST_BE_VOLATILE
#define STATUS_DEVICE_CONFIGURATION_ERROR                  LW_STATUS_DEVICE_CONFIGURATION_ERROR
#define STATUS_DRIVER_INTERNAL_ERROR                       LW_STATUS_DRIVER_INTERNAL_ERROR
#define STATUS_INVALID_DEVICE_STATE                        LW_STATUS_INVALID_DEVICE_STATE
#define STATUS_IO_DEVICE_ERROR                             LW_STATUS_IO_DEVICE_ERROR
#define STATUS_DEVICE_PROTOCOL_ERROR                       LW_STATUS_DEVICE_PROTOCOL_ERROR
#define STATUS_BACKUP_CONTROLLER                           LW_STATUS_BACKUP_CONTROLLER
#define STATUS_LOG_FILE_FULL                               LW_STATUS_LOG_FILE_FULL
#define STATUS_TOO_LATE                                    LW_STATUS_TOO_LATE
#define STATUS_NO_TRUST_LSA_SECRET                         LW_STATUS_NO_TRUST_LSA_SECRET
#define STATUS_NO_TRUST_SAM_ACCOUNT                        LW_STATUS_NO_TRUST_SAM_ACCOUNT
#define STATUS_TRUSTED_DOMAIN_FAILURE                      LW_STATUS_TRUSTED_DOMAIN_FAILURE
#define STATUS_TRUSTED_RELATIONSHIP_FAILURE                LW_STATUS_TRUSTED_RELATIONSHIP_FAILURE
#define STATUS_EVENTLOG_FILE_CORRUPT                       LW_STATUS_EVENTLOG_FILE_CORRUPT
#define STATUS_EVENTLOG_CANT_START                         LW_STATUS_EVENTLOG_CANT_START
#define STATUS_TRUST_FAILURE                               LW_STATUS_TRUST_FAILURE
#define STATUS_MUTANT_LIMIT_EXCEEDED                       LW_STATUS_MUTANT_LIMIT_EXCEEDED
#define STATUS_NETLOGON_NOT_STARTED                        LW_STATUS_NETLOGON_NOT_STARTED
#define STATUS_ACCOUNT_EXPIRED                             LW_STATUS_ACCOUNT_EXPIRED
#define STATUS_POSSIBLE_DEADLOCK                           LW_STATUS_POSSIBLE_DEADLOCK
#define STATUS_NETWORK_CREDENTIAL_CONFLICT                 LW_STATUS_NETWORK_CREDENTIAL_CONFLICT
#define STATUS_REMOTE_SESSION_LIMIT                        LW_STATUS_REMOTE_SESSION_LIMIT
#define STATUS_EVENTLOG_FILE_CHANGED                       LW_STATUS_EVENTLOG_FILE_CHANGED
#define STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT           LW_STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT
#define STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT           LW_STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT
#define STATUS_NOLOGON_SERVER_TRUST_ACCOUNT                LW_STATUS_NOLOGON_SERVER_TRUST_ACCOUNT
#define STATUS_DOMAIN_TRUST_INCONSISTENT                   LW_STATUS_DOMAIN_TRUST_INCONSISTENT
#define STATUS_FS_DRIVER_REQUIRED                          LW_STATUS_FS_DRIVER_REQUIRED
#define STATUS_NO_USER_SESSION_KEY                         LW_STATUS_NO_USER_SESSION_KEY
#define STATUS_USER_SESSION_DELETED                        LW_STATUS_USER_SESSION_DELETED
#define STATUS_RESOURCE_LANG_NOT_FOUND                     LW_STATUS_RESOURCE_LANG_NOT_FOUND
#define STATUS_INSUFF_SERVER_RESOURCES                     LW_STATUS_INSUFF_SERVER_RESOURCES
#define STATUS_INVALID_BUFFER_SIZE                         LW_STATUS_INVALID_BUFFER_SIZE
#define STATUS_INVALID_ADDRESS_COMPONENT                   LW_STATUS_INVALID_ADDRESS_COMPONENT
#define STATUS_INVALID_ADDRESS_WILDCARD                    LW_STATUS_INVALID_ADDRESS_WILDCARD
#define STATUS_TOO_MANY_ADDRESSES                          LW_STATUS_TOO_MANY_ADDRESSES
#define STATUS_ADDRESS_ALREADY_EXISTS                      LW_STATUS_ADDRESS_ALREADY_EXISTS
#define STATUS_ADDRESS_CLOSED                              LW_STATUS_ADDRESS_CLOSED
#define STATUS_CONNECTION_DISCONNECTED                     LW_STATUS_CONNECTION_DISCONNECTED
#define STATUS_CONNECTION_RESET                            LW_STATUS_CONNECTION_RESET
#define STATUS_TOO_MANY_NODES                              LW_STATUS_TOO_MANY_NODES
#define STATUS_TRANSACTION_ABORTED                         LW_STATUS_TRANSACTION_ABORTED
#define STATUS_TRANSACTION_TIMED_OUT                       LW_STATUS_TRANSACTION_TIMED_OUT
#define STATUS_TRANSACTION_NO_RELEASE                      LW_STATUS_TRANSACTION_NO_RELEASE
#define STATUS_TRANSACTION_NO_MATCH                        LW_STATUS_TRANSACTION_NO_MATCH
#define STATUS_TRANSACTION_RESPONDED                       LW_STATUS_TRANSACTION_RESPONDED
#define STATUS_TRANSACTION_INVALID_ID                      LW_STATUS_TRANSACTION_INVALID_ID
#define STATUS_TRANSACTION_INVALID_TYPE                    LW_STATUS_TRANSACTION_INVALID_TYPE
#define STATUS_NOT_SERVER_SESSION                          LW_STATUS_NOT_SERVER_SESSION
#define STATUS_NOT_CLIENT_SESSION                          LW_STATUS_NOT_CLIENT_SESSION
#define STATUS_CANNOT_LOAD_REGISTRY_FILE                   LW_STATUS_CANNOT_LOAD_REGISTRY_FILE
#define STATUS_DEBUG_ATTACH_FAILED                         LW_STATUS_DEBUG_ATTACH_FAILED
#define STATUS_SYSTEM_PROCESS_TERMINATED                   LW_STATUS_SYSTEM_PROCESS_TERMINATED
#define STATUS_DATA_NOT_ACCEPTED                           LW_STATUS_DATA_NOT_ACCEPTED
#define STATUS_NO_BROWSER_SERVERS_FOUND                    LW_STATUS_NO_BROWSER_SERVERS_FOUND
#define STATUS_VDM_HARD_ERROR                              LW_STATUS_VDM_HARD_ERROR
#define STATUS_DRIVER_CANCEL_TIMEOUT                       LW_STATUS_DRIVER_CANCEL_TIMEOUT
#define STATUS_REPLY_MESSAGE_MISMATCH                      LW_STATUS_REPLY_MESSAGE_MISMATCH
#define STATUS_MAPPED_ALIGNMENT                            LW_STATUS_MAPPED_ALIGNMENT
#define STATUS_IMAGE_CHECKSUM_MISMATCH                     LW_STATUS_IMAGE_CHECKSUM_MISMATCH
#define STATUS_LOST_WRITEBEHIND_DATA                       LW_STATUS_LOST_WRITEBEHIND_DATA
#define STATUS_CLIENT_SERVER_PARAMETERS_INVALID            LW_STATUS_CLIENT_SERVER_PARAMETERS_INVALID
#define STATUS_PASSWORD_MUST_CHANGE                        LW_STATUS_PASSWORD_MUST_CHANGE
#define STATUS_NOT_FOUND                                   LW_STATUS_NOT_FOUND
#define STATUS_NOT_TINY_STREAM                             LW_STATUS_NOT_TINY_STREAM
#define STATUS_RECOVERY_FAILURE                            LW_STATUS_RECOVERY_FAILURE
#define STATUS_STACK_OVERFLOW_READ                         LW_STATUS_STACK_OVERFLOW_READ
#define STATUS_FAIL_CHECK                                  LW_STATUS_FAIL_CHECK
#define STATUS_DUPLICATE_OBJECTID                          LW_STATUS_DUPLICATE_OBJECTID
#define STATUS_OBJECTID_EXISTS                             LW_STATUS_OBJECTID_EXISTS
#define STATUS_CONVERT_TO_LARGE                            LW_STATUS_CONVERT_TO_LARGE
#define STATUS_RETRY                                       LW_STATUS_RETRY
#define STATUS_FOUND_OUT_OF_SCOPE                          LW_STATUS_FOUND_OUT_OF_SCOPE
#define STATUS_ALLOCATE_BUCKET                             LW_STATUS_ALLOCATE_BUCKET
#define STATUS_PROPSET_NOT_FOUND                           LW_STATUS_PROPSET_NOT_FOUND
#define STATUS_MARSHALL_OVERFLOW                           LW_STATUS_MARSHALL_OVERFLOW
#define STATUS_INVALID_VARIANT                             LW_STATUS_INVALID_VARIANT
#define STATUS_DOMAIN_CONTROLLER_NOT_FOUND                 LW_STATUS_DOMAIN_CONTROLLER_NOT_FOUND
#define STATUS_ACCOUNT_LOCKED_OUT                          LW_STATUS_ACCOUNT_LOCKED_OUT
#define STATUS_HANDLE_NOT_CLOSABLE                         LW_STATUS_HANDLE_NOT_CLOSABLE
#define STATUS_CONNECTION_REFUSED                          LW_STATUS_CONNECTION_REFUSED
#define STATUS_GRACEFUL_DISCONNECT                         LW_STATUS_GRACEFUL_DISCONNECT
#define STATUS_ADDRESS_ALREADY_ASSOCIATED                  LW_STATUS_ADDRESS_ALREADY_ASSOCIATED
#define STATUS_ADDRESS_NOT_ASSOCIATED                      LW_STATUS_ADDRESS_NOT_ASSOCIATED
#define STATUS_CONNECTION_INVALID                          LW_STATUS_CONNECTION_INVALID
#define STATUS_CONNECTION_ACTIVE                           LW_STATUS_CONNECTION_ACTIVE
#define STATUS_NETWORK_UNREACHABLE                         LW_STATUS_NETWORK_UNREACHABLE
#define STATUS_HOST_UNREACHABLE                            LW_STATUS_HOST_UNREACHABLE
#define STATUS_PROTOCOL_UNREACHABLE                        LW_STATUS_PROTOCOL_UNREACHABLE
#define STATUS_PORT_UNREACHABLE                            LW_STATUS_PORT_UNREACHABLE
#define STATUS_REQUEST_ABORTED                             LW_STATUS_REQUEST_ABORTED
#define STATUS_CONNECTION_ABORTED                          LW_STATUS_CONNECTION_ABORTED
#define STATUS_BAD_COMPRESSION_BUFFER                      LW_STATUS_BAD_COMPRESSION_BUFFER
#define STATUS_USER_MAPPED_FILE                            LW_STATUS_USER_MAPPED_FILE
#define STATUS_AUDIT_FAILED                                LW_STATUS_AUDIT_FAILED
#define STATUS_TIMER_RESOLUTION_NOT_SET                    LW_STATUS_TIMER_RESOLUTION_NOT_SET
#define STATUS_CONNECTION_COUNT_LIMIT                      LW_STATUS_CONNECTION_COUNT_LIMIT
#define STATUS_LOGIN_TIME_RESTRICTION                      LW_STATUS_LOGIN_TIME_RESTRICTION
#define STATUS_LOGIN_WKSTA_RESTRICTION                     LW_STATUS_LOGIN_WKSTA_RESTRICTION
#define STATUS_IMAGE_MP_UP_MISMATCH                        LW_STATUS_IMAGE_MP_UP_MISMATCH
#define STATUS_INSUFFICIENT_LOGON_INFO                     LW_STATUS_INSUFFICIENT_LOGON_INFO
#define STATUS_BAD_DLL_ENTRYPOINT                          LW_STATUS_BAD_DLL_ENTRYPOINT
#define STATUS_BAD_SERVICE_ENTRYPOINT                      LW_STATUS_BAD_SERVICE_ENTRYPOINT
#define STATUS_LPC_REPLY_LOST                              LW_STATUS_LPC_REPLY_LOST
#define STATUS_IP_ADDRESS_CONFLICT1                        LW_STATUS_IP_ADDRESS_CONFLICT1
#define STATUS_IP_ADDRESS_CONFLICT2                        LW_STATUS_IP_ADDRESS_CONFLICT2
#define STATUS_REGISTRY_QUOTA_LIMIT                        LW_STATUS_REGISTRY_QUOTA_LIMIT
#define STATUS_PATH_NOT_COVERED                            LW_STATUS_PATH_NOT_COVERED
#define STATUS_NO_CALLBACK_ACTIVE                          LW_STATUS_NO_CALLBACK_ACTIVE
#define STATUS_LICENSE_QUOTA_EXCEEDED                      LW_STATUS_LICENSE_QUOTA_EXCEEDED
#define STATUS_PWD_TOO_SHORT                               LW_STATUS_PWD_TOO_SHORT
#define STATUS_PWD_TOO_RECENT                              LW_STATUS_PWD_TOO_RECENT
#define STATUS_PWD_HISTORY_CONFLICT                        LW_STATUS_PWD_HISTORY_CONFLICT
#define STATUS_PLUGPLAY_NO_DEVICE                          LW_STATUS_PLUGPLAY_NO_DEVICE
#define STATUS_UNSUPPORTED_COMPRESSION                     LW_STATUS_UNSUPPORTED_COMPRESSION
#define STATUS_INVALID_HW_PROFILE                          LW_STATUS_INVALID_HW_PROFILE
#define STATUS_INVALID_PLUGPLAY_DEVICE_PATH                LW_STATUS_INVALID_PLUGPLAY_DEVICE_PATH
#define STATUS_DRIVER_ORDINAL_NOT_FOUND                    LW_STATUS_DRIVER_ORDINAL_NOT_FOUND
#define STATUS_DRIVER_ENTRYPOINT_NOT_FOUND                 LW_STATUS_DRIVER_ENTRYPOINT_NOT_FOUND
#define STATUS_RESOURCE_NOT_OWNED                          LW_STATUS_RESOURCE_NOT_OWNED
#define STATUS_TOO_MANY_LINKS                              LW_STATUS_TOO_MANY_LINKS
#define STATUS_QUOTA_LIST_INCONSISTENT                     LW_STATUS_QUOTA_LIST_INCONSISTENT
#define STATUS_FILE_IS_OFFLINE                             LW_STATUS_FILE_IS_OFFLINE
#define STATUS_EVALUATION_EXPIRATION                       LW_STATUS_EVALUATION_EXPIRATION
#define STATUS_ILLEGAL_DLL_RELOCATION                      LW_STATUS_ILLEGAL_DLL_RELOCATION
#define STATUS_LICENSE_VIOLATION                           LW_STATUS_LICENSE_VIOLATION
#define STATUS_DLL_INIT_FAILED_LOGOFF                      LW_STATUS_DLL_INIT_FAILED_LOGOFF
#define STATUS_DRIVER_UNABLE_TO_LOAD                       LW_STATUS_DRIVER_UNABLE_TO_LOAD
#define STATUS_DFS_UNAVAILABLE                             LW_STATUS_DFS_UNAVAILABLE
#define STATUS_VOLUME_DISMOUNTED                           LW_STATUS_VOLUME_DISMOUNTED
#define STATUS_WX86_INTERNAL_ERROR                         LW_STATUS_WX86_INTERNAL_ERROR
#define STATUS_WX86_FLOAT_STACK_CHECK                      LW_STATUS_WX86_FLOAT_STACK_CHECK
#define STATUS_VALIDATE_CONTINUE                           LW_STATUS_VALIDATE_CONTINUE
#define STATUS_NO_MATCH                                    LW_STATUS_NO_MATCH
#define STATUS_NO_MORE_MATCHES                             LW_STATUS_NO_MORE_MATCHES
#define STATUS_NOT_A_REPARSE_POINT                         LW_STATUS_NOT_A_REPARSE_POINT
#define STATUS_IO_REPARSE_TAG_INVALID                      LW_STATUS_IO_REPARSE_TAG_INVALID
#define STATUS_IO_REPARSE_TAG_MISMATCH                     LW_STATUS_IO_REPARSE_TAG_MISMATCH
#define STATUS_IO_REPARSE_DATA_INVALID                     LW_STATUS_IO_REPARSE_DATA_INVALID
#define STATUS_IO_REPARSE_TAG_NOT_HANDLED                  LW_STATUS_IO_REPARSE_TAG_NOT_HANDLED
#define STATUS_REPARSE_POINT_NOT_RESOLVED                  LW_STATUS_REPARSE_POINT_NOT_RESOLVED
#define STATUS_DIRECTORY_IS_A_REPARSE_POINT                LW_STATUS_DIRECTORY_IS_A_REPARSE_POINT
#define STATUS_RANGE_LIST_CONFLICT                         LW_STATUS_RANGE_LIST_CONFLICT
#define STATUS_SOURCE_ELEMENT_EMPTY                        LW_STATUS_SOURCE_ELEMENT_EMPTY
#define STATUS_DESTINATION_ELEMENT_FULL                    LW_STATUS_DESTINATION_ELEMENT_FULL
#define STATUS_ILLEGAL_ELEMENT_ADDRESS                     LW_STATUS_ILLEGAL_ELEMENT_ADDRESS
#define STATUS_MAGAZINE_NOT_PRESENT                        LW_STATUS_MAGAZINE_NOT_PRESENT
#define STATUS_REINITIALIZATION_NEEDED                     LW_STATUS_REINITIALIZATION_NEEDED
#define STATUS_ENCRYPTION_FAILED                           LW_STATUS_ENCRYPTION_FAILED
#define STATUS_DECRYPTION_FAILED                           LW_STATUS_DECRYPTION_FAILED
#define STATUS_RANGE_NOT_FOUND                             LW_STATUS_RANGE_NOT_FOUND
#define STATUS_NO_RECOVERY_POLICY                          LW_STATUS_NO_RECOVERY_POLICY
#define STATUS_NO_EFS                                      LW_STATUS_NO_EFS
#define STATUS_WRONG_EFS                                   LW_STATUS_WRONG_EFS
#define STATUS_NO_USER_KEYS                                LW_STATUS_NO_USER_KEYS
#define STATUS_FILE_NOT_ENCRYPTED                          LW_STATUS_FILE_NOT_ENCRYPTED
#define STATUS_NOT_EXPORT_FORMAT                           LW_STATUS_NOT_EXPORT_FORMAT
#define STATUS_FILE_ENCRYPTED                              LW_STATUS_FILE_ENCRYPTED
#define STATUS_WMI_GUID_NOT_FOUND                          LW_STATUS_WMI_GUID_NOT_FOUND
#define STATUS_WMI_INSTANCE_NOT_FOUND                      LW_STATUS_WMI_INSTANCE_NOT_FOUND
#define STATUS_WMI_ITEMID_NOT_FOUND                        LW_STATUS_WMI_ITEMID_NOT_FOUND
#define STATUS_WMI_TRY_AGAIN                               LW_STATUS_WMI_TRY_AGAIN
#define STATUS_SHARED_POLICY                               LW_STATUS_SHARED_POLICY
#define STATUS_POLICY_OBJECT_NOT_FOUND                     LW_STATUS_POLICY_OBJECT_NOT_FOUND
#define STATUS_POLICY_ONLY_IN_DS                           LW_STATUS_POLICY_ONLY_IN_DS
#define STATUS_VOLUME_NOT_UPGRADED                         LW_STATUS_VOLUME_NOT_UPGRADED
#define STATUS_REMOTE_STORAGE_NOT_ACTIVE                   LW_STATUS_REMOTE_STORAGE_NOT_ACTIVE
#define STATUS_REMOTE_STORAGE_MEDIA_ERROR                  LW_STATUS_REMOTE_STORAGE_MEDIA_ERROR
#define STATUS_NO_TRACKING_SERVICE                         LW_STATUS_NO_TRACKING_SERVICE
#define STATUS_SERVER_SID_MISMATCH                         LW_STATUS_SERVER_SID_MISMATCH
#define STATUS_DS_NO_ATTRIBUTE_OR_VALUE                    LW_STATUS_DS_NO_ATTRIBUTE_OR_VALUE
#define STATUS_DS_INVALID_ATTRIBUTE_SYNTAX                 LW_STATUS_DS_INVALID_ATTRIBUTE_SYNTAX
#define STATUS_DS_ATTRIBUTE_TYPE_UNDEFINED                 LW_STATUS_DS_ATTRIBUTE_TYPE_UNDEFINED
#define STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS                LW_STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS
#define STATUS_DS_BUSY                                     LW_STATUS_DS_BUSY
#define STATUS_DS_UNAVAILABLE                              LW_STATUS_DS_UNAVAILABLE
#define STATUS_DS_NO_RIDS_ALLOCATED                        LW_STATUS_DS_NO_RIDS_ALLOCATED
#define STATUS_DS_NO_MORE_RIDS                             LW_STATUS_DS_NO_MORE_RIDS
#define STATUS_DS_INCORRECT_ROLE_OWNER                     LW_STATUS_DS_INCORRECT_ROLE_OWNER
#define STATUS_DS_RIDMGR_INIT_ERROR                        LW_STATUS_DS_RIDMGR_INIT_ERROR
#define STATUS_DS_OBJ_CLASS_VIOLATION                      LW_STATUS_DS_OBJ_CLASS_VIOLATION
#define STATUS_DS_CANT_ON_NON_LEAF                         LW_STATUS_DS_CANT_ON_NON_LEAF
#define STATUS_DS_CANT_ON_RDN                              LW_STATUS_DS_CANT_ON_RDN
#define STATUS_DS_CANT_MOD_OBJ_CLASS                       LW_STATUS_DS_CANT_MOD_OBJ_CLASS
#define STATUS_DS_CROSS_DOM_MOVE_FAILED                    LW_STATUS_DS_CROSS_DOM_MOVE_FAILED
#define STATUS_DS_GC_NOT_AVAILABLE                         LW_STATUS_DS_GC_NOT_AVAILABLE
#define STATUS_DIRECTORY_SERVICE_REQUIRED                  LW_STATUS_DIRECTORY_SERVICE_REQUIRED
#define STATUS_REPARSE_ATTRIBUTE_CONFLICT                  LW_STATUS_REPARSE_ATTRIBUTE_CONFLICT
#define STATUS_CANT_ENABLE_DENY_ONLY                       LW_STATUS_CANT_ENABLE_DENY_ONLY
#define STATUS_FLOAT_MULTIPLE_FAULTS                       LW_STATUS_FLOAT_MULTIPLE_FAULTS
#define STATUS_FLOAT_MULTIPLE_TRAPS                        LW_STATUS_FLOAT_MULTIPLE_TRAPS
#define STATUS_DEVICE_REMOVED                              LW_STATUS_DEVICE_REMOVED
#define STATUS_JOURNAL_DELETE_IN_PROGRESS                  LW_STATUS_JOURNAL_DELETE_IN_PROGRESS
#define STATUS_JOURNAL_NOT_ACTIVE                          LW_STATUS_JOURNAL_NOT_ACTIVE
#define STATUS_NOINTERFACE                                 LW_STATUS_NOINTERFACE
#define STATUS_DS_ADMIN_LIMIT_EXCEEDED                     LW_STATUS_DS_ADMIN_LIMIT_EXCEEDED
#define STATUS_DRIVER_FAILED_SLEEP                         LW_STATUS_DRIVER_FAILED_SLEEP
#define STATUS_MUTUAL_AUTHENTICATION_FAILED                LW_STATUS_MUTUAL_AUTHENTICATION_FAILED
#define STATUS_CORRUPT_SYSTEM_FILE                         LW_STATUS_CORRUPT_SYSTEM_FILE
#define STATUS_DATATYPE_MISALIGNMENT_ERROR                 LW_STATUS_DATATYPE_MISALIGNMENT_ERROR
#define STATUS_WMI_READ_ONLY                               LW_STATUS_WMI_READ_ONLY
#define STATUS_WMI_SET_FAILURE                             LW_STATUS_WMI_SET_FAILURE
#define STATUS_COMMITMENT_MINIMUM                          LW_STATUS_COMMITMENT_MINIMUM
#define STATUS_REG_NAT_CONSUMPTION                         LW_STATUS_REG_NAT_CONSUMPTION
#define STATUS_TRANSPORT_FULL                              LW_STATUS_TRANSPORT_FULL
#define STATUS_DS_SAM_INIT_FAILURE                         LW_STATUS_DS_SAM_INIT_FAILURE
#define STATUS_ONLY_IF_CONNECTED                           LW_STATUS_ONLY_IF_CONNECTED
#define STATUS_DS_SENSITIVE_GROUP_VIOLATION                LW_STATUS_DS_SENSITIVE_GROUP_VIOLATION
#define STATUS_PNP_RESTART_ENUMERATION                     LW_STATUS_PNP_RESTART_ENUMERATION
#define STATUS_JOURNAL_ENTRY_DELETED                       LW_STATUS_JOURNAL_ENTRY_DELETED
#define STATUS_DS_CANT_MOD_PRIMARYGROUPID                  LW_STATUS_DS_CANT_MOD_PRIMARYGROUPID
#define STATUS_SYSTEM_IMAGE_BAD_SIGNATURE                  LW_STATUS_SYSTEM_IMAGE_BAD_SIGNATURE
#define STATUS_PNP_REBOOT_REQUIRED                         LW_STATUS_PNP_REBOOT_REQUIRED
#define STATUS_POWER_STATE_INVALID                         LW_STATUS_POWER_STATE_INVALID
#define STATUS_DS_INVALID_GROUP_TYPE                       LW_STATUS_DS_INVALID_GROUP_TYPE
#define STATUS_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN       LW_STATUS_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN
#define STATUS_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN        LW_STATUS_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN
#define STATUS_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER            LW_STATUS_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER
#define STATUS_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER        LW_STATUS_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER
#define STATUS_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER         LW_STATUS_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER
#define STATUS_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER      LW_STATUS_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER
#define STATUS_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER LW_STATUS_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER
#define STATUS_DS_HAVE_PRIMARY_MEMBERS                     LW_STATUS_DS_HAVE_PRIMARY_MEMBERS
#define STATUS_WMI_NOT_SUPPORTED                           LW_STATUS_WMI_NOT_SUPPORTED
#define STATUS_INSUFFICIENT_POWER                          LW_STATUS_INSUFFICIENT_POWER
#define STATUS_SAM_NEED_BOOTKEY_PASSWORD                   LW_STATUS_SAM_NEED_BOOTKEY_PASSWORD
#define STATUS_SAM_NEED_BOOTKEY_FLOPPY                     LW_STATUS_SAM_NEED_BOOTKEY_FLOPPY
#define STATUS_DS_CANT_START                               LW_STATUS_DS_CANT_START
#define STATUS_DS_INIT_FAILURE                             LW_STATUS_DS_INIT_FAILURE
#define STATUS_SAM_INIT_FAILURE                            LW_STATUS_SAM_INIT_FAILURE
#define STATUS_DS_GC_REQUIRED                              LW_STATUS_DS_GC_REQUIRED
#define STATUS_DS_LOCAL_MEMBER_OF_LOCAL_ONLY               LW_STATUS_DS_LOCAL_MEMBER_OF_LOCAL_ONLY
#define STATUS_DS_NO_FPO_IN_UNIVERSAL_GROUPS               LW_STATUS_DS_NO_FPO_IN_UNIVERSAL_GROUPS
#define STATUS_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED           LW_STATUS_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED
#define STATUS_MULTIPLE_FAULT_VIOLATION                    LW_STATUS_MULTIPLE_FAULT_VIOLATION
#define STATUS_NOT_SUPPORTED_ON_SBS                        LW_STATUS_NOT_SUPPORTED_ON_SBS
#define STATUS_ASSERTION_FAILURE                           LW_STATUS_ASSERTION_FAILURE

#endif /* LW_STRICT_NAMESPACE */

#ifndef _DCE_IDL_
#include <lw/attrs.h>

LW_PCSTR
LwNtStatusToSymbolicName(
    LW_IN LW_NTSTATUS code
    );

LW_PCSTR
LwNtStatusToDescription(
    LW_IN LW_NTSTATUS code
    );

int
LwNtStatusToUnixErrno(
    LW_IN LW_NTSTATUS code
    );

#ifndef LW_STRICT_NAMESPACE
#define NtStatusToSymbolicName(code) LwNtStatusToSymbolicName(code)
#define NtStatusToDescription(code)  LwNtStatusToDescription(code)
#define NtStatusToUnixErrno(code)    LwNtStatusToUnixErrno(code)
#endif /* LW_STRICT_NAMESPACE */
#endif

#endif
