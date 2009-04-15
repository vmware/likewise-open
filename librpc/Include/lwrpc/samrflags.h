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

#ifndef _SAMRFLAGS_H_
#define _SAMRFLAGS_H_

/* Connect access mask flags */
#define SAMR_ACCESS_CONNECT_TO_SERVER          0x00000001
#define SAMR_ACCESS_SHUTDOWN_SERVER            0x00000002
#define SAMR_ACCESS_INITIALIZE_SERVER          0x00000004
#define SAMR_ACCESS_CREATE_DOMAIN              0x00000008
#define SAMR_ACCESS_ENUM_DOMAINS               0x00000010
#define SAMR_ACCESS_OPEN_DOMAIN                0x00000020

/* Domain access mask flags */
#define DOMAIN_ACCESS_LOOKUP_INFO_1            0x00000001
#define DOMAIN_ACCESS_SET_INFO_1               0x00000002
#define DOMAIN_ACCESS_LOOKUP_INFO_2            0x00000004
#define DOMAIN_ACCESS_SET_INFO_2               0x00000008
#define DOMAIN_ACCESS_CREATE_USER              0x00000010
#define DOMAIN_ACCESS_CREATE_GROUP             0x00000020
#define DOMAIN_ACCESS_CREATE_ALIAS             0x00000040
#define DOMAIN_ACCESS_LOOKUP_ALIAS             0x00000080
#define DOMAIN_ACCESS_ENUM_ACCOUNTS            0x00000100
#define DOMAIN_ACCESS_OPEN_ACCOUNT             0x00000200
#define DOMAIN_ACCESS_SET_INFO_3               0x00000400

/* User access mask flags */
#define USER_ACCESS_GET_NAME_ETC               0x00000001
#define USER_ACCESS_GET_LOCALE                 0x00000002
#define USER_ACCESS_SET_LOC_COM                0x00000004
#define USER_ACCESS_GET_LOGONINFO              0x00000008
#define USER_ACCESS_GET_ATTRIBUTES             0x00000010
#define USER_ACCESS_SET_ATTRIBUTES             0x00000020
#define USER_ACCESS_CHANGE_PASSWORD            0x00000040
#define USER_ACCESS_SET_PASSWORD               0x00000080
#define USER_ACCESS_GET_GROUPS                 0x00000100
#define USER_ACCESS_GET_GROUP_MEMBERSHIP       0x00000200
#define USER_ACCESS_CHANGE_GROUP_MEMBERSHIP    0x00000400

/* Alias access mask flags */
#define ALIAS_ACCESS_ADD_MEMBER                0x00000001
#define ALIAS_ACCESS_REMOVE_MEMBER             0x00000002
#define ALIAS_ACCESS_GET_MEMBERS               0x00000004
#define ALIAS_ACCESS_LOOKUP_INFO               0x00000008
#define ALIAS_ACCESS_SET_INFO                  0x00000010


/* client version for SamrConnect[45] */
#define SAMR_CONNECT_PRE_WIN2K                 (1)
#define SAMR_CONNECT_WIN2K                     (2)
#define SAMR_CONNECT_POST_WIN2K                (3)


#endif /* _SAMRFLAGS_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
