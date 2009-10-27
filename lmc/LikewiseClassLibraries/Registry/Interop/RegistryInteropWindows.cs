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
using Likewise.LMC.LMConsoleUtils;
using System.Runtime.InteropServices;

//Use the DebugMarshal for Debug builds, and the standard Marshal in release builds
#if DEBUG
using Marshal = Likewise.LMC.LMConsoleUtils.DebugMarshal;
#endif

namespace Likewise.LMC.Registry
{
    public class RegistryInteropWindows
    {
        private const string advapiDllPath = "advapi32.dll";

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegCloseKey(IntPtr Buffer);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegConnectRegistryA(string lpmachineName, long hKey, out long phKResult);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegCreateKeyEx(
            int hKey,
            [MarshalAs(UnmanagedType.LPStr)] string lpSubKey,
            int Reserved,
            [MarshalAs(UnmanagedType.LPStr)] string lpClass,
            RegistryApi.RegOption dwOptions,
            RegistryApi.RegSAM samDesired,
            [MarshalAs(UnmanagedType.LPStr)] string[] lpSecurityAttributes,
            out int phkResult,
            out RegistryApi.RegResult lpdwDisposition);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegCreateKeyA(uint hKey, string lpSubKey, ref IntPtr phkResult);

        [DllImport(advapiDllPath, EntryPoint = "RegDeleteKey", SetLastError = true)]
        public static extern int RegDeleteKey(
            UIntPtr hKey,     // Handle to an open registry key
            string lpSubKey); // The name of the key to be deleted.

        [DllImport(advapiDllPath, EntryPoint = "RegDeleteKey", SetLastError = true)]
        public static extern int RegDeleteKeyEx(
            UIntPtr hKey,
            string lpSubKey,
            uint samDesired, // see Notes below
            uint Reserved);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegDisableReflectionKey(IntPtr hBase);      

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegEnableReflectionKey(IntPtr hBase);

        [DllImport(advapiDllPath, EntryPoint = "RegEnumKeyEx", CharSet = CharSet.Unicode, SetLastError = true)]       
        public extern static int RegEnumKeyEx(
            long hkey,
            long index,
            string lpName,
            long lpcbName,
            long reserved,
            string lpClass,
            long lpcbClass,
            ref RegistryApi.FILETIME lpftLastWriteTime);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegEnumValue(
              string hKey,
              uint dwIndex,
              ref string lpValueName,
              ref int lpcValueName,
              IntPtr lpReserved,
              IntPtr lpType,
              IntPtr lpData,
              IntPtr lpcbData);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern long RegGetValue(
            IntPtr hkey,
            string lpSubKey,
            string lpValue,
            uint dwFlags,
            out IntPtr pdwType,
            out IntPtr pvData,
            IntPtr pcbData);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegLoadKey(
            IntPtr hkey,
            string lpSubKey,
            string lpFile);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegNotifyChangeKeyValue(IntPtr hKey, bool watchSubtree,
            int dwNotifyFilter, 
            IntPtr hEvent, 
            bool fAsynchronous);
        
        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegisterTraceGuids(); //TODO

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern long RegOverridePredefKey(IntPtr hkey, IntPtr hnewKey);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegOpenKeyA(int hKey, string lpSubKey, ref int phkResult);
       
