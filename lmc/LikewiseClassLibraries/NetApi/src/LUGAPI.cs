using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.NETAPI
{
    public class LUGAPI
    {
        #region Net API Imports

        private const string netAPIDllPath = "netapi32.dll";
        private const string mprDllPath = "mpr.dll";

        [DllImport(netAPIDllPath)]
        private extern static int NetUserDel(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string userName
            );

        [DllImport(netAPIDllPath)]
        private extern static int NetUserGetLocalGroups(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string userName,
            int level,
            uint flags,
            out IntPtr bufPtr,
            int prefMaxLen,
            out int entriesRead,
            out int totalEntries
            );

        [DllImport(netAPIDllPath)]
        private extern static int NetUserSetInfo(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string userName,
            int level,
            IntPtr buf,
            IntPtr parmErr
            );

        [DllImport(netAPIDllPath)]
        private extern static int NetUserGetInfo(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string userName,
            int level,
            out IntPtr buf
            );

        [DllImport(netAPIDllPath)]
        private extern static int NetApiBufferFree(
            IntPtr buffer
            );

        [DllImport(netAPIDllPath)]
        private extern static int NetUserAdd(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            UInt32 level,
            IntPtr userInfo,
            IntPtr parmErr
            );

        [DllImport(netAPIDllPath, CharSet = CharSet.Unicode)]
        private extern static int NetUserEnum(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            int level,
            int filter,
            out IntPtr bufPtr,
            int prefMaxLen,
            out int entriesRead,
            out int totalEntries,
            out int resumeHandle
            );

        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupAdd(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            UInt32 level,
            IntPtr groupInfo,
            IntPtr parmErr
            );

        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupDel(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string groupName
            );

        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupEnum(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            int level,
            out IntPtr bufPtr,
            int prefMaxLen,
            out int entriesRead,
            out int totalEntries,
            ref int resumeHandle
            );

        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupAddMembers(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string groupName,
            int level,
            IntPtr buf,
            int totalEntries
            );

        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupDelMembers(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string groupName,
            int level,
            IntPtr buf,
            int totalEntries
            );

        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupSetInfo(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string groupName,
            int level,
            IntPtr buf,
            IntPtr parmErr
            );

        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupGetInfo(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string groupName,
            int level,
            out IntPtr buf
            );

        [DllImport(netAPIDllPath)]
        private extern static int NetLocalGroupGetMembers(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string localGroupName,
            int level,
            out IntPtr bufPtr,
            int prefMaxLen,
            out int entriesRead,
            out int totalEntries,
            IntPtr resumeHandle
            );

        [DllImport(mprDllPath, SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern int WNetAddConnection2(
            IntPtr netResource,
            [MarshalAs(UnmanagedType.LPWStr)] string sPassword,
            [MarshalAs(UnmanagedType.LPWStr)] string sUsername,
            uint dwFlags
            );

        [DllImport(mprDllPath, SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern int WNetCancelConnection2(
            [MarshalAs(UnmanagedType.LPWStr)] string sName,
            uint dwFlags,
            bool bForce
            );

        // This doesn't exist in the windows netapi32.dll, but is needed for
        // libnetapi.so
        [DllImport(netAPIDllPath)]
        private extern static int NetInitMemory();

        #endregion

        #region Net API Helper Functions

        private static uint apiNetUserEnum(
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
            uint result;
            bufPtr = IntPtr.Zero;

            result = (uint)NetUserEnum(
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
            if (result != (uint)ErrorCodes.WIN32Enum.ERROR_MORE_DATA)
            {
                return result;
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

            return result;
        }

        private static uint apiNetLocalGroupEnum(
            string serverName,
            int level,
            out IntPtr bufPtr,
            int prefMaxLen,
            out int entriesRead,
            out int totalEntries,
            ref int resumeHandle)
        {
            uint result = (uint)NetLocalGroupEnum(
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

            return result;
        }

        private static uint apiNetLocalGroupGetMembers(
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
            uint result = (uint)NetLocalGroupGetMembers(
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

            return result;
        }

        #endregion

        #region MPR API wrappers

        public static uint
        NetAddConnection(
            string sServer,
            string sUsername,
            string sPassword
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(sUsername))
            {
                sUsername = null;
            }

            if (String.IsNullOrEmpty(sPassword))
            {
                sPassword = null;
            }

            // set up a NETRESOURCE structure
            NETRESOURCE nr = new NETRESOURCE();
            nr.dwScope = 0;
            nr.dwType = 0;
            nr.dwDisplayType = 0;
            nr.dwUsage = 0;
            nr.LocalName = null;
            nr.RemoteName = @"\\" + sServer + @"\IPC$";
            nr.Comment = null;
            nr.Provider = null;

            IntPtr bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(nr));

            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                Marshal.StructureToPtr(nr, bufptr, false);

                result = (uint)WNetAddConnection2(
                    bufptr,
                    sPassword,
                    sUsername,
                    0);

                // another session is preventing us from connecting... close it and
                // retry
                if (result == (uint)ErrorCodes.WIN32Enum.ERROR_SESSION_CREDENTIAL_CONFLICT)
                {
                    if (NetCancelConnection(sServer) == 0)
                    {
                        result = (uint)WNetAddConnection2(
                            bufptr,
                            sPassword,
                            sUsername,
                            0);
                    }
                }
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                Marshal.DestroyStructure(bufptr, nr.GetType());
                Marshal.FreeHGlobal(bufptr);
            }

            return result;
        }

        public static uint
        NetCancelConnection(
            string sServer
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                result = (uint)WNetCancelConnection2(
                    @"\\" + sServer + @"\IPC$",
                    1,
                    true);
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
            }

            return result;
        }

        #endregion

        #region Net API wrappers

        public static uint
        NetAddGroup(
            string servername,
            string groupname,
            string description
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }

            if (String.IsNullOrEmpty(groupname))
            {
                groupname = null;
            }

            if (String.IsNullOrEmpty(description))
            {
                description = null;
            }

            LOCALGROUP_INFO_1 lg1 = new LOCALGROUP_INFO_1();
            lg1.name = groupname;
            lg1.comment = description;
            UInt32 parm_err = 0;

            IntPtr bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(lg1));
            IntPtr bufptr_parm_err = Marshal.AllocHGlobal(Marshal.SizeOf(parm_err));

            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                Marshal.StructureToPtr(lg1, bufptr, false);

                result = (uint)NetLocalGroupAdd(servername, 1, bufptr, bufptr_parm_err);
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                Marshal.DestroyStructure(bufptr, lg1.GetType());
                Marshal.FreeHGlobal(bufptr);
                Marshal.FreeHGlobal(bufptr_parm_err);

            }
            return result;
        }

        public static uint
        NetAddUser(
            string servername,
            string username,
            string password,
            string fullname,
            string description,
            uint flags
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(username))
            {
                username = null;
            }
            if (String.IsNullOrEmpty(password))
            {
                password = null;
            }
            if (String.IsNullOrEmpty(fullname))
            {
                fullname = null;
            }
            if (String.IsNullOrEmpty(description))
            {
                description = null;
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
            ui1.uiFlags = flags | UF_NORMAL_ACCOUNT;
            ui1.sScript_Path = "";

            ui11.usri1011_full_name = fullname;

            IntPtr bufptr_1 = Marshal.AllocHGlobal(Marshal.SizeOf(ui1));
            IntPtr bufptr_1011 = Marshal.AllocHGlobal(Marshal.SizeOf(ui11));
            IntPtr bufptr_parm_err = Marshal.AllocHGlobal(Marshal.SizeOf(parm_err));

            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                Marshal.StructureToPtr(ui1, bufptr_1, false);
                Marshal.StructureToPtr(ui11, bufptr_1011, false);
                Marshal.StructureToPtr(parm_err, bufptr_parm_err, false);

                result = (uint)NetUserAdd(servername, 1, bufptr_1, bufptr_parm_err);

                if (result != 0)
                {
                    return result;
                }

                if (fullname != null)
                {
                    result = (uint)NetUserSetInfo(servername, username, 1011, bufptr_1011, bufptr_parm_err);

                    if (result != 0)
                    {
                        return result;
                    }
                }
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
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
                    result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
                }
            }
            return result;

        }

        public static uint
        NetChangePassword(
            string servername,
            string username,
            string password
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(username))
            {
                username = null;
            }
            if (String.IsNullOrEmpty(password))
            {
                password = null;
            }

            USER_INFO_1003 ui1003 = new USER_INFO_1003();
            ui1003.usri1003_password = password;

            IntPtr bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(ui1003));

            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                Marshal.StructureToPtr(ui1003, bufptr, false);
                result = (uint)NetUserSetInfo(servername, username, 1003, bufptr, IntPtr.Zero);
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                Marshal.DestroyStructure(bufptr, ui1003.GetType());
                Marshal.FreeHGlobal(bufptr);
            }

            return result;
        }

        public static uint
        NetDeleteUser(
            string servername,
            string username
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(username))
            {
                username = null;
            }

            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                result = (uint)NetUserDel(servername, username);

            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }

            return result;

        }

        public static uint
        NetDeleteGroup(
            string servername,
            string username
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(username))
            {
                username = null;
            }

            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                result = (uint)NetLocalGroupDel(servername, username);
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }

            return result;

        }

        public static uint
        NetGetGroups(
            string servername,
            string username,
            out string[] groups
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(username))
            {
                username = null;
            }

            groups = null;

            IntPtr bufPtr = new IntPtr(0);

            try
            {
                int entriesRead = 0;
                int totalEntries = 0;

                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                result = (uint)NetUserGetLocalGroups(servername,
                    username, 0, 0, out bufPtr, MAX_PREFERRED_LENGTH, out entriesRead, out totalEntries);

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
                            }
                            else
                            {
                            }

                            iter = (IntPtr)((long)iter + Marshal.SizeOf(typeof(LOCALGROUP_USERS_INFO_0)));
                        }
                    }
                }
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                if (bufPtr != IntPtr.Zero)
                {
                    NetApiBufferFree(bufPtr);
                }
            }

            return result;
        }

        public static uint
        NetEnumUsers(
            string servername,
            int resumeHandle,
            out LUGEnumStatus enumStatus
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }

            enumStatus = new LUGEnumStatus();
            enumStatus.initializeToNull();
            enumStatus.type = LUGType.User;

            IntPtr bufPtr = IntPtr.Zero;

            try
            {
                int localResumeHandle = resumeHandle;
                int totalEntries = 0;
                int entriesRead = 0;

                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                result = apiNetUserEnum(
                    servername,
                    20,
                    2,
                    out bufPtr,
                    MAX_PREFERRED_LENGTH,
                    ref entriesRead,
                    ref totalEntries,
                    ref localResumeHandle
                    );

                if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                {
                    return result;
                }

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
                        iter = (IntPtr)((long)iter + Marshal.SizeOf(typeof(USER_INFO_20)));

                        string[] userInfo ={"",(users[i].usri20_flags & UF_ACCOUNTDISABLE) != 0 ? DISABLED : "",users[i].usri20_name,
                                users[i].usri20_full_name,users[i].usri20_comment };



                        enumStatus.entries.Add(userInfo);
                    }
                    if (result == (int)ErrorCodes.WIN32Enum.ERROR_MORE_DATA)
                    {
                        enumStatus.moreEntries = true;
                    }
                }
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                if (bufPtr != IntPtr.Zero)
                {
                    NetApiBufferFree(bufPtr);
                }
            }

            return result;
        }

        public static uint
        NetEnumGroups(
            string servername,
            int resumeHandle,
            out LUGEnumStatus enumStatus
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }

            enumStatus = new LUGEnumStatus();
            enumStatus.initializeToNull();
            enumStatus.type = LUGType.Group;

            IntPtr bufPtr = IntPtr.Zero;

            try
            {
                int entriesRead = 0;
                int totalEntries = 0;
                int localResumeHandle = resumeHandle;

                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                result = apiNetLocalGroupEnum(
                    servername,
                    1,
                    out bufPtr,
                    MAX_PREFERRED_LENGTH,
                    out entriesRead,
                    out totalEntries,
                    ref localResumeHandle
                    );

                if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                {
                    return result;
                }

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
                        iter = (IntPtr)((long)iter + Marshal.SizeOf(typeof(LOCALGROUP_INFO_1)));

                        string[] groupInfo ={ "", group[i].name, group[i].comment };
                        enumStatus.entries.Add(groupInfo);
                    }
                }
                if (result == (int)ErrorCodes.WIN32Enum.ERROR_MORE_DATA)
                {
                    enumStatus.moreEntries = true;
                }

            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                if (bufPtr != IntPtr.Zero)
                {
                    NetApiBufferFree(bufPtr);
                }
            }

            return result;
        }

        public static uint
        NetDeleteUserFromGroup(
            string servername,
            string groupname,
            string username
            )
        {

            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(groupname))
            {
                groupname = null;
            }
            if (String.IsNullOrEmpty(username))
            {
                username = null;
            }

            LOCALGROUP_MEMBERS_INFO_3 lgmi3 = new LOCALGROUP_MEMBERS_INFO_3();
            lgmi3.lgrmi3_domainandname = username;

            IntPtr bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(lgmi3));
            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                Marshal.StructureToPtr(lgmi3, bufptr, false);
                result = (uint)NetLocalGroupDelMembers(servername, groupname, 3, bufptr, 1);
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                Marshal.DestroyStructure(bufptr, lgmi3.GetType());
                Marshal.FreeHGlobal(bufptr);
            }

            return result;
        }


        public static uint
        NetGetUserInfo(
            string servername,
            string username,
            out LUGInfo userInfo
            )
        {
            IntPtr bufPtr = IntPtr.Zero;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(username))
            {
                username = null;
            }

            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            userInfo = new LUGInfo();
            userInfo.initializeToNull();

            userInfo.fullname = "";
            userInfo.description = "";
            userInfo.flags = 0;

            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                result = (uint)NetUserGetInfo(servername, username, 20, out bufPtr);

                if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                {
                    return result;
                }

                USER_INFO_20 userinfo_20 = new USER_INFO_20();
                userinfo_20 = (USER_INFO_20)Marshal.PtrToStructure(bufPtr, typeof(USER_INFO_20));
                userInfo.fullname = userinfo_20.usri20_full_name;
                userInfo.description = userinfo_20.usri20_comment;
                userInfo.flags = userinfo_20.usri20_flags;
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                if (bufPtr != IntPtr.Zero)
                {
                    NetApiBufferFree(bufPtr);
                }
            }
            return result;
        }


        public static uint
        NetRenameUser(
            string servername,
            string oldusername,
            string username
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                 servername = null;
            }
            if (String.IsNullOrEmpty(oldusername))
            {
                 oldusername = null;
            }
            if (String.IsNullOrEmpty(username))
            {
                 username = null;
            }

            USER_INFO_0 usrinfo_0 = new USER_INFO_0();
            usrinfo_0.usri0_name = username;

            IntPtr bufptr = IntPtr.Zero;
            bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(usrinfo_0));

            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                Marshal.StructureToPtr(usrinfo_0, bufptr, false);

                result = (uint)NetUserSetInfo(servername, oldusername, 0, bufptr, IntPtr.Zero);
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                Marshal.DestroyStructure(bufptr, usrinfo_0.GetType());
                Marshal.FreeHGlobal(bufptr);
            }

            return result;

        }


        public static uint
        NetRenameGroup(
            string servername,
            string oldgroupname,
            string groupname
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(oldgroupname))
            {
                oldgroupname = null;
            }
            if (String.IsNullOrEmpty(groupname))
            {
                groupname = null;
            }

            LOCALGROUP_INFO_0 localgrouinfo_0 = new LOCALGROUP_INFO_0();
            localgrouinfo_0.lgrpi0_name = groupname;

            IntPtr bufptr = IntPtr.Zero;
            bufptr = Marshal.AllocHGlobal(
                Marshal.SizeOf(localgrouinfo_0));

            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                Marshal.StructureToPtr(localgrouinfo_0, bufptr, false);

                result = (uint)NetLocalGroupSetInfo(
                    servername,
                    oldgroupname,
                    0,
                    bufptr,
                    IntPtr.Zero);
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                Marshal.DestroyStructure(
                    bufptr,
                    localgrouinfo_0.GetType());

                Marshal.FreeHGlobal(bufptr);
            }

            return result;
        }


        public static uint
        NetEditUserFullName(
            string servername,
            string username,
            string fullname
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(username))
            {
                username = null;
            }
            if (String.IsNullOrEmpty(fullname))
            {
                fullname = null;
            }

            USER_INFO_1011 userinfo_1011 = new USER_INFO_1011();
            userinfo_1011.usri1011_full_name = fullname;

            IntPtr bufptr1011 = IntPtr.Zero;
            bufptr1011 = Marshal.AllocHGlobal(Marshal.SizeOf(userinfo_1011));

            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                Marshal.StructureToPtr(userinfo_1011, bufptr1011, false);

                result = (uint)NetUserSetInfo(servername, username, 1011, bufptr1011, IntPtr.Zero);
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                Marshal.DestroyStructure(bufptr1011, userinfo_1011.GetType());
                Marshal.FreeHGlobal(bufptr1011);
            }

            return result;
        }


        public static uint
        NetEditUserDescription(
            string servername,
            string username,
            string description
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(username))
            {
                username = null;
            }
            if (String.IsNullOrEmpty(description))
            {
                description = null;
            }

            USER_INFO_1007 userinfo_1007 = new USER_INFO_1007();
            userinfo_1007.usri1007_comment = description;

            IntPtr bufptr = IntPtr.Zero;
            bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(userinfo_1007));

            try
            {
                Marshal.StructureToPtr(userinfo_1007, bufptr, false);

                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                result = (uint)NetUserSetInfo(servername, username, 1007, bufptr, IntPtr.Zero);
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                Marshal.DestroyStructure(bufptr, userinfo_1007.GetType());
                Marshal.FreeHGlobal(bufptr);
            }

            return result;
        }



        public static uint
        NetEditUserFlags(
            string servername,
            string username,
            uint flags
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(username))
            {
                username = null;
            }

            USER_INFO_1008 userinfo_1008 = new USER_INFO_1008();
            userinfo_1008.usri1008_flags = flags;

            IntPtr bufptr1008 = IntPtr.Zero;
            bufptr1008 = Marshal.AllocHGlobal(Marshal.SizeOf(userinfo_1008));

            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                Marshal.StructureToPtr(userinfo_1008, bufptr1008, false);
                result = (uint)NetUserSetInfo(servername, username, 1008, bufptr1008, IntPtr.Zero);
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                Marshal.DestroyStructure(bufptr1008, userinfo_1008.GetType());
                Marshal.FreeHGlobal(bufptr1008);
            }

            return result;
        }


        public static uint
        NetGetGroupMembers(
            string servername,
            string groupname,
            out string [] members
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(groupname))
            {
                groupname = null;
            }

            members = null;

            IntPtr bufPtr = IntPtr.Zero;
            IntPtr bufPtrStar = Marshal.AllocHGlobal(Marshal.SizeOf(bufPtr));

            IntPtr resumeHandle = IntPtr.Zero;
            IntPtr resumeHandleStar = Marshal.AllocHGlobal(Marshal.SizeOf(resumeHandle));

            try
            {
                int entriesRead = 0;
                int totalEntries = 0;

                Marshal.StructureToPtr(
                    resumeHandle,
                    resumeHandleStar,
                    false);

                Marshal.StructureToPtr(
                    bufPtr,
                    bufPtrStar,
                    false);

                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                result = (uint)apiNetLocalGroupGetMembers(
                    servername,
                    groupname,
                    3,
                    out bufPtr,
                    MAX_PREFERRED_LENGTH,
                    out entriesRead,
                    out totalEntries,
                    resumeHandleStar);

                if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                {
                    return result;
                }

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
                        }
                        iter = (IntPtr)((long)iter + Marshal.SizeOf(typeof(LOCALGROUP_MEMBERS_INFO_3)));
                    }
                }
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
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

        public static uint
        NetGetGroupInfo(
            string servername,
            string groupname,
            out string description
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(groupname))
            {
                groupname = null;
            }

            description = null;

            IntPtr bufPtr = IntPtr.Zero;
            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                result = (uint)NetLocalGroupGetInfo(servername, groupname, 1, out bufPtr);

                if (bufPtr == IntPtr.Zero)
                {
                    return result;
                }

                LOCALGROUP_INFO_1 localrounpinfo_1 = new LOCALGROUP_INFO_1();
                localrounpinfo_1 = (LOCALGROUP_INFO_1)Marshal.PtrToStructure(bufPtr, typeof(LOCALGROUP_INFO_1));
                description = localrounpinfo_1.comment;
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                if (bufPtr != IntPtr.Zero)
                {
                    NetApiBufferFree(bufPtr);
                }
            }

            return result;
        }


        public static uint
        NetEditGroupDescription(
            string servername,
            string groupname,
            string description
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(groupname))
            {
                groupname = null;
            }
            if (String.IsNullOrEmpty(description))
            {
                description = null;
            }

            LOCALGROUP_INFO_1 groupinfo_1 = new LOCALGROUP_INFO_1();
            groupinfo_1.comment = description;

            IntPtr bufptr = IntPtr.Zero;
            bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(groupinfo_1));

            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                Marshal.StructureToPtr(groupinfo_1, bufptr, false);

                result = (uint)NetLocalGroupSetInfo(servername, groupname, 1, bufptr, IntPtr.Zero);
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }
            finally
            {
                Marshal.DestroyStructure(bufptr, groupinfo_1.GetType());
                Marshal.FreeHGlobal(bufptr);
            }

            return result;

        }


        public static uint
        NetAddGroupMember(
            string servername,
            string groupname,
            string username
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(groupname))
            {
                groupname = null;
            }
            if (String.IsNullOrEmpty(username))
            {
                username = null;
            }

            LOCALGROUP_MEMBERS_INFO_3 lgmi3 = new LOCALGROUP_MEMBERS_INFO_3();
            lgmi3.lgrmi3_domainandname = username;

            IntPtr bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(lgmi3));
            try
            {
                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                Marshal.StructureToPtr(lgmi3, bufptr, false);
                result = (uint)NetLocalGroupAddMembers(servername, groupname, 3, bufptr, 1);
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
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
                    result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
                }
            }

            return result;

        }


        public static uint
        NetDeleteGroupMember(
            string servername,
            string groupname,
            string username
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            if (String.IsNullOrEmpty(servername))
            {
                servername = null;
            }
            if (String.IsNullOrEmpty(groupname))
            {
                groupname = null;
            }
            if (String.IsNullOrEmpty(username))
            {
                username = null;
            }

            LOCALGROUP_MEMBERS_INFO_3 lgmi_3 = new LOCALGROUP_MEMBERS_INFO_3();
            lgmi_3.lgrmi3_domainandname = username;

            IntPtr bufptr = IntPtr.Zero;
            bufptr = Marshal.AllocHGlobal(Marshal.SizeOf(lgmi_3));

            try
            {
                Marshal.StructureToPtr(lgmi_3, bufptr, false);

                if (!NetApiInitCalled)
                {
                    result = NetApiInit();
                    if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                    {
                        return result;
                    }

                    NetApiInitCalled = true;
                }

                result = (uint)NetLocalGroupDelMembers(servername, groupname, 3, bufptr, 1);
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
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

        public static uint
        NetApiInit(
            )
        {
            uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            try
            {
                if (Configurations.currentPlatform != LikewiseTargetPlatform.Windows)
                {
                    result = (uint)NetInitMemory();
                }
            }
            catch (Exception)
            {
                result = (uint)ErrorCodes.WIN32Enum.ERROR_EXCEPTION_IN_SERVICE;
            }

            return result;

        }

        const string USER = "UName";
        const string FULL_NAME = "Full Name";
        const string DESCRIPTION = "Description";
        const string GROUP = "GName";
        const string DISABLED = "Disabled";

        public delegate void LUGEnumStatusDelegate(LUGEnumStatus status);

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

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct NETRESOURCE
        {
            public int dwScope;
            public int dwType;
            public int dwDisplayType;
            public int dwUsage;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string LocalName;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string RemoteName;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string Comment;
            [MarshalAs(UnmanagedType.LPWStr)]
            public string Provider;
        }

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
    }
}

