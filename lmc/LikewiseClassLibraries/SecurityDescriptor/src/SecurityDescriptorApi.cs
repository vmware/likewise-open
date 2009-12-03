using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Likewise.LMC.SecurityDesriptor
{
    public class SecurityDescriptorApi
    {
        #region Interop Apis

        private const string LibADVAPIPath = "advapi32.dll";

        // This function uses the DACL to retrieve an array of explicit entries,
        // each of which contains information about individual ACEs within the
        // DACL.
        [DllImport(LibADVAPIPath, CharSet = CharSet.Auto, SetLastError = true)]
        public static extern Int32 GetExplicitEntriesFromAcl(
        IntPtr pacl,
        ref UInt32 pcCountOfExplicitEntries,
        out IntPtr pListOfExplicitEntries);

        [DllImport(LibADVAPIPath, CharSet = CharSet.Auto)]
        public static extern uint GetNamedSecurityInfo(
            string pObjectName,
            SE_OBJECT_TYPE ObjectType,
            SECURITY_INFORMATION SecurityInfo,
            out IntPtr pSidOwner,
            out IntPtr pSidGroup,
            out IntPtr pDacl,
            out IntPtr pSacl,
            out IntPtr pSecurityDescriptor);

        [DllImport(LibADVAPIPath, CharSet = CharSet.Auto, SetLastError = true)]
        public static extern bool LookupAccountSid(
            string lpSystemName,
            [MarshalAs(UnmanagedType.LPArray)] byte[] Sid,
            System.Text.StringBuilder lpName,
            ref uint cchName,
            System.Text.StringBuilder ReferencedDomainName,
            ref uint cchReferencedDomainName,
            out SID_NAME_USE peUse);

        [DllImport(LibADVAPIPath, SetLastError = true)]
        public static extern bool GetAclInformation(IntPtr pAcl, ref ACL_SIZE_INFORMATION pAclInformation, uint nAclInformationLength, ACL_INFORMATION_CLASS dwAclInformationClass);

        [DllImport(LibADVAPIPath)]
        public static extern int GetAce(IntPtr aclPtr, int aceIndex, out IntPtr acePtr);

        [DllImport(LibADVAPIPath)]
        public static extern uint GetLengthSid(IntPtr pSid);

        [DllImport(LibADVAPIPath, CharSet = CharSet.Auto, SetLastError = true)]
        public static extern bool ConvertSidToStringSid(
            [MarshalAs(UnmanagedType.LPArray)] byte[] pSID,
            out IntPtr ptrSid);

        [DllImport(LibADVAPIPath, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool OpenProcessToken(IntPtr ProcessHandle,
            UInt32 DesiredAccess, out IntPtr TokenHandle);

        [DllImport(LibADVAPIPath, SetLastError = true, CharSet = CharSet.Auto)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool LookupPrivilegeValueW([MarshalAs(UnmanagedType.LPWStr)] string lpSystemName,
            [MarshalAs(UnmanagedType.LPWStr)] string lpName,
            out IntPtr lpLuid);

        // Use this signature if you want the previous state information returned
        [DllImport(LibADVAPIPath, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool AdjustTokenPrivileges(IntPtr TokenHandle,
           [MarshalAs(UnmanagedType.Bool)]bool DisableAllPrivileges,
           TOKEN_PRIVILEGES NewState,
           UInt32 BufferLengthInBytes,
           TOKEN_PRIVILEGES PreviousState,
           UInt32 ReturnLengthInBytes);

        // Use this signature if you do not want the previous state
        [DllImport(LibADVAPIPath, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool AdjustTokenPrivileges(IntPtr TokenHandle,
           [MarshalAs(UnmanagedType.Bool)]bool DisableAllPrivileges,
           ref TOKEN_PRIVILEGES NewState,
           UInt32 Zero,
           IntPtr Null1,
           UInt32 Null2);

        [DllImport("kernel32.dll", SetLastError = true, CallingConvention = CallingConvention.Winapi, CharSet = CharSet.Auto)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool CloseHandle(IntPtr hObject);

        [DllImport(LibADVAPIPath, SetLastError = true)]
        public static extern bool GetTokenInformation(
            IntPtr TokenHandle,
            TOKEN_INFORMATION_CLASS TokenInformationClass,
            ref IntPtr TokenInformation,
            uint TokenInformationLength,
            out uint ReturnLength);

        [DllImport(LibADVAPIPath, SetLastError = true)]
        public static extern bool GetTokenInformation(
            IntPtr TokenHandle,
            TOKEN_INFORMATION_CLASS TokenInformationClass,
            TOKEN_PRIVILEGES TokenInformation,
            uint TokenInformationLength,
            out uint ReturnLength);

        [DllImport(LibADVAPIPath, SetLastError = true)]
        public static extern bool SetTokenInformation(
            IntPtr TokenHandle,
            TOKEN_INFORMATION_CLASS TokenInformationClass,
            TOKEN_PRIVILEGES TokenInformation,
            uint TokenInformationLength);

        [DllImport(LibADVAPIPath, SetLastError = true)]
        public static extern bool BuildTrusteeWithSid(
             out IntPtr pTrustee,
             IntPtr pSid);

        [DllImport(LibADVAPIPath, SetLastError = true)]
        public static extern bool LogonUser(
            String lpszUsername,
            String lpszDomain,
            String lpszPassword,
            int dwLogonType,
            int dwLogonProvider,
            ref IntPtr phToken
            );

        #endregion

        #region Class Data

        public const int SE_OWNER_DEFAULTED = 0x1; //1
        public const int SE_GROUP_DEFAULTED = 0x2; //2
        public const int SE_DACL_PRESENT = 0x4; //4
        public const int SE_DACL_DEFAULTED = 0x8; //8
        public const int SE_SACL_PRESENT = 0x10; //16
        public const int SE_SACL_DEFAULTED = 0x20; //32
        public const int SE_DACL_AUTO_INHERIT_REQ = 0x100; //256
        public const int SE_SACL_AUTO_INHERIT_REQ = 0x200; //512
        public const int SE_DACL_AUTO_INHERITED = 0x400; //1024
        public const int SE_SACL_AUTO_INHERITED = 0x800; //2048
        public const int SE_DACL_PROTECTED = 0x1000; //4096
        public const int SE_SACL_PROTECTED = 0x2000; //8192
        public const int SE_SELF_RELATIVE = 0x8000; //32768

        //Use these for DesiredAccess
        public static UInt32 STANDARD_RIGHTS_REQUIRED = 0x000F0000;
        public static UInt32 STANDARD_RIGHTS_READ = 0x00020000;
        public static UInt32 TOKEN_ASSIGN_PRIMARY = 0x0001;
        public static UInt32 TOKEN_DUPLICATE = 0x0002;
        public static UInt32 TOKEN_IMPERSONATE = 0x0004;
        public static UInt32 TOKEN_QUERY = 0x0008;
        public static UInt32 TOKEN_QUERY_SOURCE = 0x0010;
        public static UInt32 TOKEN_ADJUST_PRIVILEGES = 0x0020;
        public static UInt32 TOKEN_ADJUST_GROUPS = 0x0040;
        public static UInt32 TOKEN_ADJUST_DEFAULT = 0x0080;
        public static UInt32 TOKEN_ADJUST_SESSIONID = 0x0100;
        public static UInt32 TOKEN_READ = (STANDARD_RIGHTS_READ | TOKEN_QUERY);
        public static UInt32 TOKEN_ALL_ACCESS = (STANDARD_RIGHTS_REQUIRED | TOKEN_ASSIGN_PRIMARY |
            TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_QUERY_SOURCE |
            TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_GROUPS | TOKEN_ADJUST_DEFAULT |
            TOKEN_ADJUST_SESSIONID);

        //Token privilege constants
        public const UInt32 SE_PRIVILEGE_ENABLED_BY_DEFAULT = 0x00000001;
        public const UInt32 SE_PRIVILEGE_ENABLED = 0x00000002;
        public const UInt32 SE_PRIVILEGE_REMOVED = 0x00000004;
        public const UInt32 SE_PRIVILEGE_USED_FOR_ACCESS = 0x80000000;


        public const string SE_ASSIGNPRIMARYTOKEN_NAME = "SeAssignPrimaryTokenPrivilege";
        public const string SE_AUDIT_NAME = "SeAuditPrivilege";
        public const string SE_BACKUP_NAME = "SeBackupPrivilege";
        public const string SE_CHANGE_NOTIFY_NAME = "SeChangeNotifyPrivilege";
        public const string SE_CREATE_GLOBAL_NAME = "SeCreateGlobalPrivilege";
        public const string SE_CREATE_PAGEFILE_NAME = "SeCreatePagefilePrivilege";
        public const string SE_CREATE_PERMANENT_NAME = "SeCreatePermanentPrivilege";
        public const string SE_CREATE_SYMBOLIC_LINK_NAME = "SeCreateSymbolicLinkPrivilege";
        public const string SE_CREATE_TOKEN_NAME = "SeCreateTokenPrivilege";
        public const string SE_DEBUG_NAME = "SeDebugPrivilege";
        public const string SE_ENABLE_DELEGATION_NAME = "SeEnableDelegationPrivilege";
        public const string SE_IMPERSONATE_NAME = "SeImpersonatePrivilege";
        public const string SE_INC_BASE_PRIORITY_NAME = "SeIncreaseBasePriorityPrivilege";
        public const string SE_INCREASE_QUOTA_NAME = "SeIncreaseQuotaPrivilege";
        public const string SE_INC_WORKING_SET_NAME = "SeIncreaseWorkingSetPrivilege";
        public const string SE_LOAD_DRIVER_NAME = "SeLoadDriverPrivilege";
        public const string SE_LOCK_MEMORY_NAME = "SeLockMemoryPrivilege";
        public const string SE_MACHINE_ACCOUNT_NAME = "SeMachineAccountPrivilege";
        public const string SE_MANAGE_VOLUME_NAME = "SeManageVolumePrivilege";
        public const string SE_PROF_SINGLE_PROCESS_NAME = "SeProfileSingleProcessPrivilege";
        public const string SE_RELABEL_NAME = "SeRelabelPrivilege";
        public const string SE_REMOTE_SHUTDOWN_NAME = "SeRemoteShutdownPrivilege";
        public const string SE_RESTORE_NAME = "SeRestorePrivilege";
        public const string SE_SECURITY_NAME = "SeSecurityPrivilege";
        public const string SE_SHUTDOWN_NAME = "SeShutdownPrivilege";
        public const string SE_SYNC_AGENT_NAME = "SeSyncAgentPrivilege";
        public const string SE_SYSTEM_ENVIRONMENT_NAME = "SeSystemEnvironmentPrivilege";
        public const string SE_SYSTEM_PROFILE_NAME = "SeSystemProfilePrivilege";
        public const string SE_SYSTEMTIME_NAME = "SeSystemtimePrivilege";
        public const string SE_TAKE_OWNERSHIP_NAME = "SeTakeOwnershipPrivilege";
        public const string SE_TCB_NAME = "SeTcbPrivilege";
        public const string SE_TIME_ZONE_NAME = "SeTimeZonePrivilege";
        public const string SE_TRUSTED_CREDMAN_ACCESS_NAME = "SeTrustedCredManAccessPrivilege";
        public const string SE_UNDOCK_NAME = "SeUndockPrivilege";
        public const string SE_UNSOLICITED_INPUT_NAME = "SeUnsolicitedInputPrivilege";

        #endregion

        #region Structures

        [StructLayoutAttribute(LayoutKind.Sequential)]
        public class TOKEN_PRIVILEGES
        {
            public UInt32 PrivilegeCount;
            //[MarshalAs(UnmanagedType.ByValArray)]
            //public LUID_AND_ATTRIBUTES[] Privileges;
            public IntPtr Luid;
            public int Attributes;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
        public class LwLUID
        {
            public Int32 LowPart;
            public Int32 HighPart;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
        public class LUID_AND_ATTRIBUTES
        {
            //public LwLUID Luid;
            public IntPtr Luid;
            public UInt32 Attributes;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto, Pack = 4)]
        public class TRUSTEE
        {
            public IntPtr pMultipleTrustee; // must be null
            public int MultipleTrusteeOperation;
            public int TrusteeForm;
            public int TrusteeType;
            public string ptstrName;
        }

        [StructLayoutAttribute(LayoutKind.Sequential)]
        public class SECURITY_DESCRIPTOR
        {
            public byte revision;
            public byte size;
            public short control;
            public IntPtr owner;
            public IntPtr group;
            public IntPtr sacl;
            public IntPtr dacl;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class ACE
        {
            public uint AccessMask;
            public uint AceFlags;
            public uint AceType;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string GuidInheritedObjectType;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string GuidObjectType;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string Trustee;
        };

        [StructLayout(LayoutKind.Sequential)]
        public class LwACL
        {
            public uint AclRevision;
            public uint Sbz1; // Padding (should be 0)
            public short AclSize;
            public short AceCount;
            public short Sbz2; // Padding (should be 0)
        }

        [StructLayout(LayoutKind.Sequential)]
        public class EXPLICIT_ACCESS
        {
            uint grfAccessPermissions;
            ACCESS_MODE grfAccessMode;
            uint grfInheritance;
            TRUSTEE Trustee;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class ACL_REVISION_INFORMATION
        {
            public uint AclRevision;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class ACL_SIZE_INFORMATION
        {
            public uint AceCount;
            public uint AclBytesInUse;
            public uint AclBytesFree;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class ACE_HEADER
        {
            public byte AceType;
            public byte AceFlags;
            public short AceSize;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct ACCESS_ALLOWED_ACE
        {
            public ACE_HEADER Header;
            public int Mask;
            public int SidStart;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class SECURITY_ATTRIBUTES
        {
            public int nLength;
            public unsafe byte* lpSecurityDescriptor;
            public int bInheritHandle;
        }

        [StructLayout(LayoutKind.Sequential)]
        public class SID_IDENTIFIER_AUTHORITY
        {
            byte Value;
        }

        #endregion

        #region Enums

        public enum ACL_INFORMATION_CLASS
        {
            AclRevisionInformation = 1,
            AclSizeInformation
        }

        public enum ACCESS_MODE
        {
            NOT_USED_ACCESS = 0,
            GRANT_ACCESS,
            SET_ACCESS,
            DENY_ACCESS,
            REVOKE_ACCESS,
            SET_AUDIT_SUCCESS,
            SET_AUDIT_FAILURE
        }

        public enum SID_NAME_USE
        {
            SidTypeUser = 1,
            SidTypeGroup,
            SidTypeDomain,
            SidTypeAlias,
            SidTypeWellKnownGroup,
            SidTypeDeletedAccount,
            SidTypeInvalid,
            SidTypeUnknown,
            SidTypeComputer
        }

        [Flags]
        public enum ACCESS_MASK : uint
        {
            DELETE = 0x00010000,
            READ_CONTROL = 0x00020000,
            WRITE_DAC = 0x00040000,
            WRITE_OWNER = 0x00080000,
            SYNCHRONIZE = 0x00100000,

            STANDARD_RIGHTS_REQUIRED = 0x000f0000,

            STANDARD_RIGHTS_READ = 0x00020000,
            STANDARD_RIGHTS_WRITE = 0x00020000,
            STANDARD_RIGHTS_EXECUTE = 0x00020000,

            STANDARD_RIGHTS_ALL = 0x001f0000,

            SPECIFIC_RIGHTS_ALL = 0x0000ffff,

            ACCESS_SYSTEM_SECURITY = 0x01000000,

            MAXIMUM_ALLOWED = 0x02000000,

            GENERIC_READ = 0x80000000,
            GENERIC_WRITE = 0x40000000,
            GENERIC_EXECUTE = 0x20000000,
            GENERIC_ALL = 0x10000000,

            DESKTOP_READOBJECTS = 0x00000001,
            DESKTOP_CREATEWINDOW = 0x00000002,
            DESKTOP_CREATEMENU = 0x00000004,
            DESKTOP_HOOKCONTROL = 0x00000008,
            DESKTOP_JOURNALRECORD = 0x00000010,
            DESKTOP_JOURNALPLAYBACK = 0x00000020,
            DESKTOP_ENUMERATE = 0x00000040,
            DESKTOP_WRITEOBJECTS = 0x00000080,
            DESKTOP_SWITCHDESKTOP = 0x00000100,

            WINSTA_ENUMDESKTOPS = 0x00000001,
            WINSTA_READATTRIBUTES = 0x00000002,
            WINSTA_ACCESSCLIPBOARD = 0x00000004,
            WINSTA_CREATEDESKTOP = 0x00000008,
            WINSTA_WRITEATTRIBUTES = 0x00000010,
            WINSTA_ACCESSGLOBALATOMS = 0x00000020,
            WINSTA_EXITWINDOWS = 0x00000040,
            WINSTA_ENUMERATE = 0x00000100,
            WINSTA_READSCREEN = 0x00000200,

            WINSTA_ALL_ACCESS = 0x0000037f
        }

        public enum SE_OBJECT_TYPE : uint
        {
            SE_UNKNOWN_OBJECT_TYPE = 0,
            SE_FILE_OBJECT,
            SE_SERVICE,
            SE_PRINTER,
            SE_REGISTRY_KEY,
            SE_LMSHARE,
            SE_KERNEL_OBJECT,
            SE_WINDOW_OBJECT,
            SE_DS_OBJECT,
            SE_DS_OBJECT_ALL,
            SE_PROVIDER_DEFINED_OBJECT,
            SE_WMIGUID_OBJECT,
            SE_REGISTRY_WOW64_32KEY
        }

        [Flags]
        public enum SECURITY_INFORMATION : uint
        {
            OWNER_SECURITY_INFORMATION = 0x00000001,
            GROUP_SECURITY_INFORMATION = 0x00000002,
            DACL_SECURITY_INFORMATION = 0x00000004,
            SACL_SECURITY_INFORMATION = 0x00000008,
            UNPROTECTED_SACL_SECURITY_INFORMATION = 0x10000000,
            UNPROTECTED_DACL_SECURITY_INFORMATION = 0x20000000,
            PROTECTED_SACL_SECURITY_INFORMATION = 0x40000000,
            PROTECTED_DACL_SECURITY_INFORMATION = 0x80000000
        }

        public enum AccessTypes : uint
        {
            Allow = 0,
            Deny = 1,
            Audit = 2,
            Allow_Object = 5,
            Deny_Object = 6,
            Audit_Object = 7
        }

        public enum ACEFlags : uint
        {
            //Non-container child objects inherit the ACE as an effective ACE.
            OBJECT_INHERIT_ACE = 0x1,

            //The ACE has an effect on child namespaces as well as the current namespace.
            CONTAINER_INHERIT_ACE = 0x2,

            //The ACE applies only to the current namespace and immediate children .
            NO_PROPAGATE_INHERIT_ACE = 0x4,

            //The ACE applies only to child namespaces.
            INHERIT_ONLY_ACE = 0x8,

            //This is not set by clients, but is reported to clients when the source of an ACE is a parent namespace.
            INHERITED_ACE = 0x10,

            VALID_INHERIT_FLAGS = 0x1F,

            SUCCESSFUL_ACCESS = 0x40,

            FAILED_ACCESS = 0x80
        }

        public enum AccessRights : uint
        {
            //Enables the account and grants the user read permissions. This is a default access right for all users and corresponds to the
            //Enable Account permission on the Security tab of the WMI Control. For more information, see Setting Namespace Security with the WMI Control.
            WBEM_ENABLE = 0x1,

            //Allows the execution of methods. Providers can perform additional access checks.
            //This is a default access right for all users and corresponds to the Execute Methods permission on the Security tab of the WMI Control.
            WBEM_METHOD_EXECUTE = 0x2,

            //Allows a user account to write to classes in the WMI repository as well as instances.
            //A user cannot write to system classes. Only members of the Administrators group have this permission.
            //BEM_FULL_WRITE_REP corresponds to the Full Write permission on the Security tab of the WMI Control.
            WBEM_FULL_WRITE_REP = 0x4,

            //Allows you to write data to instances only, not classes.
            //A user cannot write classes to the WMI repository. Only members of the Administrators group have this right.
            //WBEM_PARTIAL_WRITE_REP corresponds to the Partial Write permission on the Security tab of the WMI Control.
            WBEM_PARTIAL_WRITE_REP = 0x8,

            //Allows writing classes and instances to providers.
            //Note that providers can do additional access checks when impersonating a user.
            //This is a default access right for all users and corresponds to the Provider Write permission on the Security tab of the WMI Control.
            WBEM_WRITE_PROVIDER = 0x10,

            //Allows a user account to remotely perform any operations allowed by the permissions described above.
            //Only members of the Administrators group have this right.
            //WBEM_REMOTE_ACCESS corresponds to the Remote Enable permission on the Security tab of the WMI Control.
            WBEM_REMOTE_ACCESS = 0x20,

            //Allows you to read the namespace security descriptor.
            //Only members of the Administrators group have this right.
            //READ_CONTROL corresponds to the Read Security permission on the Security tab of the WMI Control.
            READ_CONTROL = 0x20000,

            //Allows you to change the discretionary access control list (DACL) on the namespace.
            //Only members of the Administrators group have this right.
            //WRITE_DAC corresponds to the Edit Security permission on the Security tab of the WMI Control.
            WRITE_DAC = 0x40000
        }

        //Event Security Constants
        public enum EventSecurityConstants
        {
            //Specifies that the account can publish events to the instance of __EventFilter that defines the event filter for a permanent consumer.
            WBEM_RIGHT_PUBLISH = 0x80,

            //Specifies that a consumer can subscribe to the events delivered to a sink. Used in IWbemEventSink::SetSinkSecurity.
            WBEM_RIGHT_SUBSCRIBE = 0x40,

            //Event provider indicates that WMI checks the SECURITY_DESCRIPTOR property in each event (inherited from __Event),
            //and only sends events to consumers with the appropriate access permissions.
            WBEM_S_SUBJECT_TO_SDS = 0x43003
        }

        public enum TOKEN_INFORMATION_CLASS
        {

            /// <summary>
            /// The buffer receives a TOKEN_USER structure that contains the user account of the token.
            /// </summary>
            TokenUser = 1,

            /// <summary>
            /// The buffer receives a TOKEN_GROUPS structure that contains the group accounts associated with the token.
            /// </summary>
            TokenGroups,

            /// <summary>
            /// The buffer receives a TOKEN_PRIVILEGES structure that contains the privileges of the token.
            /// </summary>
            TokenPrivileges,

            /// <summary>
            /// The buffer receives a TOKEN_OWNER structure that contains the default owner security identifier (SID) for newly created objects.
            /// </summary>
            TokenOwner,

            /// <summary>
            /// The buffer receives a TOKEN_PRIMARY_GROUP structure that contains the default primary group SID for newly created objects.
            /// </summary>
            TokenPrimaryGroup,

            /// <summary>
            /// The buffer receives a TOKEN_DEFAULT_DACL structure that contains the default DACL for newly created objects.
            /// </summary>
            TokenDefaultDacl,

            /// <summary>
            /// The buffer receives a TOKEN_SOURCE structure that contains the source of the token. TOKEN_QUERY_SOURCE access is needed to retrieve this information.
            /// </summary>
            TokenSource,

            /// <summary>
            /// The buffer receives a TOKEN_TYPE value that indicates whether the token is a primary or impersonation token.
            /// </summary>
            TokenType,

            /// <summary>
            /// The buffer receives a SECURITY_IMPERSONATION_LEVEL value that indicates the impersonation level of the token. If the access token is not an impersonation token, the function fails.
            /// </summary>
            TokenImpersonationLevel,

            /// <summary>
            /// The buffer receives a TOKEN_STATISTICS structure that contains various token statistics.
            /// </summary>
            TokenStatistics,

            /// <summary>
            /// The buffer receives a TOKEN_GROUPS structure that contains the list of restricting SIDs in a restricted token.
            /// </summary>
            TokenRestrictedSids,

            /// <summary>
            /// The buffer receives a DWORD value that indicates the Terminal Services session identifier that is associated with the token.
            /// </summary>
            TokenSessionId,

            /// <summary>
            /// The buffer receives a TOKEN_GROUPS_AND_PRIVILEGES structure that contains the user SID, the group accounts, the restricted SIDs, and the authentication ID associated with the token.
            /// </summary>
            TokenGroupsAndPrivileges,

            /// <summary>
            /// Reserved.
            /// </summary>
            TokenSessionReference,

            /// <summary>
            /// The buffer receives a DWORD value that is nonzero if the token includes the SANDBOX_INERT flag.
            /// </summary>
            TokenSandBoxInert,

            /// <summary>
            /// Reserved.
            /// </summary>
            TokenAuditPolicy,

            /// <summary>
            /// The buffer receives a TOKEN_ORIGIN value.
            /// </summary>
            TokenOrigin,

            /// <summary>
            /// The buffer receives a TOKEN_ELEVATION_TYPE value that specifies the elevation level of the token.
            /// </summary>
            TokenElevationType,

            /// <summary>
            /// The buffer receives a TOKEN_LINKED_TOKEN structure that contains a handle to another token that is linked to this token.
            /// </summary>
            TokenLinkedToken,

            /// <summary>
            /// The buffer receives a TOKEN_ELEVATION structure that specifies whether the token is elevated.
            /// </summary>
            TokenElevation,

            /// <summary>
            /// The buffer receives a DWORD value that is nonzero if the token has ever been filtered.
            /// </summary>
            TokenHasRestrictions,

            /// <summary>
            /// The buffer receives a TOKEN_ACCESS_INFORMATION structure that specifies security information contained in the token.
            /// </summary>
            TokenAccessInformation,

            /// <summary>
            /// The buffer receives a DWORD value that is nonzero if virtualization is allowed for the token.
            /// </summary>
            TokenVirtualizationAllowed,

            /// <summary>
            /// The buffer receives a DWORD value that is nonzero if virtualization is enabled for the token.
            /// </summary>
            TokenVirtualizationEnabled,

            /// <summary>
            /// The buffer receives a TOKEN_MANDATORY_LABEL structure that specifies the token's integrity level.
            /// </summary>
            TokenIntegrityLevel,

            /// <summary>
            /// The buffer receives a DWORD value that is nonzero if the token has the UIAccess flag set.
            /// </summary>
            TokenUIAccess,

            /// <summary>
            /// The buffer receives a TOKEN_MANDATORY_POLICY structure that specifies the token's mandatory integrity policy.
            /// </summary>
            TokenMandatoryPolicy,

            /// <summary>
            /// The buffer receives the token's logon security identifier (SID).
            /// </summary>
            TokenLogonSid,

            /// <summary>
            /// The maximum value for this enumeration
            /// </summary>
            MaxTokenInfoClass

        }

        public enum LogonType : int
        {
            //'This logon type is intended for batch servers, where processes may be executing on behalf of a user without
            //'their direct intervention. This type is also for higher performance servers that process many plaintext
            //'authentication attempts at a time, such as mail or Web servers.
            //'The LogonUser function does not cache credentials for this logon type.
            LOGON32_LOGON_BATCH = 4,

            //'This logon type is intended for users who will be interactively using the computer, such as a user being logged on
            //'by a terminal server, remote shell, or similar process.
            //'This logon type has the additional expense of caching logon information for disconnected operations;
            //'therefore, it is inappropriate for some client/server applications,
            //'such as a mail server.
            LOGON32_LOGON_INTERACTIVE = 2,

            //'This logon type is intended for high performance servers to authenticate plaintext passwords.
            //'The LogonUser function does not cache credentials for this logon type.
            LOGON32_LOGON_NETWORK = 3,

            //'This logon type preserves the name and password in the authentication package, which allows the server to make
            //'connections to other network servers while impersonating the client. A server can accept plaintext credentials
            //'from a client, call LogonUser, verify that the user can access the system across the network, and still
            //'communicate with other servers.
            //'NOTE: Windows NT:  This value is not supported.
            LOGON32_LOGON_NETWORK_CLEARTEXT = 8,

            //'This logon type allows the caller to clone its current token and specify new credentials for outbound connections.
            //'The new logon session has the same local identifier but uses different credentials for other network connections.
            //'NOTE: This logon type is supported only by the LOGON32_PROVIDER_WINNT50 logon provider.
            //'NOTE: Windows NT:  This value is not supported.
            LOGON32_LOGON_NEW_CREDENTIALS = 9,

            //'Indicates a service-type logon. The account provided must have the service privilege enabled.
            LOGON32_LOGON_SERVICE = 5,

            //'This logon type is for GINA DLLs that log on users who will be interactively using the computer.
            //'This logon type can generate a unique audit record that shows when the workstation was unlocked.
            LOGON32_LOGON_UNLOCK = 7
        }

        public enum LogonProvider : int
        {
            /// <summary>
            /// Use the standard logon provider for the system.
            /// The default security provider is negotiate, unless you pass NULL for the domain name and the user name
            /// is not in UPN format. In this case, the default provider is NTLM.
            /// NOTE: Windows 2000/NT:   The default security provider is NTLM.
            /// </summary>
            LOGON32_PROVIDER_DEFAULT = 0,
            LOGON32_PROVIDER_WINNT50,
            LOGON32_PROVIDER_WINNT40
        }

        #endregion
    }
}
