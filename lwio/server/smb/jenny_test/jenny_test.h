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
 *        Likewise SMB Subsystem (LWIO)
 *
 *        SMB Covering Array test library
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 */
#ifndef __JENNY_TEST_H__
#define __JENNY_TEST_H__

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "config.h"
#include "lsmbsys.h"

#include "lwio/lwio.h"

void
add_length_adjustment_u32(uint32_t *parameterArray, size_t *i);

int32_t
get_length_adjustment_i32(char cAdjustment);

void
add_length_u32(uint32_t *parameterArray, size_t *i);

uint32_t
get_length_u32(char cLength);

void
add_aligned_pointer_u8(uint32_t *parameterArray, size_t *i);

size_t
get_alignment_pointer_u8(char cAlignment);

void
add_aligned_pointer_u32(uint32_t *parameterArray, size_t *i);

void
add_string_8(uint32_t *coveringArray, size_t *i, char **pszExclusions);

uchar8_t *
get_string_8(FILE *jennyOutputFile, uint8_t *pBlock);

#endif

