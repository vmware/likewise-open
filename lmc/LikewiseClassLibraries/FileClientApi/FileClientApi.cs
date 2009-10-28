using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Likewise.LMC.FileClient
{
#region File Client error, type, and structure definitions
    public enum WinError
    {
        ERROR_SUCCESS = 0,
        ERROR_ACCESS_DENIED = 5,
        ERROR_ALREADY_ASSIGNED = 85,
        ERROR_BAD_DEV_TYPE = 66,
        ERROR_BAD_DEVICE = 1200,
        ERROR_BAD_NET_NAME = 67,
        ERROR_BAD_PROFILE = 1206,
        ERROR_BAD_PROVIDER = 1204,
        ERROR_BAD_USERNAME = 2202,
        ERROR_BUSY = 170,
        ERROR_CANCELLED = 1223,
        ERROR_CANNOT_OPEN_PROFILE = 1205,
        ERROR_DEVICE_ALREADY_REMEMBERED = 1202,
        ERROR_EXTENDED_ERROR = 1208,
        ERROR_INVALID_ADDRESS = 487,
        ERROR_INVALID_PARAMETER = 87,
        ERROR_INVALID_PASSWORD = 86,
        ERROR_LOGON_FAILURE = 1326,
        ERROR_NO_NET_OR_BAD_PATH = 1203,
        ERROR_NO_NETWORK = 1222,
        ERROR_DOWNGRADE_DETECTED = 1265,
        ERROR_SESSION_CREDENTIAL_CONFLICT = 1219,
        ERROR_LOGON_TYPE_NOT_GRANTED = 1385,
        NO_ERROR = 0,
        ERROR_NO_MORE_ITEMS = 259,
        ERROR_MORE_DATA = 234,
        ERROR_INVALID_HANDLE = 6,
        ERROR_NO_TOKEN = 1008,
        ERROR_NOT_ENOUGH_MEMORY = 8
    };

    public enum ResourceScope
    {
        RESOURCE_CONNECTED = 1,
        RESOURCE_GLOBALNET,
        RESOURCE_REMEMBERED,
        RESOURCE_RECENT,
        RESOURCE_CONTEXT
    };

    public enum ResourceType
    {
        RESOURCETYPE_ANY,
        RESOURCETYPE_DISK,
        RESOURCETYPE_PRINT,
        RESOURCETYPE_RESERVED
    };

    public enum ResourceUsage
    {
        RESOURCEUSAGE_CONNECTABLE = 0x00000001,
        RESOURCEUSAGE_CONTAINER = 0x00000002,
        RESOURCEUSAGE_NOLOCALDEVICE = 0x00000004,
        RESOURCEUSAGE_SIBLING = 0x00000008,
        RESOURCEUSAGE_ATTACHED = 0x00000010,
        RESOURCEUSAGE_ALL = (RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER | RESOURCEUSAGE_ATTACHED),
    };

    public enum ResourceDisplayType
    {
        RESOURCEDISPLAYTYPE_GENERIC = 0,
        RESOURCEDISPLAYTYPE_DOMAIN = 1,
        RESOURCEDISPLAYTYPE_SERVER = 2,
        RESOURCEDISPLAYTYPE_SHARE = 3,
        RESOURCEDISPLAYTYPE_FILE = 4,
        RESOURCEDISPLAYTYPE_GROUP = 5,
        RESOURCEDISPLAYTYPE_NETWORK = 6,
        RESOURCEDISPLAYTYPE_ROOT = 7,
        RESOURCEDISPLAYTYPE_SHAREADMIN = 8,
        RESOURCEDISPLAYTYPE_DIRECTORY = 9,
        RESOURCEDISPLAYTYPE_TREE = 10,
        RESOURCEDISPLAYTYPE_NDSCONTAINER = 10
    };

    [StructLayout(LayoutKind.Sequential)]
    public class NETRESOURCE
    {
        public ResourceScope dwScope = 0;
        public ResourceType dwType = 0;
        public ResourceDisplayType dwDisplayType = 0;
        public ResourceUsage dwUsage = 0;
        [MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)]
        public string pLocalName;
        [MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)]
        public string pRemoteName;
        [MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)]
        public string pComment;
        [MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)]
        public string pProvider;
    };

    //public const int MAX_PATH = 260;
    //public const int MAX_ALTERNATE = 14;

    [StructLayout(LayoutKind.Sequential)]
    public struct FILETIME
    {
        public uint dwLowDateTime;
        public uint dwHighDateTime;
    };

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
    public struct WIN32_FIND_DATA
    {
        public uint dwFileAttributes;
        public FILETIME ftCreationTime;
        public FILETIME ftLastAccessTime;
        public FILETIME ftLastWriteTime;
        public int nFileSizeHigh;
        public int nFileSizeLow;
        public int dwReserved0;
        public int dwReserved1;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)]
        public string cFileName;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=14)]
        public string cAlternate;
    };
