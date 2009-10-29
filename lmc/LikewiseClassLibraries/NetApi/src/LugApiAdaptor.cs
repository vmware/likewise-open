using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.AuthUtils;
using Likewise.LMC.NETAPI.Implementation;

namespace Likewise.LMC.NETAPI
{
    public class LUGAPI
    {
        private static ILugApi _lugApiImplementation = null;

        public enum LUGType
        {
            Undefined,
            Dummy,
            User,
            Group
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

        #region definitions

        public const int MAX_PREFERRED_LENGTH = -1;

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

        // We should convert to using ErrorCodes.WIN32Enum.*value*
        // values ASAP.  For now, just use these simple error codes.
        public const int ERROR_SUCCESS = 0;
        public const int ERROR_FAILURE = 1;

        #endregion

        const string USER = "UName";
        const string FULL_NAME = "Full Name";
        const string DESCRIPTION = "Description";
        const string GROUP = "GName";
        const string DISABLED = "Disabled";

        #region Accessors

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

        static
        LUGAPI()
        {
            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
            {
                _lugApiImplementation = new WinLugApi() as ILugApi;
            }
            else
            {
                _lugApiImplementation = new LwLugApi() as ILugApi;
            }
        }

        public static uint
        NetAddGroup(
            CredentialEntry ce,
            string servername,
            string groupname,
            string description
            )
        {
            return _lugApiImplementation.NetAddGroup(
                ce,
                servername,
                groupname,
                description
                );
        }

        public static uint
        NetAddUser(
            CredentialEntry ce,
            string servername,
            string username,
            string password,
            string fullname,
            string description,
            uint flags
            )
        {
            return _lugApiImplementation.NetAddUser(
                ce,
                servername,
                username,
                password,
                fullname,
                description,
                flags
                );
        }

        public static uint
        NetChangePassword(
            CredentialEntry ce,
            string servername,
            string username,
            string password
            )
        {
            return _lugApiImplementation.NetChangePassword(
                ce,
                servername,
                username,
                password
                );
        }

        public static bool
        NetDeleteUser(
            CredentialEntry ce,
            string servername,
            string username
            )
        {
            return _lugApiImplementation.NetDeleteUser(
                ce,
                servername,
                username
                );
        }

        public static bool
        NetDeleteGroup(
            CredentialEntry ce,
            string servername,
            string username
            )
        {
            return _lugApiImplementation.NetDeleteGroup(
                ce,
                servername,
                username
                );
        }

        public static uint
        NetGetGroups(
            CredentialEntry ce,
            string servername,
            string username,
            out string[] groups
            )
        {
            return _lugApiImplementation.NetGetGroups(
                ce,
                servername,
                username,
                out groups
                );
        }

        public static void
        NetEnumUsers(
            CredentialEntry ce,
            string servername,
            int resumeHandle,
            out LUGEnumStatus enumStatus
            )
        {
            _lugApiImplementation.NetEnumUsers(
                ce,
                servername,
                resumeHandle,
                out enumStatus
                );
        }

        public static void
        NetEnumGroups(
            CredentialEntry ce,
            string servername,
            int resumeHandle,
            out LUGEnumStatus enumStatus
            )
        {
            _lugApiImplementation.NetEnumGroups(
                ce,
                servername,
                resumeHandle,
                out enumStatus
                );
        }

        public static uint
        NetDeleteUserFromGroup(
            CredentialEntry ce,
            string servername,
            string groupname,
            string username
            )
        {
            return _lugApiImplementation.NetDeleteUserFromGroup(
                ce,
                servername,
                groupname,
                username
                );
        }

        public static uint
        NetGetUserInfo(
            CredentialEntry ce,
            string servername,
            string username,
            out LUGInfo userInfo
            )
        {
            return _lugApiImplementation.NetGetUserInfo(
                ce,
                servername,
                username,
                out userInfo
                );
        }

        public static bool
        NetRenameUser(
            CredentialEntry ce,
            string servername,
            string oldusername,
            string username
            )
        {
            return _lugApiImplementation.NetRenameUser(
                ce,
                servername,
                oldusername,
                username
                );
        }

        public static bool
        NetRenameGroup(
            CredentialEntry ce,
            string servername,
            string oldgroupname,
            string groupname
            )
        {
            return _lugApiImplementation.NetRenameGroup(
                ce,
                servername,
                oldgroupname,
                groupname
                );
        }

        public static uint
        NetEditUserFullName(
            CredentialEntry ce,
            string servername,
            string username,
            string fullname
            )
        {
            return _lugApiImplementation.NetEditUserFullName(
                ce,
                servername,
                username,
                fullname
                );
        }

        public static uint
        NetEditUserDescription(
            CredentialEntry ce,
            string servername,
            string username,
            string description
            )
        {
            return _lugApiImplementation.NetEditUserDescription(
                ce,
                servername,
                username,
                description
                );
        }

        public static uint
        NetEditUserFlags(
            CredentialEntry ce,
            string servername,
            string username,
            uint flags
            )
        {
            return _lugApiImplementation.NetEditUserFlags(
                ce,
                servername,
                username,
                flags
                );
        }

        public static uint
        NetGetGroupMembers(
            CredentialEntry ce,
            string servername,
            string groupname,
            out string[] members
            )
        {
            return _lugApiImplementation.NetGetGroupMembers(
                ce,
                servername,
                groupname,
                out members
                );
        }

        public static uint
        NetGetGroupInfo(
            CredentialEntry ce,
            string servername,
            string groupname,
            out string description
            )
        {
            return _lugApiImplementation.NetGetGroupInfo(
                ce,
                servername,
                groupname,
                out description
                );
        }

        public static uint
        NetEditGroupDescription(
            CredentialEntry ce,
            string servername,
            string groupname,
            string description
            )
        {
            return _lugApiImplementation.NetEditGroupDescription(
                ce,
                servername,
                groupname,
                description
                );
        }

        public static uint
        NetAddGroupMember(
            CredentialEntry ce,
            string servername,
            string groupname,
            string username
            )
        {
            return _lugApiImplementation.NetAddGroupMember(
                ce,
                servername,
                groupname,
                username
                );
        }

        public static uint
        NetDeleteGroupMember(
            CredentialEntry ce,
            string servername,
            string groupname,
            string username
            )
        {
            return _lugApiImplementation.NetDeleteGroupMember(
                ce,
                servername,
                groupname,
                username
                );
        }

        public static uint
        NetInitMemory()
        {
            return _lugApiImplementation.NetInitMemory();
        }

        public static bool
        NetInitMemory(
            CredentialEntry ce,
            string servername
            )
		{
            return _lugApiImplementation.NetInitMemory(
                ce,
                servername
                );
		}
    }
}

