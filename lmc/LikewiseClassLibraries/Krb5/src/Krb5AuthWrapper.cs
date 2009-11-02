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
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;

namespace Likewise.LMC.Krb5
{
    public class Krb5AuthWrapper
    {
        #region Class Data

        //Iniialization of kerebros ticket expiration time
        public static UInt32 KrbTicketExpiryTime = 0;

        private static System.Object lockThis_InitKerberos = new System.Object();

        #endregion

        #region public helper methods


        /// <summary>
        /// Used to initial call to Kerneros ticket expiration check
        /// And iniiailze and alternate kerberos cache path
        /// </summary>
        /// <param name="domainName"></param>
        /// <param name="hostname"></param>
        /// <returns></returns>
        public static UInt32 InitKerberos(string domainName, string username, string password)
        {
            int ret = 0;
            int ticketExpiryTime = 0;
            string str = null;
            string krb5HostName = string.Empty;
            string krb5CachePath = string.Empty;
            string errorMassage = "";

            lock (lockThis_InitKerberos)
            {
                //Check for expired time if kerebros TGT

                //hostname reinitailization to defualt domian that machine is joined
                krb5HostName = username;

                if (!Configurations.SSOFailed)
                {                    
                    ret = PrivateLsassApi.LsaKrb5GetUserCachePath(
                                          Convert.ToInt32(Configurations.GetUID),
                                          (int)Krb5CacheType.KRB5_File_Cache,
                                          out krb5CachePath);
                    Logger.Log(string.Format("Krb5AuthWrapper.LsaKrb5GetUserCachePath(Uid ={0}, out cachePath={1}, ret={2}", Configurations.GetUID, krb5CachePath, ret.ToString()), Logger.Krb5LogLevel);
                    if (ret != 0)
                    {
                        errorMassage = "CachePath does not exists";
                        Logger.Log(errorMassage + ret.ToString());
                    }

                    ret = PrivateLsassApi.LsaKrb5GetTgt(
                                          krb5HostName,                  
                                          null,
                                          krb5CachePath,
                                          out ticketExpiryTime);
                    Logger.Log(string.Format("Krb5AuthWrapper.LsaKrb5GetTgt(krb5HostName ={0},password ={1}, krb5CachePath ={2}, ticketExpiryTime ={3}, ret={4}", krb5HostName, password, krb5CachePath, ticketExpiryTime.ToString(), ret.ToString()), Logger.Krb5LogLevel);
                    if (ret != 0)
                    {
                        errorMassage = "krb5 does not find the ticketExpiryTime";
                        Logger.Log(errorMassage + ret.ToString());
                    }
                }
                else
                {
                    krb5HostName = string.Concat(krb5HostName, "$@", domainName).ToUpper();

                    ret = PrivateLsassApi.LsaKrb5GetUserCachePath(
                                          Convert.ToInt32(Configurations.GetUID),
                                          (int)Krb5CacheType.KRB5_InMemory_Cache,
                                          out krb5CachePath);
                    Logger.Log(string.Format("Krb5AuthWrapper.LsaKrb5GetUserCachePath(Uid ={0}, out cachePath={1}, ret={2}", Configurations.GetUID, krb5CachePath, ret.ToString()), Logger.Krb5LogLevel);
                    if (ret != 0)
                    {
                        errorMassage = "CachePath does not exists";
                        Logger.Log(errorMassage + ret.ToString());
                    }

                    ret = PrivateLsassApi.LsaKrb5GetTgt(
                                     krb5HostName,
                                     password,
                                     krb5CachePath,
                                     out ticketExpiryTime);
                    Logger.Log(string.Format("Krb5AuthWrapper.LsaKrb5GetTgt(krb5HostName ={0},password ={1}, krb5CachePath ={2}, ticketExpiryTime ={3}, ret={4}", krb5HostName, password, krb5CachePath, ticketExpiryTime.ToString(), ret.ToString()), Logger.Krb5LogLevel);
                    if (ret != 0)
                    {
                        errorMassage = "krb5 does not find the ticketExpiryTime";
                        Logger.Log(errorMassage + ret.ToString());
                    }
                }

                ret = PrivateLsassApi.LsaKrb5SetDefaultCachePath(
                                      krb5CachePath,
                                      out str);
                Logger.Log(string.Format("Krb5AuthWrapper.LsaKrb5SetDefaultCachePath(krb5CachePath ={0}, , ret={1}", krb5CachePath, ret.ToString()), Logger.Krb5LogLevel);
                if (ret != 0)
                {
                    errorMassage = "CachePath does not exists";
                    Logger.Log(errorMassage + ret.ToString());
                }

                KrbTicketExpiryTime = Convert.ToUInt32(ticketExpiryTime);
            }

            return Convert.ToUInt32(ret);
        }
        
        #endregion

    }
}
