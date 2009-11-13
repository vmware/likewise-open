/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

using System;
using System.Collections.Generic;
using System.Text;
using Likewise.LMC.Utilities;
using System.Runtime.InteropServices;

//Use the DebugMarshal for Debug builds, and the standard Marshal in release builds
#if DEBUG
using Marshal = Likewise.LMC.Utilities.DebugMarshal;
#endif

namespace Likewise.LMC.Registry
{
    public class RegistryApi
    {
        public const int KEY_QUERY_VALUE = 0x0001;
        public const int KEY_SET_VALUE = 0x0002;
        public const int KEY_CREATE_SUB_KEY = 0x0004;
        public const int KEY_ENUMERATE_SUB_KEYS = 0x0008;
        public const int KEY_NOTIFY = 0x0010;
        public const int KEY_CREATE_LINK = 0x0020;
        public const int KEY_WOW64_32KEY = 0x0200;
        public const int KEY_WOW64_64KEY = 0x0100;
        public const int KEY_WOW64_RES = 0x0300;

        public const int REG_SECONDS_IN_MINUTE = 60;
        public const int REG_SECONDS_IN_HOUR = (60 * REG_SECONDS_IN_MINUTE);
        public const int REG_SECONDS_IN_DAY = (24 * REG_SECONDS_IN_HOUR);

        public const string PATH_SEPARATOR_STR_WIN = @"\";
        public const string PATH_SEPARATOR_STR_Linux = "/";

        public const uint RRF_RT_REG_NONE = 0x00000001;
        public const uint RRF_RT_REG_SZ = 0x00000002; //Restrict type to REG_SZ.
        public const uint RRF_RT_REG_EXPAND_SZ = 0x00000004;//Restrict type to REG_EXPAND_SZ.
        public const uint RRF_RT_REG_BINARY = 0x00000008; //Restrict type to REG_BINARY.
        public const uint RRF_RT_REG_DWORD = 0x00000010; //Restrict type to REG_DWORD.
        public const uint RRF_RT_REG_MULTI_SZ = 0x00000020;//Restrict type to REG_MULTI_SZ.
        public const uint RRF_RT_REG_QWORD = 0x00000040; //Restrict type to REG_QWORD.
        public const uint RRF_RT_DWORD = RRF_RT_REG_BINARY | RRF_RT_REG_DWORD;
        public const uint RRF_RT_QWORD = RRF_RT_REG_BINARY | RRF_RT_REG_QWORD;
        public const uint RRF_RT_ANY = 0x0000FFFF; //No type restriction.
        public const uint RRF_NOEXPAND = 0x10000000;
        public const uint RRF_ZEROONFAILURE = 0x20000000;        
        
        //REG_DATA_TYPE
        public const ulong REG_NONE = 0; // No value type
        public const ulong REG_SZ = 1;   // Unicode null terminated string
        public const ulong REG_EXPAND_SZ = 2;
        public const ulong REG_BINARY = 3;
        public const ulong REG_DWORD = 4;
        public const ulong REG_DWORD_LITTLE_ENDIAN = 4;
        public const ulong REG_DWORD_BIG_ENDIAN = 5;
        public const ulong REG_LINK = 6;
        public const ulong REG_MULTI_SZ = 7;
        public const ulong REG_RESOURCE_LIST = 8;
        public const ulong REG_FULL_RESOURCE_DESCRIPTOR = 9;
        public const ulong REG_RESOURCE_REQUIREMENTS_LIST = 10;
        public const ulong REG_QWORD = 11;
        public const ulong REG_QWORD_LITTLE_ENDIAN = 11;
        public const ulong REG_KEY = 12;
        public const ulong REG_KEY_DEFAULT = 13;
        public const ulong REG_PLAIN_TEXT = 14;
        public const ulong REG_UNKNOWN = 15;

        [Flags]
        public enum RegOption
        {
            NonVolatile = 0x0,
            Volatile = 0x1,
            CreateLink = 0x2,
            BackupRestore = 0x4,
            OpenLink = 0x8
        }

