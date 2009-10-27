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
    public class NetlogonAPI
    {
        #region definitions

        private const string NETLOGON_DLL_PATH = "libnetapi.dll";

        #endregion

        #region structures

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct POSIX_TIME
        {
              public int tm_sec;   /* Seconds.     [0-60] (1 leap second) */
              public int tm_min;   /* Minutes.     [0-59] */
              public int tm_hour;  /* Hours.       [0-23] */
              public int tm_mday;  /* Day.         [1-31] */
              public int tm_mon;   /* Month.       [0-11] */
              public int tm_year;  /* Year - 1900.  */
              public int tm_wday;  /* Day of week. [0-6] */
              public int tm_yday;  /* Days in year.[0-365] */
              public int tm_isdst; /* DST.         [-1/0/1]*/
        }


        #endregion

        #region api

        [DllImport(NETLOGON_DLL_PATH)]
        public static extern uint LWNetGetDCName(
            string serverFQDN,
            string domainFQDN,
            string siteName,
            uint flags,
            out IntPtr pDCInfo
            );

        [DllImport(NETLOGON_DLL_PATH)]
        public static extern uint LWNetGetDomainController(
            string domainFQDN,
            out string domainControllerFQDN
            );

        [DllImport(NETLOGON_DLL_PATH)]
        public static extern uint LWNetGetDCTime(
            string domainFQDN,
            out POSIX_TIME DCTime
            );

        [DllImport(NETLOGON_DLL_PATH)]
        public static extern uint LWNetGetCurrentDomain(
            out string domainFQDN
            );


        #endregion
      
    }
}
