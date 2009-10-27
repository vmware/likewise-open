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
using System.IO;
using Likewise.LMC.LMConsoleUtils;

namespace Likewise.LMC.Krb5
{
    class Krb5CredsCache
    {
        #region Class Data

        #endregion

        #region Constructors

        #endregion

        #region public methods      

        public static int BuildCredsContext(string sUPNName,
                                            string sPassword,
                                            string sDomain,
                                            out IntPtr CrdesCache)
        {
            int ret = -1;
            string sCredsCache = string.Empty;
            string sPrinicalName = string.Empty;
            IntPtr pAccessToken = IntPtr.Zero;
            CRED_CONTEXT creds = new CRED_CONTEXT();
            CrdesCache = IntPtr.Zero;

            try
            {
                Logger.Log("sUPNName :" + sUPNName);

                if (String.IsNullOrEmpty(sUPNName) ||
                    sUPNName.Equals("administrator", StringComparison.InvariantCultureIgnoreCase))
                {
                    ret = Krb5GetDomainCredsCache(sUPNName, sDomain, sPassword);
                    Logger.Log(string.Format("Krb5CredsCache.BuildCredsContext: Krb5GetDomainCredsCache(sDomain={0}) ret={1}", sDomain, ret.ToString()));
                    if (ret != 0)
                    {
                        return ret;
                    }
                    ret = Krb5GetSystemCachePath(out sCredsCache);
                    Logger.Log(string.Format("Krb5CredsCache.BuildCredsContext: Krb5GetSystemCachePath(out sCredsCache={0}) ret={1}", sCredsCache, ret.ToString()));
                    if (ret != 0)
                    {
                        return ret;
                    }
                }
                else
                {
                    ret = Krb5GetUserCachePath(sUPNName, out sCredsCache);
                    Logger.Log(string.Format("Krb5CredsCache.BuildCredsContext: Krb5GetUserCachePath(sUPNName={0}, out sCredsCache={1}) ret={1}", sUPNName, sCredsCache, ret.ToString()));
                    if (ret != 0)
                    {
                        return ret;
                    }
                }

                ret = Krb5GetPrincipalName(sCredsCache, out sPrinicalName);
                Logger.Log(string.Format("Krb5CredsCache.BuildCredsContext: Krb5GetPrincipalName(sCredsCache={0}, out sPrinicalName={1}) ret={2}", sCredsCache, sPrinicalName, ret.ToString()));
                if (ret != 0)
                {
                    return ret;
                }

                ret = apiLwIoCreateKrb5AccessTokenA(sPrinicalName, sCredsCache, out pAccessToken);
                Logger.Log(string.Format("Krb5CredsCache.BuildCredsContext: apiLwIoCreateKrb5AccessTokenA(sPrinicalName={0}, sCredsCache={1},out pAccessToken={2}) ret={3}", sPrinicalName, sCredsCache, pAccessToken.ToInt32().ToString(), ret.ToString()));
                if (ret != 0)
                {
                    return ret;
                }

                creds.pszCachePath = sCredsCache;
                creds.pszPrincipalName = sPrinicalName;
                creds.pAccessToken = pAccessToken;

                CrdesCache = IntPtr.Zero;
                CrdesCache = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(CRED_CONTEXT)));
                Marshal.StructureToPtr(creds, CrdesCache, false);

                return ret;
            }
            catch (Exception ex)
            {
                Logger.LogException("Krb5CredsCache.BuildCredsContext", ex);
            }

