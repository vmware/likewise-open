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
using System.Runtime.InteropServices;
using System.Security.Principal;
using System.Security.Permissions;

using Microsoft.Win32;

using Likewise.LMC.Utilities;
using Likewise.LMC.SecurityDesriptor;

namespace Likewise.LMC.Registry
{
    //Reg data types
    [ComVisible(true)]
    public enum LWRegistryValueKind : ulong
    {
        REG_NONE = 0,
        REG_SZ,
        REG_EXPAND_SZ,
        REG_BINARY,
        REG_DWORD,
        REG_DWORD_BIG_ENDIAN,
        REG_LINK,
        REG_MULTI_SZ,
        REG_RESOURCE_LIST,
        REG_FULL_RESOURCE_DESCRIPTOR,
        REG_RESOURCE_REQUIREMENTS_LIST,
        REG_QUADWORD,
        REG_KEY,                        
        REG_KEY_DEFAULT,                
        REG_PLAIN_TEXT,
        REG_UNKNOWN
    } 
     //Key types
    public enum HKEY : long
    {
        HKEY_CLASSES_ROOT = 0x80000000,
        HEKY_CURRENT_USER = 0x80000001,
        HKEY_LOCAL_MACHINE = 0x80000002,
        HKEY_USERS = 0x80000003,
        HKEY_PERFORMENCE_DATA = 0x80000004,
        HKEY_CURRENT_CONFIG = 0x80000005,
        HKEY_DYN_DATA = 0x80000006,

        //for specific to likewise
        HKEY_LIKEWISE = 0x80000007,
        HKEY_LIKEWISE_SUBKEY = 0x80000008
    }   

    public class RegistryInteropWrapperWindows
    {
        public enum LogonType : int
        {
            //'This logon type is intended for batch servers, where processes may be executing on behalf of a user without 
            //'their direct intervention. This type is also for higher performance servers that process many plaintext
            //'authentication attempts at a time, such as mail or Web servers. 
            //'The LogonUser function does not cache credentials for this logon type.
            LOGON32_LOGON_BATCH = 4,

            //'This logon type is intended for users who will be interactively using the computer, such as a user being logged on 
            //'by a terminal server, remote shell, or similar process.
            //'This logon type has the additional expense of caching logon information for disconnected operations; 
            //'therefore, it is inappropriate for some client/server applications,
            //'such as a mail server.
            LOGON32_LOGON_INTERACTIVE = 2,

            //'This logon type is intended for high performance servers to authenticate plaintext passwords.
            //'The LogonUser function does not cache credentials for this logon type.
            LOGON32_LOGON_NETWORK = 3,

            //'This logon type preserves the name and password in the authentication package, which allows the server to make 
            //'connections to other network servers while impersonating the client. A server can accept plaintext credentials 
            //'from a client, call LogonUser, verify that the user can access the system across the network, and still 
            //'communicate with other servers.
            //'NOTE: Windows NT:  This value is not supported. 
            LOGON32_LOGON_NETWORK_CLEARTEXT = 8,

            //'This logon type allows the caller to clone its current token and specify new credentials for outbound connections.
            //'The new logon session has the same local identifier but uses different credentials for other network connections. 
            //'NOTE: This logon type is supported only by the LOGON32_PROVIDER_WINNT50 logon provider.
            //'NOTE: Windows NT:  This value is not supported. 
            LOGON32_LOGON_NEW_CREDENTIALS = 9,

            //'Indicates a service-type logon. The account provided must have the service privilege enabled. 
            LOGON32_LOGON_SERVICE = 5,

            //'This logon type is for GINA DLLs that log on users who will be interactively using the computer. 
            //'This logon type can generate a unique audit record that shows when the workstation was unlocked. 
            LOGON32_LOGON_UNLOCK = 7
        }

