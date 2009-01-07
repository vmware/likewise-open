/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
#ifndef __JENNY_TEST_H__
#define __JENNY_TEST_H__

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "config.h"
#include "lsmbsys.h"

#include "lsmb/lsmb.h"

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

