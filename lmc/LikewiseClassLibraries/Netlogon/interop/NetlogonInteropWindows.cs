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
using System.Runtime.InteropServices;
using Likewise.LMC.Netlogon;

namespace Likewise.LMC.Netlogon.Interop
{
    public class NetlogonWindowsAPI
    {
        #region definitions

        private const string NETLOGON_DLL_PATH = "netapi32.dll";
        #endregion

        #region structures

        [StructLayout(LayoutKind.Sequential)]
        public struct TIME_OF_DAY_INFO_WINDOWS
        {
              public uint tod_elapsedt;
              public uint tod_msecs;
              public uint tod_hours;
              public uint tod_mins;
              public uint tod_secs;
              public uint tod_hunds;
              public long tod_timezone;
              public uint tod_tinterval;
              public uint tod_day;
              public uint tod_month;
              public uint tod_year;
              public uint tod_weekday;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct DOMAIN_CONTROLLER_INFO
        {
            [MarshalAs(UnmanagedType.LPTStr)]
            public string DomainControllerName;
            [MarshalAs(UnmanagedType.LPTStr)]
            public string DomainControllerAddress;
            public uint DomainControllerAddressType;
            public Guid DomainGuid;
            [MarshalAs(UnmanagedType.LPTStr)]
            public string DomainName;
            [MarshalAs(UnmanagedType.LPTStr)]
            public string DnsForestName;
            public uint Flags;
            [MarshalAs(UnmanagedType.LPTStr)]
            public string DcSiteName;
            [MarshalAs(UnmanagedType.LPTStr)]
            public string ClientSiteName;
        }

        #endregion

        #region api

        [DllImport(NETLOGON_DLL_PATH, CharSet = CharSet.Auto, SetLastError = true)]
        public static extern int DsGetDcName
        (
            [MarshalAs(UnmanagedType.LPTStr)]
            string ComputerName,
            [MarshalAs(UnmanagedType.LPTStr)]
            string DomainName,
            [In] IntPtr DomainGuid,
            [MarshalAs(UnmanagedType.LPTStr)]
            string SiteName,
            int Flags,
            out IntPtr pDomainControllerInfo
        );

        [DllImport(NETLOGON_DLL_PATH)]
        public static extern uint NetRemoteTOD(
              string UncServerName,
              out IntPtr BufPtr
        );

        [DllImport(NETLOGON_DLL_PATH)]
        public static extern uint NetApiBufferFree(
             IntPtr Buffer
             );

        #endregion

    }
}
