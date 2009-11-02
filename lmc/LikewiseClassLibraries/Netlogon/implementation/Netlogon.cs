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
using Likewise.LMC.Netlogon.Interop;
using System.Runtime.InteropServices;
using Likewise.LMC.Utilities;
using Likewise.LMC.Netlogon;

namespace Likewise.LMC.Netlogon.Implementation
{
    public class UnixNetlogon : INetlogon
    {

        #region non-windows private functions

        public uint GetDCName(
            string domainFQDN,
            uint flags,
            out CNetlogon.LWNET_DC_INFO DCInfo
            )
        {
            CNetlogon.LWNET_DC_INFO info = new CNetlogon.LWNET_DC_INFO();
            DCInfo = info;
            IntPtr pDCInfo = IntPtr.Zero;
            uint error = 0;

            try
            {

                Logger.Log(String.Format("GetDCName({0}, {1}) called",
                    domainFQDN,
                    flags),
                    Logger.NetlogonLogLevel);               

                error = NetlogonAPI.LWNetGetDCName(
                            null,
                            domainFQDN,
                            null,
                            flags,
                            out pDCInfo);

                DCInfo = (CNetlogon.LWNET_DC_INFO)Marshal.PtrToStructure(pDCInfo, typeof(CNetlogon.LWNET_DC_INFO));

                Logger.Log(String.Format("GetDCName() returned {0}, error={1}",
                    CNetlogon.LWNETDCInfoToString(DCInfo), error),
                    Logger.NetlogonLogLevel);

            }

            catch (Exception e)
            {
                Logger.LogException("Netlogon.GetDCName", e);
                if (error == 0)
                {
                    error = CNetlogon.Definitions.LWNET_ERROR_INTEROP_EXCEPTION;
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

            try
            {

                Logger.Log(String.Format("GetDomainController({0}) called",
                    domainFQDN),
                    Logger.NetlogonLogLevel);

                error = NetlogonAPI.LWNetGetDomainController(
                            domainFQDN,
                            out domainControllerFQDN);

                Logger.Log(String.Format("GetDomainController() returned {0}, error={1}",
                    domainControllerFQDN, error),
                    Logger.NetlogonLogLevel);

            }

            catch (Exception e)
            {
                Logger.LogException("Netlogon.GetDomainController", e);
                if (error == 0)
                {
                    error = CNetlogon.Definitions.LWNET_ERROR_INTEROP_EXCEPTION;
                }
            }

            return error;
        }


        public uint GetDCTime(
            string domainFQDN,
            out DateTime DCTime
            )
        {
            DCTime = new DateTime();
            uint error = 0;

            NetlogonAPI.POSIX_TIME posixDCTime = new NetlogonAPI.POSIX_TIME();

            try
            {

                Logger.Log(String.Format("GetDCTime({0}) called",
                    domainFQDN),
                    Logger.NetlogonLogLevel);

                error = NetlogonAPI.LWNetGetDCTime(
                            domainFQDN,
                            out posixDCTime);

                DCTime = PosixTimeToDateTime(posixDCTime);

                Logger.Log(String.Format("GetDCTime() returned {0}, error={1}",
                    DCTime, error),
                    Logger.NetlogonLogLevel);

            }

            catch (Exception e)
            {
                Logger.LogException("Netlogon.GetDCTime", e);
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
            domainFQDN = null;
            uint error = 0;

            try
            {

                Logger.Log("GetCurrentDomain() called", Logger.NetlogonLogLevel);

                error = NetlogonAPI.LWNetGetCurrentDomain(out domainFQDN);

                Logger.Log(String.Format("GetCurrentDomain() returned {0}, error={1}",
                    domainFQDN, error),
                    Logger.NetlogonLogLevel);
            }

            catch (Exception e)
            {
                Logger.LogException("Netlogon.GetCurrentDomain", e);
                if (error == 0)
                {
                    error = CNetlogon.Definitions.LWNET_ERROR_INTEROP_EXCEPTION;
                }
            }

            return error;
        }

        #endregion

        #region helper functions

        private static DateTime PosixTimeToDateTime(NetlogonAPI.POSIX_TIME posixDCTime)
        {

            DateTime DCDateTime = new DateTime(
                posixDCTime.tm_year + 1900,
                posixDCTime.tm_mon,
                posixDCTime.tm_mday,
                posixDCTime.tm_hour,
                posixDCTime.tm_min,
                posixDCTime.tm_sec
                );

            return DCDateTime;

        }

        #endregion

    }
}
