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
    /// <summary>
    /// Errror codes returns for Kerberos authentication
    /// </summary>
    public enum Krb5ErrorCodes
    {
        KRB5_FCC_NOFILE = 196,
        KRB5_FCC_INTERNAL = 197
    }

    public class Krb5Wrapper
    {
        #region Class Data

        //Iniialization of kerebros ticket expiration time
        public static UInt32 KrbTicketExpiryTime = 0;

        private static System.Object lockThis_InitKRB5 = new System.Object();

        #endregion

        #region public helper methods

        /// <summary>
        /// Used to initial call to Kerneros ticket expiration check
        /// And iniiailze and alternate kerberos cache path
        /// </summary>
        /// <param name="domainName"></param>
        /// <param name="hostname"></param>
        /// <returns></returns>
        public static UInt32 InitKerberos(string domainName, string hostname)
        {
            UInt32 ret = 0;
            UInt32 ticketExpiryTime = 0;
            string krb5HostName = string.Empty;
            string krb5CachePath = string.Empty;

            lock (lockThis_InitKRB5)
            {
                //Check for expired time if kerebros TGT
                if (Krb5TicketHasExpired())
                    krb5HostName = string.Empty;
                else
                    return ret;

                //hostname reinitailization to defualt domian that machine is joined
                krb5HostName = hostname;
                krb5HostName = string.Concat(krb5HostName, "$@", domainName).ToUpper();

                //Call to make keberos cache path for the application
                ret = Krb5GetSystemCachePath(out krb5CachePath);
                if (ret != 0)
                {
                    Logger.Log(string.Format("InitKRB5.Krb5GetSystemCachePath() returns ret={0} ", ret));
                }

                //Call to get the kerberos TGT from the cached kaytab
                ret = Krb5GetTGTFromKeytab(krb5HostName, null, krb5CachePath, out ticketExpiryTime);
                if (ret != 0)
                {
                    Logger.Log(string.Format("InitKRB5.Krb5GetTGTFromKeytab() returns ret={0} ", ret));
                }

                //set the expired time specified in kerberos as default
                KrbTicketExpiryTime = ticketExpiryTime;
            }

            return ret;
        }

        /// <summary>
        /// Check for cached kerberos expired time with the defualt specified time
        /// </summary>
        /// <returns></returns>
        public static bool Krb5TicketHasExpired()
        {
            bool bTiketExpired = false;

            if (KrbTicketExpiryTime == 0)
                bTiketExpired = true;
            else if (TimeSpan.Parse(KrbTicketExpiryTime.ToString()).TotalSeconds < PrivateKrb5Api.ExpiryGraceSeconds)
            {
                bTiketExpired = true;
            }

            return bTiketExpired;
        }

        /// <summary>
        /// Make and return the alternate cache path for kerberos keytab caching
        /// </summary>
        /// <param name="krb5CachePath"></param>
        /// <returns></returns>
        public static UInt32 Krb5GetSystemCachePath(out string krb5CachePath)
        {
            UInt32 ret = 0;

            try
            {
                if (!(Directory.Exists(PrivateKrb5Api.ADMIN_CACHE_DIR)))
                {
                    Directory.CreateDirectory(PrivateKrb5Api.ADMIN_CACHE_DIR);
                }

                krb5CachePath = Path.Combine(PrivateKrb5Api.ADMIN_CACHE_DIR, PrivateKrb5Api.KRB5_CACHEPATH);
                krb5CachePath = string.Concat("FILE:", krb5CachePath);

                Logger.Log(string.Format("Krb5Wrapper.Krb5GetSystemCachePath krb5CachePath = {0} ", krb5CachePath));
            }
            catch (Exception ex)
            {
                ret = 1;
                krb5CachePath = string.Empty;
                Logger.LogException("Krb5Wrapper.Krb5GetSystemCachePath", ex);
            }

            return ret;
        }

        public static UInt32 Krb5GetTGTFromKeytab(string userName,
                                            string passWord,
                                            string krb5CachePath,
                                            out UInt32 ticketExpiryTime)
        {
            UInt32 ret = 0;
            IntPtr iRet = IntPtr.Zero;     //krb5_error_code
            KRB5API.krb5_ccache stcc = new KRB5API.krb5_ccache();
            KRB5API.krb5_context stCtx = new KRB5API.krb5_context();

            ticketExpiryTime = 0;

            try
            {
                stCtx.db_context = IntPtr.Zero;
                stCtx.default_realm = IntPtr.Zero;
                stCtx.in_tkt_ktypes = IntPtr.Zero;
                stCtx.locate_fptrs = IntPtr.Zero;
                stCtx.preauth_context = IntPtr.Zero;
                stCtx.profile = IntPtr.Zero;
                stCtx.prompt_types = IntPtr.Zero;
                stCtx.ser_ctx = IntPtr.Zero;
                stCtx.tgs_ktypes = IntPtr.Zero;
                stCtx.vtbl = IntPtr.Zero;
                IntPtr ctx = Marshal.AllocHGlobal(Marshal.SizeOf(stCtx));
                Marshal.StructureToPtr(stCtx, ctx, false);
                ret = PrivateKrb5Api.krb5_init_context(out ctx);
                Logger.Log(string.Format("Krb5Wrapper.Krb5GetTGTFromKeytab.krb5_init_context(ctx = {0},iRet= {1}", ctx.ToString(), iRet.ToString()), Logger.Krb5LogLevel);
                if (iRet != IntPtr.Zero) {
                    Logger.Log(string.Format("krb5_init_context returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
                }

                //Default initialization for KRB5API.krb5_ccache
                stcc.data = IntPtr.Zero;
                stcc.ops = IntPtr.Zero;
                IntPtr cc = Marshal.AllocHGlobal(Marshal.SizeOf(stcc));
                Marshal.StructureToPtr(stcc, cc, false);
                if (!String.IsNullOrEmpty(krb5CachePath))
                {
                    /* use krb5_cc_resolve to get an alternate cache */
                    iRet = PrivateKrb5Api.krb5_cc_resolve(ctx, krb5CachePath, ref cc);
                    Logger.Log(string.Format("Krb5Wrapper.Krb5GetTGTFromKeytab.krb5_cc_resolve(ctx = {0},krb5CachePath= {1}, cc={2}", ctx.ToString(), krb5CachePath, cc.ToString()), Logger.Krb5LogLevel);
                    if (iRet != IntPtr.Zero) {
                        Logger.Log(string.Format("krb5_cc_resolve returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
                    }
                }
                else
                {
                    /* use krb5_cc_resolve to get an alternate cache */
                    iRet = PrivateKrb5Api.krb5_cc_default(ctx, ref cc);
                    Logger.Log(string.Format("Krb5Wrapper.Krb5GetTGTFromKeytab.krb5_cc_default(ctx = {0}, cc={1}", ctx.ToString(), cc.ToString()), Logger.Krb5LogLevel);
                    if (iRet != IntPtr.Zero) {
                        Logger.Log(string.Format("krb5_cc_resolve returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
                    }
                }

                KRB5API.krb5_keytab stKeytab = new KRB5API.krb5_keytab();
                stKeytab.data = IntPtr.Zero;
                stKeytab.ops = IntPtr.Zero;
                IntPtr keytab = Marshal.AllocHGlobal(Marshal.SizeOf(stKeytab));
                Marshal.StructureToPtr(stKeytab, keytab, false);
                iRet = PrivateKrb5Api.krb5_kt_default(ctx, out keytab);
                Logger.Log(string.Format("Krb5Wrapper.Krb5GetTGTFromKeytab.krb5_cc_default(ctx = {0}, cc={1}", ctx.ToString(), cc.ToString()), Logger.Krb5LogLevel);
                if (iRet != IntPtr.Zero)
                {
                    Logger.Log(string.Format("krb5_cc_default( keytab={0}, iRet={1}", keytab.ToString(), iRet.ToString()));
                }

                KRB5API.krb5_principal cli_principal = new KRB5API.krb5_principal();
                cli_principal.data = IntPtr.Zero;
                iRet = PrivateKrb5Api.krb5_parse_name(ctx, userName, out cli_principal);
                Logger.Log(string.Format("Krb5Wrapper.Krb5GetTGTFromKeytab.krb5_parse_name(ctx = {0},userName ={1}, client_principal={2}, iRet= {3}", ctx.ToString(), userName, Marshal.PtrToStringAuto(cli_principal.realm.data), iRet.ToString()), Logger.Krb5LogLevel);
                if (iRet != IntPtr.Zero) {
                    Logger.Log(string.Format("krb5_parse_name returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
                }

                KRB5API.krb5_creds in_Stcreds = new KRB5API.krb5_creds();
                in_Stcreds.addresses = IntPtr.Zero;
                in_Stcreds.authdata = IntPtr.Zero;
                iRet = PrivateKrb5Api.krb5_get_init_creds_keytab(
                                        ctx,
                                        ref in_Stcreds,
                                        cli_principal,
                                        keytab,
                                        0,
                                        null,
                                        IntPtr.Zero
                                        );
                Logger.Log(string.Format("krb5_get_init_creds_keytab( in_Stcreds={0}, iRet={1}", in_Stcreds.ToString(), iRet.ToString()));
                if (iRet != IntPtr.Zero)
                {
                    Logger.Log(string.Format("krb5_get_init_creds_keytab( in_Stcreds={0}, iRet={1}", in_Stcreds.ToString(), iRet.ToString()));
                }

                iRet = PrivateKrb5Api.krb5_cc_store_cred(
                                        ctx,
                                        cc,
                                        in_Stcreds
                                        );
                Logger.Log(string.Format("krb5_get_init_creds_keytab( in_Stcreds={0}, iRet={1}", in_Stcreds.ToString(), iRet.ToString()));
                if (iRet != IntPtr.Zero)
                {
                    Logger.Log(string.Format("krb5_get_init_creds_keytab( in_Stcreds={0}, iRet={1}", in_Stcreds.ToString(), iRet.ToString()));
                }

                if (in_Stcreds != null)
                {
                    Logger.Log("Marshal.PtrToStructure(stCreds.times, times) is success", Logger.Krb5LogLevel);

                    if (in_Stcreds != null && in_Stcreds.times != null)
                    {
                        ticketExpiryTime = Convert.ToUInt32(in_Stcreds.times.endtime);

                        Logger.Log("times.authtime is " + in_Stcreds.times.authtime.ToString(), Logger.Krb5LogLevel);
                        Logger.Log("times.endtime is " + in_Stcreds.times.endtime.ToString(), Logger.Krb5LogLevel);
                        Logger.Log("times.renew_till is " + in_Stcreds.times.renew_till.ToString(), Logger.Krb5LogLevel);
                        Logger.Log("times.starttime is " + in_Stcreds.times.starttime.ToString(), Logger.Krb5LogLevel);
                        Logger.Log("ticketExpiryTime is " + ticketExpiryTime, Logger.Krb5LogLevel);
                    }
                }

                if (ctx != IntPtr.Zero)
                {
                    if (cc != IntPtr.Zero)
                    {
                        PrivateKrb5Api.krb5_cc_close(ctx, cc);
                    }

                    PrivateKrb5Api.krb5_free_context(ctx);
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("Krb5Wrapper.Krb5GetTGTFromKeytab", ex);
            }

            return ret;
        }

        public static UInt32 Kerb5DestroyCache(string dirCachePath)
        {
            UInt32 ret = 0;
            IntPtr iRet = IntPtr.Zero;  //krb5_error_code
            IntPtr ctx;                 //krb5_context
            IntPtr cc = IntPtr.Zero;    //krb5_ccache

            ret = PrivateKrb5Api.krb5_init_context(out ctx);
            Logger.Log(string.Format("Krb5Wrapper.Kerb5DestroyCache.krb5_init_context(ctx = {0},iRet= {1}", ctx.ToString(),iRet.ToString()), Logger.Krb5LogLevel);
            if (iRet != IntPtr.Zero) {
                Logger.Log(string.Format("krb5_init_context returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
            }

            /* use krb5_cc_resolve to get an alternate cache */
            iRet = PrivateKrb5Api.krb5_cc_resolve(ctx, dirCachePath, ref cc);
            Logger.Log(string.Format("Krb5Wrapper.Kerb5DestroyCache.krb5_cc_resolve(ctx = {0},dirCachePath= {1}, cc={2}", ctx.ToString(),dirCachePath,cc.ToString()), Logger.Krb5LogLevel);
            if (iRet != IntPtr.Zero) {
                Logger.Log(string.Format("krb5_cc_resolve returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
            }

            iRet = PrivateKrb5Api.krb5_cc_destroy(ctx, cc);
            if (iRet != IntPtr.Zero)
            {
                if (iRet.ToInt32() != (int)Krb5ErrorCodes.KRB5_FCC_NOFILE)
                {
                    iRet = IntPtr.Zero;
                }
            }

            if (ctx != IntPtr.Zero)
            {
                iRet = PrivateKrb5Api.krb5_free_context(ctx);
            }

            return ret;
        }

        #endregion

    }

}