        [Flags]
        public enum RegSAM : int
        {
            QueryValue = 0x0001,
            SetValue = 0x0002,
            CreateSubKey = 0x0004,
            EnumerateSubKeys = 0x0008,
            Notify = 0x0010,
            CreateLink = 0x0020,
            WOW64_32Key = 0x0200,
            WOW64_64Key = 0x0100,
            WOW64_Res = 0x0300,
            Read = 0x00020019,
            Write = 0x00020006,
            Execute = 0x00020019,
            AllAccess = 0x000f003f
        }

        public enum RegResult
        {
            CreatedNewKey = 0x00000001,
            OpenedExistingKey = 0x00000002
        }

        public enum RegWow64Options
        {
            None = 0,
            KEY_WOW64_64KEY = 0x0200,
            KEY_WOW64_32KEY = 0x0100
        }

        public enum RegistryRights
        {
            ReadKey = 131097,
            WriteKey = 131078
        }

        public enum KeyTypes
        {
            REG_NONE = 0,
            REG_SZ = 1,
            REG_EXPAND_SZ = 2,
            REG_BINARY = 3,
            REG_DWORD = 4,
            REG_DWORD_LITTLE_ENDIAN = 4,
            REG_DWORD_BIG_ENDIAN = 5,
            REG_LINK = 6,
            REG_MULTI_SZ = 7
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct FILETIME
        {
            public long dwLowDateTime;
            public long dwHighDateTime;
        };

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct VALENT
        {
            [MarshalAs(UnmanagedType.LPWStr)]
            public string ve_valuename;
            public uint ve_valuelen;
            public IntPtr ve_valueptr;
            public ulong ve_type;
        };      

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct SECURITY_ATTRIBUTES
        {
            public long nLength;
            public long pSecurityDescriptor;
            public bool bInheritHandle;
        };

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct _REG_PARSE_ITEM
        {
            ulong type;
            ulong valueType;
            public string keyName;
            public string valueName;
            uint lineNumber;
            IntPtr value;
            uint valueLen;
        }
    }

    public class RegistryInterop
    {
        private const string libadvapiPath = "libregclient.dll";

        //DWORD
        //RegCreateKeyEx(
        //    HANDLE hRegConnection,
        //    HKEY hKey,
        //    PCWSTR pSubKey,
        //    DWORD Reserved,
        //    PWSTR pClass,
        //    DWORD dwOptions,
        //    REGSAM samDesired,
        //    PSECURITY_ATTRIBUTES pSecurityAttributes,
        //    PHKEY phkResult,
        //    PDWORD pdwDisposition
        //    );
        //    }
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegCreateKeyExW(
            IntPtr hRegConnection,
            IntPtr hKey,           
            IntPtr pSubKey,
            int Reserved,            
            IntPtr pClass,
            int dwOptions,
            RegistryApi.RegSAM samDesired,
            ref RegistryApi.SECURITY_ATTRIBUTES pSecurityAttributes,
            out IntPtr phkResult,
            out ulong pdwDisposition);

        //DWORD
        //RegEnumRootKeys(
        //    IN HANDLE hRegConnection,
        //    OUT PSTR** pppszRootKeyNames,
        //    OUT PDWORD pdwNumRootKeys
        //)

        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegEnumRootKeysW(
            IntPtr hRegConnection,
            out IntPtr pppszRootKeyNames,
            out int pdwNumRootKeys);

        //DWORD
        //RegCloseKey(
        //    HANDLE hRegConnection,
        //    HKEY hKey
        //);
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegCloseKey(
            IntPtr hRegConnection,
            IntPtr hKey);

        //DWORD
        //RegDeleteKey(
        //    HANDLE hRegConnection,
        //    HKEY hKey,
        //    PCWSTR pSubKey
        //    );
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegDeleteKeyW(
            IntPtr hRegConnection,
            IntPtr hKey,
            [MarshalAs(UnmanagedType.LPWStr)] string pSubKey);
         
        //DWORD
        //RegDeleteKeyValue(
        //    HANDLE hRegConnection,
        //    HKEY hKey,
        //    PCWSTR pSubKey,
        //    PCWSTR pValueName
        //    );
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegDeleteKeyValueW(
            IntPtr hRegConnection,
            IntPtr hKey,
            [MarshalAs(UnmanagedType.LPWStr)] StringBuilder pSubKey,
            [MarshalAs(UnmanagedType.LPWStr)] string pValueName);

