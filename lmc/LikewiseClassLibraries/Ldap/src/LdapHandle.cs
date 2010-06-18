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
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.Utilities;
using System.Runtime.InteropServices;
using Likewise.LMC.LDAP;
using System.Threading;

namespace Likewise.LMC.LDAP
{
    /// <summary>
    /// Summary description for LdapHandle.
    /// </summary>
    public class LdapHandle
    {
        #region class data

        private const int LDAP_VERSION3 = 3;
        private const int LDAP_NO_LIMIT = 0;
        private const int LDAP_OPT_PROTOCOL_VERSION = 0x0011;
        private const int LDAP_OPT_SIZELIMIT = 0x0003;
        private const int LDAP_OPT_TIMELIMIT = 0x0004;
        private const int LDAP_OPT_SSPI_FLAGS = 0x92;
        private const int LDAP_OPT_REFERRALS = 0x0008;

        private const int GSS_C_MUTUTAL_FLAG = 2;
        private const int GSS_C_REPLAY_FLAG = 4;
        private const int GSS_C_INTEG_FLAG = 32;
        private const int GSS_C_CONF_FLAG = 16;

        private const int ISC_REQ_MUTUAL_AUTH = GSS_C_MUTUTAL_FLAG;
        private const int ISC_REQ_REPLAY_DETECT = GSS_C_REPLAY_FLAG;
        private const int ISC_REQ_INTEGRITY = 0x00010000;//GSS_C_INTEG_FLAG;
        private const int ISC_REQ_CONFIDENTIALITY = 0x00000010;//GSS_C_CONF_FLAG;
        private const int LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL = 0x6201;

        private const int LBER_USE_DER = 0x01;
        private const int LDAP_ENCODING_ERROR = -3;
        private const int LDAP_DECODING_ERROR = -4;
        private const int LDAP_NO_MEMORY = -10;
        private const int LDAP_CONTROL_NOT_FOUND = -13;
        private const int LDAP_OPT_SERVER_CONTROLS = 0x0012;

        private DirectoryContext dirContext = null;

        private static System.Object Ldap_ApiCallSync = new System.Object();

        #endregion

        private IntPtr _ld;

        public LdapHandle()
        {
            _ld = IntPtr.Zero;
        }

        public LdapHandle(IntPtr ld)
            : this()
        {
            _ld = ld;
        }

        public LdapHandle(IntPtr ld, DirectoryContext dircontext)
            : this()
        {
            _ld = ld;
            dirContext = dircontext;
        }

        public IntPtr Handle
        {
            get
            {
                return _ld;
            }
        }

        public DirectoryContext DirectoryContext
        {
            set
            {
                dirContext = value;
            }
        }

