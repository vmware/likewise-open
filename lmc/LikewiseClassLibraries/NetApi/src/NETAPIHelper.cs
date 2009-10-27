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
        #region definitions

        const int MAX_PREFERRED_LENGTH = -1;

        // Flags used by UI.  These need to be keep in sync with plumb/samba/source/include.ads.h
        public const uint UF_SCRIPT = 0x00000001;
        public const uint UF_ACCOUNTDISABLE = 0x00000002;

        public const uint UF_LOCKOUT = 0x00000010;
        public const uint UF_PASSWD_NOTREQD = 0x00000020;
        public const uint UF_PASSWD_CANT_CHANGE = 0x00000040;

        public const uint UF_NORMAL_ACCOUNT = 0x00000200;

        public const uint UF_DONT_EXPIRE_PASSWD = 0x00010000;

        public const uint UF_PASSWORD_EXPIRED = 0x00800000;

        public const int NETAPI_MAX_USER_NAME_LENGTH = 20;
        public const int NETAPI_MAX_GROUP_NAME_LENGTH = 256;
        public const int NETAPI_MAX_DOMAIN_NAME_LENGTH = 255;

        #endregion

        #region Class data

        const string USER = "UName";
        const string FULL_NAME = "Full Name";
        const string DESCRIPTION = "Description";
        const string GROUP = "GName";
        const string DISABLED = "Disabled";

        public enum LUGType
        {
            Undefined,
            Dummy,
            User,
            Group
        }

        #endregion

        #region Structures

        private struct NETAPIArguments
        {
            public CredentialEntry ce;
            public string servername;
            public string username;
            public string oldusername;
            public string fullname;
            public string groupname;
            public string oldgroupname;
            public string domain;
            public string password;
            public string description;
            public uint flags;
            public int resumeHandle;
            public boolDelegate boolCallback;
            public stringDelegate stringCallback;
            public stringArrDelegate stringArrCallback;
            public stringArrListDelegate stringArrListCallback;
            public LUGInfoDelegate LUGInfoCallback;
            public LUGAPI.LUGEnumStatusDelegate LUGEnumStatusCallback;

            public void initializeToNull()
            {
                ce = null;
                servername = null;
                username = null;
                oldusername = null;
                fullname = null;
                groupname = null;
                oldgroupname = null;
                domain = null;
                password = null;
                description = null;
                flags = 0;
                resumeHandle = 0;
                boolCallback = null;
                stringCallback = null;
                stringArrCallback = null;
                stringArrListCallback = null;
                LUGInfoCallback = null;
                LUGEnumStatusCallback = null;
            }

        }

        public struct LUGInfo
        {
            public string username;
            public string groupname;
            public string fullname;
            public string description;
            public uint flags;

            public void initializeToNull()
            {
                username = null;
                groupname = null;
                fullname = null;
                description = null;
                flags = 0;
            }
        }

        public struct LUGEnumStatus
        {
            public List<string[]> entries;
            public int totalEntries;
            public int entriesRead;
            public int resumeHandle;
            public bool moreEntries;
            public LUGType type;

            public void initializeToNull()
            {
                entries = null;
                totalEntries = 0;
                entriesRead = 0;
                resumeHandle = 0;
                moreEntries = false;
                type = LUGType.Undefined;
            }
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct USER_INFO_0
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string usri0_name;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct LOCALGROUP_INFO_0
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string lgrpi0_name;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct LOCALGROUP_MEMBERS_INFO_3
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string lgrmi3_domainandname;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct USER_INFO_1003
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string usri1003_password;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct USER_INFO_1012
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string usri1012_usr_comment;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct USER_INFO_1007
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string usri1007_comment;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct USER_INFO_1011
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string usri1011_full_name;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct USER_INFO_1008
        {
            public uint usri1008_flags;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct LOCALGROUP_USERS_INFO_0
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string lgrui0_name;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct USER_INFO_1
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string sUsername;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string sPassword;
            public uint uiPasswordAge;
            public uint uiPriv;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string sHome_Dir;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string sComment;
            public uint uiFlags;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string sScript_Path;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct USER_INFO_23
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string usri23_name;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string usri23_full_name;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string usri23_usr_comment;
            public uint usri1008_flags;
            public uint usri23_user_sid;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct USER_INFO_20
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string usri20_name;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string usri20_full_name;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string usri20_comment;
            public uint usri20_flags;
            public uint usri20_user_id;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct LOCALGROUP_INFO_1
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string name;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string comment;
        }

        #endregion

        #region delegates

        public delegate void boolDelegate(bool boolResult);

        public delegate void stringDelegate(string stringResult);

        public delegate void stringArrDelegate(string[] stringArrResult);

        public delegate void stringArrListDelegate(List<string[]> stringArrListResult);

        public delegate void LUGInfoDelegate(LUGInfo info);

        public delegate void LUGEnumStatusDelegate(LUGEnumStatus status);

        public delegate void objectDelegate(object o);

        #endregion

        #region Helper Methods

        private static void HandleNETAPIException(string methodName, Exception e)
        {
            
            if (e is NETAPIMarshal.NETAPIException)
            {
                NETAPIMarshal.NETAPIException netAPIEx = (NETAPIMarshal.NETAPIException)e;
                Logger.LogException(String.Format(
                    "{0} failed with NetAPI error code {1}", methodName, netAPIEx.errorCode), e);
            }
            else
            {
                Logger.LogException(methodName, e);
            }
        }

        private static void LogArgs(string methodName, NETAPIArguments args)
        {
            Logger.Log(String.Format("{0}({1}{2}{3}{4}{5}{6}{7}{8}{9}{10}{11}{12}) called",
                methodName,
                (args.ce == null) ? "" : args.ce.ToString(),
                (args.servername == null) ? "" : ", servername = " + args.servername,
                (args.username == null) ? "" : ", username = " + args.username,
                (args.oldusername == null) ? "" : ", oldusername = " + args.oldusername,
                (args.fullname == null) ? "" : ", fullname = " + args.fullname,
                (args.groupname == null) ? "" : ", groupname = " + args.groupname,
                (args.oldgroupname == null) ? "" : ", oldgroupname = " + args.oldgroupname,
                (args.domain == null) ? "" : ", domain = " + args.domain,
                (args.password == null) ? "" : ", password = " + args.password,
                (args.description == null) ? "" : ", description = " + args.description,
                (args.flags == 0) ? "" : flagDescription(args.flags),
                (args.resumeHandle == 0) ? "" : ", resumeHandle = " + args.resumeHandle),
                Logger.netAPILogLevel);
        }

        public static string flagDescription(uint flags)
        {
            return String.Format("Flags: {0}{1}{2}{3}{4}{5}{6}{7}{8}{9}{10}{11}{12}{13}{14}{15}\n",
                                 ",\nUF_SCRIPT:\r\t\t\t\t", ((flags & UF_SCRIPT) != 0),
                                 ",\nUF_ACCOUNTDISABLE:\r\t\t\t\t", ((flags & UF_ACCOUNTDISABLE) != 0),
                                 ",\nUF_LOCKOUT:\r\t\t\t\t", ((flags & UF_LOCKOUT) != 0),
                                 ",\nUF_PASSWD_NOTREQD:\r\t\t\t\t", ((flags & UF_PASSWD_NOTREQD) != 0),
                                 ",\nUF_PASSWD_CANT_CHANGE:\r\t\t\t\t", ((flags & UF_PASSWD_CANT_CHANGE) != 0),
                                 ",\nUF_NORMAL_ACCOUNT:\r\t\t\t\t", ((flags & UF_NORMAL_ACCOUNT) != 0),
                                 ",\nUF_DONT_EXPIRE_PASSWD:\r\t\t\t\t", ((flags & UF_DONT_EXPIRE_PASSWD) != 0),
                                 ",\nUF_PASSWORD_EXPIRED:\r\t\t\t\t", ((flags & UF_PASSWORD_EXPIRED) != 0));

        }

        private static string scrubString(string input, int maxLength)
        {
            if (input == null) 
            {
                return null;
            }

            char[] inputArr = input.ToCharArray();

            if (inputArr == null)
            {
                return null;
            }

            char[] outputArr = new char[inputArr.Length];
            int idxOut = 0;
            int idxIn = 0;

            foreach (char c in inputArr)
            {
                if (idxOut >= maxLength)
                {
                    continue;
                }
                //HACK: this is filter out presumed invalid non-English usernames resulting from plumb bug#
                //TODO: remove the check for (int)c > 127; this HACK prevents internationalization.
                else if ((int)c > 127)
                {
                    Logger.Log(String.Format("Non-ASCII character scrubbed from string at index {0}!: {1}",
                        idxIn, (int)c), Logger.netAPILogLevel);
                }
                else if (!(Char.IsLetterOrDigit(c) || Char.IsPunctuation(c) || Char.IsWhiteSpace(c)))
                {
                    Logger.Log(String.Format("Invalid character scrubbed from string at index {0}!: {1}",
                        idxIn, (int)c), Logger.netAPILogLevel);
                }
                else
                {
                    Logger.Log(String.Format("Valid character included string at index {0}: {1} == '{2}'",
                        idxIn, (int)c, c), Logger.netAPILogLevel);

                    outputArr[idxOut] = c;
                    idxOut++;
                }
                idxIn++;
            }

            return new string(outputArr, 0, idxOut);

        }

        #endregion

        #region accessors

        public static string UserColumn
        {
            get
            {
                return USER;
            }
        }

        public static string GroupColumn
        {
            get
            {
                return GROUP;
            }
        }

        public static string FullNameColumn
        {
            get
            {
                return FULL_NAME;
            }
        }

        public static string DescriptionColumn
        {
            get
            {
                return DESCRIPTION;
            }
        }

        public static string Disabled
        {
            get
            {
                return DISABLED;
            }
        }

        #endregion

    }
}
