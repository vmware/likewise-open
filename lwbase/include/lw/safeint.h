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
 *        swab.h
 *
 * Abstract:
 *
 *        Safe Integer Arithmetic
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#ifndef __LWBASE_SAFEINT_H__
#define __LWBASE_SAFEINT_H__

inline
static
NTSTATUS
RtlSafeMultiplyULONG(
    OUT PULONG Result,
    IN ULONG A,
    IN ULONG B
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG64 result = ((ULONG64) A) * ((ULONG64) B);
    if (result > ((ULONG)-1))
    {
        status = STATUS_INTEGER_OVERFLOW;
        *Result = ((ULONG)-1);
    }
    else
    {
        status = STATUS_SUCCESS;
    }   *Result = (ULONG) result;
    return status;
}

inline
static
NTSTATUS
RtlSafeAddULONG(
    OUT PULONG Result,
    IN ULONG A,
    IN ULONG B
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG result = A + B;
    if (result < A)
    {
        status = STATUS_INTEGER_OVERFLOW;
        *Result = ((ULONG)-1);
    }
    else
    {
        status = STATUS_SUCCESS;
    }   *Result = result;
    return status;
}

inline
static
NTSTATUS
RtlSafeAddUSHORT(
    OUT PUSHORT Result,
    IN USHORT A,
    IN USHORT B
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT result = A + B;
    if (result < A)
    {
        status = STATUS_INTEGER_OVERFLOW;
        *Result = ((USHORT)-1);
    }
    else
    {
        status = STATUS_SUCCESS;
    }   *Result = result;
    return status;
}

#endif /* __LWBASE_SAFEINT_H__ */
