/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

#ifndef _LMSHARE_H_
#define _LMSHARE_H_

#include <lw/types.h>

#ifndef CONNECTION_INFO_1_DEFINED
#define CONNECTION_INFO_1_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef CONNECTION_INFO_1_DEFINED")
cpp_quote("#define CONNECTION_INFO_1_DEFINED 1")
#endif


typedef struct _CONNECTION_INFO_1 {
    UINT32 coni1_id;
    UINT32 coni1_type;
    UINT32 coni1_num_open;
    UINT32 coni1_num_users;
    UINT32 coni1_time;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR coni1_username;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR coni1_netname;
} CONNECTION_INFO_1, *PCONNECTION_INFO_1;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_0_DEFINED
#define SHARE_INFO_0_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_0_DEFINED")
cpp_quote("#define SHARE_INFO_0_DEFINED 1")
#endif

typedef struct _SHARE_INFO_0 {
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi0_netname;
} SHARE_INFO_0, *PSHARE_INFO_0;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_1_DEFINED
#define SHARE_INFO_1_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1_DEFINED")
cpp_quote("#define SHARE_INFO_1_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1 {
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi1_netname;
    UINT32 shi1_type;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi1_remark;
} SHARE_INFO_1, *PSHARE_INFO_1, *LPSHARE_INFO_1;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_2_DEFINED
#define SHARE_INFO_2_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_2_DEFINED")
cpp_quote("#define SHARE_INFO_2_DEFINED 1")
#endif

typedef struct _SHARE_INFO_2 {
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi2_netname;
    UINT32 shi2_type;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi2_remark;
    UINT32 shi2_permissions;
    UINT32 shi2_max_uses;
    UINT32 shi2_current_uses;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi2_path;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi2_password;
} SHARE_INFO_2, *PSHARE_INFO_2, *LPSHARE_INFO_2;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_501_DEFINED
#define SHARE_INFO_501_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_501_DEFINED")
cpp_quote("#define SHARE_INFO_501_DEFINED 1")
#endif

typedef struct _SHARE_INFO_501 {
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi501_netname;
    UINT32 shi501_type;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi501_remark;
    UINT32 shi501_flags;
} SHARE_INFO_501, *PSHARE_INFO_501, *LPSHARE_INFO_501;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_502_DEFINED
#define SHARE_INFO_502_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_502_DEFINED")
cpp_quote("#define SHARE_INFO_502_DEFINED 1")
#endif

typedef struct _SHARE_INFO_502 {
    PWSTR shi502_netname;
    UINT32 shi502_type;
    PWSTR shi502_remark;
    UINT32 shi502_permissions;
    UINT32 shi502_max_uses;
    UINT32 shi502_current_uses;
    PWSTR shi502_path;
    PWSTR shi502_password;
    UINT32 shi502_reserved;
#ifdef _DCE_IDL_
    [size_is(shi502_reserved)]
#endif
    PBYTE shi502_security_descriptor;
} SHARE_INFO_502, *PSHARE_INFO_502, *LPSHARE_INFO_502;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#ifndef SHARE_INFO_1004_DEFINED
#define SHARE_INFO_1004_DEFINED 1


#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1004_DEFINED")
cpp_quote("#define SHARE_INFO_1004_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1004 {
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR shi1004_remark;
} SHARE_INFO_1004, *PSHARE_INFO_1004, *LPSHARE_INFO_1004;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_1005_DEFINED
#define SHARE_INFO_1005_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1005_DEFINED")
cpp_quote("#define SHARE_INFO_1005_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1005 {
    UINT32 shi1005_flags;
} SHARE_INFO_1005, *PSHARE_INFO_1005, *LPSHARE_INFO_1005;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_1006_DEFINED
#define SHARE_INFO_1006_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1006_DEFINED")
cpp_quote("#define SHARE_INFO_1006_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1006 {
    UINT32 shi1006_max_uses;
} SHARE_INFO_1006, *PSHARE_INFO_1006, *LPSHARE_INFO_1006;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif


#ifndef SHARE_INFO_1501_DEFINED
#define SHARE_INFO_1501_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef SHARE_INFO_1501_DEFINED")
cpp_quote("#define SHARE_INFO_1501_DEFINED 1")
#endif

typedef struct _SHARE_INFO_1501 {
    UINT32 shi1501_reserved;
#ifdef _DCE_IDL_
    [size_is(shi1501_reserved)]
#endif
    PBYTE shi1501_security_descriptor;
} SHARE_INFO_1501, *PSHARE_INFO_1501, *LPSHARE_INFO_1501;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif



#endif /* _LMSHARE_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
