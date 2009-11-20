using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Likewise.LMC.FileClient
{
    public class InteropWindows
    {
        #region Copy/Delete/Move File APIs

		[DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
		public static extern bool CreateDirectory(
		    string lpDirectoryName,
		    IntPtr lpSecurityAttributes
		    );

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool CopyFile(
            string lpExistingFileName,
            string lpNewFileName,
            bool bFailIfExists
            );

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool DeleteFile(
            string lpFileName
            );

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool MoveFile(
            string lpExistingFileName,
            string lpNewFileName
            );

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool RemoveDirectory(
            string lpDirectoryName
            );

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool Rename(
            string lpOldName,
		    string lpNewName
            );

        #endregion

        #region Local and Connected Share File Enumeration APIs

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern IntPtr FindFirstFileW(
            string lpFileName,
            ref WIN32_FIND_DATA FindFileData
            );

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool FindNextFileW(
            IntPtr hFindFile,
            ref WIN32_FIND_DATA FindFileData
            );

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern long FileTimeToSystemTime(
            ref FILETIME FileTime,
            ref SYSTEMTIME SystemTime
            );

        #endregion

        #region Remote File Resource Enumeration APIs

        [DllImport("mpr.dll", CharSet = CharSet.Unicode)]
        public static extern WinError WNetAddConnection2(
            NETRESOURCE netResource,
            string password,
            string username,
            int flags
            );

        [DllImport("mpr.dll", CharSet = CharSet.Unicode)]
        public static extern WinError WNetCancelConnection2(
            string name,
            int flags,
            bool force
            );

        [DllImport("mpr.dll", CharSet = CharSet.Unicode)]
        public static extern WinError WNetOpenEnum(
            ResourceScope dwScope,
            ResourceType dwType,
            ResourceUsage dwUsage,
            NETRESOURCE pNetResource,
            out IntPtr phEnum
            );

        [DllImport("mpr.dll", CharSet = CharSet.Auto)]
        public static extern WinError WNetEnumResource(
            IntPtr hEnum,
            ref int pcCount,
            IntPtr pBuffer,
            ref int pBufferSize
            );

        [DllImport("mpr.dll")]
        public static extern WinError WNetCloseEnum(IntPtr hEnum);

        #endregion
    }
}