        public static LdapHandle Ldap_Open(string HostName, int PortNumber)
        {
            //IntPtr ld = LdapAPI.ldap_open(HostName, PortNumber);
            //return new LdapHandle(ld);
            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                unsafe
                {
                    Logger.Log(string.Format("In Ldap_Open calling ldap_open(HostName{0},PortNumber{1})", HostName, PortNumber), Logger.ldapLogLevel);

                    IntPtr ld = LdapAPI.ldap_open(HostName, PortNumber);

                    Logger.Log(string.Format("Result of ldap_open : {0}", ld), Logger.ldapLogLevel);
                    return new LdapHandle(ld);
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Open", ex);
                return null;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Set_Version(int version)
        {
            IntPtr ptrVersion = Marshal.AllocHGlobal(Marshal.SizeOf(version));
            Marshal.WriteInt32(ptrVersion, version);

            return Ldap_Set_Option(LDAP_OPT_PROTOCOL_VERSION, ptrVersion);
        }

        public int Ldap_Set_Option(int option, IntPtr ptrVersion)
        {
            int ret = -1;
            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                unsafe
                {
                    Logger.Log(String.Format("LdapHandle.Ldap_Set_Option: _ld={0}, option={1}, version={2}",
                        _ld, option, ptrVersion),
                        Logger.ldapLogLevel);

                    ret = LdapAPI.ldap_set_option(_ld, option, ptrVersion);

                    Logger.Log(String.Format("LdapHandle.Ldap_Set_Option: ret={0}", ret), Logger.ldapLogLevel);

                    return (ret);
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Set_Option", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Get_Option(int option, IntPtr version)
        {
            //return LdapAPI.ldap_get_option(_ld, option, version);
            int ret = -1;
            Logger.Log(string.Format("Calling LdapHandle.Ldap_Get_Option(option{0},version{1})", option, version), Logger.ldapLogLevel);
            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                unsafe
                {
                    Logger.Log(String.Format("Calling ldap_get_option: _ld={0}, option={1}, version={2}",
                        _ld, option, version),
                        Logger.ldapLogLevel);

                    ret = LdapAPI.ldap_get_option(_ld, option, version);

                    Logger.Log(String.Format("Result of ldap_get_option: ret={0}", ret), Logger.ldapLogLevel);

                    return (ret);
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Get_Option", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Cancel(IntPtr[] serverCtrls)
        {
            int ret = -1;
            int msgidp = 0;

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                unsafe
                {
                    Logger.Log(string.Format("In Ldap_Cancel calling ldap_cancel(serverCtrls {0}", serverCtrls[0].ToString()), Logger.ldapLogLevel);

                    ret = LdapAPI.ldap_cancel(_ld,
                                               (int)1,
                                               null,
                                               null,
                                               out msgidp
                                               );

                    Ldap_Abandon(msgidp);
                }

                Logger.Log(string.Format("Result of Ldap_Cancel : {0}", ret), Logger.ldapLogLevel);
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Cancel", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }

            return ret;
        }

        public int Ldap_Abandon(int msgidp)
        {
            int ret = -1;

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                unsafe
                {
                    ret = LdapAPI.ldap_abandon(_ld,
                                               msgidp);

                    Logger.Log(string.Format("Result of ldap_abandon : {0}", ret), Logger.ldapLogLevel);
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.ldap_abandon", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }

            return ret;
        }

        public int Ldap_Bind_S(string dn, string cred, ulong method)
        {
            int ret = -1;

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                unsafe
                {
                    Logger.Log(String.Format("LdapHandle.Ldap_Bind_S: dn={0}, cred={1}, method={2}",
                        dn, cred, method),
                        Logger.ldapLogLevel);

                    ret = LdapAPI.ldap_bind_s(_ld, dn, cred, method);

                    Logger.Log(String.Format("LdapHandle.Ldap_Bind_S: ret={0}",
                    ret), Logger.ldapLogLevel);
                    if (ret != 0)
                    {
                        Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                    }
                    return ret;
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Bind_S", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Bind(string dn, string cred, int method)
        {
            //return LdapAPI.ldap_bind(_ld, dn, cred, method);
            int ret = -1;

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                unsafe
                {
                    Logger.Log(String.Format("Calling Ldap_Bind(_Id={0}, dn={1}, cred={2}, method={3})",
                         _ld, dn, cred, method), Logger.ldapLogLevel);

                    ret = LdapAPI.ldap_bind(_ld, dn, cred, method);

                    Logger.Log(String.Format("Result of LdapHandle.Ldap_Bind_S: ret={0}",
                                              ret), Logger.ldapLogLevel);
                    if (ret != 0)
                    {
                        Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                    }
                    return ret;
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Bind", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Simple_Bind(string dn, string password)
        {
            //return LdapAPI.ldap_simple_bind(_ld, dn, password);
            int ret = -1;

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                Logger.Log(String.Format("Calling Ldap_Simple_Bind(_Id={0}, dn={1}, password={2})",
                     _ld, dn, password), Logger.ldapLogLevel);

                unsafe
                {
                    ret = LdapAPI.ldap_simple_bind(_ld, dn, password);
                }

                Logger.Log(String.Format("Result of LdapHandle.Ldap_Simple_Bind: ret={0}",
                                          ret), Logger.ldapLogLevel);
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                }
                return ret;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Simple_Bind", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }


        public int Ldap_Simple_Bind_S(string dn, string password)
        {
            //return LdapAPI.ldap_simple_bind_s(_ld, dn, password);
            int ret = -1;

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                Logger.Log(String.Format("Calling Ldap_Simple_Bind_S(_Id={0}, dn={1}, password={2})",
                     _ld, dn, password), Logger.ldapLogLevel);

                unsafe
                {
                    ret = LdapAPI.ldap_simple_bind_s(_ld, dn, password);
                }

                Logger.Log(String.Format("Result of LdapHandle.Ldap_Simple_Bind_S: ret={0}",
                                          ret), Logger.ldapLogLevel);
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                }
                return ret;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.ldap_simple_bind_s", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Unbind_S()
        {
            //return LdapAPI.ldap_unbind_s(_ld);
            int ret = -1;

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                Logger.Log(String.Format("Calling Ldap_Unbind_S(_Id={0})",
                     _ld), Logger.ldapLogLevel);

                unsafe
                {
                    ret = LdapAPI.ldap_unbind_s(_ld);
                }

                Logger.Log(String.Format("Result of LdapHandle.Ldap_Unbind_S: ret={0}",
                                          ret), Logger.ldapLogLevel);
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                }
                return ret;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Unbind_S", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Search_ST(
            string basedn,
            int scope,
            string filter,
            string[] attrs,
            int attrsonly,
            LdapTimeVal tv,
            out LdapMessage ldapMessage
            )
        {
            DateTime beforeSearch = DateTime.Now;
            DateTime afterSearch = DateTime.Now;
            int ret = -1;
            ldapMessage = null;

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                unsafe
                {
                    IntPtr umldapMessage = new IntPtr();
                    IntPtr timeout = tv.ConvertToUM();
                    Logger.Log(String.Format("Calling Ldap_Search_ST(basedn={0}, scope={1}, filter={2})",
                                              basedn, scope, filter),
                                              Logger.ldapLogLevel);

                    ret = Ldap_Rebind_S();
                    if (ret != 0)
                    {
                        Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                        ldapMessage = null;
                        return ret;
                    }

                    ret = LdapAPI.ldap_search_st(_ld, basedn, scope, filter, attrs, attrsonly, timeout, out umldapMessage);

                    if (ret != 0)
                    {
                        Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                        ldapMessage = null;
                        return ret;
                    }

                    if (umldapMessage == IntPtr.Zero)
                    {
                        ldapMessage = null;
                        return ret;
                    }
                    ldapMessage = new LdapMessage(this, umldapMessage);

                    return ret;
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Search_ST", ex);
                return ret;
            }
            finally
            {
                afterSearch = DateTime.Now;
                Logger.Log(String.Format("Ldap_Search_ST complete.  Time elapsed: {0}",
                afterSearch - beforeSearch), Logger.ldapLogLevel);

                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Search_S(
            string basedn,
            int scope,
            string filter,
            string[] attrs,
            int attrsonly,
            out LdapMessage ldapMessage
            )
        {
            DateTime beforeSearch = DateTime.Now;
            DateTime afterSearch = DateTime.Now;
            int ret = -1;
            ldapMessage = null;

            if (String.IsNullOrEmpty(basedn))
            {
                return ret;
            }

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                unsafe
                {
                    IntPtr umldapMessage = IntPtr.Zero;
                    Logger.Log(String.Format("Calling ldap_search_s(basedn={0}, scope={1}, filter={2})",
                        basedn, scope, filter), Logger.ldapLogLevel);

                    ret = Ldap_Rebind_S();
                    if (ret != 0)
                    {
                        Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                        ldapMessage = null;
                        return ret;
                    }

                    ret = LdapAPI.ldap_search_s(_ld, basedn, scope, filter, attrs, attrsonly, out umldapMessage);

                    if (ret != 0 && ret != (int)Likewise.LMC.Utilities.ErrorCodes.LDAPEnum.LDAP_SIZELIMIT_EXCEEDED && ret != 1)
                    {
                        Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                        ldapMessage = null;
                        return ret;
                    }

                    if (umldapMessage == IntPtr.Zero)
                    {
                        ldapMessage = null;
                        return ret;
                    }

                    ldapMessage = new LdapMessage(this, umldapMessage);
                    return ret;
                }
            }
            catch (AccessViolationException)
            {
                //Catching this exception type separately is a workaround HACK for bug# 4878.
                Logger.Log(String.Format(
                    "LdapHandle.LdapSearch_S: Caught AccessViolationException, possibly due to MS Windows LDAP Bug.  baseDN={0}",
                    basedn), Logger.ldapLogLevel);
                return ret;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Search_s", ex);
                return ret;
            }
            finally
            {
                afterSearch = DateTime.Now;
                Logger.Log(String.Format("ldap_search_s complete.  Time elapsed: {0}",
                    afterSearch - beforeSearch), Logger.ldapLogLevel);

                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Search_Ext(
                IntPtr ld,
                string basedn,
                int scope,
                string filter,
                string[] attrs,
                int attrsonly,
                LDAPControl[] ServerControls,
                LDAPControl[] ClientControls,
                int TimeLimit,
                int SizeLimit,
                out LdapMessage ldapMessage
                )
        {
            int ret = -1;
            ldapMessage = null;
            IntPtr umldapMessage = new IntPtr();
            LdapTimeVal tm = new LdapTimeVal(5, 6);

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                Logger.Log(String.Format("Calling Ldap_Search_Ext(basedn={0}, scope={1}, filter={2})",
                     basedn, scope, filter), Logger.ldapLogLevel);

                unsafe
                {
                    ret = Ldap_Rebind_S();
                    if (ret != 0)
                    {
                        Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                        ldapMessage = null;
                        return ret;
                    }

                    ret = LdapAPI.ldap_search_s(_ld, basedn, scope, filter, attrs, attrsonly, out umldapMessage);
                    if (ret != 0)
                    {
                        Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                        ldapMessage = null;
                        return ret;
                    }
                    if (umldapMessage == IntPtr.Zero)
                    {
                        ldapMessage = null;
                        return ret;
                    }
                    ldapMessage = new LdapMessage(this, umldapMessage);
                    return ret;
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Search_Ext", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Search_Ext_S(
            string basedn,
            int scope,
            string filter,
            string[] attrs,
            int attrsonly,
            //LDAPControl[] ServerControls,
            IntPtr[] ServerControls,
            IntPtr[] ClientControls,
            LdapTimeVal timeout,
            int SizeLimit,
            out LdapMessage ldapMessage
            )
        {
            IntPtr umldapMessage = new IntPtr();
            // IntPtr[] umServerControls = null;
            //IntPtr umClientControls = IntPtr.Zero;
            IntPtr umtimeout = IntPtr.Zero;

            /*  if (ServerControls != null && ServerControls.Length > 0)
              {
                  umServerControls = new IntPtr[ServerControls.Length];
                  for (int i = 0; i < ServerControls.Length; i++)
                  {
                      umServerControls[i] = ServerControls[i].ConvertToUM();
                  }
              }*/

            //if (ClientControls != null && ClientControls.Length > 0)
            //{
            //    umClientControls = new IntPtr[ClientControls.Length];
            //    for (int i = 0; i < ClientControls.Length; i++)
            //    {
            //        umClientControls[i] = ClientControls[i].ConvertToUM();
            //    }
            //}

            if (timeout != null) umtimeout = timeout.ConvertToUM();

            int ret = -1;
            ldapMessage = null;

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                Logger.Log(String.Format("Calling Ldap_Search_Ext_S(basedn={0}, scope={1}, filter={2})",
                      basedn, scope, filter), Logger.ldapLogLevel);

                /* int ret = LdapApi.ldap_search_ext_s(
                           _ld, basedn, scope, filter, attrs, attrsonly,
               umServerControls, umClientControls, umtimeout, SizeLimit, out umldapMessage);*/

                unsafe
                {
                    ret = Ldap_Rebind_S();
                    if (ret != 0)
                    {
                        Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                        ldapMessage = null;
                        return ret;
                    }

                    ret = LdapAPI.ldap_search_ext_s(_ld,
                                                    basedn,
                                                    scope,
                                                    filter,
                                                    attrs,
                                                    0,
                                                    ServerControls,
                                                    ClientControls,
                                                    umtimeout,
                                                    SizeLimit,
                                                    out umldapMessage);
                }

                Logger.Log(String.Format("LdapHandle.Ldap_Search_Ext_S: ret={0}",
                ret), Logger.ldapLogLevel);

                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                }

                if (umldapMessage == IntPtr.Zero)
                {
                    ldapMessage = null;
                    return ret;
                }
                ldapMessage = new LdapMessage(this, umldapMessage);
                return ret;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Search_Ext_S", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Add_S(
            string basedn,
            LDAPMod[] attrs
            )
        {
            int ret = -1;

            IntPtr[] umattrs = new IntPtr[attrs.Length + 1];
            for (int i = 0; i < attrs.Length; i++)
            {
                umattrs[i] = attrs[i].ConvertToUM();
            }

            umattrs[attrs.Length] = IntPtr.Zero;

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                Logger.Log(string.Format("Calling Ldap_Add_S(basedn={0})",
                     basedn), Logger.ldapLogLevel);

                ret = Ldap_Rebind_S();
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                    return ret;
                }

                unsafe
                {
                    ret = LdapAPI.ldap_add_s(_ld, basedn, umattrs);
                }

                Logger.Log(String.Format("LdapHandle.Ldap_Add_S: ret={0}",
                ret), Logger.ldapLogLevel);

                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                }

                for (int i = 0; i < attrs.Length; i++)
                {
                    attrs[i].ldapfree();
                }

                return ret;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Add_S", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Add(
            string basedn,
            LDAPMod[] attrs
            )
        {
            int ret = -1;
            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                IntPtr[] umattrs = new IntPtr[attrs.Length + 1];
                for (int i = 0; i < attrs.Length; i++)
                {
                    umattrs[i] = attrs[i].ConvertToUM();
                }

                umattrs[attrs.Length] = IntPtr.Zero;

                Logger.Log(string.Format("Calling Ldap_Add(basedn={0})",
                     basedn), Logger.ldapLogLevel);

                ret = Ldap_Rebind_S();
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                    return ret;
                }
                unsafe
                {
                    ret = LdapAPI.ldap_add(_ld, basedn, umattrs);
                }

                Logger.Log(String.Format("LdapHandle.Ldap_Add: ret={0}",
                ret), Logger.ldapLogLevel);

                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                }

                for (int i = 0; i < attrs.Length; i++)
                {
                    attrs[i].ldapfree();
                }

                return ret;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Add", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Add_Ext(
            string dn,
            LDAPMod[] attrs,
            LDAPControl[] ServerControls,
            LDAPControl[] ClientControls,
            out int MessageNumber
            )
        {
            int ret = -1;
            MessageNumber = -1;
            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                IntPtr[] umattrs = new IntPtr[attrs.Length + 1];
                for (int i = 0; i < attrs.Length; i++)
                {
                    umattrs[i] = attrs[i].ConvertToUM();
                }
                umattrs[attrs.Length] = IntPtr.Zero;

                IntPtr[] umServerControls = new IntPtr[ServerControls.Length];
                for (int i = 0; i < ServerControls.Length; i++)
                {
                    umServerControls[i] = ServerControls[i].ConvertToUM();
                }


                IntPtr[] umClientControls = new IntPtr[ClientControls.Length];
                for (int i = 0; i < ClientControls.Length; i++)
                {
                    umClientControls[i] = ClientControls[i].ConvertToUM();
                }

                Logger.Log(string.Format("Calling Ldap_Add_Ext(_ld={0}, dn={1}, umattrs={2})",
                     _ld, dn, umattrs), Logger.ldapLogLevel);

                ret = Ldap_Rebind_S();
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                    return ret;
                }

                unsafe
                {
                    ret = LdapAPI.ldap_add_ext(_ld, dn, umattrs, umServerControls, umClientControls, out MessageNumber);
                }

                Logger.Log(String.Format("LdapHandle.Ldap_Add_Ext: ret={0}",
                ret), Logger.ldapLogLevel);

                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                }

                for (int i = 0; i < attrs.Length; i++)
                {
                    attrs[i].ldapfree();
                }
                return ret;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Add_Ext", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Add_Ext_S(
            string dn,
            LDAPMod[] attrs,
            LDAPControl[] ServerControls,
            LDAPControl[] ClientControls,
            out LdapMessage ldapMessage
            )
        {

            int ret = -1;
            ldapMessage = null;
            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                IntPtr umldapMessage = new IntPtr();
                IntPtr[] umattrs = new IntPtr[attrs.Length + 1];
                for (int i = 0; i < attrs.Length; i++)
                {
                    umattrs[i] = attrs[i].ConvertToUM();
                }
                umattrs[attrs.Length] = IntPtr.Zero;


                IntPtr[] umServerControls = new IntPtr[ServerControls.Length];
                for (int i = 0; i < ServerControls.Length; i++)
                {
                    umServerControls[i] = ServerControls[i].ConvertToUM();
                }


                IntPtr[] umClientControls = new IntPtr[ClientControls.Length];
                for (int i = 0; i < ClientControls.Length; i++)
                {
                    umClientControls[i] = ClientControls[i].ConvertToUM();
                }

                Logger.Log(string.Format("Calling Ldap_Add_Ext_S(_ld={0}, dn={1}, umattrs={2})",
                     _ld, dn, umattrs), Logger.ldapLogLevel);

                ret = Ldap_Rebind_S();
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                    return ret;
                }

                unsafe
                {
                    ret = LdapAPI.ldap_add_ext_s(_ld, dn, umattrs, umServerControls, umClientControls, out umldapMessage);
                }

                Logger.Log(String.Format("LdapHandle.Ldap_Add_Ext_S: ret={0}",
                ret), Logger.ldapLogLevel);

                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                }

                ldapMessage = new LdapMessage(this, umldapMessage);

                for (int i = 0; i < attrs.Length; i++)
                {
                    attrs[i].ldapfree();
                }

                return ret;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Add_Ext_S", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Modify_S(
            string basedn,
            LDAPMod[] attrs
            )
        {
            int ret = -1;
            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                IntPtr[] umattrs = new IntPtr[attrs.Length + 1];
                for (int i = 0; i < attrs.Length; i++)
                {
                    umattrs[i] = attrs[i].ConvertToUM();
                }

                umattrs[attrs.Length] = IntPtr.Zero;

                Logger.Log(string.Format("Calling Ldap_Modify_S(_ld={0}, basedn={1}, umattrs={2})",
                     _ld, basedn, umattrs), Logger.ldapLogLevel);

                ret = Ldap_Rebind_S();
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                    return ret;
                }

                unsafe
                {
                    ret = LdapAPI.ldap_modify_s(_ld, basedn, umattrs);
                }

                Logger.Log(String.Format("LdapHandle.Ldap_Modify_S: ret={0}",
                ret), Logger.ldapLogLevel);

                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                }

                for (int i = 0; i < attrs.Length; i++)
                {
                    attrs[i].ldapfree();
                }
                return ret;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Modify_S", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Modify(
            string basedn,
            LDAPMod[] attrs
            )
        {
            int ret = -1;
            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                IntPtr[] umattrs = new IntPtr[attrs.Length + 1];
                for (int i = 0; i < attrs.Length; i++)
                {
                    umattrs[i] = attrs[i].ConvertToUM();
                }
                umattrs[attrs.Length] = IntPtr.Zero;
                for (int i = 0; i < attrs.Length; i++)
                {
                    attrs[i].ldapfree();
                }

                Logger.Log(String.Format("LdapHandle.ldap_add_s: _ld={0}, basedn={1}, umattrs={2}",
                   _ld, basedn, umattrs),
                   Logger.ldapLogLevel);

                ret = Ldap_Rebind_S();
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                    return ret;
                }

                unsafe
                {
                    ret = LdapAPI.ldap_add_s(_ld, basedn, umattrs);
                }

                Logger.Log(String.Format("LdapHandle.ldap_add_s: ret={0}",
                ret), Logger.ldapLogLevel);

                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                }

                return (ret);
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.ldap_add_s", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Delete_S(
            string dn
            )
        {
            //return LdapAPI.ldap_delete_s(_ld, dn);
            int ret = -1;
            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                Logger.Log(String.Format("LdapHandle.ldap_delete_s: _ld={0}, dn={1}",
                    _ld, dn),
                    Logger.ldapLogLevel);

                ret = Ldap_Rebind_S();
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                    return ret;
                }

                unsafe
                {
                    ret = LdapAPI.ldap_delete_s(_ld, dn);
                }

                Logger.Log(String.Format("LdapHandle.ldap_delete_s: ret={0}",
                ret), Logger.ldapLogLevel);

                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                }

