using System;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.AuthUtils;

namespace Likewise.LMC.NETAPI
{
    public interface ILugApi
    {
        uint
        NetAddGroup(
            CredentialEntry ce,
            string servername,
            string groupname,
            string description
            );

        uint
        NetAddUser(
            CredentialEntry ce,
            string servername,
            string username,
            string password,
            string fullname,
            string description,
            uint flags
            );

        uint
        NetChangePassword(
            CredentialEntry ce,
            string servername,
            string username,
            string password
            );

        bool
        NetDeleteUser(
            CredentialEntry ce,
            string servername,
            string username
            );

        bool
        NetDeleteGroup(
            CredentialEntry ce,
            string servername,
            string username
            );

        uint
        NetGetGroups(
            CredentialEntry ce,
            string servername,
            string username,
            out string[] groups
            );

        void
        NetEnumUsers(
            CredentialEntry ce,
            string servername,
            int resumeHandle,
            out LUGAPI.LUGEnumStatus enumStatus
            );

        void
        NetEnumGroups(
            CredentialEntry ce,
            string servername,
            int resumeHandle,
            out LUGAPI.LUGEnumStatus enumStatus
            );

        uint
        NetDeleteUserFromGroup(
            CredentialEntry ce,
            string servername,
            string groupname,
            string username
            );

        uint
        NetGetUserInfo(
            CredentialEntry ce,
            string servername,
            string username,
            out LUGAPI.LUGInfo userInfo
            );

        bool
        NetRenameUser(
            CredentialEntry ce,
            string servername,
            string oldusername,
            string username
            );

        bool
        NetRenameGroup(
            CredentialEntry ce,
            string servername,
            string oldgroupname,
            string groupname
            );

        uint
        NetEditUserFullName(
            CredentialEntry ce,
            string servername,
            string username,
            string fullname
            );

        uint
        NetEditUserDescription(
            CredentialEntry ce,
            string servername,
            string username,
            string description
            );

        uint
        NetEditUserFlags(
            CredentialEntry ce,
            string servername,
            string username,
            uint flags
            );

        uint
        NetGetGroupMembers(
            CredentialEntry ce,
            string servername,
            string groupname,
            out string[] members
            );

        uint
        NetGetGroupInfo(
            CredentialEntry ce,
            string servername,
            string groupname,
            out string description
            );

        uint
        NetEditGroupDescription(
            CredentialEntry ce,
            string servername,
            string groupname,
            string description
            );

        uint
        NetAddGroupMember(
            CredentialEntry ce,
            string servername,
            string groupname,
            string username
            );

        uint
        NetDeleteGroupMember(
            CredentialEntry ce,
            string servername,
            string groupname,
            string username
            );

        uint
        NetInitMemory(
            );

		bool
        NetInitMemory(
            CredentialEntry ce,
            string servername
            );
    }
}