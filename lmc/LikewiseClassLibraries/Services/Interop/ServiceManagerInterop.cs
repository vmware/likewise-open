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
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    struct _LW_SERVICE_INFO
    {
        [MarshalAs(UnmanagedType.LPWStr)]
        public string pwszName;
        [MarshalAs(UnmanagedType.LPWStr)]
        public string pwszDescription;
        public ServiceManagerApi.LW_SERVICE_TYPE type;
        [MarshalAs(UnmanagedType.LPWStr)]
        public string pwszPath;
        public IntPtr ppwszArgs;
        public IntPtr ppwszDependencies;
        [MarshalAs(UnmanagedType.Bool)]
        public bool bAutostart;
    }

    class ServiceManagerInterop
    {
        private const string advapiDllPath = "liblwsm.dll";

        public static IntPtr MarshalStringList(string[] values)
        {
            if (values == null)
            {
                return IntPtr.Zero;
            }
            else
            {
                IntPtr arrPtr = Marshal.AllocHGlobal((values.Length + 1) * IntPtr.Size);

                for (int i = 0; i < values.Length; i++)
                {
                    Marshal.WriteIntPtr(arrPtr, i * IntPtr.Size, Marshal.StringToHGlobalUni(values[i]));
                }

                Marshal.WriteIntPtr(arrPtr, values.Length * IntPtr.Size, IntPtr.Zero);

                return arrPtr;
            }
        }

        public static string[] UnmarshalStringList(IntPtr pNativeData)
        {
            if (pNativeData == IntPtr.Zero)
            {
                return null;
            }
            else
            {
                string[] res = null;
                int len = 0;
                int i = 0;

                for (len = 0; Marshal.ReadIntPtr(pNativeData, len * IntPtr.Size) != IntPtr.Zero; len++);

                res = new string[len];

                for (i = 0; i < len; i++)
                {
                    res[i] = Marshal.PtrToStringUni(Marshal.ReadIntPtr(pNativeData, i * IntPtr.Size));
                }

                return res;
            }
        }

        public static ServiceManagerApi.LW_SERVICE_INFO UnmarshalServiceInfo(IntPtr pNativeData)
        {
            _LW_SERVICE_INFO _info = (_LW_SERVICE_INFO) Marshal.PtrToStructure(pNativeData, typeof(_LW_SERVICE_INFO));
            ServiceManagerApi.LW_SERVICE_INFO info = new ServiceManagerApi.LW_SERVICE_INFO();

            info.type = _info.type;
            info.pwszName = _info.pwszName;
            info.pwszPath = _info.pwszPath;
            info.pwszDescription = _info.pwszDescription;
            info.bAutostart = _info.bAutostart;
            info.ppwszArgs = ServiceManagerInterop.UnmarshalStringList(_info.ppwszArgs);
            info.ppwszDependencies = ServiceManagerInterop.UnmarshalStringList(_info.ppwszDependencies);

            return info;
        }

        public static IntPtr MarshalServiceInfo(object ManagedObj)
        {
            ServiceManagerApi.LW_SERVICE_INFO info = (ServiceManagerApi.LW_SERVICE_INFO) ManagedObj;
            _LW_SERVICE_INFO _info = new _LW_SERVICE_INFO();
            IntPtr infoPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(_LW_SERVICE_INFO)));

            _info.pwszName = info.pwszName;
            _info.pwszPath = info.pwszPath;
            _info.pwszDescription = info.pwszDescription;
            _info.bAutostart = info.bAutostart;
            _info.ppwszArgs = ServiceManagerInterop.MarshalStringList(info.ppwszArgs);
            _info.ppwszDependencies = ServiceManagerInterop.MarshalStringList(info.ppwszDependencies);

            Marshal.StructureToPtr(_info, infoPtr, false);

            return infoPtr;
        }


        //DWORD
        //LwSmAcquireServiceHandle(
        //        PCWSTR pwszName,
        //        PLW_SERVICE_HANDLE phHandle);
        [DllImport(advapiDllPath)]
        public static extern int LwSmAcquireServiceHandle(
                [MarshalAs(UnmanagedType.LPWStr)] string pName,
                out IntPtr phHandle);

        //VOID
        //LwSmReleaseServiceHandle(
        //    LW_SERVICE_HANDLE hHandle
        //    );
        [DllImport(advapiDllPath)]
        public static extern int LwSmReleaseServiceHandle(IntPtr phHandle);

        //VOID
        //LwSmStartService(
        //    LW_SERVICE_HANDLE hHandle
        //    );
        [DllImport(advapiDllPath)]
        public static extern int LwSmStartService(IntPtr phHandle);

        //VOID
        //LwSmStopService(
        //    LW_SERVICE_HANDLE hHandle
        //    );
        [DllImport(advapiDllPath)]
        public static extern int LwSmStopService(IntPtr phHandle);

        //VOID
        //LwSmRefreshService(
        //    LW_SERVICE_HANDLE hHandle
        //    );
        [DllImport(advapiDllPath)]
        public static extern int LwSmRefreshService(IntPtr phHandle);

        //VOID
        //LwSmQueryServiceStatus(
        //    LW_SERVICE_HANDLE hHandle
        //    );
        [DllImport(advapiDllPath)]
        public static extern int LwSmQueryServiceStatus(IntPtr phHandle, out ServiceManagerApi.LW_SERVICE_STATUS pStatus);

        //DWORD
        //LwSmSrvEnumerateServices(
        //    PWSTR** pppwszServiceNames
        //    );
        [DllImport(advapiDllPath)]
        public static extern int LwSmEnumerateServices(
            [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(ServiceNameListMarshaler))]
            out string[] pppwszServiceNames);

        [DllImport(advapiDllPath)]
        public static extern void LwSmFreeServiceNameList(IntPtr pList);

        [DllImport(advapiDllPath)]
        public static extern void LwSmFreeServiceInfo(IntPtr pList);

        //DWORD
        //LwSmQueryServiceInfo(
        //    LW_SERVICE_HANDLE hHandle
        //    );
        [DllImport(advapiDllPath)]
        public static extern int LwSmQueryServiceInfo(
            IntPtr phHandle,
            out IntPtr ppInfo);

        //DWORD
        //LwSmQueryServiceDependencyClosure(
        //    LW_SERVICE_HANDLE hHandle,
        //    PWSTR** pppwszServiceList
        //    )
        [DllImport(advapiDllPath)]
        public static extern int LwSmQueryServiceDependencyClosure(
            IntPtr phHandle,
		    [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(ServiceNameListMarshaler))]
            out string[] ppInfo);

		//DWORD
		//LwSmQueryServiceReverseDependencyClosure(
		//    LW_SERVICE_HANDLE hHandle,
		//    PWSTR** pppwszServiceList
		//    );
		[DllImport(advapiDllPath)]
        public static extern int LwSmQueryServiceReverseDependencyClosure(
            IntPtr phHandle,
		    [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(ServiceNameListMarshaler))]
            out string[] pppwszServiceList);

    }

    class ServiceNameListMarshaler: ICustomMarshaler
    {
        static ICustomMarshaler GetInstance(string cookie)
        {
            return new ServiceNameListMarshaler();
        }

        void ICustomMarshaler.CleanUpManagedData (object ManagedObj)
        {
            return;
        }

        void ICustomMarshaler.CleanUpNativeData (IntPtr pNativeData)
        {
            ServiceManagerInterop.LwSmFreeServiceNameList(pNativeData);
        }

        int ICustomMarshaler.GetNativeDataSize ()
        {
            return Marshal.SizeOf(typeof(IntPtr));
        }

        IntPtr ICustomMarshaler.MarshalManagedToNative (object ManagedObj)
        {
            return ServiceManagerInterop.MarshalStringList((string[]) ManagedObj);
        }

        object ICustomMarshaler.MarshalNativeToManaged (IntPtr pNativeData)
        {
            return ServiceManagerInterop.UnmarshalStringList(pNativeData);
        }
    }

    class ServiceInfoMarshaler: ICustomMarshaler
    {
        static ICustomMarshaler GetInstance(string cookie)
        {
            return new ServiceInfoMarshaler();
        }

        void ICustomMarshaler.CleanUpManagedData (object ManagedObj)
        {
            return;
        }

        void ICustomMarshaler.CleanUpNativeData (IntPtr pNativeData)
        {
            ServiceManagerInterop.LwSmFreeServiceInfo(pNativeData);
        }

        int ICustomMarshaler.GetNativeDataSize ()
        {
            return IntPtr.Size;
        }

        IntPtr ICustomMarshaler.MarshalManagedToNative (object ManagedObj)
        {
            return ServiceManagerInterop.MarshalServiceInfo(ManagedObj);
        }

        object ICustomMarshaler.MarshalNativeToManaged (IntPtr pNativeData)
        {
            return ServiceManagerInterop.UnmarshalServiceInfo(pNativeData);
        }
    }
}