        public enum LogonProvider : int
        {
            /// <summary>
            /// Use the standard logon provider for the system. 
            /// The default security provider is negotiate, unless you pass NULL for the domain name and the user name 
            /// is not in UPN format. In this case, the default provider is NTLM. 
            /// NOTE: Windows 2000/NT:   The default security provider is NTLM.
            /// </summary>
            LOGON32_PROVIDER_DEFAULT = 0,
            LOGON32_PROVIDER_WINNT50,
            LOGON32_PROVIDER_WINNT40
        }

        public static uint MAX_REG_KEYNAME_SIZE = ((102 * 1024) * 1024);           

        //Registry datatypes
        public static long REG_DWORD = 4;
        public static long REG_DWORD_BIG_ENDIAN = 5;

        private static WindowsImpersonationContext impersonatedUser = null;

        private static System.Security.AccessControl.RegistrySecurity regSecurity = null;

        private static string sHostName = string.Empty;

        #region ADVAPI32 Wrapper implementation
       
        // If you incorporate this code into a DLL, be sure to demand that it
        // runs with FullTrust.
        [PermissionSetAttribute(SecurityAction.Demand, Name = "FullTrust")]
        public static bool RegLogonUser(out IntPtr phToken, string userName, string domain, string passWord)
        {
            phToken = IntPtr.Zero;
            bool bResult = false;

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
            {
                try
                {
                    bResult = RegistryInteropWindows.LogonUser(
                                                userName,
                                                domain,
                                                passWord,
                                                (int)LogonType.LOGON32_LOGON_NEW_CREDENTIALS,
                                                (int)LogonProvider.LOGON32_PROVIDER_DEFAULT,
                                                ref phToken);
                    if (!bResult)
                    {
                        phToken = IntPtr.Zero;
                        int ret = Marshal.GetLastWin32Error();
                        Logger.Log("RegistryInteropWrapperWindows.RegLogonUser ret = {0}" + ret.ToString());
                        return bResult;
                    }

                    WindowsIdentity newId = new WindowsIdentity(phToken);
                    impersonatedUser = newId.Impersonate();

                    NTAccount ntAccount = new NTAccount(domain, userName);
                    IdentityReference identityReference = ntAccount.Translate(typeof(NTAccount));
                    regSecurity = new System.Security.AccessControl.RegistrySecurity();
                    System.Security.AccessControl.RegistryAccessRule accessRule =
                                                 new System.Security.AccessControl.RegistryAccessRule(
                                                 identityReference,
                                                 System.Security.AccessControl.RegistryRights.FullControl,
                                                 System.Security.AccessControl.InheritanceFlags.ContainerInherit,
                                                 System.Security.AccessControl.PropagationFlags.InheritOnly,
                                                 System.Security.AccessControl.AccessControlType.Allow);
                    regSecurity.SetAccessRule(accessRule);

                    Logger.Log("RegistryInteropWrapperWindows.RegLogonUser is successfull handle = {0}" + phToken.ToInt32());
                }
                catch (Exception ex)
                {
                    Logger.LogException("RegistryInteropWrapperWindows.RegLogonUser", ex);
                }
            }

            return bResult;
        }

        public static bool HandleClose(IntPtr tokenHandle)
        {
            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
            {
                if (impersonatedUser != null)
                    impersonatedUser.Undo();

                if (tokenHandle != IntPtr.Zero)
                {
                    bool iResult = RegistryInteropWindows.CloseHandle(tokenHandle);
                    if (!iResult)
                    {
                        tokenHandle = IntPtr.Zero;
                        int ret = Marshal.GetLastWin32Error();
                        Logger.Log("RegistryInteropWrapperWindows.RegLogonUser ret = {0}" + ret.ToString());
                    }
                    return iResult;
                }
            }

            return true;
        }        

        public static object RegGetValue(RegistryHive hive, string key, string sValue, out int type)
        {
            type = 0;
            if (string.IsNullOrEmpty(sHostName))
            {
                return null;
            }

            const int ErrorMoreDataIsAvailable = 234;
            byte[] buffer = null;
            int size = 0;
            IntPtr hKey = (IntPtr)0, hSubKey = (IntPtr)0, data = (IntPtr)0;

