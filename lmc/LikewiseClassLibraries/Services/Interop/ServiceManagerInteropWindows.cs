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

namespace Likewise.LMC.Services
{
    class ServiceManagerInteropWindows
    {       
        private const string advapiDllPath = "advapi32.dll";

        public const long SERVICE_STATE_ALL = 0x00000003;//&H3;
        //'Service Types (Bit Mask)
        //'corresponds to SERVICE_STATUS.dwServiceType
        public static long SERVICE_KERNEL_DRIVER = 0x00000001;//&H1;
        public static long SERVICE_FILE_SYSTEM_DRIVER = 0x00000002;//&H2;
        public static long SERVICE_ADAPTER = 0x00000004;//&H4;
        public static long SERVICE_RECOGNIZER_DRIVER = 0x00000008;//&H8;
        public static long SERVICE_WIN32_OWN_PROCESS = 0x00000010;//&H10;
        public static long SERVICE_WIN32_SHARE_PROCESS = 0x00000020;//&H20;
        public static long SERVICE_INTERACTIVE_PROCESS = 0x00000100;//&H100;

        public static long SERVICE_WIN32 = SERVICE_WIN32_OWN_PROCESS |
                                          SERVICE_WIN32_SHARE_PROCESS;

        public static long SERVICE_DRIVER = SERVICE_KERNEL_DRIVER |
                                           SERVICE_FILE_SYSTEM_DRIVER |
                                           SERVICE_RECOGNIZER_DRIVER;

        public static long SERVICE_TYPE_ALL = SERVICE_WIN32 |
                                             SERVICE_ADAPTER |
                                             SERVICE_DRIVER |
                                             SERVICE_INTERACTIVE_PROCESS;

        public static uint SERVICE_NO_CHANGE = 0xffffffff; //this value is found in winsvc.h
        public static uint SERVICE_QUERY_CONFIG = 0x00000001;
        public static uint SERVICE_CHANGE_CONFIG = 0x00000002;
        public static uint SERVICE_QUERY_STATUS = 0x00000004;
        public static uint SERVICE_ENUMERATE_DEPENDENTS = 0x00000008;
        public static uint SERVICE_START = 0x00000010;
        public static uint SERVICE_STOP = 0x00000020;
        public static uint SERVICE_PAUSE_CONTINUE = 0x00000040;
        public static uint SERVICE_INTERROGATE = 0x00000080;
        public static uint SERVICE_USER_DEFINED_CONTROL = 0x00000100;
        public static uint STANDARD_RIGHTS_REQUIRED = 0x000F0000;
        public static uint SERVICE_ALL_ACCESS = (STANDARD_RIGHTS_REQUIRED | SERVICE_QUERY_CONFIG |
                            SERVICE_CHANGE_CONFIG |
                            SERVICE_QUERY_STATUS |
                            SERVICE_ENUMERATE_DEPENDENTS |
                            SERVICE_START |
                            SERVICE_STOP |
                            SERVICE_PAUSE_CONTINUE |
                            SERVICE_INTERROGATE |
                            SERVICE_USER_DEFINED_CONTROL);

        public static Int32 SC_MANAGER_ALL_ACCESS = 0x000F003F;        
        public static UInt32 SERVICE_CONFIG_DESCRIPTION = 0x01;
        public static UInt32 SERVICE_CONFIG_FAILURE_ACTIONS = 0x02;

