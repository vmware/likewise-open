using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.AuthUtils;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.NETAPI.Implementation
{
    public class LwLugApi : ILugApi
    {
        #region Net API Helper Functions

        private static int apiNetUserEnum(
            string serverName,
            int level,
            int filter,
            out IntPtr bufPtr,
            int prefMaxLen,
            ref int entriesRead,
            ref int totalEntries,
            ref int resumeHandle
            )
        {
            int ret = 0;
            bufPtr = IntPtr.Zero;

            ret = Interop.LwLugApi.NetUserEnum(
                serverName,
                level,
                filter,
                out bufPtr,
                prefMaxLen,
                out entriesRead,
                out totalEntries,
                out resumeHandle
                );

            //allow non-zero error code if indicates more data must be read.
            if (ret != (int)ErrorCodes.WIN32Enum.ERROR_MORE_DATA)
            {
                return ret;
            }

            //HACK: the following code should NOT be necessary.
            //This is a likely bug in Plumb
            if (totalEntries < entriesRead)
            {
                totalEntries = entriesRead;
            }
            //END HACK

            validateEntriesRead(
                "apiNetUserEnum",
                bufPtr,
                ref entriesRead,
                ref totalEntries
                );

            return ret;
        }

        private static int apiNetLocalGroupEnum(
            string serverName,
            int level,
            out IntPtr bufPtr,
            int prefMaxLen,
            out int entriesRead,
            out int totalEntries,
            ref int resumeHandle)
        {
            int ret = Interop.LwLugApi.NetLocalGroupEnum(
                serverName,
                level,
                out bufPtr,
                prefMaxLen,
                out entriesRead,
                out totalEntries,
                ref resumeHandle
                );

            validateEntriesRead(
                "apiNetLocalGroupEnum",
                bufPtr,
                ref entriesRead,
                ref totalEntries
                );

            return ret;
        }

        private static int apiNetLocalGroupGetMembers(
            string serverName,
            string localGroupName,
            int level,
            out IntPtr bufPtr,
            int prefMaxLen,
            out int entriesRead,
            out int totalEntries,
            IntPtr resumeHandle
            )
        {
            int ret = Interop.LwLugApi.NetLocalGroupGetMembers(
                serverName,
                localGroupName,
                level,
                out bufPtr,
                prefMaxLen,
                out entriesRead,
                out totalEntries,
                resumeHandle
                );

            validateEntriesRead(
                "apiNetLocalGroupGetMembers",
                bufPtr,
                ref entriesRead,
                ref totalEntries
                );

            return ret;
        }

        #endregion

        #region Net API wrappers

        public uint
        NetAddGroup(
            CredentialEntry ce,
            string servername,
            string groupname,
            string description
            )
        {
            uint result = LUGAPI.ERROR_FAILURE;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            LOCALGROUP_INFO_1 lg1 = new LOCALGROUP_INFO_1();
            lg1.name = groupname;
            lg1.comment = description;
            UInt32 parm_err = 0;

            IntPtr bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(lg1));
            IntPtr bufptr_parm_err = Marshal.AllocHGlobal(Marshal.SizeOf(parm_err));

            try
            {
                Marshal.StructureToPtr(lg1, bufptr, false);

                result = (uint)Interop.LwLugApi.NetLocalGroupAdd(servername, 1, bufptr, bufptr_parm_err);
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
            return result;
        }

        public uint
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
            uint result = LUGAPI.ERROR_FAILURE;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            USER_INFO_1 ui1 = new USER_INFO_1();
            USER_INFO_1011 ui11 = new USER_INFO_1011();
            UInt32 parm_err = 0;

            ui1.sUsername = username;
            ui1.sPassword = password;
            ui1.uiPasswordAge = 0;
            ui1.uiPriv = 1; // USER_PRIV_USER
            ui1.sHome_Dir = "";
            ui1.sComment = description;
            ui1.uiFlags = flags | LUGAPI.UF_NORMAL_ACCOUNT;
            ui1.sScript_Path = "";

            ui11.usri1011_full_name = fullname;

            IntPtr bufptr_1 = Marshal.AllocHGlobal(Marshal.SizeOf(ui1));
            IntPtr bufptr_1011 = Marshal.AllocHGlobal(Marshal.SizeOf(ui11));
            IntPtr bufptr_parm_err = Marshal.AllocHGlobal(Marshal.SizeOf(parm_err));

            try
            {
                Marshal.StructureToPtr(ui1, bufptr_1, false);
                Marshal.StructureToPtr(ui11, bufptr_1011, false);
                Marshal.StructureToPtr(parm_err, bufptr_parm_err, false);

                result = (uint)Interop.LwLugApi.NetUserAdd(servername, 1, bufptr_1, bufptr_parm_err);

                if (result != 0)
                {
                    return result;
                }

                result = (uint)Interop.LwLugApi.NetUserSetInfo(servername, username, 1011, bufptr_1011, bufptr_parm_err);

                if (result != 0)
                {
                    return result;
                }

                result = (uint)NetAddGroupMember(ce, servername, null, username);
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
                    result = LUGAPI.ERROR_FAILURE;
                }
            }
            return result;

        }

        public uint
        NetChangePassword(
            CredentialEntry ce,
            string servername,
            string username,
            string password
            )
        {
            uint result = LUGAPI.ERROR_FAILURE;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            USER_INFO_1003 ui1003 = new USER_INFO_1003();
            ui1003.usri1003_password = password;

            IntPtr bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(ui1003));

            try
            {
                Marshal.StructureToPtr(ui1003, bufptr, false);
                result = (uint)Interop.LwLugApi.NetUserSetInfo(servername, username, 1003, bufptr, IntPtr.Zero);
            }
            catch (Exception e)
            {
                result = LUGAPI.ERROR_FAILURE;
                HandleNETAPIException("ChangePassword", e);
            }
            finally
            {
                Marshal.DestroyStructure(bufptr, ui1003.GetType());
                Marshal.FreeHGlobal(bufptr);
            }

            return result;
        }

        public bool
        NetDeleteUser(
            CredentialEntry ce,
            string servername,
            string username
            )
        {
            bool result = false;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            try
            {
                if (Interop.LwLugApi.NetUserDel(servername, username) == 0)
                {
                    result = true;
                }
            }
            catch (Exception e)
            {
                HandleNETAPIException("DeleteUser", e);
            }

            return result;

        }

        public bool
        NetDeleteGroup(
            CredentialEntry ce,
            string servername,
            string username
            )
        {
            bool result = false;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            try
            {
                if (Interop.LwLugApi.NetLocalGroupDel(servername, username) == 0)
                {
                    result = true;
                }
            }
            catch (Exception e)
            {
                HandleNETAPIException("DeleteGroup", e);
            }

            return result;

        }

        public uint
        NetGetGroups(
            CredentialEntry ce,
            string servername,
            string username,
            out string[] groups
            )
        {
            uint result = LUGAPI.ERROR_FAILURE;
            groups = null;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            IntPtr bufPtr = new IntPtr(0);

            try
            {
                int entriesRead = 0;
                int totalEntries = 0;

                result = (uint)Interop.LwLugApi.NetUserGetLocalGroups(servername,
                    username, 0, 0, out bufPtr, LUGAPI.MAX_PREFERRED_LENGTH, out entriesRead, out totalEntries);

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
                                groups[i] = scrubString(groupsStruct.lgrui0_name, LUGAPI.NETAPI_MAX_GROUP_NAME_LENGTH);
                            }
                            else
                            {
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
                    Interop.LwLugApi.NetApiBufferFree(bufPtr);
                }
            }

            return result;
        }

        public void
        NetEnumUsers(
            CredentialEntry ce,
            string servername,
            int resumeHandle,
            out LUGAPI.LUGEnumStatus enumStatus
            )
        {
            enumStatus = new LUGAPI.LUGEnumStatus();
            enumStatus.initializeToNull();
            enumStatus.type = LUGAPI.LUGType.User;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return;
			}

            //Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return;
            }

            IntPtr bufPtr = IntPtr.Zero;

            try
            {
                int statusCode = 0;
                int localResumeHandle = resumeHandle;
                int totalEntries = 0;
                int entriesRead = 0;

                statusCode = apiNetUserEnum(
                    servername,
                    20,
                    2,
                    out bufPtr,
                    LUGAPI.MAX_PREFERRED_LENGTH,
                    ref entriesRead,
                    ref totalEntries,
                    ref localResumeHandle
                    );

                enumStatus.entriesRead = entriesRead;
                enumStatus.totalEntries = totalEntries;
                enumStatus.resumeHandle = localResumeHandle;

                if (entriesRead > 0)
                {
                    USER_INFO_20[] users = new USER_INFO_20[entriesRead];
                    IntPtr iter = bufPtr;

                    enumStatus.entries = new List<string[]>();

                    for (int i = 0; i < entriesRead; i++)
                    {
                        users[i] = (USER_INFO_20)Marshal.PtrToStructure(iter, typeof(USER_INFO_20));
                        iter = (IntPtr)((int)iter + Marshal.SizeOf(typeof(USER_INFO_20)));

                        string[] userInfo ={"",(users[i].usri20_flags & LUGAPI.UF_ACCOUNTDISABLE) != 0 ? LUGAPI.Disabled : "",users[i].usri20_name,
                                users[i].usri20_full_name,users[i].usri20_comment };



                        enumStatus.entries.Add(userInfo);
                    }
                    if (statusCode == (int)ErrorCodes.WIN32Enum.ERROR_MORE_DATA)
                    {
                        enumStatus.moreEntries = true;
                    }
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
                    Interop.LwLugApi.NetApiBufferFree(bufPtr);
                }
            }

            return;
        }

        public void
        NetEnumGroups(
            CredentialEntry ce,
            string servername,
            int resumeHandle,
            out LUGAPI.LUGEnumStatus enumStatus
            )
        {
            enumStatus = new LUGAPI.LUGEnumStatus();
            enumStatus.initializeToNull();
            enumStatus.type = LUGAPI.LUGType.Group;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return;
			}

            IntPtr bufPtr = IntPtr.Zero;

            try
            {

                int statusCode = 0;
                int entriesRead = 0;
                int totalEntries = 0;
                int localResumeHandle = resumeHandle;

                statusCode = apiNetLocalGroupEnum(
                    servername,
                    1,
                    out bufPtr,
                    LUGAPI.MAX_PREFERRED_LENGTH,
                    out entriesRead,
                    out totalEntries,
                    ref localResumeHandle
                    );

                enumStatus.entriesRead = entriesRead;
                enumStatus.totalEntries = totalEntries;
                enumStatus.resumeHandle = localResumeHandle;

                if (entriesRead > 0)
                {
                    LOCALGROUP_INFO_1[] group = new LOCALGROUP_INFO_1[entriesRead];

                    IntPtr iter = bufPtr;

                    enumStatus.entries = new List<string[]>();

                    for (int i = 0; i < entriesRead; i++)
                    {
                        group[i] = (LOCALGROUP_INFO_1)Marshal.PtrToStructure(iter, typeof(LOCALGROUP_INFO_1));
                        iter = (IntPtr)((int)iter + Marshal.SizeOf(typeof(LOCALGROUP_INFO_1)));

                        string[] groupInfo ={ "", group[i].name, group[i].comment };
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
                    Interop.LwLugApi.NetApiBufferFree(bufPtr);
                }
            }
        }

        public uint
        NetDeleteUserFromGroup(
            CredentialEntry ce,
            string servername,
            string groupname,
            string username
            )
        {

            uint result = LUGAPI.ERROR_FAILURE;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            LOCALGROUP_MEMBERS_INFO_3 lgmi3 = new LOCALGROUP_MEMBERS_INFO_3();
            lgmi3.lgrmi3_domainandname = username;

            IntPtr bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(lgmi3));
            try
            {
                Marshal.StructureToPtr(lgmi3, bufptr, false);
                int ret = Interop.LwLugApi.NetLocalGroupDelMembers(servername, groupname, 3, bufptr, 1);
                if (ret == 0)
                {
                    result = LUGAPI.ERROR_SUCCESS;
                }
            }
            catch (Exception e)
            {
                HandleNETAPIException("DeleteUserFromGroup", e);
                result = 0;
            }
            finally
            {
                Marshal.DestroyStructure(bufptr, lgmi3.GetType());
                Marshal.FreeHGlobal(bufptr);
            }

            return result;

        }


        public uint
        NetGetUserInfo(
            CredentialEntry ce,
            string servername,
            string username,
            out LUGAPI.LUGInfo userInfo
            // we need an out for userInfo which use to be an _arg (I believe)
            )
        {
            IntPtr bufPtr = IntPtr.Zero;

            uint result = LUGAPI.ERROR_FAILURE;

            userInfo = new LUGAPI.LUGInfo();
            userInfo.initializeToNull();

            userInfo.fullname = "";
            userInfo.description = "";
            userInfo.flags = 0;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            try
            {
                result = (uint)Interop.LwLugApi.NetUserGetInfo(servername, username, 20, out bufPtr);

                USER_INFO_20 userinfo_20 = new USER_INFO_20();
                userinfo_20 = (USER_INFO_20)Marshal.PtrToStructure(bufPtr, typeof(USER_INFO_20));
                userInfo.fullname = userinfo_20.usri20_full_name;
                userInfo.description = userinfo_20.usri20_comment;
                userInfo.flags = userinfo_20.usri20_flags;
            }
            catch (Exception e)
            {
                HandleNETAPIException("GetUserInfo", e);
            }
            finally
            {
                if (bufPtr != IntPtr.Zero)
                {
                    Interop.LwLugApi.NetApiBufferFree(bufPtr);
                }
            }
            return result;

        }


        public bool
        NetRenameUser(
            CredentialEntry ce,
            string servername,
            string oldusername,
            string username
            )
        {
            bool result = false;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            USER_INFO_0 usrinfo_0 = new USER_INFO_0();
            usrinfo_0.usri0_name = username;

            IntPtr bufptr = IntPtr.Zero;
            bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(usrinfo_0));

            try
            {
                Marshal.StructureToPtr(usrinfo_0, bufptr, false);

                result = !Convert.ToBoolean(Interop.LwLugApi.NetUserSetInfo(servername, oldusername, 0, bufptr, IntPtr.Zero));
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

            return result;

        }


        public bool
        NetRenameGroup(
            CredentialEntry ce,
            string servername,
            string oldgroupname,
            string groupname
            )
        {
            bool result = false;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            LOCALGROUP_INFO_0 localgrouinfo_0 = new LOCALGROUP_INFO_0();
            localgrouinfo_0.lgrpi0_name = groupname;

            IntPtr bufptr = IntPtr.Zero;
            bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(localgrouinfo_0));

            try
            {
                Marshal.StructureToPtr(localgrouinfo_0, bufptr, false);

                if (Interop.LwLugApi.NetLocalGroupSetInfo(servername, oldgroupname, 0, bufptr, IntPtr.Zero) == 0)
                {
                    result = true;
                }
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

            return result;

        }


        public uint
        NetEditUserFullName(
            CredentialEntry ce,
            string servername,
            string username,
            string fullname
            )
        {
            uint result = LUGAPI.ERROR_FAILURE;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            USER_INFO_1011 userinfo_1011 = new USER_INFO_1011();
            userinfo_1011.usri1011_full_name = fullname;

            IntPtr bufptr1011 = IntPtr.Zero;
            bufptr1011 = Marshal.AllocHGlobal(Marshal.SizeOf(userinfo_1011));

            try
            {
                Marshal.StructureToPtr(userinfo_1011, bufptr1011, false);

                result = (uint)Interop.LwLugApi.NetUserSetInfo(servername, username, 1011, bufptr1011, IntPtr.Zero);
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

            return result;
        }


        public uint
        NetEditUserDescription(
            CredentialEntry ce,
            string servername,
            string username,
            string description
            )
        {
            uint result = LUGAPI.ERROR_FAILURE;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            LOCALGROUP_INFO_1 groupinfo_1 = new LOCALGROUP_INFO_1();
            groupinfo_1.comment = description;

            IntPtr bufptr = IntPtr.Zero;
            bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(groupinfo_1));

            try
            {
                Marshal.StructureToPtr(groupinfo_1, bufptr, false);

                // groupname was a part of the NETAPIArguments struct and is not used here... clear it for now
                //apiNetLocalGroupSetInfo(servername, groupname, 1, bufptr, IntPtr.Zero);
                result = (uint)Interop.LwLugApi.NetLocalGroupSetInfo(servername, null, 1, bufptr, IntPtr.Zero);
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

            return result;
        }



        public uint
        NetEditUserFlags(
            CredentialEntry ce,
            string servername,
            string username,
            uint flags
            )
        {
            uint result = LUGAPI.ERROR_FAILURE;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            USER_INFO_1008 userinfo_1008 = new USER_INFO_1008();
            userinfo_1008.usri1008_flags = flags;

            IntPtr bufptr1008 = IntPtr.Zero;
            bufptr1008 = Marshal.AllocHGlobal(Marshal.SizeOf(userinfo_1008));

            try
            {
                Marshal.StructureToPtr(userinfo_1008, bufptr1008, false);
                result = (uint)Interop.LwLugApi.NetUserSetInfo(servername, username, 1008, bufptr1008, IntPtr.Zero);
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

            return result;
        }


        public uint
        NetGetGroupMembers(
            CredentialEntry ce,
            string servername,
            string groupname,
            out string[] members
            )
        {
            uint result = LUGAPI.ERROR_FAILURE;

            members = null;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }


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

                result = (uint)apiNetLocalGroupGetMembers(servername, groupname, 3, out bufPtr,
                    LUGAPI.MAX_PREFERRED_LENGTH, out entriesRead, out totalEntries, resumeHandleStar);

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
                            members[i] = memberStruct.lgrmi3_domainandname;
                            //scrubString(memberStruct.lgrmi3_domainandname,
                            //NETAPI_MAX_DOMAIN_NAME_LENGTH + 1 + NETAPI_MAX_USER_NAME_LENGTH);
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
                //apiNetApiBufferFree(bufPtrStar);
                //apiNetApiBufferFree(resumeHandleStar);

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
                for (int i = 0; i < j; i++)
                {
                    members[i] = scratch[i];
                }

            }
            return result;

        }

        public uint
        NetGetGroupInfo(
            CredentialEntry ce,
            string servername,
            string groupname,
            out string description
            )
        {
            uint result = LUGAPI.ERROR_FAILURE;

            description = null;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            IntPtr bufPtr = IntPtr.Zero;
            try
            {
                result = (uint)Interop.LwLugApi.NetLocalGroupGetInfo(servername, groupname, 1, out bufPtr);

                if (bufPtr == IntPtr.Zero)
                {
                    return result;
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
                    Interop.LwLugApi.NetApiBufferFree(bufPtr);
                }
            }

            return result;
        }


        public uint
        NetEditGroupDescription(
            CredentialEntry ce,
            string servername,
            string groupname,
            string description
            )
        {
            uint result = LUGAPI.ERROR_FAILURE;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            USER_INFO_1007 userinfo_1007 = new USER_INFO_1007();
            userinfo_1007.usri1007_comment = description;

            IntPtr bufptr1007 = IntPtr.Zero;
            bufptr1007 = Marshal.AllocHGlobal(Marshal.SizeOf(userinfo_1007));

            try
            {
                Marshal.StructureToPtr(userinfo_1007, bufptr1007, false);

                // username was a part of NETAPIArguments which is not set... clear it for now.
                //apiNetUserSetInfo(servername, username, 1007, bufptr1007, IntPtr.Zero);
                result = (uint)Interop.LwLugApi.NetUserSetInfo(servername, null, 1007, bufptr1007, IntPtr.Zero);
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

            return result;

        }


        public uint
        NetAddGroupMember(
            CredentialEntry ce,
            string servername,
            string groupname,
            string username
            )
        {
            uint result = LUGAPI.ERROR_FAILURE;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }
            else
            {
                LOCALGROUP_MEMBERS_INFO_3 lgmi3 = new LOCALGROUP_MEMBERS_INFO_3();
                lgmi3.lgrmi3_domainandname = username;

                IntPtr bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(lgmi3));
                try
                {
                    Marshal.StructureToPtr(lgmi3, bufptr, false);
                    result = (uint)Interop.LwLugApi.NetLocalGroupAddMembers(servername, groupname, 3, bufptr, 1);
                }
                catch (Exception e)
                {
                    result = 0;
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
                        result = 0;
                    }
                }
            }

            return result;

        }


        public uint
        NetDeleteGroupMember(
            CredentialEntry ce,
            string servername,
            string groupname,
            string username
            )
        {
            uint result = LUGAPI.ERROR_FAILURE;

			if (NetInitMemory() != LUGAPI.ERROR_SUCCESS)
			{
				return result;
			}

            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return result;
            }

            LOCALGROUP_MEMBERS_INFO_3 lgmi_3 = new LOCALGROUP_MEMBERS_INFO_3();
            lgmi_3.lgrmi3_domainandname = username;

            IntPtr bufptr = IntPtr.Zero;
            bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(lgmi_3));

            try
            {
                Marshal.StructureToPtr(lgmi_3, bufptr, false);

                result = (uint)Interop.LwLugApi.NetLocalGroupDelMembers(servername, groupname, 3, bufptr, 1);
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

            return result;
        }

        public uint
        NetInitMemory()
        {
            uint result = 0;

            if (NetApiInitDone)
            {
                result = LUGAPI.ERROR_SUCCESS;
            }
            else
            {
                result = (uint)Interop.LwLugApi.NetInitMemory();
                if (result != 0)
                {
                    NetApiInitDone = false;
                }
                else
                {
                    NetApiInitDone = true;
                }
            }

            return result;
        }

        public bool
        NetInitMemory(
            CredentialEntry ce,
            string servername
            )
        {
            // Ensure we have creds
            if (!Session.EnsureNullSession(servername, ce))
            {
                return false;
            }

			return true;
        }

		#endregion

        #region Helper Functions

        private static bool validateEntriesRead(
            string location,
            IntPtr bufPtr,
            ref int entriesRead,
            ref int totalEntries
            )
        {
            if (totalEntries < entriesRead)
            {
                entriesRead = totalEntries;
            }

            if (totalEntries == 0)
            {
                return false;
            }

            else if (bufPtr == IntPtr.Zero)
            {
                entriesRead = 0;
                return false;
            }

            return true;
        }

        private static string scrubString(
            string input,
            int maxLength
            )
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

            foreach (char c in inputArr)
            {
                if (idxOut >= maxLength)
                {
                    continue;
                }

                //HACK: this is filter out presumed invalid non-English usernames resulting from plumb bug#
                //TODO: remove the check for (int)c > 127; this HACK prevents internationalization.
                if (Char.IsLetterOrDigit(c) && (int)c <= 127)
                {
                    outputArr[idxOut] = c;
                    idxOut++;
                }
            }

            return new string(outputArr, 0, idxOut);

        }

        #endregion

        private static void HandleNETAPIException(string methodName, Exception e)
        {
            if (e is NETAPIException)
            {
                NETAPIException netAPIEx = (NETAPIException)e;
                //Logger.LogException(String.Format(
                //    "{0} failed with NetAPI error code {1}", methodName, netAPIEx.errorCode), e);
            }
            //else
            //{
            //    Logger.LogException(methodName, e);
            //}
        }

        public delegate void LUGEnumStatusDelegate(LUGAPI.LUGEnumStatus status);

        public class NETAPIException : Exception
        {
            public int errorCode = 0;

            public NETAPIException(string message, int error)
                :
            base(message)
            {
                errorCode = error;
            }
        }

        public static bool bIsNetApiInitDone = false;

        public static bool NetApiInitDone
        {
            set
            {
                bIsNetApiInitDone = value;
            }
            get
            {
                return bIsNetApiInitDone;
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

   }
}