            if ((RegistryInteropWindows.RegConnectRegistry(RegistryInteropWrapperWindows.sHostName, hive, out hKey)) == 0)
            {
                try
                {
                    if ((RegistryInteropWindows.RegOpenKey(hKey, key, out hSubKey)) == 0)
                    {
                        buffer = new byte[512];
                        size = buffer.Length;
                        if ((RegistryInteropWindows.RegQueryValueEx(hSubKey, sValue, 0, out type, buffer, ref size)) == ErrorMoreDataIsAvailable)
                        {
                            // Resize buffer and perform query again
                            Array.Resize<byte>(ref buffer, size);
                            size = buffer.Length;
                            RegistryInteropWindows.RegQueryValueEx(hSubKey, sValue, 0, out type, buffer, ref size);
                        }
                    }
                }
                finally
                {
                    if ((int)hSubKey > 0)
                    {
                        // Attempt to dispose of key
                        RegistryInteropWindows.RegCloseKey(hSubKey);
                    }

                    if ((int)hKey > 0)
                    {
                        // Attempt to dispose of hive
                        RegistryInteropWindows.RegCloseKey(hKey);
                    }
                }
            }

            return buffer;
        }

        public static int RegSetValue(RegistryHive hive, string key, string sValue, object data)
        {
            int ret = -1;
            int cData;
            IntPtr hKey = (IntPtr)0, phSubKey = (IntPtr)0;

            if ((RegistryInteropWindows.RegConnectRegistry(RegistryInteropWrapperWindows.sHostName, hive, out hKey)) == 0)
            {
                try
                {
                    if ((RegistryInteropWindows.RegOpenKey(hKey, key, out phSubKey)) == 0)
                    {
                        byte[] buffer = data as byte[];
                        cData = buffer.Length;
                        ret = RegistryInteropWindows.RegSetValueEx(phSubKey, sValue, 0,
                                        RegistryValueKind.Unknown, buffer, cData);
                    }
                }
                finally
                {
                    if ((int)phSubKey > 0)
                    {
                        // Attempt to dispose of key
                        RegistryInteropWindows.RegCloseKey(phSubKey);
                    }

                    if ((int)hKey > 0)
                    {
                        // Attempt to dispose of hive
                        RegistryInteropWindows.RegCloseKey(hKey);
                    }
                }

                return ret;
            }

            return ret;
        }

        public static uint ApiRegGetKeySecurity(RegistryHive hive, string _sObjectname)
        {
            uint iRet = 0;

            Logger.Log(string.Format("RegistryInteropWrapperWindows.ApiRegGetKeySecurity(_sObjectname = {0})", _sObjectname), Logger.LogLevel.Verbose);

            IntPtr hKey = (IntPtr)0, phSubKey = (IntPtr)0;
            IntPtr pSecurityDescriptor = IntPtr.Zero;
            ulong lpcbSecurityDescriptor = 0;

            if ((RegistryInteropWindows.RegConnectRegistry(RegistryInteropWrapperWindows.sHostName, hive, out hKey)) == 0)
            {
                try
                {
                    if ((RegistryInteropWindows.RegOpenKey(hKey, _sObjectname, out phSubKey)) == 0)
                    {
                        iRet = RegistryInteropWindows.RegGetKeySecurity(phSubKey,
                                                     SecurityDescriptorApi.SECURITY_INFORMATION.OWNER_SECURITY_INFORMATION |
                                                     SecurityDescriptorApi.SECURITY_INFORMATION.GROUP_SECURITY_INFORMATION |
                                                     SecurityDescriptorApi.SECURITY_INFORMATION.DACL_SECURITY_INFORMATION |
                                                     SecurityDescriptorApi.SECURITY_INFORMATION.SACL_SECURITY_INFORMATION,
                                                     ref pSecurityDescriptor,
                                                     ref lpcbSecurityDescriptor);

                        if (iRet != 0)
                        {
                            Logger.Log(string.Format("RegistryInteropWrapperWindows.ApiRegGetKeySecurity returns error code; " + iRet), Logger.LogLevel.Verbose);
                            return iRet;
                        }
                    }
                }
                finally
                {
                    if ((int)phSubKey > 0)
                    {
                        // Attempt to dispose of key
                        RegistryInteropWindows.RegCloseKey(phSubKey);
                    }

                    if ((int)hKey > 0)
                    {
                        // Attempt to dispose of hive
                        RegistryInteropWindows.RegCloseKey(hKey);
                    }
                }
            }

            return iRet;
        }