        //DWORD
        //RegDeleteTree(
        //    HANDLE hRegConnection,
        //    HKEY hKey,
        //    PCWSTR pSubKey
        //    );
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegDeleteTreeW(
            IntPtr hRegConnection,
            IntPtr hKey,
            [MarshalAs(UnmanagedType.LPWStr)] string pSubKey);

        //DWORD
        //RegDeleteValue(
        //    HANDLE hRegConnection,
        //    HKEY hKey,
        //    PCWSTR pSubKey,
        //    PCWSTR pValueName
        //    );
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegDeleteValueW(
            IntPtr hRegConnection,
            IntPtr hKey,          
            [MarshalAs(UnmanagedType.LPWStr)] string pValueName);

        //DWORD
        //RegEnumKeyEx(
        //    HANDLE hRegConnection,
        //    HKEY hKey,
        //    DWORD dwIndex,
        //    PWSTR pName,
        //    PDWORD pcName,
        //    PDWORD pReserved,
        //    PWSTR pClass,
        //    PDWORD pcClass,
        //    PFILETIME pftLastWriteTime
        //    );
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegEnumKeyExW(
            IntPtr hRegConnection,
            IntPtr hKey,
            uint dwIndex,             
            [MarshalAs(UnmanagedType.LPWStr)] StringBuilder pName,
            out uint pcName,
            uint pReserved,
            [MarshalAs(UnmanagedType.LPWStr)] ref string pClass,
            ref long pcClass,
            out RegistryApi.FILETIME pftLastWriteTime);

        //DWORD
        //RegEnumValue(
        //   HANDLE hRegConnection,
        //   HKEY hKey,
        //   DWORD dwIndex,
        //   PWSTR pValueName,
        //   PDWORD pcchValueName,
        //   PDWORD pReserved,
        //   PDWORD pType,
        //   PBYTE pData,
        //   PDWORD pcbData
        //   );
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegEnumValueW(
            IntPtr hRegConnection,
            IntPtr hKey,
            uint dwIndex,
            [MarshalAs(UnmanagedType.LPWStr)] StringBuilder pValueName,
            ref uint pcchValueName,
            int pReserved,
            out ulong pType,
            byte[] pData,
            ref uint pcbData);

        //DWORD
        //RegGetValue(
        //    HANDLE hRegConnection,
        //    HKEY hKey,
        //    PCWSTR pSubKey,
        //    PCWSTR pValue,
        //    DWORD dwFlags,
        //    PDWORD pdwType,
        //    PVOID pvData,
        //    PDWORD pcbData
        //    );
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegGetValueW(
            IntPtr hRegConnection,
            IntPtr hKey,
            [MarshalAs(UnmanagedType.LPWStr)] string pSubKey,
            [MarshalAs(UnmanagedType.LPWStr)] string pValue,
            uint dwFlags,
            out ulong pdwType,
            byte[] pData,
            ref uint pcbData);

        //DWORD
        //RegQueryMultipleValues(
        //    IN HANDLE hRegConnection,
        //    IN HKEY hKey,
        //    OUT PVALENT val_list,
        //    IN DWORD num_vals,
        //    OUT OPTIONAL PWSTR pValueBuf,
        //    IN OUT OPTIONAL PDWORD dwTotsize
        //    )
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegQueryMultipleValuesW(
            IntPtr hRegConnection,
            IntPtr hKey,
            IntPtr val_list,
            uint num_vals,
            [MarshalAs(UnmanagedType.LPWStr)] StringBuilder pValueBuf,
            ref uint dwTotsize);

        //DWORD
        //RegQueryValueEx(
        //    HANDLE hRegConnection,
        //    HKEY hKey,
        //    PCWSTR pValueName,
        //    PDWORD pReserved,
        //    PDWORD pType,
        //    PBYTE pData,
        //    PDWORD pcbData
        //    );
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegQueryValueExW(
            IntPtr hRegConnection,
            IntPtr hKey,
            [MarshalAs(UnmanagedType.LPWStr)] string pValueName,
            uint pReserved,
            out ulong pType,
            out byte[] pData,
            out uint pcbData);

