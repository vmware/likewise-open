using System;
using System.Collections.Generic;
using System.Text;

namespace FileClientApi
{
    public class FileClientApi
    {
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        static extern bool CopyFile(string lpExistingFileName, string lpNewFileName, bool bFailIfExists);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool CopyFile(
          [MarshalAs(UnmanagedType.LPStr)]string lpExistingFileName,
          [MarshalAs(UnmanagedType.LPStr)]string lpNewFileName,
          [MarshalAs(UnmanagedType.Bool)]bool bFailIfExists);

        public static bool apiCopyFile(string lpExistingFileName, string lpNewFileName, bool bFaileIfExists)
        {
            bool ret = CopyFile(lpExistingFileName, lpNewFileName, bFailIfExists);

            return ret;
        }
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool DeleteFile(string lpFileName);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool DeleteFileA([MarshalAs(UnmanagedType.LPStr)]string lpFileName);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool DeleteFileW([MarshalAs(UnmanagedType.LPWStr)]string lpFileName);

        public static bool apiDeleteFile(string lpFileName)
        {
            bool ret = DeleteFileW(lpFileName);

            return ret;
     
        
        }

        [DllImport("kernel32.dll")]
        static extern bool MoveFile(string lpExistingFileName, string lpNewFileName);

        [DllImport("KERNEL32.DLL", EntryPoint = "MoveFileW", SetLastError = true,
             CharSet = CharSet.Unicode, ExactSpelling = true,
             CallingConvention = CallingConvention.StdCall)]
        public static extern bool MoveFile(String src, String dst);

        public static bool apiMoveFile(string src, string dst)
        {
            bool ret = MoveFile(src, dst);

            return ret;


        }
      [StructLayout(LayoutKind.Sequential)]
        public struct FILETIME {
        public uint dwLowDateTime;
        public uint dwHighDateTime;
     }; 

    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)] 
    public struct WIN32_FIND_DATA {
        public FileAttributes dwFileAttributes;
        public FILETIME ftCreationTime; 
        public FILETIME ftLastAccessTime; 
        public FILETIME ftLastWriteTime; 
        public int nFileSizeHigh;
        public int nFileSizeLow;
        public int dwReserved0;
        public int dwReserved1;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=MAX_PATH)] 
        public string cFileName; 
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=MAX_ALTERNATE)] 
        public string cAlternate; 
    };

        DllImport("kernel32.dll", CharSet=CharSet.Auto)]
        static extern IntPtr FindFirstFile(string lpFileName, out WIN32_FIND_DATA lpFindFileData);
             public static bool apiMoveFile(string src, string dst)
        {
            bool ret = MoveFile(src, dst);

            return ret;


        }

    }
}
