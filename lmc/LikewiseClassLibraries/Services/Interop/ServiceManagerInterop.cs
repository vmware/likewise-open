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

namespace Likewise.LMC.Services
{
    class ServiceManagerInterop
    {
        private const string advapiDllPath = "liblwsm.dll";

        //DWORD
        //LwSmAcquireServiceHandle(
        //        PCWSTR pwszName,
        //        PLW_SERVICE_HANDLE phHandle);       
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern int LwSmAcquireServiceHandle(
                [MarshalAs(UnmanagedType.LPWStr)] string pName,
                IntPtr phHandle);

        //VOID
        //LwSmReleaseServiceHandle(
        //    LW_SERVICE_HANDLE hHandle
        //    );       
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern int LwSmReleaseServiceHandle(IntPtr phHandle);

        //VOID
        //LwSmStartService(
        //    LW_SERVICE_HANDLE hHandle
        //    );       
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern int LwSmStartService(IntPtr phHandle);

        //VOID
        //LwSmStopService(
        //    LW_SERVICE_HANDLE hHandle
        //    );       
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern int LwSmStopService(IntPtr phHandle);

        //VOID
        //LwSmRefreshService(
        //    LW_SERVICE_HANDLE hHandle
        //    );       
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern int LwSmRefreshService(IntPtr phHandle);

        //VOID
        //LwSmQueryServiceStatus(
        //    LW_SERVICE_HANDLE hHandle
        //    );       
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern int LwSmQueryServiceStatus(IntPtr phHandle, IntPtr pStatus);

        //DWORD
        //LwSmSrvEnumerateServices(
        //    PWSTR** pppwszServiceNames
        //    );      
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern int LwSmEnumerateServices([MarshalAs(UnmanagedType.LPWStr)] out string[] pppwszServiceNames);

        //DWORD
        //LwSmSrvGetServiceStatus(
        //    LW_SERVICE_HANDLE hHandle,
        //    PLW_SERVICE_STATUS pStatus
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern void LwSmSrvGetServiceStatus(IntPtr phHandle, IntPtr pStatus);

        //DWORD
        //LwSmSrvStartService(
        //    LW_SERVICE_HANDLE hHandle
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern void LwSmSrvStartService(IntPtr phHandle);

        //DWORD
        //LwSmSrvStopService(
        //    LW_SERVICE_HANDLE hHandle
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern void LwSmSrvStopService(IntPtr phHandle);

        //DWORD
        //LwSmQueryServiceInfo(
        //    LW_SERVICE_HANDLE hHandle
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern int LwSmQueryServiceInfo(IntPtr phHandle, out IntPtr ppInfo);

        //DWORD
        //LwSmSrvGetServiceInfo(
        //    LW_SERVICE_HANDLE hHandle,
        //    PLW_SERVICE_INFO* ppInfo
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern uint LwSmSrvGetServiceInfo(IntPtr phHandle, IntPtr ppInfo);

        //LWMsgDispatchSpec*
        //LwSmGetDispatchSpec(
        //    void
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern IntPtr LwSmGetDispatchSpec();

        //DWORD
        //LwSmTableGetEntry(
        //    PCWSTR pwszName,
        //    PSM_TABLE_ENTRY* ppEntry
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern IntPtr LwSmTableGetEntry([MarshalAs(UnmanagedType.LPWStr)] string pwszName,
                                    IntPtr ppEntry);

        //DWORD
        //LwSmTableEnumerateEntries(
        //    PWSTR** pppwszServiceNames
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern IntPtr LwSmTableEnumerateEntries([MarshalAs(UnmanagedType.LPWStr)] string[] pppwszServiceNames);

        //VOID
        //LwSmTableRetainEntry(
        //    PSM_TABLE_ENTRY pEntry
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern IntPtr LwSmTableRetainEntry(IntPtr pEntry);

        //VOID
        //LwSmTableReleaseEntry(
        //    PSM_TABLE_ENTRY pEntry
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern IntPtr LwSmTableReleaseEntry(IntPtr pEntry);

        //DWORD
        //LwSmTableStartEntry(
        //    PSM_TABLE_ENTRY pEntry
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern IntPtr LwSmTableStartEntry(IntPtr pEntry);

        //DWORD
        //LwSmTableStopEntry(
        //    PSM_TABLE_ENTRY pEntry
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern IntPtr LwSmTableStopEntry(IntPtr pEntry);

        //VOID
        //LwSmTableNotifyEntryChanged(
        //    PSM_TABLE_ENTRY pEntry
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern IntPtr LwSmTableNotifyEntryChanged(IntPtr pEntry);

        //DWORD
        //LwSmTableWaitEntryChanged(
        //    PSM_TABLE_ENTRY pEntry
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern IntPtr LwSmTableWaitEntryChanged(IntPtr pEntry);

        //DWORD
        //LwSmTableGetEntryStatus(
        //    PSM_TABLE_ENTRY pEntry,
        //    PLW_SERVICE_STATUS pStatus
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern IntPtr LwSmTableGetEntryStatus(IntPtr pEntry, IntPtr pStatus);

        //DWORD
        //LwSmRegistryEnumServices(
        //    HANDLE hReg,
        //    PWSTR** pppwszNames
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern IntPtr LwSmRegistryEnumServices([MarshalAs(UnmanagedType.LPWStr)] out string[] pppwszNames);

        //DWORD
        //LwSmRegistryReadServiceInfo(
        //    HANDLE hReg,
        //    PCWSTR pwszName,
        //    PLW_SERVICE_INFO* ppInfo
        //    );
        [DllImport(advapiDllPath, SetLastError = true)]
        public static extern uint LwSmRegistryEnumServices(IntPtr hReg,
                            [MarshalAs(UnmanagedType.LPWStr)] string pwszName,
                            IntPtr ppInfo);

    }
}