                return (ret);
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.ldap_delete_s", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Delete(
            string basedn,
            LDAPMod[] attrs
            )
        {
            //return LdapAPI.ldap_delete_s(_ld, basedn);
            int ret = -1;
            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                Logger.Log(String.Format("LdapHandle.ldap_delete_s: _ld={0}, basedn={1}",
                    _ld, basedn),
                    Logger.ldapLogLevel);

                ret = Ldap_Rebind_S();
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                    return ret;
                }

                unsafe
                {
                    ret = LdapAPI.ldap_delete_s(_ld, basedn);
                }

                Logger.Log(String.Format("LdapHandle.ldap_delete_s: ret={0}",
                ret), Logger.ldapLogLevel);

                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                }

                return (ret);
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.ldap_delete_s", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        /// <summary>
        /// Rename an LDAP entry.
        ///
        /// Note on newSuperior:
        /// The newparent parameter should always be NULL when using version 2 of the LDAP protocol;
        /// otherwise the server's behavior is undefined. If this parameter is NULL, only the RDN of the entry is changed.
        /// The root DN may be specified by passing a zero length string, "".
        ///
        /// Note on deleteOldRdn:
        /// When set to 1, the old RDN value is to be deleted from the entry.
        /// When set to 0, the old RDN value should be retained as a non-distinguished value.
        /// This parameter only has meaning if newrdn is different from the old RDN.
        /// </summary>
        /// <param name="basedn">Specifies the DN of the entry whose DN is to be changed.</param>
        /// <param name="newdn">Specifies the new RDN to be given to the entry.</param>
        /// <param name="newSuperior">Specifies the new parent, or superior entry. </param>
        /// <param name="deleteOldRdn">Specifies a boolean value; 1=delete the old rdn; 0=retain the old rdn</param>
        /// <returns>The error code (0 for success)</returns>
        public int Ldap_Rename_S(
            string basedn,
            string newdn,
            string newSuperior,
            int deleteOldRdn
            )
        {
            //return
                //LdapAPI.ldap_rename_s(_ld, basedn, newdn, newSuperior, deleteOldRdn, null, null);
            int ret = -1;
            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                Logger.Log(String.Format("LdapHandle.ldap_rename_s: _ld={0}, basedn={1}, newdn={2}",
                    _ld, basedn, newdn),
                    Logger.ldapLogLevel);

                ret = Ldap_Rebind_S();
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                    return ret;
                }

                unsafe
                {
                    ret = LdapAPI.ldap_rename_s(_ld, basedn, newdn, newSuperior, deleteOldRdn, null, null);
                }
                Logger.Log(String.Format("LdapHandle.ldap_rename_s: ret={0}",
                ret), Logger.ldapLogLevel);

                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                }
                return (ret);
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.ldap_rename_s", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        /// <summary>
        /// Rename an LDAP entry.
        ///
        /// Note on newSuperior:
        /// The newparent parameter should always be NULL when using version 2 of the LDAP protocol;
        /// otherwise the server's behavior is undefined. If this parameter is NULL, only the RDN of the entry is changed.
        /// The root DN may be specified by passing a zero length string, "".
        ///
        /// Note on deleteOldRdn:
        /// When set to 1, the old RDN value is to be deleted from the entry.
        /// When set to 0, the old RDN value should be retained as a non-distinguished value.
        /// This parameter only has meaning if newrdn is different from the old RDN.
        /// </summary>
        /// <param name="basedn">Specifies the DN of the entry whose DN is to be changed.</param>
        /// <param name="newdn">Specifies the new RDN to be given to the entry.</param>
        /// <param name="newSuperior">Specifies the new parent, or superior entry. </param>
        /// <param name="deleteOldRdn">Specifies a boolean value; 1=delete the old rdn; 0=retain the old rdn</param>
        /// <param name="msgId">the message ID</param>
        /// <returns>The error code (0 for success)</returns>
        public int Ldap_Rename(
            string basedn,
            string newdn,
            string newSuperior,
            int deleteOldRdn,
            IntPtr msgId
            )
        {
            //return
            //    LdapAPI.ldap_rename(_ld, basedn, newdn, newSuperior, deleteOldRdn, null, null, msgId);
            int ret = -1;
            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                Logger.Log(String.Format("LdapHandle.ldap_rename_s: _ld={0}, basedn={1}, newdn={2}",
                    _ld, basedn, newdn),
                    Logger.ldapLogLevel);

                ret = Ldap_Rebind_S();
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                    return ret;
                }
                unsafe
                {
                    ret = LdapAPI.ldap_rename(_ld, basedn, newdn, newSuperior, deleteOldRdn, null, null, msgId);
                }
                Logger.Log(String.Format("LdapHandle.ldap_rename_s: ret={0}",
                ret), Logger.ldapLogLevel);
                if (ret != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(ret), Logger.ldapLogLevel);
                }

                return (ret);
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.ldap_rename", ex);
                return ret;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        /// <summary>
        /// Return the text error string for a given LDAP error code.
        /// </summary>
        /// <param name="ldapErrorCode">The LDAP error code</param>
        /// <returns>The LDAP error message</returns>
        public string GetErrorString(int ldapErrorCode)
        {
            return ErrorCodes.LDAPString(ldapErrorCode);
        }

        /// <summary>
        /// Return a pointer to BerElement
        /// </summary>
        /// <param name="berVal">The BerVal</param>
        /// <returns>The BerElement to return</returns>
        public IntPtr Ber_Init(Berval berVal)
        {
            IntPtr ptrberVal = IntPtr.Zero;

            ptrberVal = Marshal.AllocHGlobal(Marshal.SizeOf(berVal));

            Marshal.StructureToPtr(berVal, ptrberVal, false);

            return LdapAPI.ber_init(out ptrberVal);
        }


        public int setserver_pageControloption()
        {
            //set server control LDAP_OPT_SERVER_CONTROLS
            Berval berVal = new Berval(0, IntPtr.Zero);
            LDAPControl c = new LDAPControl(berVal, LdapMessage.LDAP_CONTROL_PAGEDRESULTS, 'F');
            IntPtr[] ctrls = new IntPtr[2];
            ctrls[0] = c.ConvertToUM();
            ctrls[1] = IntPtr.Zero;

            IntPtr arrayCtrls = IntPtr.Zero;

            // Allocate memory to store the array of pointers
            int sizeOfIntPtr = Marshal.SizeOf(typeof(IntPtr));
            arrayCtrls = Marshal.AllocCoTaskMem(sizeOfIntPtr * ctrls.Length);

            // Stuff the pointers into the array
            for (int counter = 0; counter < ctrls.Length; counter++)
                Marshal.WriteIntPtr(arrayCtrls, counter * sizeOfIntPtr, ctrls[counter]);

            int ret = Ldap_Set_Option(LDAP_OPT_SERVER_CONTROLS, arrayCtrls);

            return ret;
        }


        public int unsetserver_pageControloption()
        {
            //unset server control LDAP_OPT_SERVER_CONTROLS
            IntPtr[] ctrls = new IntPtr[1];
            ctrls[0] = IntPtr.Zero;

            IntPtr arrayCtrls = IntPtr.Zero;

            // Allocate memory to store the array of pointers
            int sizeOfIntPtr = Marshal.SizeOf(typeof(IntPtr));
            arrayCtrls = Marshal.AllocCoTaskMem(sizeOfIntPtr * ctrls.Length);

            // Stuff the pointers into the array
            for (int counter = 0; counter < ctrls.Length; counter++)
                Marshal.WriteIntPtr(arrayCtrls, counter * sizeOfIntPtr, ctrls[counter]);

            int ret = Ldap_Set_Option(LDAP_OPT_SERVER_CONTROLS, arrayCtrls);

            return ret;
        }

        /* unsigned long pageSize, struct berval *cookie,  const char isCritical, LDAPControl   **control)**/
        public int Ldap_Create_Page_Control(int pageSize, IntPtr cookie, char pagingCriticality, out IntPtr control)
        {
            int rv = -1;
            control = new IntPtr();

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                Logger.Log(string.Format("Calling ldap_create_page_control(pageSize{0},cookie{1},isCritical{2})", pageSize, cookie, pagingCriticality), Logger.ldapLogLevel);

                rv = Ldap_Rebind_S();
                if (rv != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(rv), Logger.ldapLogLevel);
                    return rv;
                }

                unsafe
                {
                    rv = LdapAPI.ldap_create_page_control(_ld, pageSize, cookie, pagingCriticality, out control);
                }

                Logger.Log(String.Format("Result of ldap_create_page_control: rv={0}", rv), Logger.ldapLogLevel);

                return rv;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.Ldap_Create_Page_Control", ex);
                return rv;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        /* unsigned long pageSize, struct berval *cookie,  const char isCritical, LDAPControl   **control)**/
        public int Ldap_Parse_Page_Control(IntPtr ctrls, out ulong countp, out IntPtr cookie)
        {
            int rv = -1;
            ulong countpRet = 0;
            IntPtr cookieRet;

            countp = 0;
            cookie = IntPtr.Zero;

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                rv = Ldap_Rebind_S();
                if (rv != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(rv), Logger.ldapLogLevel);
                    return rv;
                }

                unsafe
                {
                    rv = LdapAPI.ldap_parse_page_control(_ld, ctrls, out countpRet, out cookieRet);
                }

                countp = countpRet;
                cookie = cookieRet;

                Logger.Log(String.Format("Result of ldap_parse_page_control: rv={0}", rv), Logger.ldapLogLevel);

                return rv;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.ldap_parse_page_control", ex);

                return rv;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Parse_Result(IntPtr ldapMsgRes, out int errcodep, out IntPtr serverLdapControl)
        {
            int rv = -1;
            int errcodepRet = 0;
            IntPtr serverLdapCtrlRet = IntPtr.Zero;

            errcodep = 0;
            serverLdapControl = IntPtr.Zero;

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                rv = Ldap_Rebind_S();
                if (rv != 0)
                {
                    Logger.Log(ErrorCodes.LDAPString(rv), Logger.ldapLogLevel);
                    return rv;
                }

                unsafe
                {
                    rv = LdapAPI.ldap_parse_result(_ld,
                                                   ldapMsgRes,
                                                   out errcodepRet,
                                                   IntPtr.Zero,
                                                   IntPtr.Zero,
                                                   IntPtr.Zero,
                                                   out serverLdapCtrlRet,
                                                   0);
                }

                errcodep = errcodepRet;
                serverLdapControl = serverLdapCtrlRet;

                Logger.Log(String.Format("Result of ldap_parse_result: rv={0}", rv), Logger.ldapLogLevel);

                return rv;
            }
            catch (Exception ex)
            {
                Logger.LogException("LdapHandle.ldap_parse_result", ex);
                return rv;
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }
        }

        public int Ldap_Rebind_S()
        {
            int rv = -1;

            Monitor.Enter(Ldap_ApiCallSync);
            try
            {
                unsafe
                {
                    if (_ld != IntPtr.Zero)
                    {
                        rv = Ldap_Unbind_S();
                    }

                    _ld = LdapAPI.ldap_open(dirContext.DomainControllerIP, dirContext.PortNumber);

                    if (_ld == IntPtr.Zero)
                    {
                        string errorMessage =
                            "The specified domain either does not exist or could not be contacted.  " +
                            "Please contact your system administrator to verify that your domain is " +
                            "properly configured and is currently online.";
                        Logger.Log("Ldap_Rebind_S:LdapAPI.ldap_open(): " + errorMessage);
                        return rv;
                    }

                    int version = LDAP_VERSION3;
                    IntPtr ptrVersion = IntPtr.Zero;
                    ptrVersion = Marshal.AllocHGlobal(Marshal.SizeOf(version));
                    Marshal.WriteInt32(ptrVersion, version);

                    rv = LdapAPI.ldap_set_option(_ld, LDAP_OPT_PROTOCOL_VERSION, ptrVersion);
                    Logger.Log(String.Format("Result of LdapAPI.ldap_set_option(LDAP_OPT_PROTOCOL_VERSION): rv={0}", rv), Logger.ldapLogLevel);

                    rv = LdapAPI.ldap_set_option(_ld, LDAP_OPT_REFERRALS, IntPtr.Zero);
                    Logger.Log(String.Format("Result of LdapAPI.ldap_set_option(LDAP_OPT_REFERRALS): rv={0}", rv), Logger.ldapLogLevel);

                    if (dirContext.BindMethod)
                    {
                        string distinguishedName = string.Format("cn={0},cn=Users,{1}", dirContext.UserName, dirContext.RootDN);
                        if (!String.IsNullOrEmpty(dirContext.UserName) && !dirContext.UserName.Equals("administrator", StringComparison.InvariantCultureIgnoreCase))
                            distinguishedName = dirContext.UserName;

                        Logger.Log(String.Format("Ldap_Rebind_S: baseDn", distinguishedName), Logger.ldapLogLevel);

                        Logger.Log(String.Format("LdapAPI.ldap_bind_s(ld = {0}, distinguishedName = {1} , Password = {2} , (ulong)LDAP_AUTH.LDAP_AUTH_SIMPLE)", _ld.ToString(), distinguishedName, dirContext.Password), Logger.ldapLogLevel);

                        if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                        {
                            rv = LdapAPI.ldap_bind_s(_ld, distinguishedName, dirContext.Password, (ulong)LDAP_AUTHWindows.LDAP_AUTH_SIMPLE);
                        }
                        else
                        {
                            rv = LdapAPI.ldap_bind_s(_ld, distinguishedName, dirContext.Password, (ulong)LDAP_AUTHLinux.LDAP_AUTH_SIMPLE);
                        }
                        Logger.Log(String.Format("Result of LdapAPI.ldap_bind_s: rv={0}", rv), Logger.ldapLogLevel);
                    }
                    else
                    {
                        if (Configurations.currentPlatform != LikewiseTargetPlatform.Windows)
                        {
                            //set GSS-SPNEGO options
                            int secGssflags = ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT;

                            IntPtr ptrGssSecflags = IntPtr.Zero;
                            ptrGssSecflags = Marshal.AllocHGlobal(Marshal.SizeOf(secGssflags));
                            Marshal.WriteInt32(ptrGssSecflags, secGssflags);
                            rv = LdapAPI.ldap_set_option(_ld, LDAP_OPT_SSPI_FLAGS, ptrGssSecflags);

                            char ber_pvt_opt_on = '1';
                            IntPtr ptrRemotePrincipalflags = IntPtr.Zero;
                            ptrRemotePrincipalflags = Marshal.AllocHGlobal(Marshal.SizeOf(ber_pvt_opt_on));
                            Marshal.WriteInt32(ptrRemotePrincipalflags, ber_pvt_opt_on);
                            rv = LdapAPI.ldap_set_option(_ld, LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL, ptrRemotePrincipalflags);
                            Logger.Log(String.Format("Result of LdapAPI.ldap_set_option (LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL): rv={0}", rv), Logger.ldapLogLevel);
                            if (rv != 0)
                            {
                                return rv;
                            }
                        }


                        //set GSS-SPNEGO options
                        int secflags = ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT;

                        IntPtr ptrSecflags = IntPtr.Zero;
                        ptrSecflags = Marshal.AllocHGlobal(Marshal.SizeOf(secflags));
                        Marshal.WriteInt32(ptrSecflags, secflags);
                        rv = LdapAPI.ldap_set_option(_ld, LDAP_OPT_SSPI_FLAGS, ptrSecflags);
                        Logger.Log(String.Format("Result of LdapAPI.ldap_set_option (LDAP_OPT_SSPI_FLAGS): rv={0}", rv), Logger.ldapLogLevel);
                        if (rv != 0)
                        {
                            return rv;
                        }

                        secflags |= ISC_REQ_INTEGRITY;

                        ptrSecflags = IntPtr.Zero;
                        ptrSecflags = Marshal.AllocHGlobal(Marshal.SizeOf(secflags));
                        Marshal.WriteInt32(ptrSecflags, secflags);
                        rv = LdapAPI.ldap_set_option(_ld, LDAP_OPT_SSPI_FLAGS, ptrSecflags);
                        Logger.Log(String.Format("Result of LdapAPI.ldap_set_option (LDAP_OPT_SSPI_FLAGS): rv={0}", rv), Logger.ldapLogLevel);
                        if (rv != 0)
                        {
                            return rv;
                        }

                        secflags |= ISC_REQ_CONFIDENTIALITY;

                        ptrSecflags = IntPtr.Zero;
                        ptrSecflags = Marshal.AllocHGlobal(Marshal.SizeOf(secflags));
                        Marshal.WriteInt32(ptrSecflags, secflags);
                        rv = LdapAPI.ldap_set_option(_ld, LDAP_OPT_SSPI_FLAGS, ptrSecflags);
                        Logger.Log(String.Format("Result of LdapAPI.ldap_set_option (LDAP_OPT_SSPI_FLAGS): rv={0}", rv), Logger.ldapLogLevel);
                        if (rv != 0)
                        {
                            return rv;
                        }


                        Logger.Log(String.Format("LdapAPI.ldap_bind_s(ld = {0}, null, null, (ulong)LDAP_AUTH.LDAP_AUTH_NEGOTIATE)", _ld.ToString()), Logger.ldapLogLevel);

                        if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                        {
                            rv = LdapAPI.ldap_bind_s(_ld, null, null, (ulong)LDAP_AUTHWindows.LDAP_AUTH_NEGOTIATE);
                        }
                        else
                        {
                            rv = LdapAPI.ldap_bind_s(_ld, null, null, (ulong)LDAP_AUTHLinux.LDAP_AUTH_NEGOTIATE);
                        }

                        Logger.Log(String.Format("Result of LdapAPI.ldap_bind_s: rv={0}", rv), Logger.ldapLogLevel);
                    }
                }
            }
            catch(Exception e)
            {
                Logger.LogException("Ldap_Rebind_S()", e);
            }
            finally
            {
                Monitor.Exit(Ldap_ApiCallSync);
            }

            return rv;
        }

    }
}
