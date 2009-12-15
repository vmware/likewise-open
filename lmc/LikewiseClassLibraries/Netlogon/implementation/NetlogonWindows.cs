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
using System.Text;
using Likewise.LMC.Netlogon.Interop;
using System.Runtime.InteropServices;
using Likewise.LMC.Utilities;
using Likewise.LMC.Netlogon;

namespace Likewise.LMC.Netlogon.Implementation
{

    public class WindowsNetlogon : INetlogon
    {

        #region windows private functions

        public uint GetDCName(
            string domainFQDN,
            uint flags,
            out CNetlogon.LWNET_DC_INFO DCInfo
            )
        {
            uint error = 0;
            CNetlogon.LWNET_DC_INFO info = new CNetlogon.LWNET_DC_INFO();
            NetlogonWindowsAPI.DOMAIN_CONTROLLER_INFO DCInfoWindows;
            DCInfo = info;
            IntPtr pDCInfo = IntPtr.Zero;

            try
            {

                Logger.Log(String.Format("GetDCNameWindows({0}, {1}) called",
                    domainFQDN,
                    flags),
                    Logger.NetlogonLogLevel);

                error = (uint)NetlogonWindowsAPI.DsGetDcName(
                            "",
                            "",
                            IntPtr.Zero,
                            "",
                            (int)flags,
                            out pDCInfo);

                if (error != 0)
                {
                    return (uint)error;
                }

                DCInfoWindows = (NetlogonWindowsAPI.DOMAIN_CONTROLLER_INFO)Marshal.PtrToStructure(pDCInfo, typeof(NetlogonWindowsAPI.DOMAIN_CONTROLLER_INFO));

                Logger.Log(String.Format("DsGetDcName() returned {0}, error={1}",
                    DCINFOToString(DCInfoWindows), error),
                    Logger.NetlogonLogLevel);

                DCInfo.pucDomainGUID = new byte[CNetlogon.Definitions.LWNET_GUID_SIZE];

                DCInfo.DomainControllerName = DCInfoWindows.DomainControllerName;
                DCInfo.DomainControllerAddress = DCInfoWindows.DomainControllerAddress;
                DCInfoWindows.DomainGuid.ToByteArray().CopyTo(DCInfo.pucDomainGUID, 0);
                DCInfo.FullyQualifiedDomainName = DCInfoWindows.DomainName;
                DCInfo.DnsForestName = DCInfoWindows.DnsForestName;
                DCInfo.Flags = (uint)DCInfoWindows.Flags;
                DCInfo.DCSiteName = DCInfoWindows.DcSiteName;
                DCInfo.ClientSiteName = DCInfoWindows.ClientSiteName;


                if (DCInfo.DomainControllerName.StartsWith(@"\\"))
                {
                    DCInfo.DomainControllerName = DCInfo.DomainControllerName.Substring(2);
                }

                if (DCInfo.DomainControllerAddress.StartsWith(@"\\"))
                {
                    DCInfo.DomainControllerAddress = DCInfo.DomainControllerAddress.Substring(2);
                }

                Logger.Log(String.Format("GetDCNameWindows() returning {0}, error={1}",
                    CNetlogon.LWNETDCInfoToString(DCInfo), error),
                    Logger.LogLevel.Debug);


                error = NetlogonWindowsAPI.NetApiBufferFree(pDCInfo);

                if (error != 0)
                {
                    Logger.Log("NetApiBufferFree() in GetDCNameWindows() failed!",
                        Logger.LogLevel.Error);
                }
            }
            catch (Exception e)
            {
                Logger.LogException("Netlogon.GetDCName", e);
                if (error == 0)
                {
                    error = CNetlogon.Definitions.LWNET_ERROR_INTEROP_EXCEPTION;
                }
                if (!IntPtr.Zero.Equals(pDCInfo))
                {
                    uint error2 = NetlogonWindowsAPI.NetApiBufferFree(pDCInfo);
                    if (error2 != 0)
                    {
                        Logger.Log("Netlogon.DCName: NetAPiBufferFree failed: " + error2);
                    }
                }
            }

            return error;
        }


