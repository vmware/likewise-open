/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        jenny_test.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSMB)
 *
 *        SMB Covering Array test library
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 */

#include "jenny_test.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

ptrdiff_t alignments[] = { 0, 1, 2, 3 };
size_t lengths_powers[] = { 0, 1, 2, 4, 8, 16, 32, 64, 128, 256 };
ssize_t length_adjustments[] = { -3, -2, -1, 0, 1, 2, 3 };
/* lengths_random */
/* starting buffer 0, starting buffer 1 */

void
add_length_adjustment_u32(uint32_t *parameterArray, size_t *i)
{
    parameterArray[*i] = sizeof(length_adjustments) /
        sizeof(length_adjustments[0]);
    (*i)++;
}

int32_t
get_length_adjustment_i32(char cAdjustment)
{
    return length_adjustments[cAdjustment - 'a'];
}

void
add_length_u32(uint32_t *parameterArray, size_t *i)
{
    parameterArray[*i]= sizeof(lengths_powers) / sizeof(lengths_powers[0]);
    (*i)++;
}

uint32_t
get_length_u32(char cLength)
{
    return lengths_powers[cLength - 'a'];
}

void
add_aligned_pointer_u8(uint32_t *parameterArray, size_t *i)
{
    parameterArray[*i] = sizeof(alignments) / sizeof(alignments[0]);
    (*i)++;
}

size_t
get_alignment_pointer_u8(char cAlignment)
{
    return alignments[cAlignment - 'a'];
}

void
add_aligned_pointer_u32(uint32_t *parameterArray, size_t *i)
{
    add_aligned_pointer_u8(parameterArray, i);
}

void
add_string_8(uint32_t *coveringArray, size_t *i, char **pszExclusions)
{
    size_t n = 0, m = 0;
    add_aligned_pointer_u8(coveringArray, i);
    add_length_u32(coveringArray, i);
    add_length_adjustment_u32(coveringArray, i);

    for (; n < sizeof(lengths_powers) / sizeof(lengths_powers[0]); n++)
    {
        for (; m < sizeof(length_adjustments) / sizeof(length_adjustments[0]);
              m++)
        {
            if (length_adjustments[m] > lengths_powers[n])
            {
                #if 0
                asprintf(pszExclusions, "-w%d%c%d%c", (int) n + 1,
                    (int) (n + 'a'), (int) m + 1, (int) (m + 'a'));
                #endif
            }
        }

    }
    /* length + adjustment must be greater or equal to zero */
}

uchar8_t *
get_string_8(FILE *jennyOutputFile, uint8_t *pBlock)
{
    char alignChar = 0, lengthChar = 0, adjChar = 0;
    uint32_t alignment = 0, length = 0;
    int32_t adjustment = 0;
    uchar8_t *pAligned = NULL;
    size_t i = 0;

#if 0
    uint32_t dummy = 0;
    fscanf(jennyOutputFile, " %d%c ", &dummy, &alignChar);
    fscanf(jennyOutputFile, " %d%c ", &dummy, &lengthChar);
    fscanf(jennyOutputFile, " %d%c ", &dummy, &adjChar);
#endif

    alignment = get_alignment_pointer_u8(alignChar);
    length = get_length_u32(lengthChar);
    adjustment = get_length_adjustment_i32(adjChar);

    pAligned = pBlock + alignment;

    for (i = 0; i < length; i++)
    {
        pAligned[i] = (rand() % 255) + 1;
    }
    pAligned[length] = sizeof(NUL);

    return pAligned;
}