        [DllImport(advapiDllPath, EntryPoint = "RegOpenKeyEx", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegOpenKeyEx(
            long hKey, //Intptr
            string subKey,
            int options,
            int sam,
            out UIntPtr phkResult);

        [DllImport(advapiDllPath, EntryPoint = "RegQueryInfoKey", CallingConvention = CallingConvention.Winapi, SetLastError = true, CharSet = CharSet.Unicode)]        
        public extern static int RegQueryInfoKey(
            UIntPtr hkey,
            out StringBuilder lpClass,
            ref uint lpcbClass,
            IntPtr lpReserved,
            out uint lpcSubKeys,
            out uint lpcbMaxSubKeyLen,
            out uint lpcbMaxClassLen,
            out uint lpcValues,
            out uint lpcbMaxValueNameLen,
            out uint lpcbMaxValueLen,
            out uint lpcbSecurityDescriptor,
            IntPtr lpftLastWriteTime);

        //For Binary
        /// <summary>
        /// Retrieves the data associated with the default or unnamed value of a specified registry key. The data must be a null-terminated string.
        /// </summary>
        /// <param name="hKey">[in] A handle to an open registry key. The key must have been opened with the KEY_QUERY_VALUE access right.</param>
        /// <param name="lpSubKey">[in] The name of the subkey of the hKey parameter for which the default value is retrieved. Key names are not case sensitive. If this parameter is NULL or points to an empty string, the function retrieves the default value for the key identified by hKey.</param>
        /// <param name="lpValue">[out] A pointer to a buffer that receives the default value of the specified key. If lpValue is NULL, and lpcbValue is non-NULL, the function returns ERROR_SUCCESS, and stores the size of the data, in bytes, in the variable pointed to by lpcbValue. This enables an application to determine the best way to allocate a buffer for the value's data.</param>
        /// <param name="lpcbValue">[in, out] A pointer to a variable that specifies the size of the buffer pointed to by the lpValue parameter, in bytes. When the function returns, this variable contains the size of the data copied to lpValue, including any terminating null characters. If the data has the REG_SZ, REG_MULTI_SZ or REG_EXPAND_SZ type, this size includes any terminating null character or characters. For more information, see Remarks. If the buffer specified lpValue is not large enough to hold the data, the function returns ERROR_MORE_DATA and stores the required buffer size in the variable pointed to by lpcbValue. In this case, the contents of the lpValue buffer are undefined.</param>
        /// <returns></returns>
        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern UInt32 RegQueryValue(
            IntPtr hKey,
            [MarshalAs(UnmanagedType.LPStr)]string lpSubKey,
            byte[] lpValue,
            ref UInt32 lpcbValue);

        //for strings
        /// <summary>
        /// Retrieves the data associated with the default or unnamed value of a specified registry key. The data must be a null-terminated string.
        /// </summary>
        /// <param name="hKey">[in] A handle to an open registry key. The key must have been opened with the KEY_QUERY_VALUE access right.</param>
        /// <param name="lpSubKey">[in] The name of the subkey of the hKey parameter for which the default value is retrieved. Key names are not case sensitive. If this parameter is NULL or points to an empty string, the function retrieves the default value for the key identified by hKey.</param>
        /// <param name="lpValue">[out] A pointer to a buffer that receives the default value of the specified key. If lpValue is NULL, and lpcbValue is non-NULL, the function returns ERROR_SUCCESS, and stores the size of the data, in bytes, in the variable pointed to by lpcbValue. This enables an application to determine the best way to allocate a buffer for the value's data.</param>
        /// <param name="lpcbValue">[in, out] A pointer to a variable that specifies the size of the buffer pointed to by the lpValue parameter, in bytes. When the function returns, this variable contains the size of the data copied to lpValue, including any terminating null characters. If the data has the REG_SZ, REG_MULTI_SZ or REG_EXPAND_SZ type, this size includes any terminating null character or characters. For more information, see Remarks. If the buffer specified lpValue is not large enough to hold the data, the function returns ERROR_MORE_DATA and stores the required buffer size in the variable pointed to by lpcbValue. In this case, the contents of the lpValue buffer are undefined.</param>
        /// <returns></returns>
        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern UInt32 RegQueryValue(
            IntPtr hKey,
            [MarshalAs(UnmanagedType.LPStr)]string lpSubKey,
            StringBuilder lpValue,
            ref UInt32 lpcbValue);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, EntryPoint = "RegQueryValueExW", SetLastError = true)]
        public static extern int RegQueryValueEx(
            IntPtr hKey,
            string lpValueName,
            int lpReserved,
            out int lpType,
            StringBuilder lpData,
            ref uint lpcbData);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegSetValueEx(
            IntPtr hKey,
            [MarshalAs(UnmanagedType.LPStr)] string lpValueName,
            int Reserved, //Should be zero
            Microsoft.Win32.RegistryValueKind dwType,
            byte[] lpData,
            int cbData);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int RegUnLoadKey(  //TODO
            IntPtr hKey,
            [MarshalAs(UnmanagedType.LPStr)]string lpSubKey
            );

        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern bool LogonUser(
            String lpszUsername,
            String lpszDomain,
            String lpszPassword,
            int dwLogonType,
            int dwLogonProvider,
            ref IntPtr phToken
            );

        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern int RegConnectRegistry(
           string lpMachineName,
           Microsoft.Win32.RegistryHive hive,
           out IntPtr phkResult
        );

        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern int RegOpenKey(IntPtr hKey, string lpSubKey, out IntPtr phkResult);

        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern int RegQueryValueEx(
            IntPtr hKey,
            string lpValueName,
            int lpReserved,
            out int lpType,
            byte[] lpData,
            ref int lpcbData);

        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        public extern static bool CloseHandle(IntPtr handle);
    }    
}