        public uint GetDomainController(
            string domainFQDN,
            out string domainControllerFQDN
            )
        {
            domainControllerFQDN = null;
            uint error = 0;

            CNetlogon.LWNET_DC_INFO DCInfo = new CNetlogon.LWNET_DC_INFO();

            error = GetDCName(domainFQDN, 0, out DCInfo);

            if (error == 0 &&
               !String.IsNullOrEmpty(DCInfo.DomainControllerName))
            {
                domainControllerFQDN = DCInfo.DomainControllerName;
            }

            return error;
        }


        public uint GetDCTime(
            string domainFQDN,
            out DateTime DCTime
            )
        {
            DCTime = new DateTime();
            NetlogonWindowsAPI.TIME_OF_DAY_INFO_WINDOWS WindowsDCTime = new NetlogonWindowsAPI.TIME_OF_DAY_INFO_WINDOWS();
            uint error = 0;

            IntPtr pBuffer = IntPtr.Zero;

            try
            {
                error = NetlogonWindowsAPI.NetRemoteTOD(
                        domainFQDN,
                        out pBuffer);

                WindowsDCTime = (NetlogonWindowsAPI.TIME_OF_DAY_INFO_WINDOWS)Marshal.PtrToStructure(
                    pBuffer, typeof(NetlogonWindowsAPI.TIME_OF_DAY_INFO_WINDOWS));

                error = NetlogonWindowsAPI.NetApiBufferFree(pBuffer);
                if (error != 0)
                {
                    Logger.Log("NetApiBufferFree() in GetDCTimeWindows() failed!",
                        Logger.LogLevel.Error);
                }
            }
            catch (Exception e)
            {
                Logger.LogException("Netlogon.GetDCTimeWindows", e);
                if (error == 0)
                {
                    error = CNetlogon.Definitions.LWNET_ERROR_INTEROP_EXCEPTION;
                }
            }

            return error;
        }


        public uint GetCurrentDomain(
            out string domainFQDN
            )
        {
            domainFQDN = System.Environment.UserDomainName;

            return 0;
        }

        #endregion

        #region helper functions

        private static string DCINFOToString(NetlogonWindowsAPI.DOMAIN_CONTROLLER_INFO DCInfoWindows)
        {

            int i = 0;

            string[] domainGUIDStrings = new string[CNetlogon.Definitions.LWNET_GUID_SIZE];
            string domainGUIDString = null;
            byte[] guidArr = DCInfoWindows.DomainGuid.ToByteArray();

            for (i = 0; i < guidArr.Length; i++)
            {
                domainGUIDStrings[i] = String.Format("{0:x2} ", guidArr[i]);
            }

            domainGUIDString = String.Concat(domainGUIDStrings);

            StringBuilder sb = new StringBuilder();
            sb.Append("DomainControllerName:");
            sb.Append(DCInfoWindows.DomainControllerName);
            sb.Append(";");

            sb.Append("DomainControllerAddress:");
            sb.Append(DCInfoWindows.DomainControllerAddress);
            sb.Append(";");

            sb.Append("domainGUID: ");
            sb.Append(domainGUIDString);
            sb.Append(";");

            sb.Append("DomainName:");
            sb.Append(DCInfoWindows.DomainName);
            sb.Append(";");

            sb.Append("DnsForestName:");
            sb.Append(DCInfoWindows.DnsForestName);
            sb.Append(";");

            sb.Append("Flags:");
            sb.Append(DCInfoWindows.Flags);
            sb.Append(";");

            sb.Append("DCSiteName:");
            sb.Append(DCInfoWindows.DcSiteName);
            sb.Append(";");

            sb.Append("ClientSiteName:");
            sb.Append(DCInfoWindows.ClientSiteName);
            sb.Append(";");

            return sb.ToString();

        }

        private static DateTime WindowsTimeToDateTime(NetlogonWindowsAPI.TIME_OF_DAY_INFO_WINDOWS windowsDCTime)
        {

            DateTime DCDateTime = new DateTime(
                (int)windowsDCTime.tod_year,
                (int)windowsDCTime.tod_month - 1,
                (int)windowsDCTime.tod_day - 1,
                (int)windowsDCTime.tod_hours,
                (int)windowsDCTime.tod_mins,
                (int)windowsDCTime.tod_secs
                );

            return DCDateTime;

        }

        #endregion

    }
}