        [DllImport(advapiDllPath, EntryPoint = "OpenSCManagerW", ExactSpelling = true, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern IntPtr OpenSCManager(string machineName, string databaseName, uint dwAccess);

        [DllImport(advapiDllPath, EntryPoint = "EnumServicesStatusW", ExactSpelling = true, SetLastError = true)]
        public static extern bool EnumServicesStatus(IntPtr hSCManager,
                            IntPtr dwServiceType,
                            ServiceManagerApi.SERVICE_STATE dwServiceState,
                            IntPtr lpServices,
                            int cbBufSize,
                            ref int pcbBytesNeeded,
                            ref int lpServicesReturned,
                            ref int lpResumeHandle);

        [DllImport(advapiDllPath, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool CloseServiceHandle(IntPtr hSCObject);

        //The ChangeServiceConfig function changes the configuration parameters of a service. 
        //To change the optional configuration parameters, use the ChangeServiceConfig2 function. 
        //The ChangeServiceConfig function changes the configuration information for the specified 
        //service in the service control manager database. You can obtain the current configuration 
        //information by using the QueryServiceConfig function.

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern Boolean ChangeServiceConfig(IntPtr hService, UInt32 nServiceType, UInt32 nStartType, UInt32 nErrorControl, String lpBinaryPathName, String lpLoadOrderGroup, IntPtr lpdwTagId, String lpDependencies, String lpServiceStartName, String lpPassword, String lpDisplayName);

        //Correction:
        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern Boolean ChangeServiceConfig(IntPtr hService, UInt32 nServiceType, UInt32 nStartType, UInt32 nErrorControl, String lpBinaryPathName, String lpLoadOrderGroup, IntPtr lpdwTagId, [In] char[] lpDependencies, String lpServiceStartName, String lpPassword, String lpDisplayName);

        [DllImport(advapiDllPath, SetLastError = true, CharSet = CharSet.Auto)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool ChangeServiceConfig2(
            IntPtr hService,
            int dwInfoLevel,
            IntPtr lpInfo);      

        [DllImport(advapiDllPath, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool ControlService(IntPtr hService, ServiceManagerApi.SERVICE_CONTROL dwControl, ref IntPtr lpServiceStatus);

        [DllImport(advapiDllPath, SetLastError = true, CharSet = CharSet.Auto)]
        public static extern IntPtr CreateService(
            IntPtr hSCManager,
            string lpServiceName,
            string lpDisplayName,
            uint dwDesiredAccess,
            uint dwServiceType,
            uint dwStartType,
            uint dwErrorControl,
            string lpBinaryPathName,
            string lpLoadOrderGroup,
            string lpdwTagId,
            string lpDependencies,
            string lpServiceStartName,
            string lpPassword);

        [DllImport(advapiDllPath, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool DeleteService(IntPtr hService);

        //The EnumDependentServices function retrieves the name and status of each service that depends on the specified service; 
        //that is, the specified service must be running before the dependent services can run.
        [DllImport(advapiDllPath, EntryPoint = "EnumDependentServicesW", ExactSpelling = true, SetLastError = true)]
        public static extern bool EnumDependentServices(IntPtr hService,
                               IntPtr dwServiceState,
                               ref IntPtr lpServices,
                               int cbBufSize,
                               ref int pcbBytesNeeded,
                               ref int lpServicesReturned);

        [DllImport(advapiDllPath, EntryPoint = "EnumServicesStatusW", ExactSpelling = true, SetLastError = true)]
        public static extern bool EnumServicesStatus(IntPtr hSCManager,
                            IntPtr dwServiceType,
                            IntPtr dwServiceState,
                            IntPtr lpServices,
                            int cbBufSize,
                            ref int pcbBytesNeeded,
                            ref int lpServicesReturned,
                            ref int lpResumeHandle);

        [DllImport(advapiDllPath, SetLastError = true, CharSet = CharSet.Auto)]
        public static extern IntPtr OpenService(IntPtr hSCManager, string lpServiceName, uint dwDesiredAccess);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern Boolean QueryServiceConfig(IntPtr hService, IntPtr intPtrQueryConfig, UInt32 cbBufSize, out UInt32 pcbBytesNeeded);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true, EntryPoint = "QueryServiceConfig2W")]
        public static extern Boolean QueryServiceConfig2(IntPtr hService, UInt32 dwInfoLevel, IntPtr buffer, UInt32 cbBufSize, out UInt32 pcbBytesNeeded);

        //The QueryServiceObjectSecurity function retrieves a copy of the security descriptor associated with a service object. 
        //You can also use the GetNamedSecurityInfo function to retrieve a security descriptor.
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern bool QueryServiceObjectSecurity(IntPtr serviceHandle, System.Security.AccessControl.SecurityInfos secInfo, ref IntPtr lpSecDesrBuf, uint bufSize, out uint bufSizeNeeded);

        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern bool QueryServiceObjectSecurity(SafeHandle serviceHandle, System.Security.AccessControl.SecurityInfos secInfo, byte[] lpSecDesrBuf, uint bufSize, out uint bufSizeNeeded);

        [DllImport(advapiDllPath, EntryPoint = "QueryServiceStatus", CharSet = CharSet.Auto)]
        public static extern bool QueryServiceStatus(IntPtr hService, ref IntPtr dwServiceStatus);

        [DllImport(advapiDllPath, CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern bool QueryServiceStatusEx(IntPtr serviceHandle, int infoLevel, IntPtr buffer, int bufferSize, out int bytesNeeded);

        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern bool SetServiceObjectSecurity(SafeHandle serviceHandle, System.Security.AccessControl.SecurityInfos secInfos, byte[] lpSecDesrBuf);

        [DllImport(advapiDllPath, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool StartService(
                        IntPtr hService,
                    int dwNumServiceArgs,
                    string[] lpServiceArgVectors
                    );
    }
}
