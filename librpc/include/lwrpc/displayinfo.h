/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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

#ifndef _SAMR_DISPLAYINFO_H_
#define _SAMR_DISPLAYINFO_H_


typedef struct samr_display_entry_full {
    uint32 idx;
    uint32 rid;
    uint32 account_flags;
    UnicodeString account_name;
    UnicodeString description;
    UnicodeString full_name;
} SamrDisplayEntryFull;


typedef struct samr_display_info_full {
    uint32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SamrDisplayEntryFull *entries;
} SamrDisplayInfoFull;


typedef struct samr_display_entry_general {
    uint32 idx;
    uint32 rid;
    uint32 account_flags;
    UnicodeString account_name;
    UnicodeString description;
} SamrDisplayEntryGeneral;


typedef struct samr_display_info_general {
    uint32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SamrDisplayEntryGeneral *entries;
} SamrDisplayInfoGeneral;


typedef struct samr_display_entry_general_group {
    uint32 idx;
    uint32 rid;
    uint32 account_flags;
    UnicodeString account_name;
    UnicodeString description;
} SamrDisplayEntryGeneralGroup;

typedef struct samr_display_info_general_groups {
    uint32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SamrDisplayEntryGeneralGroup *entries;
} SamrDisplayInfoGeneralGroups;


typedef struct samr_display_entry_ascii {
    uint32 idx;
    ANSI_STRING account_name;
} SamrDisplayEntryAscii;


typedef struct samr_display_info_ascii {
    uint32 count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    SamrDisplayEntryAscii *entries;
} SamrDisplayInfoAscii;


#ifndef _DCE_IDL_
typedef union samr_display_info  {
    SamrDisplayInfoFull          info1;
    SamrDisplayInfoGeneral       info2;
    SamrDisplayInfoGeneralGroups info3;
    SamrDisplayInfoAscii         info4;
    SamrDisplayInfoAscii         info5;
} SamrDisplayInfo;
#endif /* _DCE_IDL_ */

#endif /* _SAMR_DISPLAYINFO_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
