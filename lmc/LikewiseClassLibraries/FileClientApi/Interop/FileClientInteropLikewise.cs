using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Likewise.LMC.FileClient
{
    public class InteropLikewise
    {
        #region Copy/Delete/Move File APIs

        [DllImport("liblwfileclient.so", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool CopyFile(
            string lpExistingFileName,
            string lpNewFileName,
            bool bFailIfExists
            );

        [DllImport("liblwfileclient.so", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool DeleteFile(
            string lpFileName
            );

        [DllImport("liblwfileclient.so", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool MoveFile(
            string lpExistingFileName,
            string lpNewFileName
            );

        [DllImport("liblwfileclient.so", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool RemoveDirectory(
            string lpDirectoryName
            );

        #endregion

        #region Local and Connected Share File Enumeration APIs

        [DllImport("liblwfileclient.so", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern IntPtr FindFirstFile(
            string lpFileName,
            ref LIKEWISE_FIND_DATA FindFileData
            );

        [DllImport("liblwfileclient.so", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool FindNextFile(
            IntPtr hFindFile,
            ref LIKEWISE_FIND_DATA FindFileData
            );

        [DllImport("liblwfileclient.so", SetLastError = true)]
        public static extern long FileTimeToSystemTime(
            ref FILETIME FileTime,
            ref SYSTEMTIME SystemTime
            );

        #endregion

        #region Remote File Resource Enumeration APIs

        [DllImport("libmpr.so", CharSet = CharSet.Unicode)]
        public static extern WinError WNetAddConnection2(
            NETRESOURCE netResource,
            string password,
            string username,
            int flags
            );

        [DllImport("libmpr.so", CharSet = CharSet.Unicode)]
        public static extern WinError WNetCancelConnection2(
            string name,
            int flags,
            bool force
            );

/* Not provided in Likewise library
        [DllImport("libmpr.so", CharSet = CharSet.Unicode)]
        public static extern WinError WNetOpenEnum(
            ResourceScope dwScope,
            ResourceType dwType,
            ResourceUsage dwUsage,
            NETRESOURCE pNetResource,
            out IntPtr phEnum
            );

        [DllImport("libmpr.so", CharSet = CharSet.Auto)]
        public static extern WinError WNetEnumResource(
            IntPtr hEnum,
            ref int pcCount,
            IntPtr pBuffer,
            ref int pBufferSize
            );

        [DllImport("libmpr.so")]
        public static extern WinError WNetCloseEnum(IntPtr hEnum);
*/
        #endregion
    }
}