#endregion

    public class FileClient
    {
        #region Copy/Delete/Move File APIs

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern bool CopyFile(
            string lpExistingFileName,
            string lpNewFileName,
            bool bFailIfExists
            );

        public static int apiCopyFile(
            string lpExistingFileName,
            string lpNewFileName,
            bool bFailIfExists
            )
        {
            bool copied = CopyFile(lpExistingFileName, lpNewFileName, bFailIfExists);
            int error = 0;

            if (!copied)
            {
                error = Marshal.GetLastWin32Error();
            }

            return error;
        }

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern bool DeleteFile(
            string lpFileName
            );

        public static int apiDeleteFile(
            string lpFileName
            )
        {
            bool deleted = DeleteFile(lpFileName);
            int error = 0;

            if (!deleted)
            {
                error = Marshal.GetLastWin32Error();
            }

            return error;
        }

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern bool MoveFile(
            string lpExistingFileName,
            string lpNewFileName
            );

        public static int apiMoveFile(
            string lpExistingFileName,
            string lpNewFileName
            )
        {
            bool moved = MoveFile(lpExistingFileName, lpNewFileName);
            int error = 0;

            if (!moved)
            {
                error = Marshal.GetLastWin32Error();
            }

            return error;
        }
        #endregion

        #region Local and Connected Share File Enumeration APIs

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern IntPtr FindFirstFile(
            string lpFileName,
            out WIN32_FIND_DATA lpFindFileData
            );

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern bool FindNextFile(
            IntPtr hFindFile,
            out WIN32_FIND_DATA lpFindFileData
            );

        public static IntPtr apiFindFirstFile(
            string lpFileName,
            out WIN32_FIND_DATA lpFindFileData
            )
        {
            IntPtr handle = FindFirstFile(lpFileName, out lpFindFileData);

            return handle;
        }

        #endregion

        #region Remote File Resource Enumeration APIs

        [DllImport("mpr.dll", CharSet = CharSet.Unicode)]
        private static extern WinError WNetAddConnection2W(
            NETRESOURCE netResource,
            string password,
            string username,
            int flags
            );

        public static WinError CreateConnection(
            string networkName,
            string username,
            string password
            )
        {
            NETRESOURCE netResource = new NETRESOURCE();

            netResource.dwScope = ResourceScope.RESOURCE_GLOBALNET;
            netResource.dwType = ResourceType.RESOURCETYPE_DISK;
            netResource.dwDisplayType = ResourceDisplayType.RESOURCEDISPLAYTYPE_SHARE;
            netResource.dwUsage = ResourceUsage.RESOURCEUSAGE_ALL;
            netResource.pRemoteName = networkName;

            return WNetAddConnection2W(netResource, password, username, 0);
        }

        [DllImport("mpr.dll", CharSet = CharSet.Unicode)]
        private static extern WinError WNetCancelConnection2W(
            string name,
            int flags,
            bool force
            );

        public static WinError DeleteConnection(
            string networkName
            )
        {
            return WNetCancelConnection2W(networkName, 0, true);
        }

        [DllImport("mpr.dll", CharSet = CharSet.Unicode)]
        private static extern WinError WNetOpenEnumW(
            ResourceScope dwScope,
            ResourceType dwType,
            ResourceUsage dwUsage,
            NETRESOURCE pNetResource,
            out IntPtr phEnum
            );

        public static WinError BeginEnumNetResources(
            ResourceScope dwScope,
            ResourceType dwType,
            ResourceUsage dwUsage,
            NETRESOURCE pNetResource,
            out IntPtr enumHandle
            )
        {
            WinError error = WinError.NO_ERROR;

            error = WNetOpenEnumW(dwScope, dwType, dwUsage, pNetResource, out enumHandle);

            if (error == WinError.ERROR_EXTENDED_ERROR)
            {
                error = (WinError) Marshal.GetLastWin32Error();
            }

            return error;
        }

        [DllImport("mpr.dll", CharSet = CharSet.Auto)]
        private static extern WinError WNetEnumResource(
            IntPtr hEnum,
            ref int pcCount,
            IntPtr pBuffer,
            ref int pBufferSize
            );

        public static WinError EnumNetResources(
            IntPtr enumHandle,
            out List<NETRESOURCE> NetResources
            )
        {
            WinError error = WinError.NO_ERROR;
            List<NETRESOURCE> nrList = new List<NETRESOURCE>();
            int cEntries = -1;
            int bufferSize = 16384;
            IntPtr ptrBuffer = Marshal.AllocHGlobal(bufferSize);
            NETRESOURCE nr;

            for ( ; ; )
            {
                cEntries = -1;
                bufferSize = 16384;

                error = WNetEnumResource(enumHandle, ref cEntries, ptrBuffer, ref bufferSize);
                if ((error != WinError.NO_ERROR) || (cEntries < 1))
                {
                    if (error == WinError.ERROR_NO_MORE_ITEMS)
                    {
                        error = WinError.NO_ERROR;
                    }
                    break;
                }

                Int32 ptr = ptrBuffer.ToInt32();
                for (int i = 0; i < cEntries; i++)
                {
                    nr = (NETRESOURCE)Marshal.PtrToStructure(new IntPtr(ptr), typeof(NETRESOURCE));

                    nrList.Add(nr);
                    ptr += Marshal.SizeOf(nr);
                }
            }

            Marshal.FreeHGlobal(ptrBuffer);

            NetResources = nrList;
            return error;
        }

        [DllImport("mpr.dll")]
        private static extern WinError WNetCloseEnum(IntPtr hEnum);

        public static WinError EndEnumNetResources(
            IntPtr enumHandle
            )
        {
            WinError error = WinError.NO_ERROR;

            error = WNetCloseEnum(enumHandle);

            if (error == WinError.ERROR_EXTENDED_ERROR)
            {
                error = (WinError)Marshal.GetLastWin32Error();
            }

            return error;
        }

        #endregion
    }
}