        //DWORD
        //RegOpenKeyEx(
        //    HANDLE hRegConnection,
        //    HKEY hKey,
        //    PCWSTR pSubKey,
        //    DWORD ulOptions,
        //    REGSAM samDesired,
        //    PHKEY phkResult
        //    );
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegOpenKeyExW(
            IntPtr hRegConnection,
            IntPtr hKey,
            [MarshalAs(UnmanagedType.LPWStr)] string pSubKey,
            uint ulOptions,
            RegistryApi.RegSAM samDesired,
            out IntPtr phkResult);

        //DWORD
        //RegQueryInfoKey(
        //    HANDLE hRegConnection,
        //    HKEY hKey,
        //    PWSTR pClass,
        //    PDWORD pcClass,
        //    PDWORD pReserved,
        //    PDWORD pcSubKeys,
        //    PDWORD pcMaxSubKeyLen,
        //    PDWORD pcMaxClassLen,
        //    PDWORD pcValues,
        //    PDWORD pcMaxValueNameLen,
        //    PDWORD pcMaxValueLen,
        //    PDWORD pcbSecurityDescriptor,
        //    PFILETIME pftLastWriteTime
        //    );
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegQueryInfoKeyW(
            IntPtr hRegConnection,
            IntPtr hKey,
            [MarshalAs(UnmanagedType.LPWStr)] out string pClass,
            ref ulong pcClass,
            int pReserved,
            out uint lpcSubKeys,
            out uint lpcbMaxSubKeyLen,
            out ulong lpcbMaxClassLen,
            out uint lpcValues,
            out uint lpcbMaxValueNameLen,
            out uint lpcbMaxValueLen,
            out IntPtr lpcbSecurityDescriptor,
            out RegistryApi.FILETIME pftLastWriteTime);     

        //DWORD
        //RegSetValueEx(
        //    HANDLE hRegConnection,
        //    HKEY hKey,
        //    PCWSTR pValueName,
        //    DWORD Reserved,
        //    DWORD dwType,
        //    const BYTE *pData,
        //    DWORD cbData
        //    );
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegSetValueExW(
            IntPtr hRegConnection,
            IntPtr hKey,            
            [MarshalAs(UnmanagedType.LPWStr)] string pValueName,
            int pReserved,
            uint dwType,
            byte[] pData,
            uint pcbData);

        //DWORD
        //RegSetKeyValue(
        //IN HANDLE hRegConnection,
        //IN HKEY hKey,
        //IN OPTIONAL PCWSTR lpSubKey,
        //IN OPTIONAL PCWSTR lpValueName,
        //IN DWORD dwType,
        //IN OPTIONAL PCVOID lpData,
        //IN DWORD cbData
        //)
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegSetKeyValueW(
            IntPtr hRegConnection,
            IntPtr hKey,
            [MarshalAs(UnmanagedType.LPWStr)] StringBuilder lpSubKey,
            [MarshalAs(UnmanagedType.LPWStr)] StringBuilder lpValueName,           
            int dwType,
            [MarshalAs(UnmanagedType.LPWStr)] string pData,
            uint pcbData);              

        //RPC Server Api calls

        //DWORD
        //RegOpenServer(
        //    PHANDLE phConnection
        //    );
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegOpenServer(out IntPtr hRegConnection);

        //DWORD
        //RegCloseServer(
        //    HANDLE hConnection);
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegCloseServer(IntPtr hRegConnection);        

        [DllImport("kernel32.dll")]
        public static extern int MultiByteToWideChar(
          uint CodePage,
          uint dwFlags,
          [MarshalAs(UnmanagedType.LPArray)] Byte[] lpMultiByteStr,
          int cbMultiByte,
          [Out, MarshalAs(UnmanagedType.LPArray)] Byte[] lpWideCharStr,
          int cchWideChar);

        //DWORD
        //RegOpenServer(
        //    PHANDLE phConnection
        //    );
        [DllImport(libadvapiPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegParseOpen(out IntPtr hRegConnection);
    }
}
