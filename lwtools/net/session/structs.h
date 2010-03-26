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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 */

typedef DWORD NET_SESSION_CTRL_CODE;

#define NET_SESSION_UNKNOWN 0
#define NET_SESSION_HELP 1
#define NET_SESSION_DEL  2
#define NET_SESSION_ENUM 3

#define NET_SESSION_COMMAND_HELP "HELP"
#define NET_SESSION_COMMAND_DEL "DEL"

#define NET_SESSION_NAME_TITLE "Session name"

typedef struct _NET_SESSION_DEL_INFO_PARAMS
{
    PWSTR pwszServerName;
    PWSTR pwszSessionName;
} NET_SESSION_DEL_INFO_PARAMS, *PNET_SESSION_DEL_INFO_PARAMS;

typedef struct _NET_SESSION_ENUM_INFO_PARAMS
{
    PWSTR pwszServerName;
} NET_SESSION_ENUM_INFO_PARAMS, *PNET_SESSION_ENUM_INFO_PARAMS;

typedef struct _NET_SESSION_COMMAND_INFO
{
    NET_SESSION_CTRL_CODE dwControlCode;

    union
    {
        NET_SESSION_ENUM_INFO_PARAMS SessionEnumInfo;
        NET_SESSION_DEL_INFO_PARAMS SessionDelInfo;
    };
} NET_SESSION_COMMAND_INFO, *PNET_SESSION_COMMAND_INFO;