        #endregion

        #region Win32.Registry Wrapper implementation

        public static void Win32RegOpenRemoteBaseKey(HKEY hKeyType, out RegistryKey Key)
        {
            RegistryKey hKey = null;
            string sHostname = string.Concat(@"\\", System.Environment.MachineName);
            sHostName = sHostname;

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
            {
                switch (hKeyType)
                {
                    case HKEY.HEKY_CURRENT_USER:
                        hKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.CurrentUser, sHostname);
                        break;

                    case HKEY.HKEY_CLASSES_ROOT:
                        hKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.ClassesRoot, sHostname);
                        break;

                    case HKEY.HKEY_CURRENT_CONFIG:
                        hKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.CurrentConfig, sHostname);
                        break;

                    case HKEY.HKEY_DYN_DATA:
                        hKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.DynData, sHostname);
                        break;

                    case HKEY.HKEY_LOCAL_MACHINE:
                        hKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, sHostname);
                        break;

                    case HKEY.HKEY_PERFORMENCE_DATA:
                        hKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.PerformanceData, sHostname);
                        break;

                    case HKEY.HKEY_USERS:
                        hKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.Users, sHostname);
                        break;

                    default:
                        break;
                }
                //hKey.SetAccessControl(regSecurity);
            }
            Key = hKey;
        }
        
        public static void Win32RegSubKeyList(RegistryKey hKey, out Array sKeys)
        {
            sKeys = null;

            string[] keys = hKey.GetSubKeyNames();
            if (keys != null && keys.Length != 0)
            {
                sKeys = Array.CreateInstance(typeof(string), keys.Length);
                keys.CopyTo(sKeys, 0);
            }
        }

        public static void Win32RegSubKeyValueList(RegistryKey hKey, out Array sValues)
        {
            sValues = null;

            string[] values = hKey.GetValueNames();
            if (values != null && values.Length != 0)
            {
                sValues = Array.CreateInstance(typeof(string), values.Length);
                values.CopyTo(sValues, 0);
            }
            else
            {
                object defaultValue = hKey.GetValue(hKey.Name, "", RegistryValueOptions.DoNotExpandEnvironmentNames);
                if (defaultValue != null)
                {
                    values = new string[] { defaultValue as string };
                }
            }
        }

        public static RegistryKey Win32RegOpenRemoteSubKey(RegistryKey hKey, string sSubKey)
        {
            RegistryKey sInnerKey = null;
            try
            {
                sInnerKey = hKey.OpenSubKey(sSubKey, true);           
            }
            catch
            {
                sInnerKey = null;
            }

            return sInnerKey;
        }

        public static void Win32RegValueKind(RegistryKey hKey, string sValue, SubKeyValueInfo keyValueInfo)
        {
            string sDataType = string.Empty;
            StringBuilder sbTemp = new StringBuilder();
            keyValueInfo.sData = string.Empty;
            keyValueInfo.sDataBuf = null;
            object sDataBuf = null;
            int type;
            RegistryValueKind valueKind = hKey.GetValueKind(sValue);

            switch (valueKind)
            {
                case RegistryValueKind.Unknown:
                    keyValueInfo.RegDataType = LWRegistryValueKind.REG_RESOURCE_LIST;
                    string[] sKey = hKey.ToString().Split(new char[] { '\\' } , 2);
                    sDataBuf = RegGetValue(GetRegistryHive(keyValueInfo.hKey), sKey[1], sValue, out type);
                    keyValueInfo.intDataType = type;
                    if (sDataBuf != null)
                    {
                        byte[] sBinaryData = sDataBuf as byte[];
                        foreach (byte byt in sBinaryData)
                        {
                            string stringValue = BitConverter.ToString(new byte[] { byt });
                            if (stringValue.Length == 1)
                                stringValue = "0" + stringValue;
                            sbTemp.Append(stringValue);
                            sbTemp.Append(" ");
                        }
                        keyValueInfo.sData = sbTemp.ToString();
                    }
                    break;

                case RegistryValueKind.Binary:
                    keyValueInfo.RegDataType = LWRegistryValueKind.REG_BINARY;
                    sDataBuf = hKey.GetValue(sValue, null, RegistryValueOptions.DoNotExpandEnvironmentNames);
                    if (sDataBuf != null)
                    {
                        byte[] sBinaryData = sDataBuf as byte[];
                        if (sBinaryData != null)
                        {
                            foreach (byte byt in sBinaryData)
                            {
                                string stringValue = BitConverter.ToString(new byte[] { byt });
                                if (stringValue.Length == 1)
                                    stringValue = "0" + stringValue;
                                sbTemp.Append(stringValue);
                                sbTemp.Append(" ");
                            }
                        }
                    }
                    keyValueInfo.sData = sbTemp.ToString();
                    break;

                case RegistryValueKind.DWord:
                    keyValueInfo.RegDataType = LWRegistryValueKind.REG_DWORD;
                    keyValueInfo.sDataBuf = hKey.GetValue(sValue, null, RegistryValueOptions.DoNotExpandEnvironmentNames);
                    if (keyValueInfo.sDataBuf != null)
                    {
                        string sTemp = keyValueInfo.sDataBuf.ToString();
                        sTemp = RegistryUtils.DecimalToBase((UInt32)Convert.ToInt32(keyValueInfo.sDataBuf), 16);
                        keyValueInfo.sData = string.Concat("0x", sTemp.PadLeft(8,'0'), "(", ((uint)Convert.ToInt32(keyValueInfo.sDataBuf)).ToString(), ")");
                    }
                    break;

                case RegistryValueKind.ExpandString:
                    keyValueInfo.RegDataType = LWRegistryValueKind.REG_EXPAND_SZ;
                    sDataBuf = hKey.GetValue(sValue, null, RegistryValueOptions.DoNotExpandEnvironmentNames);
                    if (sDataBuf != null)
                    {
                        keyValueInfo.sData = sDataBuf.ToString();
                    }
                    break;

                case RegistryValueKind.MultiString:
                    keyValueInfo.RegDataType = LWRegistryValueKind.REG_MULTI_SZ;
                    sDataBuf = hKey.GetValue(sValue, null, RegistryValueOptions.DoNotExpandEnvironmentNames);
                    if (sDataBuf != null)
                    {
                        string[] sStringData = sDataBuf as string[];
                        if (sStringData != null)
                        {
                            foreach (string sr in sStringData)
                            {
                                sbTemp.Append(sr);
                                sbTemp.Append(" ");
                            }
                        }
                    }
                    keyValueInfo.sData = sbTemp.ToString();
                    break;

                case RegistryValueKind.QWord:
                    keyValueInfo.RegDataType = LWRegistryValueKind.REG_QUADWORD;
                    sDataBuf = hKey.GetValue(sValue, null, RegistryValueOptions.DoNotExpandEnvironmentNames);
                    if (sDataBuf != null)
                    {
                        string sTemp = sDataBuf.ToString();
                        sTemp = RegistryUtils.DecimalToBase((UInt64)Convert.ToInt64(sDataBuf), 16);
                        keyValueInfo.sData = string.Concat("0x", sTemp.PadLeft(16,'0'), "(", ((ulong)Convert.ToInt64(sDataBuf)).ToString(), ")");
                    }
                    break;

                case RegistryValueKind.String:
                    keyValueInfo.RegDataType = LWRegistryValueKind.REG_SZ;
                    sDataBuf = hKey.GetValue(sValue, null, RegistryValueOptions.DoNotExpandEnvironmentNames);
                    if (sDataBuf != null)
                    {
                        keyValueInfo.sData = sDataBuf.ToString();
                    }
                    break;              
               
                default:
                    keyValueInfo.RegDataType = LWRegistryValueKind.REG_NONE;               
                    sDataBuf = hKey.GetValue(sValue, null, RegistryValueOptions.DoNotExpandEnvironmentNames);
                    if (sDataBuf != null)                 
                        keyValueInfo.sData = sDataBuf.ToString();                   
                    else
                        keyValueInfo.sData = "(zero-length binary value)";  
                    break;
            }
        }
        
        public static void Win32RegKeyValueData(RegistryKey hKey, string sValue, out object DataBuf)
        {
            DataBuf = hKey.GetValue(sValue, null, RegistryValueOptions.DoNotExpandEnvironmentNames);
        }

        public static LWRegistryValueKind Win32RegKeyValueKind(RegistryKey hKey, string sValue)
        {
            try
            {
                RegistryValueKind valueKind = hKey.GetValueKind(sValue);

                switch (valueKind)
                {
                    case RegistryValueKind.Binary:
                        return LWRegistryValueKind.REG_BINARY;
                    case RegistryValueKind.String:
                        return LWRegistryValueKind.REG_SZ;
                    case RegistryValueKind.DWord:
                        return LWRegistryValueKind.REG_DWORD;
                    case RegistryValueKind.ExpandString:
                        return LWRegistryValueKind.REG_EXPAND_SZ;
                    case RegistryValueKind.Unknown:
                        return LWRegistryValueKind.REG_RESOURCE_LIST;
                    case RegistryValueKind.MultiString:
                        return LWRegistryValueKind.REG_MULTI_SZ;
                    case RegistryValueKind.QWord:
                        return LWRegistryValueKind.REG_QUADWORD;
                    default:
                        return LWRegistryValueKind.REG_BINARY;
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("Win32RegKeyValueKind : ", ex);
                return LWRegistryValueKind.REG_SZ;
            }
        }

        public static RegistryKey Win32CreateSubKey(RegistryKey hKey, string Keyname)
        {
            RegistryKey subKey = null;

            try
            {
                subKey = hKey.CreateSubKey(Keyname, RegistryKeyPermissionCheck.ReadWriteSubTree);
            }
            catch(Exception ex)
            {
                Logger.LogException("Win32CreateSubKey : ", ex);
            }

            return subKey;
        }

        public static int Win32DeleteSubKeyValue(RegistryKey hKey, string value)
        {
            int iRet = 0;

            try
            {
                hKey.DeleteValue(value, true);
            }
            catch (Exception ex)
            {
                iRet = -1;
                Logger.LogException("Win32DeleteSubKeyValue : ", ex);
            }

            return iRet;
        }

        public static int Win32DeleteSubKey(RegistryKey parentKey, string key)
        {
            int iRet = 0;

            try
            {
                parentKey.DeleteSubKey(key, true);
            }
            catch (Exception ex)
            {
                iRet = -1;
                Logger.LogException("Win32DeleteSubKeyValue : ", ex);
            }

            return iRet;
        }

        public static int Win32DeleteSubKeyTree(RegistryKey parentKey, string key)
        {
            int iRet = 0;

            try
            {
                parentKey.DeleteSubKeyTree(key);
            }
            catch (Exception ex)
            {
                iRet = -1;
                Logger.LogException("Win32DeleteSubKeyValue : ", ex);
            }

            return iRet;
        }

        public static bool Win32AddSubKeyValue(SubKeyValueInfo valueInfo)
        {
            try
            {
                string[] sValues = valueInfo.sParentKey.GetValueNames();

                foreach (string sValue in sValues)
                    if (sValue.Equals(valueInfo.sValue))
                        return false;

                return Win32ModifySubKeyValue(valueInfo);
            }
            catch (Exception ex)
            {
                Logger.LogException("Win32CreateSubKey : ", ex);
                return false;
            }
        }

        public static bool Win32CheckDuplicateValue(object key, string valueName)
        {
            RegistryKey Key = key as RegistryKey;

            object value = Key.GetValue(valueName);

            if (value != null)
            {
                return true;
            }

            return false;
        }

        public static bool Win32CheckDuplicateKey(object key, string keyName)
        {
            RegistryKey Key = key as RegistryKey;

            try
            {
                Key = Key.OpenSubKey(keyName);
                if (Key != null)
                    return true;
            }
            catch
            {
                return true;
            }

            return false;
        }

        public static bool Win32ModifySubKeyValue(SubKeyValueInfo valueInfo)
        {
            try
            {
                RegistryKey subKey = valueInfo.sParentKey;

                switch (valueInfo.RegDataType)
                {
                    case LWRegistryValueKind.REG_BINARY:
                        subKey.SetValue(valueInfo.sValue,
                                valueInfo.sDataBuf, RegistryValueKind.Binary);
                        break;

                    case LWRegistryValueKind.REG_DWORD:
                        //Use sDataBuf
                        subKey.SetValue(valueInfo.sValue,
                                valueInfo.sDataBuf, RegistryValueKind.DWord);
                        break;

                    case LWRegistryValueKind.REG_EXPAND_SZ:
                        //Use sData
                        subKey.SetValue(valueInfo.sValue,
                                valueInfo.sData, RegistryValueKind.ExpandString);
                        break;

                    case LWRegistryValueKind.REG_QUADWORD:
                        subKey.SetValue(valueInfo.sValue,
                                valueInfo.sDataBuf, RegistryValueKind.QWord);
                        break;

                    case LWRegistryValueKind.REG_SZ:
                        //Use sData
                        subKey.SetValue(valueInfo.sValue,
                                valueInfo.sData, RegistryValueKind.String);
                        break;
                    case LWRegistryValueKind.REG_MULTI_SZ:
                        //Use sDataBuf
                        subKey.SetValue(valueInfo.sValue,
                                valueInfo.sDataBuf, RegistryValueKind.MultiString);
                        break;

                    case LWRegistryValueKind.REG_RESOURCE_LIST:
                    case LWRegistryValueKind.REG_RESOURCE_REQUIREMENTS_LIST:
                    case LWRegistryValueKind.REG_FULL_RESOURCE_DESCRIPTOR:
                        //Use sDataBuf
                        //subKey.SetValue(valueInfo.sValue,
                        //        valueInfo.sDataBuf, RegistryValueKind.Unknown);
                        string[] sKey = valueInfo.sParentKey.Name.Split(new char[] { '\\' } , 2);
                        RegSetValue(GetRegistryHive(valueInfo.hKey), sKey[1], valueInfo.sValue, valueInfo.sDataBuf);
                        break;

                    case LWRegistryValueKind.REG_NONE:
                        subKey.SetValue(valueInfo.sValue,
                                valueInfo.sData, RegistryValueKind.Unknown);
                        break;
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("Win32CreateSubKey : ", ex); 

                return false;
            }

            return true;
        }

        public static RegistryHive GetRegistryHive(HKEY hKey)
        {
            RegistryHive hive;

            switch (hKey)
            {
                case HKEY.HKEY_CLASSES_ROOT:
                    hive = RegistryHive.ClassesRoot;
                    break;

                case HKEY.HKEY_LOCAL_MACHINE:
                    hive = RegistryHive.LocalMachine;
                    break;

                case HKEY.HEKY_CURRENT_USER:
                    hive = RegistryHive.CurrentUser;
                    break;

                case HKEY.HKEY_CURRENT_CONFIG:
                    hive = RegistryHive.CurrentConfig;
                    break;

                case HKEY.HKEY_DYN_DATA:
                    hive = RegistryHive.DynData;
                    break;

                case HKEY.HKEY_PERFORMENCE_DATA:
                    hive = RegistryHive.PerformanceData;
                    break;

                case HKEY.HKEY_USERS:
                    hive = RegistryHive.Users;
                    break;

                default:
                    hive = RegistryHive.LocalMachine;
                    break;
            }

            return hive;
        }

        #endregion

        #region Helper functions 
       
        #endregion
    }
}