            return ret;
        }

        private static int Krb5GetDomainCredsCache(string sUPNName, string sDomainName, string sPassword)
        {
            int ret = 0;
            string sHostName = string.Empty;
            string sCachePath = string.Empty;
            UInt32 dwTicketExpiryTime = 0;

            try
            {
                sHostName = System.Environment.MachineName;
                sHostName = string.Concat(sHostName, "$@", sDomainName);
                sHostName = sHostName.ToUpper();

                ret = Krb5GetSystemCachePath(out sCachePath);
                Logger.Log(string.Format("Krb5CredsCache.Krb5GetDomainCredsCache: Krb5GetSystemCachePath(out sCachePath={0}) ret={1}", sCachePath, ret.ToString()));
                if (ret != 0)
                {
                    return ret;
                }

                ret = Krb5GetTGTFromKeytab(sHostName, sPassword, sCachePath, out dwTicketExpiryTime);
                Logger.Log(string.Format("Krb5CredsCache.Krb5GetDomainCredsCache: Kerb5GetTGTFromKeytab(out dwTicketExpiryTime={0}) ret={1}", dwTicketExpiryTime.ToString(), ret.ToString()));
                if (ret != 0)
                {
                    return ret;
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("Krb5CredsCache.Krb5GetDomainCredsCache", ex);
            }
            return ret;
        }

        public static int Krb5GetTGTFromKeytab(string userName,
                                               string passWord,
                                               string krb5CachePath,
                                               out UInt32 ticketExpiryTime)
        {
            int ret = 0;
            IntPtr iRet = IntPtr.Zero;             
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
                uint uret = PrivateKrb5Api.krb5_init_context(out ctx);
                Logger.Log(string.Format("Krb5Wrapper.Krb5GetTGTFromKeytab.krb5_init_context(ctx = {0},iRet= {1}", ctx.ToString(), iRet.ToString()), Logger.Krb5LogLevel);
                if (uret != 0)
                {
                    //ret = Convert.ToUInt32(iRet.ToInt32());
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
                    if (iRet != IntPtr.Zero)
                    {
                        //ret = Convert.ToUInt32(iRet.ToInt32());
                        Logger.Log(string.Format("krb5_cc_resolve returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
                    }
                }
                else
                {
                    /* use krb5_cc_resolve to get an alternate cache */
                    iRet = PrivateKrb5Api.krb5_cc_default(ctx, ref cc);
                    Logger.Log(string.Format("Krb5Wrapper.Krb5GetTGTFromKeytab.krb5_cc_default(ctx = {0}, cc={1}", ctx.ToString(), cc.ToString()), Logger.Krb5LogLevel);
                    if (iRet != IntPtr.Zero)
                    {
                        //ret = Convert.ToUInt32(iRet.ToInt32());
                        Logger.Log(string.Format("krb5_cc_resolve returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
                    }
                }

                KRB5API.krb5_keytab stKeytab = new KRB5API.krb5_keytab();
                stKeytab.data = IntPtr.Zero;
                stKeytab.ops = IntPtr.Zero;
                IntPtr keytab = Marshal.AllocHGlobal(Marshal.SizeOf(stKeytab));
                Marshal.StructureToPtr(stKeytab, keytab, false);
                iRet = PrivateKrb5Api.krb5_kt_default(ctx, out keytab);
                Logger.Log(string.Format("Krb5Wrapper.Krb5GetTGTFromKeytab.krb5_kt_default(ctx = {0}, out keytab={1}", ctx.ToString(), keytab.ToInt32().ToString()), Logger.Krb5LogLevel);
                if (iRet != IntPtr.Zero)
                {
                    Logger.Log(string.Format("krb5_kt_default( keytab={0}, iRet={1}", keytab.ToString(), iRet.ToString()));
                }

                KRB5API.krb5_principal cli_principal = new KRB5API.krb5_principal();
                cli_principal.data = IntPtr.Zero;
                iRet = PrivateKrb5Api.krb5_parse_name(ctx, userName, out cli_principal);
                Logger.Log(string.Format("Krb5Wrapper.Krb5GetTGTFromKeytab.krb5_parse_name(ctx = {0},userName ={1}, client_principal={2}, iRet= {3}", ctx.ToString(), userName, Marshal.PtrToStringAuto(cli_principal.realm.data), iRet.ToString()), Logger.Krb5LogLevel);
                if (iRet != IntPtr.Zero)
                {
                    //ret = Convert.ToUInt32(iRet.ToInt32());
                    Logger.Log(string.Format("krb5_parse_name returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
                }

                KRB5API.krb5_creds in_Stcreds = new KRB5API.krb5_creds();
                in_Stcreds.client = cli_principal;
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
                Logger.Log(string.Format("krb5_cc_store_cred( in_Stcreds={0}, iRet={1}", in_Stcreds.ToString(), iRet.ToString()));
                if (iRet != IntPtr.Zero)
                {
                    Logger.Log(string.Format("krb5_cc_store_cred( in_Stcreds={0}, iRet={1}", in_Stcreds.ToString(), iRet.ToString()));
                }                

                if (in_Stcreds != null)
                {
                    //Marshal.PtrToStructure(out_creds, out_Stcreds);
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

        /// <summary>
        /// Make and return the alternate cache path for kerberos keytab caching
        /// </summary>
        /// <param name="krb5CachePath"></param>
        /// <returns></returns>
        public static int Krb5GetSystemCachePath(out string krb5CachePath)
        {
            int ret = 0;

            try
            {
                if (!(Directory.Exists(PrivateKrb5Api.ADMIN_TEMP_CACHE_DIR)))
                {
                    Directory.CreateDirectory(PrivateKrb5Api.ADMIN_TEMP_CACHE_DIR);
                }

                krb5CachePath = Path.Combine(PrivateKrb5Api.ADMIN_TEMP_CACHE_DIR, PrivateKrb5Api.KRB5_CACHEPATH);
                krb5CachePath = string.Concat("FILE:", krb5CachePath);

                Logger.Log(string.Format("Krb5CredsCache.Krb5GetSystemCachePath out krb5CachePath = {0} ", krb5CachePath));
            }
            catch (Exception ex)
            {
                ret = 1;
                krb5CachePath = string.Empty;
                Logger.LogException("Krb5CredsCache.Krb5GetSystemCachePath", ex);
            }

            return ret;
        }

        public static int Krb5GetUserCachePath(string UPNName, out string krb5CachePath)
        {
            int ret = 0;
            krb5CachePath = string.Empty;
            try
            {
                //Nothing to do for the domain user
            }
            catch (Exception ex)
            {
                ret = 1;
                Logger.LogException("Krb5CredsCache.Krb5GetUserCachePath", ex);
            }

            return ret;
        }

        public static int Krb5GetPrincipalName(string sCredsCache, out string sPrincipalName)
        {
            UInt32 ret = 0;
            IntPtr iRet = IntPtr.Zero;
            IntPtr cc = IntPtr.Zero;
            IntPtr ctx = IntPtr.Zero;
            IntPtr pKrb5Principal=IntPtr.Zero;
            string sPrincipalname = string.Empty;
            sPrincipalName = null;

            try
            {
                ret = PrivateKrb5Api.krb5_init_context(out ctx);
                Logger.Log(string.Format("PrivateKrb5Api.krb5_init_context(out ctx = {0}),iRet= {1}", ctx.ToString(), iRet.ToString()), Logger.Krb5LogLevel);
                if (ret != 0)
                {
                    Logger.Log(string.Format("PrivateKrb5Api.krb5_init_context returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
                }

                if (!String.IsNullOrEmpty(sCredsCache))
                {
                    /* use krb5_cc_resolve to get an alternate cache */
                    iRet = PrivateKrb5Api.krb5_cc_resolve(ctx, sCredsCache, ref cc);
                    Logger.Log(string.Format("PrivateKrb5Api.krb5_cc_resolve(ctx = {0},sCredsCache= {1}, ref cc={2}", ctx.ToString(), sCredsCache, cc.ToString()), Logger.Krb5LogLevel);
                    if (iRet != IntPtr.Zero)
                    {
                        Logger.Log(string.Format("PrivateKrb5Api.krb5_cc_resolve returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
                    }
                }
                else
                {
                    /* use krb5_cc_resolve to get an alternate cache */
                    iRet = PrivateKrb5Api.krb5_cc_default(ctx, ref cc);
                    Logger.Log(string.Format("PrivateKrb5Api.krb5_cc_default(ctx = {0}, ref cc={1}", ctx.ToString(), cc.ToString()), Logger.Krb5LogLevel);
                    if (iRet != IntPtr.Zero)
                    {
                        Logger.Log(string.Format("PrivateKrb5Api.krb5_cc_default returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
                    }
                }

                iRet = PrivateKrb5Api.krb5_cc_get_principal(ctx, cc, out pKrb5Principal);
                Logger.Log(string.Format("PrivateKrb5Api.krb5_cc_get_principal(ctx = {0}, cc={1} out pKrb5Principal={2}), ret={3}", ctx.ToString(), cc.ToString(), pKrb5Principal.ToString(), iRet.ToInt32().ToString()), Logger.Krb5LogLevel);
                if (iRet != IntPtr.Zero)
                {
                    Logger.Log(string.Format("PrivateKrb5Api.krb5_cc_get_principal returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
                }

                iRet = PrivateKrb5Api.krb5_unparse_name(ctx, pKrb5Principal, out sPrincipalname);
                Logger.Log(string.Format("PrivateKrb5Api.krb5_unparse_name(ctx = {0}, pKrb5Principal={1} out sPrincipalname={2}), ret={3}", ctx.ToString(), pKrb5Principal.ToString(), sPrincipalname, iRet.ToInt32().ToString()), Logger.Krb5LogLevel);
                if (iRet != IntPtr.Zero)
                {
                    Logger.Log(string.Format("PrivateKrb5Api.krb5_unparse_name returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
                }

                sPrincipalName = sPrincipalname;
            }
            catch (Exception ex)
            {
                ret = 1;
                sPrincipalName = string.Empty;
                Logger.LogException("Krb5CredsCache.Krb5GetPrincipalName", ex);
            }

            return (int)ret;
        }

        public static int apiLwIoCreateKrb5AccessTokenA(string sPrincipal, string krb5CachePath, out IntPtr pAccessToken)
        {
            int ret = 0;
            pAccessToken = IntPtr.Zero;

            try
            {
                ret = PrivateLwioApi.LwIoCreateKrb5AccessTokenA(sPrincipal, krb5CachePath, out pAccessToken);
                Logger.Log(string.Format("PrivateLwioApi.LwIoCreateKrb5AccessTokenA(sPrincipal = {0}, krb5CachePath={1} out pAccessToken={2}), ret={3}", sPrincipal, krb5CachePath, pAccessToken.ToInt32().ToString(), ret.ToString()), Logger.Krb5LogLevel);
                if (ret != 0)
                {
                    Logger.Log(string.Format("PrivateLwioApi.LwIoCreateKrb5AccessTokenA returns non-zero ret value iRet= {0}", ret.ToString()), Logger.Krb5LogLevel);
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("Krb5CredsCache.apiLwIoCreateKrb5AccessTokenA", ex);
            }

            return ret;
        }

        public static UInt32 Krb5GetDomainPrincipalRealm(string sUsername, out string sPrincipalRealm)
        {
            UInt32 ret = 0;
            IntPtr iRet = IntPtr.Zero;
            KRB5API.krb5_ccache stcc = new KRB5API.krb5_ccache();
            KRB5API.krb5_context stCtx = new KRB5API.krb5_context();

            sPrincipalRealm = string.Empty;
            
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
                Logger.Log(string.Format("Krb5Wrapper.Krb5GetDomainPrincipalRealm.krb5_init_context(ctx = {0},iRet= {1}", ctx.ToString(), iRet.ToString()), Logger.Krb5LogLevel);
                if (ret != 0)
                {
                    //ret = Convert.ToUInt32(iRet.ToInt32());
                    Logger.Log(string.Format("krb5_init_context returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
                }

                KRB5API.krb5_principal cli_principal = new KRB5API.krb5_principal();
                cli_principal.data = IntPtr.Zero;
                iRet = PrivateKrb5Api.krb5_parse_name(ctx, sUsername, out cli_principal);
                Logger.Log(string.Format("Krb5Wrapper.Krb5GetDomainPrincipalRealm.krb5_parse_name(ctx = {0},userName ={1}, client_principal={2}, iRet= {3}", ctx.ToString(), sUsername, Marshal.PtrToStringAuto(cli_principal.realm.data), iRet.ToString()), Logger.Krb5LogLevel);
                if (iRet != IntPtr.Zero)
                {
                    ret = Convert.ToUInt32(iRet.ToString());
                    Logger.Log(string.Format("krb5_parse_name returns non-zero ret value iRet= {0}", iRet.ToString()), Logger.Krb5LogLevel);
                }

                sPrincipalRealm = Marshal.PtrToStringAuto(cli_principal.realm.data);
            }
            catch (Exception ex)
            {
                ret = 1;
                Logger.LogException("Krb5CredsCache.apiLwIoCreateKrb5AccessTokenA", ex);
            }

            return ret;
        }


        #endregion
    }
}
