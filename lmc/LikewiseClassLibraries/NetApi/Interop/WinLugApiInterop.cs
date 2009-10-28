using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.AuthUtils;

namespace Likewise.LMC.NETAPI.Interop
{
    public class WinLugApi
    {
        #region Net API Imports

        private const string NETAPI_DLL_PATH = "netapi32.dll";

        [DllImport(NETAPI_DLL_PATH)]
        public extern static int NetUserDel(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string userName
            );

        [DllImport(NETAPI_DLL_PATH)]
        public extern static int NetUserGetLocalGroups(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string userName,
            int level,
            uint flags,
            out IntPtr bufPtr,
            int prefMaxLen,
            out int entriesRead,
            out int totalEntries
            );

        [DllImport(NETAPI_DLL_PATH)]
        public extern static int NetUserSetInfo(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string userName,
            int level,
            IntPtr buf,
            IntPtr parmErr
            );

        [DllImport(NETAPI_DLL_PATH)]
        public extern static int NetUserGetInfo(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string userName,
            int level,
            out IntPtr buf
            );

        [DllImport(NETAPI_DLL_PATH)]
        public extern static int NetApiBufferFree(
            IntPtr buffer
            );

        [DllImport(NETAPI_DLL_PATH)]
        public extern static int NetUserAdd(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            UInt32 level,
            IntPtr userInfo,
            IntPtr parmErr
            );

        [DllImport(NETAPI_DLL_PATH, CharSet = CharSet.Unicode)]
        public extern static int NetUserEnum(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            int level,
            int filter,
            out IntPtr bufPtr,
            int prefMaxLen,
            out int entriesRead,
            out int totalEntries,
            out int resumeHandle
            );

        [DllImport(NETAPI_DLL_PATH)]
        public extern static int NetLocalGroupAdd(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            UInt32 level,
            IntPtr groupInfo,
            IntPtr parmErr
            );

        [DllImport(NETAPI_DLL_PATH)]
        public extern static int NetLocalGroupDel(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string groupName
            );

        [DllImport(NETAPI_DLL_PATH)]
        public extern static int NetLocalGroupEnum(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            int level,
            out IntPtr bufPtr,
            int prefMaxLen,
            out int entriesRead,
            out int totalEntries,
            ref int resumeHandle
            );

        [DllImport(NETAPI_DLL_PATH)]
        public extern static int NetLocalGroupAddMembers(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string groupName,
            int level,
            IntPtr buf,
            int totalEntries
            );

        [DllImport(NETAPI_DLL_PATH)]
        public extern static int NetLocalGroupDelMembers(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string groupName,
            int level,
            IntPtr buf,
            int totalEntries
            );

        [DllImport(NETAPI_DLL_PATH)]
        public extern static int NetLocalGroupSetInfo(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string groupName,
            int level,
            IntPtr buf,
            IntPtr parmErr
            );

        [DllImport(NETAPI_DLL_PATH)]
        public extern static int NetLocalGroupGetInfo(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string groupName,
            int level,
            out IntPtr buf
            );

        [DllImport(NETAPI_DLL_PATH)]
        public extern static int NetLocalGroupGetMembers(
            [MarshalAs(UnmanagedType.LPWStr)]string serverName,
            [MarshalAs(UnmanagedType.LPWStr)]string localGroupName,
            int level,
            out IntPtr bufPtr,
            int prefMaxLen,
            out int entriesRead,
            out int totalEntries,
            IntPtr resumeHandle
            );

        #endregion
    }
}

