/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;
using System.ComponentModel;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.NETAPI.APIMarshal;
using Likewise.LMC.AuthUtils;

//Use the DebugMarshal for Debug builds, and the standard Marshal in release builds
#if DEBUG
using Marshal = Likewise.LMC.LMConsoleUtils.DebugMarshal;
#endif

namespace Likewise.LMC.NETAPI
{

    public partial class NETAPIWrapper
    {
        public static bool bIsNetApiInitCalled = false;

        public static bool NetApiInitCalled
        {
            set
            {
                bIsNetApiInitCalled = value;
            }
            get
            {
                return bIsNetApiInitCalled;
            }
        }

        #region Static Interface

        public static void AddGroup(CredentialEntry ce, string servername, 
            string groupname, string description, boolDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.groupname = groupname;
            args.description = description;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_AddGroupAsynchronous));
            t.Start(args);
        }

        public static bool AddGroupSynchronous(CredentialEntry ce, string servername,
            string groupname, string description)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.groupname = groupname;
            args.description = description;

            return doWork_AddGroup(args);
        }

        public static void AddUser(CredentialEntry ce, string servername, 
            string username, string password, string fullname, string description, 
            uint flags, boolDelegate callback) 
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;
            args.password = password;
            args.fullname = fullname;
            args.description = description;
            args.flags = flags;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_AddUserAsynchronous));
            t.Start(args);
        }

        public static bool AddUserSynchronous(CredentialEntry ce, string servername,
             string username, string password, string fullname, string description,
             uint flags)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;
            args.password = password;
            args.fullname = fullname;
            args.description = description;
            args.flags = flags;

            return doWork_AddUser(args);
        }

        public static void ChangePassword(CredentialEntry ce, string servername, 
            string username, string password, boolDelegate callback) 
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;
            args.password = password;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_ChangePasswordAsynchronous));
            t.Start(args);
        }

        public static bool ChangePasswordSynchronous(CredentialEntry ce, string servername,
              string username, string password)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;
            args.password = password;

            return doWork_ChangePassword(args);
        }

        public static void DeleteUser(CredentialEntry ce, string servername, 
            string username, boolDelegate callback) 
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_DeleteUserAsynchronous));
            t.Start(args);
        }

        public static bool DeleteUserSynchronous(CredentialEntry ce, string servername,
            string username)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;

            return doWork_DeleteUser(args);
        }

        public static void DeleteGroup(CredentialEntry ce, string servername, 
            string username, boolDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_DeleteGroupAsynchronous));
            t.Start(args);
        }

        public static bool DeleteGroupSynchronous(CredentialEntry ce, string servername,
            string username)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;

            return doWork_DeleteGroup(args);
        }

        public static void GetGroups(CredentialEntry ce, string servername, 
            string username, stringArrDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;
            args.stringArrCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_GetGroupsAsynchronous));
            t.Start(args);
        }

        public static string[] GetGroupsSynchronous(CredentialEntry ce, string servername,
             string username)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;

            return doWork_GetGroups(args);
        }

        public static void EnumUsers(CredentialEntry ce, string servername, int resumeHandle,
            LUGAPI.LUGEnumStatusDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.resumeHandle = resumeHandle;
            args.LUGEnumStatusCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_EnumUsersAsynchronous));

            t.Priority = ThreadPriority.BelowNormal;

            t.Start(args);
        }

        public static LUGAPI.LUGEnumStatus EnumUsersSynchronous(CredentialEntry ce, string servername,
            int resumeHandle)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.resumeHandle = resumeHandle;

            return doWork_EnumUsers(args);
        }

        public static void EnumGroups(CredentialEntry ce, string servername, int resumeHandle,
            LUGAPI.LUGEnumStatusDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.resumeHandle = resumeHandle;
            args.LUGEnumStatusCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_EnumGroupsAsynchronous));

            t.Priority = ThreadPriority.BelowNormal;

            t.Start(args);
        }

        public static LUGAPI.LUGEnumStatus EnumGroupsSynchronous(CredentialEntry ce, string servername,
            int resumeHandle)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.resumeHandle = resumeHandle;

            return doWork_EnumGroups(args);
        }

        public static void AddUserToGroup(CredentialEntry ce, string servername, 
            string groupname, string username, boolDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.groupname = groupname;
            args.username = username;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_AddUserToGroupAsynchronous));
            t.Start(args);
        }

        public static bool AddUserToGroupSynchronous(CredentialEntry ce, string servername,
            string groupname, string username)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.groupname = groupname;
            args.username = username;

            return doWork_AddUserToGroup(args);
        }

        public static void DeleteUserFromGroup(CredentialEntry ce, string servername, 
            string groupname, string username, boolDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.groupname = groupname;
            args.username = username;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_DeleteUserFromGroupAsynchronous));
            t.Start(args);
        }

        public static bool DeleteUserFromGroupSynchronous(CredentialEntry ce, string servername,
             string groupname, string username)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.groupname = groupname;
            args.username = username;

            return doWork_DeleteUserFromGroup(args);
        }

        public static void GetUserInfo(CredentialEntry ce, string servername, 
            string username, LUGInfoDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;
            args.LUGInfoCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_GetUserInfoAsynchronous));
            t.Start(args);
        }

        public static LUGInfo GetUserInfoSynchronous(CredentialEntry ce, string servername,
            string username)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;

            return doWork_GetUserInfo(args);
        }

        public static void RenameUser(CredentialEntry ce, string servername, 
            string oldusername, string username, boolDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.oldusername = oldusername;
            args.username = username;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_RenameUserAsynchronous));
            t.Start(args);
        }

        public static bool RenameUserSynchronous(CredentialEntry ce, string servername,
            string oldusername, string username)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.oldusername = oldusername;
            args.username = username;

            return doWork_RenameUser(args);
        }

        public static void RenameGroup(CredentialEntry ce, string servername, 
            string oldgroupname, string groupname, boolDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.oldgroupname = oldgroupname;
            args.groupname = groupname;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_RenameGroupAsynchronous));
            t.Start(args);
        }

        public static bool RenameGroupSynchronous(CredentialEntry ce, string servername,
            string oldgroupname, string groupname)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.oldgroupname = oldgroupname;
            args.groupname = groupname;

            return doWork_RenameGroup(args);
        }

        public static void EditUserFullName(CredentialEntry ce, string servername, 
            string username, string fullname, boolDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;
            args.fullname = fullname;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_EditUserFullNameAsynchronous));
            t.Start(args);
        }

        public static bool EditUserFullNameSynchronous(CredentialEntry ce, string servername,
            string username, string fullname)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;
            args.fullname = fullname;

            return doWork_EditUserFullName(args);
        }

        public static void EditUserDescription(CredentialEntry ce, string servername, 
            string username, string description, boolDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;
            args.description = description;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_EditUserCommentAsynchronous));
            t.Start(args);
        }

        public static bool EditUserDescriptionSynchronous(CredentialEntry ce, string servername,
            string username, string description)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;
            args.description = description;

            return doWork_EditUserComment(args);
        }

        public static void EditUserFlags(CredentialEntry ce, string servername, 
            string username, uint flags, boolDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;
            args.flags = flags;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_EditUserFlagsAsynchronous));
            t.Start(args);
        }

        public static bool EditUserFlagsSynchronous(CredentialEntry ce, string servername,
             string username, uint flags)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.username = username;
            args.flags = flags;

            return doWork_EditUserFlags(args);
        }

        public static void GetGroupMembers(CredentialEntry ce, string servername, 
            string groupname, stringArrDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.groupname = groupname;
            args.stringArrCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_GetGroupMembersAsynchronous));
            t.Start(args);
        }

        public static string[] GetGroupMembersSynchronous(CredentialEntry ce, string servername,
            string groupname)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.groupname = groupname;

            return doWork_GetGroupMembers(args);
        }

        public static void GetGroupInfo(CredentialEntry ce, string servername, 
            string groupname, LUGInfoDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.groupname = groupname;
            args.LUGInfoCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_GetGroupInfoAsynchronous));
            t.Start(args);
        }

        public static string GetGroupInfoSynchronous(CredentialEntry ce, string servername,
            string groupname)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.groupname = groupname;

            return doWork_GetGroupInfo(args);
        }

        public static void EditGroupDescription(CredentialEntry ce, string servername, 
            string groupname, string description, boolDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.groupname = groupname;
            args.description = description;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_EditGroupDescriptionAsynchronous));
            t.Start(args);
        }

        public static bool EditGroupDescriptionSynchronous(CredentialEntry ce, string servername,
            string groupname, string description)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.servername = servername;
            args.groupname = groupname;
            args.description = description;

            return doWork_EditGroupDescription(args);
        }

        public static void AddGroupMember(CredentialEntry ce, string domain, string servername, 
            string groupname, string username, boolDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.domain = domain;
            args.servername = servername;
            args.groupname = groupname;
            args.username = username;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_AddGroupMemberAsynchronous));
            t.Start(args);
        }

        public static bool AddGroupMemberSynchronous(CredentialEntry ce, string domain, string servername,
            string groupname, string username)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.domain = domain;
            args.servername = servername;
            args.groupname = groupname;
            args.username = username;

            return doWork_AddGroupMember(args);
        }

        public static void DeleteGroupMember(CredentialEntry ce, string domain, string servername, 
            string groupname, string username, boolDelegate callback)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.domain = domain;
            args.servername = servername;
            args.groupname = groupname;
            args.username = username;
            args.boolCallback = callback;

            Thread t = new Thread(new ParameterizedThreadStart(doWork_DeleteGroupMemberAsynchronous));
            t.Start(args);
        }

        public static bool DeleteGroupMemberSynchronous(CredentialEntry ce, string domain, string servername,
            string groupname, string username)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.domain = domain;
            args.servername = servername;
            args.groupname = groupname;
            args.username = username;

            return doWork_DeleteGroupMember(args);
        }

        public static bool NetInitMemory(CredentialEntry ce, string domain, string servername)
        {
            NETAPIArguments args = new NETAPIArguments();
            args.initializeToNull();

            args.ce = ce;
            args.domain = domain;
            args.servername = servername;

            return doWork_NetInitMemory(args);
        }

        #endregion

        #region asynchronous background workers

        private static void doWork_AddGroupAsynchronous(object args) 
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_AddGroup(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }
                
        }

        private static void doWork_AddUserAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_AddUser(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }

        }

        private static void doWork_ChangePasswordAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_ChangePassword(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }

        }

        private static void doWork_DeleteUserAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_DeleteUser(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }

        }

        private static void doWork_DeleteGroupAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_DeleteGroup(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }

        }

        private static void doWork_GetGroupsAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            string[] result = doWork_GetGroups(_args);

            if (_args.stringArrCallback != null)
            {
                _args.stringArrCallback(result);
            }

        }

        private static void doWork_EnumUsersAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            LUGAPI.LUGEnumStatus result = doWork_EnumUsers(_args);

            if (_args.LUGEnumStatusCallback != null)
            {
                _args.LUGEnumStatusCallback(result);
            }

        }

        private static void doWork_EnumGroupsAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            LUGAPI.LUGEnumStatus result = doWork_EnumGroups(_args);

            if (_args.LUGEnumStatusCallback != null)
            {
                _args.LUGEnumStatusCallback(result);
            }

        }

        private static void doWork_AddUserToGroupAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_AddUserToGroup(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }

        }

        private static void doWork_DeleteUserFromGroupAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_DeleteUserFromGroup(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }

        }

        private static void doWork_GetUserInfoAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            LUGInfo result = doWork_GetUserInfo(_args);

            if (_args.LUGInfoCallback != null)
            {
                _args.LUGInfoCallback(result);
            }

        }

        private static void doWork_RenameUserAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_RenameUser(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }

        }

        private static void doWork_RenameGroupAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_RenameGroup(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }

        }

        private static void doWork_EditUserFullNameAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_EditUserFullName(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }

        }

        private static void doWork_EditUserCommentAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_EditUserComment(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }

        }

        private static void doWork_EditUserFlagsAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_EditUserFlags(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }

        }

        private static void doWork_GetGroupMembersAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            string[] result = doWork_GetGroupMembers(_args);

            if (_args.stringArrCallback != null)
            {
                _args.stringArrCallback(result);
            }

        }

        private static void doWork_GetGroupInfoAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            string result = doWork_GetGroupInfo(_args);

            if (_args.stringCallback != null)
            {
                _args.stringCallback(result);
            }

        }

        private static void doWork_EditGroupDescriptionAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_EditGroupDescription(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }

        }

        private static void doWork_AddGroupMemberAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_AddGroupMember(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }

        }

        private static void doWork_DeleteGroupMemberAsynchronous(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = doWork_DeleteGroupMember(_args);

            if (_args.boolCallback != null)
            {
                _args.boolCallback(result);
            }

        }

        #endregion

        #region synchronous background workers

        private static bool doWork_NetInitMemory(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            LogArgs("doWork_NetInitMemory", _args);

            bool result = false;

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.NetInitMemory(): EnsureNullSession()=false, returning null");
            }

            try
            {
                int statuscode = NETAPIMarshal.apiNetInitMemory();
                result = true;
                Logger.Log("NETAPIMarshal.apiNetInitMemory returns " + statuscode.ToString(), Logger.netAPILogLevel);
            }
            catch (Exception e)
            {
                HandleNETAPIException("NetInitMemory", e);
            }
            finally
            {
                result = true;
            }

            return result;

        }

        private static bool doWork_AddGroup(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            LogArgs("doWork_AddGroup", _args);

            bool result = false;

            //Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.AddGroup(): EnsureNullSession()=false, returning null");
            }
            else
            {
                LOCALGROUP_INFO_1 lg1 = new LOCALGROUP_INFO_1();
                lg1.name = _args.groupname;
                lg1.comment = _args.description;
                UInt32 parm_err = 0;

                IntPtr bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(lg1));
                IntPtr bufptr_parm_err = Marshal.AllocHGlobal(Marshal.SizeOf(parm_err));

                try
                {
                    Marshal.StructureToPtr(lg1, bufptr, false);

                    NETAPIMarshal.apiNetLocalGroupAdd(_args.servername, 1, bufptr, bufptr_parm_err);
                    result = true;
                }
                catch (Exception e)
                {
                    HandleNETAPIException("AddGroup", e);
                }
                finally
                {
                    Marshal.DestroyStructure(bufptr, lg1.GetType());
                    Marshal.FreeHGlobal(bufptr);
                    Marshal.FreeHGlobal(bufptr_parm_err);

                }
            }

            return result;

        }

        private static bool doWork_AddUser(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            LogArgs("doWork_AddUser", _args);

            bool result = false;

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.AddUser(): EnsureNullSession()=false, returning null");
                return result;
            }        

            USER_INFO_1 ui1 = new USER_INFO_1();
            USER_INFO_1011 ui11 = new USER_INFO_1011();
            UInt32 parm_err = 0;

            ui1.sUsername = _args.username;
            ui1.sPassword = _args.password;
            ui1.uiPasswordAge = 0;
            ui1.uiPriv = 1; // USER_PRIV_USER
            ui1.sHome_Dir = "";
            ui1.sComment = _args.description;
            ui1.uiFlags = _args.flags | UF_NORMAL_ACCOUNT;
            ui1.sScript_Path = "";

            ui11.usri1011_full_name = _args.fullname;

            IntPtr bufptr_1 = Marshal.AllocHGlobal(Marshal.SizeOf(ui1));
            IntPtr bufptr_1011 = Marshal.AllocHGlobal(Marshal.SizeOf(ui11));
            IntPtr bufptr_parm_err = Marshal.AllocHGlobal(Marshal.SizeOf(parm_err));

            try
            {
                Marshal.StructureToPtr(ui1, bufptr_1, false);
                Marshal.StructureToPtr(ui11, bufptr_1011, false);
                Marshal.StructureToPtr(parm_err, bufptr_parm_err, false);

                NETAPIMarshal.apiNetUserAdd(_args.servername, 1, bufptr_1, bufptr_parm_err);

                NETAPIMarshal.apiNetUserSetInfo(_args.servername, _args.username, 1011, bufptr_1011, bufptr_parm_err);

                _args.groupname = "Users";
                boolDelegate tempDelegate = _args.boolCallback;
                _args.boolCallback = null;
                doWork_AddUserToGroup(_args);
                _args.boolCallback = tempDelegate;

                result = true;
            }
            catch (Exception e)
            {
                HandleNETAPIException("AddUser", e);
            }
            finally
            {
                try
                {
                    Marshal.DestroyStructure(bufptr_1, ui1.GetType());
                    Marshal.FreeHGlobal(bufptr_1);

                    Marshal.DestroyStructure(bufptr_1011, ui11.GetType());
                    Marshal.FreeHGlobal(bufptr_1011);

                    //If this is uncommented, it results in a crash
                    //TODO: figure out why it's not possible to free bufptr_parm_err
                    //Marshal.FreeHGlobal(bufptr_parm_err);
                }
                catch (Exception)
                {
                    result = false;
                    Logger.Log("AddUser memory free block (in finally block)", 
                        Logger.LogLevel.Error);
                }
            }
            return result;
        }

        private static bool doWork_ChangePassword(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            LogArgs("doWork_ChangePassword", _args);


            bool result = false;

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.ChangePassword(): EnsureNullSession()=false, returning null");
            }
            else
            {
                USER_INFO_1003 ui1003 = new USER_INFO_1003();
                ui1003.usri1003_password = _args.password;

                IntPtr bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(ui1003));

                try
                {
                    Marshal.StructureToPtr(ui1003, bufptr, false);
                    NETAPIMarshal.apiNetUserSetInfo(_args.servername, _args.username, 1003, bufptr, IntPtr.Zero);
                    result = true;
                }
                catch (Exception e)
                {
                    result = false;
                    HandleNETAPIException("ChangePassword", e);
                }
                finally
                {
                    Marshal.DestroyStructure(bufptr, ui1003.GetType());
                    Marshal.FreeHGlobal(bufptr);
                }
            }

            return result;
        }      

        private static bool doWork_DeleteUser(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = false;

            LogArgs("doWork_DeleteUser", _args);

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.DeleteUser(): EnsureNullSession()=false, returning null");
            }        
            else {
                try
                {
                    if (!NETAPIWrapper.NetApiInitCalled)
                    {
                        NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                    }

                    NETAPIMarshal.apiNetUserDel(_args.servername, _args.username);
                    result = true;
                }
                catch (Exception e)
                {
                    HandleNETAPIException("DeleteUser", e);
                }
            }

            return result;
            
        }

        private static bool doWork_DeleteGroup(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = false;

            LogArgs("doWork_DeleteGroup", _args);

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.DeleteGroup(): EnsureNullSession()=false, returning null");
            }
            else
            {
                try
                {
                    if (!NETAPIWrapper.NetApiInitCalled)
                    {
                        NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                    }

                    NETAPIMarshal.apiNetLocalGroupDel(_args.servername, _args.username);
                    result = true;
                }
                catch (Exception e)
                {
                    HandleNETAPIException("DeleteGroup", e);
                }
            }
            return result;
            
        }

        private static string[] doWork_GetGroups(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return null;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            LogArgs("doWork_GetGroups", _args);

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.GetGroups(): EnsureNullSession()=false", 
                    Logger.LogLevel.Error);
                return null;
            }

            IntPtr bufPtr = new IntPtr(0);
            
            string [] groups = null;

            try
            {
                int entriesRead = 0;
                int totalEntries = 0;

                if (!NETAPIWrapper.NetApiInitCalled)
                {
                    NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                }

                NETAPIMarshal.apiNetUserGetLocalGroups(_args.servername,
                    _args.username, 0, 0, out bufPtr, MAX_PREFERRED_LENGTH, out entriesRead, out totalEntries);

                if (entriesRead > 0)
                {
                    groups = new string[entriesRead];
                    LOCALGROUP_USERS_INFO_0 groupsStruct = new LOCALGROUP_USERS_INFO_0();

                    IntPtr iter = bufPtr;

                    for (int i = 0; i < entriesRead; i++)
                    {
                        if (iter != IntPtr.Zero)
                        {
                            groupsStruct = (LOCALGROUP_USERS_INFO_0)Marshal.PtrToStructure(iter, typeof(LOCALGROUP_USERS_INFO_0));

                            if (!String.IsNullOrEmpty(groupsStruct.lgrui0_name))
                            {
                                groups[i] = scrubString(groupsStruct.lgrui0_name, NETAPI_MAX_GROUP_NAME_LENGTH);
                                Logger.Log(String.Format("apiNetUserGetLocalGroups: groupName: {0}",
                                    groups[i]), Logger.netAPILogLevel);
                            }
                            else
                            {
                                Logger.ShowUserError(String.Format("Could not retrieve group {0} out of {1} belonged to by user {2}!",
                                    i, entriesRead, _args.username));
                            }

                            iter = (IntPtr)((int)iter + Marshal.SizeOf(typeof(LOCALGROUP_USERS_INFO_0)));
                        }
                    }
                }
            }
            catch (Exception e)
            {
                HandleNETAPIException("GetGroups", e);
            }
            finally
            {
                if (bufPtr != IntPtr.Zero)
                {
                    NETAPIMarshal.apiNetApiBufferFree(bufPtr);
                }
            }

            return groups;       
        }

        private static LUGAPI.LUGEnumStatus doWork_EnumUsers(object args)
        {
            LUGAPI.LUGEnumStatus enumStatus = new LUGAPI.LUGEnumStatus();
            enumStatus.initializeToNull();
            enumStatus.type = LUGAPI.LUGType.User;

            if (!(args is NETAPIArguments))
            {
                return enumStatus;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            LogArgs("doWork_EnumUsers", _args);

            //Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.EnumUsers(): EnsureNullSession()=false, returning null");
                return enumStatus;
            }

            IntPtr bufPtr = IntPtr.Zero;

            try
            {
                int statusCode = 0;
                int resumeHandle = _args.resumeHandle;
                int totalEntries = 0;
                int entriesRead = 0;

                if (!NETAPIWrapper.NetApiInitCalled)
                {
                    NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                }

                statusCode = NETAPIMarshal.apiNetUserEnum(
                    _args.servername, 20, 2, out bufPtr, MAX_PREFERRED_LENGTH,
                    ref entriesRead, ref totalEntries, ref resumeHandle);

                enumStatus.entriesRead = entriesRead;
                enumStatus.totalEntries = totalEntries;
                enumStatus.resumeHandle = resumeHandle;

                if (entriesRead > 0)
                {
                    USER_INFO_20[] users = new USER_INFO_20[entriesRead];
                    IntPtr iter = bufPtr;

                    enumStatus.entries = new List<string[]>();

                    for (int i = 0; i < entriesRead; i++)
                    {
                        users[i] = (USER_INFO_20)Marshal.PtrToStructure(iter, typeof(USER_INFO_20));
                        iter = (IntPtr)((int)iter + Marshal.SizeOf(typeof(USER_INFO_20)));

                        string[] userInfo ={"",(users[i].usri20_flags & UF_ACCOUNTDISABLE) != 0 ? DISABLED : "",users[i].usri20_name,
                                users[i].usri20_full_name,users[i].usri20_comment };



                        enumStatus.entries.Add(userInfo);
                    }
                    if (statusCode == (int)ErrorCodes.WIN32Enum.ERROR_MORE_DATA)
                    {
                        enumStatus.moreEntries = true;
                    }
                    Logger.Log(String.Format("NetUserEnum: Enumerated {0}/{1} users", entriesRead, totalEntries),
                        Logger.netAPILogLevel);
                }
            }
            catch (Exception e)
            {
                HandleNETAPIException("EnumUsers", e);
            }
            finally
            {
                if (bufPtr != IntPtr.Zero)
                {
                    NETAPIMarshal.apiNetApiBufferFree(bufPtr);
                }
            }

            return enumStatus;

        }

        private static LUGAPI.LUGEnumStatus doWork_EnumGroups(object args)
        {
            LUGAPI.LUGEnumStatus enumStatus = new LUGAPI.LUGEnumStatus();
            enumStatus.initializeToNull();
            enumStatus.type = LUGAPI.LUGType.Group;

            if (!(args is NETAPIArguments))
            {
                return enumStatus;
            }
            NETAPIArguments _args = (NETAPIArguments)args;



            LogArgs("doWork_EnumGroups", _args);

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.EnumGroups(): EnsureNullSession()=false, returning null");
                return enumStatus;
            }         

            IntPtr bufPtr = IntPtr.Zero;

            try
            {

                int statusCode = 0;
                int entriesRead = 0;
                int totalEntries = 0;
                int resumeHandle = _args.resumeHandle;

                if (!NETAPIWrapper.NetApiInitCalled)
                {
                    NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                }

                statusCode = NETAPIMarshal.apiNetLocalGroupEnum(_args.servername, 
                    1, out bufPtr, MAX_PREFERRED_LENGTH, out entriesRead, out totalEntries, ref resumeHandle);             

                enumStatus.entriesRead = entriesRead;
                enumStatus.totalEntries = totalEntries;
                enumStatus.resumeHandle = resumeHandle;

                if (entriesRead > 0)
                {
                    LOCALGROUP_INFO_1[] group = new LOCALGROUP_INFO_1[entriesRead];

                    IntPtr iter = bufPtr;

                    enumStatus.entries = new List<string[]>();

                    for (int i = 0; i < entriesRead; i++)
                    {
                        group[i] = (LOCALGROUP_INFO_1)Marshal.PtrToStructure(iter, typeof(LOCALGROUP_INFO_1));
                        iter = (IntPtr)((int)iter + Marshal.SizeOf(typeof(LOCALGROUP_INFO_1)));

                        string[] groupInfo ={ "",group[i].name, group[i].comment };
                        enumStatus.entries.Add(groupInfo);                        
                    }
                }
                if (statusCode == (int)ErrorCodes.WIN32Enum.ERROR_MORE_DATA)
                {
                    enumStatus.moreEntries = true;
                }
            }
            catch (Exception e)
            {
                HandleNETAPIException("EnumGroups", e);
            }
            finally
            {
                if (bufPtr != IntPtr.Zero)
                {
                    NETAPIMarshal.apiNetApiBufferFree(bufPtr);
                }
            }
            return enumStatus;
        }

        private static bool doWork_AddUserToGroup(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            LogArgs("doWork_AddUserToGroup", _args);

            bool result = false;

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.AddUserToGroup(): EnsureNullSession()=false, returning null");
            }
            else
            {
                LOCALGROUP_MEMBERS_INFO_3 lgmi3 = new LOCALGROUP_MEMBERS_INFO_3();
                lgmi3.lgrmi3_domainandname = _args.username;

                IntPtr bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(lgmi3));
                try
                {
                    if (!NETAPIWrapper.NetApiInitCalled)
                    {
                        NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                    }

                    Marshal.StructureToPtr(lgmi3, bufptr, false);
                    NETAPIMarshal.apiNetLocalGroupAddMembers(_args.servername, _args.groupname, 3, bufptr, 1);
                    result = true;
                }
                catch (Exception e)
                {
                    result = false;
                    HandleNETAPIException("AddUserToGroup", e);
                }
                finally
                {
                    try
                    {
                        Marshal.DestroyStructure(bufptr, lgmi3.GetType());
                        Marshal.FreeHGlobal(bufptr);
                    }
                    catch (Exception)
                    {
                        result = false;
                    }
                }
            }

            return result;

        }

        private static bool doWork_DeleteUserFromGroup(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            LogArgs("doWork_DeleteUserFromGroup", _args);

            bool result = false;

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.DeleteUserFromGroup(): EnsureNullSession()=false, returning null");
            }
            else
            {
                LOCALGROUP_MEMBERS_INFO_3 lgmi3 = new LOCALGROUP_MEMBERS_INFO_3();
                lgmi3.lgrmi3_domainandname = _args.username;

                IntPtr bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(lgmi3));
                try
                {
                    if (!NETAPIWrapper.NetApiInitCalled)
                    {
                        NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                    }

                    Marshal.StructureToPtr(lgmi3, bufptr, false);
                    int ret = NETAPIMarshal.apiNetLocalGroupDelMembers(_args.servername, _args.groupname, 3, bufptr, 1);
                    if (ret == 0)
                    {
                        result = true;
                    }
                }
                catch (Exception e)
                {
                    HandleNETAPIException("DeleteUserFromGroup", e);
                    result = false;
                }
                finally
                {
                    Marshal.DestroyStructure(bufptr, lgmi3.GetType());
                    Marshal.FreeHGlobal(bufptr);
                }
            }
            return result;
        }

        private static LUGInfo doWork_GetUserInfo(object args)
        {

            LUGInfo userInfo = new LUGInfo();
            userInfo.initializeToNull();

            if (!(args is NETAPIArguments))
            {
                return userInfo;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            LogArgs("doWork_GetUserInfo", _args);


            IntPtr bufPtr = IntPtr.Zero;

            userInfo.fullname = "";
            userInfo.description = "";
            userInfo.flags = 0;

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.GetUserInfo(): EnsureNullSession()=false", 
                    Logger.LogLevel.Error);
                return userInfo;
            }


            try
            {
                if (!NETAPIWrapper.NetApiInitCalled)
                {
                    NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                }

                NETAPIMarshal.apiNetUserGetInfo(_args.servername, _args.username, 20, out bufPtr);

                USER_INFO_20 userinfo_20 = new USER_INFO_20();
                userinfo_20 = (USER_INFO_20)Marshal.PtrToStructure(bufPtr, typeof(USER_INFO_20));
                userInfo.fullname = userinfo_20.usri20_full_name;
                userInfo.description = userinfo_20.usri20_comment;
                userInfo.flags = userinfo_20.usri20_flags;

                Logger.Log(String.Format("GetUserInfo({0}): fullname={1}, comment={2}, flags={3}",
                    _args.username, userInfo.fullname, userInfo.description, flagDescription(userInfo.flags)), 
                    Logger.netAPILogLevel);


            }
            catch (Exception e)
            {
                HandleNETAPIException("GetUserInfo", e);
            }
            finally
            {
                if (bufPtr != IntPtr.Zero)
                {
                    NETAPIMarshal.apiNetApiBufferFree(bufPtr);
                }
            }
            return userInfo;
        }

        private static bool doWork_RenameUser(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = false;

            LogArgs("doWork_RenameUser", _args);

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.RenameUser(): EnsureNullSession()=false, returning null");
            }
            else
            {
                USER_INFO_0 usrinfo_0 = new USER_INFO_0();
                usrinfo_0.usri0_name = _args.username;

                IntPtr bufptr = IntPtr.Zero;
                bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(usrinfo_0));

                try
                {
                    if (!NETAPIWrapper.NetApiInitCalled)
                    {
                        NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                    }                 

                    Marshal.StructureToPtr(usrinfo_0, bufptr, false);

                    NETAPIMarshal.apiNetUserSetInfo(_args.servername, _args.oldusername, 0, bufptr, IntPtr.Zero);
                    result = true;
                }
                catch (Exception e)
                {
                    HandleNETAPIException("RenameUser", e);
                }
                finally
                {
                    Marshal.DestroyStructure(bufptr, usrinfo_0.GetType());
                    Marshal.FreeHGlobal(bufptr);
                }
            }
            return result;
        }

        private static bool doWork_RenameGroup(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = false;

            LogArgs("doWork_RenameGroup", _args);

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.RenameGroup(): EnsureNullSession()=false, returning null");
            }
            else
            {
                LOCALGROUP_INFO_0 localgrouinfo_0 = new LOCALGROUP_INFO_0();
                localgrouinfo_0.lgrpi0_name = _args.groupname;

                IntPtr bufptr = IntPtr.Zero;
                bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(localgrouinfo_0));

                try
                {
                    if (!NETAPIWrapper.NetApiInitCalled)
                    {
                        NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                    }

                    Marshal.StructureToPtr(localgrouinfo_0, bufptr, false);

                    NETAPIMarshal.apiNetLocalGroupSetInfo(_args.servername, _args.oldgroupname, 0, bufptr, IntPtr.Zero);
                    result = true;
                }
                catch (Exception e)
                {
                    HandleNETAPIException("RenameGroup", e);
                }
                finally
                {
                    Marshal.DestroyStructure(bufptr, localgrouinfo_0.GetType());
                    Marshal.FreeHGlobal(bufptr);
                }
            }
            return result;
        }

        private static bool doWork_EditUserFullName(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = false;

            LogArgs("doWork_EditUserFullName", _args);

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.EditUserFullName(): EnsureNullSession()=false, returning null");
            }
            else
            {
                USER_INFO_1011 userinfo_1011 = new USER_INFO_1011();
                userinfo_1011.usri1011_full_name = _args.fullname;

                IntPtr bufptr1011 = IntPtr.Zero;
                bufptr1011 = Marshal.AllocHGlobal(Marshal.SizeOf(userinfo_1011));

                try
                {
                    if (!NETAPIWrapper.NetApiInitCalled)
                    {
                        NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                    }

                    Marshal.StructureToPtr(userinfo_1011, bufptr1011, false);

                    NETAPIMarshal.apiNetUserSetInfo(_args.servername, _args.username, 1011, bufptr1011, IntPtr.Zero);
                    result = true;
                }
                catch (Exception e)
                {
                    HandleNETAPIException("EditUserFullName", e);
                }
                finally
                {
                    Marshal.DestroyStructure(bufptr1011, userinfo_1011.GetType());
                    Marshal.FreeHGlobal(bufptr1011);
                }
            }
            return result;
        }

        private static bool doWork_EditUserComment(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = false;

            LogArgs("doWork_EditUserComment", _args);

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.EditUserComment(): EnsureNullSession()=false, returning null");
            }
            else
            {
                USER_INFO_1007 userinfo_1007 = new USER_INFO_1007();
                userinfo_1007.usri1007_comment = _args.description;

                IntPtr bufptr1007 = IntPtr.Zero;
                bufptr1007 = Marshal.AllocHGlobal(Marshal.SizeOf(userinfo_1007));

                try
                {
                    if (!NETAPIWrapper.NetApiInitCalled)
                    {
                        NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                    }

                    Marshal.StructureToPtr(userinfo_1007, bufptr1007, false);
                    NETAPIMarshal.apiNetUserSetInfo(_args.servername, _args.username, 1007, bufptr1007, IntPtr.Zero);
                    result = true;
                }
                catch (Exception e)
                {
                    HandleNETAPIException("EditUserComment", e);
                }
                finally
                {
                    Marshal.DestroyStructure(bufptr1007, userinfo_1007.GetType());
                    Marshal.FreeHGlobal(bufptr1007);
                }
            }
            return result;
        }

        private static bool doWork_EditUserFlags(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = false;

            LogArgs("doWork_EditUserFlags", _args);


            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.EditUserFlags(): EnsureNullSession()=false, returning null");
                return false;
            }
            else
            {
                USER_INFO_1008 userinfo_1008 = new USER_INFO_1008();
                userinfo_1008.usri1008_flags = _args.flags;

                IntPtr bufptr1008 = IntPtr.Zero;
                bufptr1008 = Marshal.AllocHGlobal(Marshal.SizeOf(userinfo_1008));

                try
                {
                    if (!NETAPIWrapper.NetApiInitCalled)
                    {
                        NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                    }

                    Marshal.StructureToPtr(userinfo_1008, bufptr1008, false);
                    NETAPIMarshal.apiNetUserSetInfo(_args.servername, _args.username, 1008, bufptr1008, IntPtr.Zero);
                    result = true;
                }
                catch (Exception e)
                {
                    HandleNETAPIException("EditUserFlags", e);
                }
                finally
                {
                    Marshal.DestroyStructure(bufptr1008, userinfo_1008.GetType());
                    Marshal.FreeHGlobal(bufptr1008);
                }
            }
            return result;
        }

        private static string[] doWork_GetGroupMembers(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return null;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            LogArgs("doWork_GetGroupMembers", _args);

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.GetGroupMembers(): EnsureNullSession()=false, returning null");
                return null;
            }

            string[] members = null;

            IntPtr bufPtr = IntPtr.Zero;
            IntPtr bufPtrStar = Marshal.AllocHGlobal(Marshal.SizeOf(bufPtr));

            IntPtr resumeHandle = IntPtr.Zero;
            IntPtr resumeHandleStar = Marshal.AllocHGlobal(Marshal.SizeOf(resumeHandle));

            try
            {
                int entriesRead = 0;
                int totalEntries = 0;

                Marshal.StructureToPtr(resumeHandle, resumeHandleStar, false);
                Marshal.StructureToPtr(bufPtr, bufPtrStar, false);

                if (!NETAPIWrapper.NetApiInitCalled)
                {
                    NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                }

                NETAPIMarshal.apiNetLocalGroupGetMembers(_args.servername, _args.groupname, 3, out bufPtr, 
                    MAX_PREFERRED_LENGTH, out entriesRead, out totalEntries, resumeHandleStar);

                if (entriesRead > 0)
                {
                    members = new string[entriesRead];
                    LOCALGROUP_MEMBERS_INFO_3 memberStruct = new LOCALGROUP_MEMBERS_INFO_3();

                    IntPtr iter = bufPtr;

                    for (int i = 0; i < entriesRead; i++)
                    {
                        memberStruct = (LOCALGROUP_MEMBERS_INFO_3)Marshal.PtrToStructure(iter, typeof(LOCALGROUP_MEMBERS_INFO_3));

                        //expected format for members[]: "domainName\userName"
                        if (!String.IsNullOrEmpty(memberStruct.lgrmi3_domainandname))
                        {
                            members[i] = scrubString(memberStruct.lgrmi3_domainandname,
                                NETAPI_MAX_DOMAIN_NAME_LENGTH + 1 + NETAPI_MAX_USER_NAME_LENGTH);

                            Logger.Log(String.Format("apiNetLocalGroupGetMembers: memberName: {0}",
                                members[i]), Logger.netAPILogLevel);
                        }
                        else
                        {
                            Logger.ShowUserError(String.Format("Could not retrieve member {0} out of {1} in group {2}!",
                                i, entriesRead, _args.groupname));
                        }
                        iter = (IntPtr)((int)iter + Marshal.SizeOf(typeof(LOCALGROUP_MEMBERS_INFO_3)));
                    }
                }
            }
            catch (Exception e)
            {
                HandleNETAPIException("GetGroupMembers", e);
            }
            finally
            {
                //NETAPIMarshal.apiNetApiBufferFree(bufPtrStar);
                //NETAPIMarshal.apiNetApiBufferFree(resumeHandleStar);

            }

            //In the situation where a user account has been locally created then deleted on the target host (HOST-MACHINE)
            //then the API call NetLocalGroupGetMembers will return a result for each of these former users which is a part
            //of the specified group, but with only the domain, not the username.  The result: the 'members' array will contain
            //many elements of form 'HOST-MACHINE\' which are not relevant to LAC.
            if (members != null && members.Length > 0)
            {
                string[] scratch = new string[members.Length];
                int j = 0;

                for (int i = 0; i < members.Length; i++)
                {
                    string[] parts = members[i].Split(new char[] { '\\' });
                    if (parts != null && parts.Length == 2 && !String.IsNullOrEmpty(parts[1]))
                    {
                        scratch[j++] = members[i];
                    }
                }
                members = new string[j];
                for(int i = 0; i < j; i++) 
                {
                    members[i] = scratch[i];
                }

            }
            return members;
        }

        private static string doWork_GetGroupInfo(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return null;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            LogArgs("doWork_GetGroupInfo", _args);

            string description = null;

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.GetGroupInfo(): EnsureNullSession()=false, returning null");
            }
            else
            {
                IntPtr bufPtr = IntPtr.Zero;
                try
                {
                    if (!NETAPIWrapper.NetApiInitCalled)
                    {
                        NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                    }

                    NETAPIMarshal.apiNetLocalGroupGetInfo(_args.servername, _args.groupname, 1, out bufPtr);

                    if (bufPtr == IntPtr.Zero)
                    {
                        Logger.ShowUserError(
                            "Unable to retrieve group information: NetLocalGroupGetInfo returned a null buffer pointer");
                        return null;
                    }


                    LOCALGROUP_INFO_1 localrounpinfo_1 = new LOCALGROUP_INFO_1();
                    localrounpinfo_1 = (LOCALGROUP_INFO_1)Marshal.PtrToStructure(bufPtr, typeof(LOCALGROUP_INFO_1));
                    description = localrounpinfo_1.comment;
                }
                catch (Exception e)
                {
                    HandleNETAPIException("GetGroupInfo", e);
                }
                finally
                {
                    if (bufPtr != IntPtr.Zero)
                    {
                        NETAPIMarshal.apiNetApiBufferFree(bufPtr);
                    }
                }
            }
            return description;
        }

        private static bool doWork_EditGroupDescription(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = false;

            LogArgs("doWork_EditGroupComment", _args);

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.EditGroupComment(): EnsureNullSession()=false, returning null");
            }
            else
            {
                LOCALGROUP_INFO_1 groupinfo_1 = new LOCALGROUP_INFO_1();
                groupinfo_1.comment = _args.description;

                IntPtr bufptr = IntPtr.Zero;
                bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(groupinfo_1));

                try
                {
                    Marshal.StructureToPtr(groupinfo_1, bufptr, false);

                    if (!NETAPIWrapper.NetApiInitCalled)
                    {
                        NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                    }

                    NETAPIMarshal.apiNetLocalGroupSetInfo(_args.servername, _args.groupname, 1, bufptr, IntPtr.Zero);
                    result = true;
                }
                catch (Exception e)
                {
                    HandleNETAPIException("EditGroupComment", e);
                }
                finally
                {
                    Marshal.DestroyStructure(bufptr, groupinfo_1.GetType());
                    Marshal.FreeHGlobal(bufptr);
                }
            }
            return result;
        }

        private static bool doWork_AddGroupMember(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = true;

            LogArgs("doWork_AddGroupMember", _args);

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.AddGroupMember(): EnsureNullSession()=false, returning null");
            }
            else
            {
                LOCALGROUP_MEMBERS_INFO_3 lgmi_3 = new LOCALGROUP_MEMBERS_INFO_3();
                lgmi_3.lgrmi3_domainandname = _args.username;

                IntPtr bufptr = IntPtr.Zero;
                bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(lgmi_3));

                try
                {
                    Marshal.StructureToPtr(lgmi_3, bufptr, false);

                    if (!NETAPIWrapper.NetApiInitCalled)
                    {
                        NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                    }

                    NETAPIMarshal.apiNetLocalGroupAddMembers(_args.servername, _args.groupname, 3, bufptr, 1);
                    result = true;
                }
                catch (Exception e)
                {
                    HandleNETAPIException("AddGroupMember", e);
                }
                finally
                {
                    if (bufptr != IntPtr.Zero)
                    {
                        Marshal.DestroyStructure(bufptr, lgmi_3.GetType());
                        Marshal.FreeHGlobal(bufptr);
                    }
                }
            }
            return result;
        }

        private static bool doWork_DeleteGroupMember(object args)
        {
            if (!(args is NETAPIArguments))
            {
                return false;
            }
            NETAPIArguments _args = (NETAPIArguments)args;

            bool result = false;

            LogArgs("doWork_DeleteGroupMember", _args);

            // Ensure we have creds
            if (!Session.EnsureNullSession(_args.servername, _args.ce))
            {
                Logger.Log("NETAPIWrapper.DeleteGroupMember(): EnsureNullSession()=false, returning null");
            }
            else
            {
                LOCALGROUP_MEMBERS_INFO_3 lgmi_3 = new LOCALGROUP_MEMBERS_INFO_3();
                lgmi_3.lgrmi3_domainandname = _args.username;

                IntPtr bufptr = IntPtr.Zero;
                bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(lgmi_3));

                try
                {
                    Marshal.StructureToPtr(lgmi_3, bufptr, false);

                    if (!NETAPIWrapper.NetApiInitCalled)
                    {
                        NETAPIWrapper.NetApiInitCalled = NETAPIWrapper.NetInitMemory(_args.ce, _args.domain, _args.servername);
                    }

                    NETAPIMarshal.apiNetLocalGroupDelMembers(_args.servername, _args.groupname, 3, bufptr, 1);
                    result = true;
                }
                catch (Exception e)
                {
                    HandleNETAPIException("DeleteGroupMember", e);
                }
                finally
                {
                    if (bufptr != IntPtr.Zero)
                    {
                        Marshal.DestroyStructure(bufptr, lgmi_3.GetType());
                        Marshal.FreeHGlobal(bufptr);
                    }
                }
            }
            return result;
        }


        public static bool ChangeUserPassword(string servername, string username, string password)
        {
            USER_INFO_1003 ui1003 = new USER_INFO_1003();
            ui1003.usri1003_password = password;

            IntPtr bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(ui1003));

            bool result = false;

            try
            {
                if (!NETAPIWrapper.NetApiInitCalled)
                {
                    bool retvalue = NETAPIWrapper.NetInitMemory(null, null, servername);
                    System.Windows.Forms.MessageBox.Show("retvalue: " + retvalue);
                }

                Marshal.StructureToPtr(ui1003, bufptr, false);
                NETAPIMarshal.apiNetUserSetInfo(servername, username, 1003, bufptr, IntPtr.Zero);
                result = true;
            }
            catch (Exception e)
            {
                result = false;
                HandleNETAPIException("ChangePassword", e);
            }
            finally
            {
                Marshal.DestroyStructure(bufptr, ui1003.GetType());
                Marshal.FreeHGlobal(bufptr);
            }

            return result;
        }

#endregion
    }
}
