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
using System.Threading;
using System.Collections.Generic;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.LDAP.Interop;
using System.Net;

namespace Likewise.LMC.LDAP
{

    public enum LDAP_AUTHLinux : int
    {
        LDAP_AUTH_NONE = (int)0x00U,
        LDAP_AUTH_SIMPLE = (int)0x80U,
        LDAP_AUTH_SASL = (int)0x83U,

        //0x86L), //Windows SSO support value
        //0x86U), //Linux SSO support value
        LDAP_AUTH_OTHERKIND = (int)0x86U,
        LDAP_AUTH_EXTERNAL = (int)(LDAP_AUTH_OTHERKIND | 0x20U),
        LDAP_AUTH_SICILY = (int)(LDAP_AUTH_OTHERKIND | 0x200U),

        // This will cause the client to use the GSSAPI negotiation
        // package to determine the most appropriate authentication type.
        // This type should be used when talking to NT5.

        //0x04FFU), //Windows SSO support value
        //0x0400), //Linux SSO support value         
        LDAP_AUTH_NEGOTIATE = (int)(LDAP_AUTH_OTHERKIND | 0x04FFU),

        LDAP_AUTH_MSN = (int)(LDAP_AUTH_OTHERKIND | 0x800U),
        LDAP_AUTH_NTLM = (int)(LDAP_AUTH_OTHERKIND | 0x1000U),
        LDAP_AUTH_DIGEST = (int)(LDAP_AUTH_OTHERKIND | 0x4000U),
        LDAP_AUTH_DPA = (int)(LDAP_AUTH_OTHERKIND | 0x2000U),
        LDAP_AUTH_SSPI = (int)LDAP_AUTH_NEGOTIATE
    }

    public enum LDAP_AUTHWindows : int
    {
        LDAP_AUTH_NONE = (int)0x00U,
        LDAP_AUTH_SIMPLE = (int)0x80U,
        LDAP_AUTH_SASL = (int)0x83U,

        //0x86L), //Windows SSO support value
        //0x86U), //Linux SSO support value
        LDAP_AUTH_OTHERKIND = (int)0x86L,
        LDAP_AUTH_EXTERNAL = (int)(LDAP_AUTH_OTHERKIND | (int)0x20U),
        LDAP_AUTH_SICILY = (int)(LDAP_AUTH_OTHERKIND | 0x200U),

        // This will cause the client to use the GSSAPI negotiation
        // package to determine the most appropriate authentication type.
        // This type should be used when talking to NT5.

        //0x04FFU), //Windows SSO support value
        //0x0400), //Linux SSO support value           
        LDAP_AUTH_NEGOTIATE = (int)(LDAP_AUTH_OTHERKIND | 0x0400),

        LDAP_AUTH_MSN = (int)(LDAP_AUTH_OTHERKIND | 0x800U),
        LDAP_AUTH_NTLM = (int)(LDAP_AUTH_OTHERKIND | 0x1000U),
        LDAP_AUTH_DIGEST = (int)(LDAP_AUTH_OTHERKIND | 0x4000U),
        LDAP_AUTH_DPA = (int)(LDAP_AUTH_OTHERKIND | 0x2000U),
        LDAP_AUTH_SSPI = (int)LDAP_AUTH_NEGOTIATE
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
    public struct SEC_WINNT_AUTH_IDENTITY
    {
        public string User;
        public ulong UserLength;
        public string Domain;
        public ulong DomainLength;
        public string Password;
        public ulong PasswordLength;
        public ulong Flags;
    }

    public enum SEC_AINNT_AUTH_FLAG
    {
        SEC_WINNT_AUTH_IDENTITY_ANSI = 0x1,
        SEC_WINNT_AUTH_IDENTITY_UNICODE = 0x2
    }

    /// <summary>
    /// Summary description for DirectoryContext.
    /// </summary>
    public class DirectoryContext
    {
        #region data

        private const int LDAP_VERSION3 = 3;
        private const int LDAP_NO_LIMIT = 0;

        private const int LDAP_OPT_SIZELIMIT = 0x0003;
        private const int LDAP_OPT_TIMELIMIT = 0x0004;
        private const int LDAP_OPT_REFERRALS = 0x0008;
        private const int LDAP_OPT_PROTOCOL_VERSION = 0x0011;
        private const int LDAP_OPT_SSPI_FLAGS = 0x0092;

        private const int GSS_C_MUTUTAL_FLAG = 2;
        private const int GSS_C_REPLAY_FLAG = 4;
        private const int GSS_C_INTEG_FLAG = 32;
        private const int GSS_C_CONF_FLAG = 16;

        private const int ISC_REQ_MUTUAL_AUTH = GSS_C_MUTUTAL_FLAG;
        private const int ISC_REQ_REPLAY_DETECT = GSS_C_REPLAY_FLAG;
        private const int ISC_REQ_INTEGRITY = 0x00010000;//GSS_C_INTEG_FLAG;
        private const int ISC_REQ_CONFIDENTIALITY = 0x00000010;//GSS_C_CONF_FLAG;

        private const int LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL = 0x6201;
        
        private string _domainControllerName;
        private string _domainName;
        private string _rootDN;
        private string _distinguishedName;
        private string _userName;
        private string _password;
        private LdapHandle _ldapHandle;
        private int _portNumber = 389;

        private string _gcServerName;
        private bool _bindMethod;
        private string _domainControllerIP;
        private string _configurationNamingContext;
        private string _defaultNamingContext;
        private string _schemaNamingContext;
        private string _dnsHostName;
        private string _rootDomainNamingContext;
        private static string[] _supportedSASLMechanisms = null;
        private LDAPSchemaCache _ldapSchemaCache;

        private IntPtr[] ServerCtrls = new IntPtr[2];

        //this is in order to keep track of the aducPlugin treeview in lmc main form so that when connection has timed out, 
        //we can prompt to clear the current deadtree and ask user to reconnect
        //private LWTreeView _lmcLWTreeView;
        //private ListView _lvChildNodes;
        private List<LdapEntry> _ListChildEntriesSync;

        private System.Object lockThis_searchsyn = new System.Object();
        private System.Object lockThis_searchExtsyn = new System.Object();
        private System.Object lockThis_addSyn = new System.Object();
        private System.Object lockThis_delSyn = new System.Object();
        private System.Object lockThis_modSyn = new System.Object();        
        private System.Object lockThis_ldapCancel = new System.Object();
        private System.Object lockThis_ldapAbandon = new System.Object();

        #endregion

        #region constructors


        protected DirectoryContext(
                    string ServerName,
                    string rootDN,
                    string DistinguishedName,
                    string UserName,
                    string Password,
                    LdapHandle ldapHandle,
                    int portNumber)
        {
            _rootDN = rootDN;

            string[] domainDNcom = _rootDN.Split(',');

            string domainName = "";
            foreach (string str in domainDNcom)
            {
                string temp = string.Concat(str.Substring(3), ".");
                domainName = string.Concat(domainName, temp);
            }

            domainName = domainName.Substring(0, domainName.Length - 1);

            _domainName = domainName;

            _domainControllerName = ServerName;
            _distinguishedName = DistinguishedName;
            _userName = UserName;
            _password = Password;
            _ldapHandle = new LdapHandle(ldapHandle.Handle, this);
            _portNumber = portNumber;
        }

        public DirectoryContext(DirectoryContext dirContext)
        {

            _rootDN = dirContext.RootDN;

            string[] domainDNcom = _rootDN.Split(',');

            string domainName = "";
            foreach (string str in domainDNcom)
            {
                string temp = string.Concat(str.Substring(3), ".");
                domainName = string.Concat(domainName, temp);
            }

            domainName = domainName.Substring(0, domainName.Length - 1);

            _domainName = domainName;

            _domainControllerName = dirContext.DomainControllerName;
            _distinguishedName = dirContext.DistinguishedName;
            _userName = dirContext.UserName;
            _password = dirContext.Password;
            _ldapHandle = new LdapHandle(dirContext.LdapHandle.Handle, this);
            _portNumber = dirContext.PortNumber;
        }


        #endregion

        #region public LDAP wrapper methods

         public int ListChildEntriesSynchronous(
                                 string basedn,
                                 LdapAPI.LDAPSCOPE scope,
                                 string filter,
                                 string[] search_attrs,
                                 bool attrsonly,
                                 out List<LdapEntry> ldapEntries   
                                 )
         {
             int ret = 0;


             ldapEntries = new List<LdapEntry>();
             //LdapMessage message = null;

             //TODO: Need to replace this with Ldap_Paged_SearchSynchronous function call to query the objects based on paging
             //ret = SearchSynchronous(
             //         basedn,
             //         scope,
             //         filter,
             //         search_attrs,
             //         attrsonly,
             //         out message);
             ret = PagedSearchSynchronous(
                      basedn,
                      scope,
                      filter,
                      search_attrs,
                      out ldapEntries);
             ret = handleSearchError("ListChildEntriesSynchronous", basedn, ret);

             //process the entries returned in the current page (in ldapMessage)
             //Uncomment if we are not using paging
             //if (message != null)
             //{
             //    List<LdapEntry> ldapEntriesTemp = message.Ldap_Get_Entries();
             //    if (ldapEntriesTemp != null)
             //    {
             //        ldapEntries = ldapEntriesTemp;
             //    }
             //}

             return ret;
         }

         /// <summary>
         /// SearchSynchronous function    
         /// </summary>
         /// <param name="basedn"></param>
         /// <param name="scope"></param>
         /// <param name="filter"></param>
         /// <param name="search_attrs"></param>
         /// <param name="attrsonly"></param>
         /// <returns>LdapMessage</returns>
        public int SearchSynchronous(
                                 string basedn,
                                 LdapAPI.LDAPSCOPE scope,
                                 string filter,
                                 string[] search_attrs,
                                 bool attrsonly,
                                 out LdapMessage message 
                                 )
        {
            int ret = 0;
            message = null;

            lock (lockThis_searchsyn)
            {
                ret = _ldapHandle.Ldap_Search_S(
                    basedn, 
                    (int)scope, 
                    filter, 
                    search_attrs, 
                    0, 
                    out message);

                ret = handleSearchError("SearchSynchronous", basedn, ret); 
            }
            return ret;
        }

        //this is an overload to use the style in MMCSupport
        public LdapMessage SearchSynchronous(
                         string basedn,
                         LdapAPI.LDAPSCOPE scope,
                         string filter,
                         string[] search_attrs,
                         bool attrsonly
                         )
        {
            int ret = 0;
            LdapMessage message = null;

            ret = SearchSynchronous(
                         basedn,
                         scope,
                         filter,
                         search_attrs,
                         attrsonly,
                         out message);

            return message;
        }
 

        /// <summary>
        /// SearchExtSynchronous
        /// </summary>
        /// <param name="basedn"></param>
        /// <param name="scope"></param>
        /// <param name="filter"></param>
        /// <param name="search_attrs"></param>
        /// <param name="attrsonly"></param>
        /// <param name="ServerControls"></param>
        /// <param name="ClientControls"></param>
        /// <param name="timeout"></param>
        /// <param name="SizeLimit"></param>
        /// <param name="ret"> Return value to show up the correct pop up to the user while querying</param>
        /// <returns>LdapMessage</returns>
        public int SearchExtSynchronous(
                                 string basedn,
                                 LdapAPI.LDAPSCOPE scope,
                                 string filter,
                                 string[] search_attrs,
                                 int attrsonly,
                                 IntPtr[] ServerControls,
                                 IntPtr[] ClientControls,
                                 LdapTimeVal timeout,
                                 int SizeLimit,
                                 out LdapMessage message
                                 )
        {
            message = null;
            int ret = 0;

            lock (lockThis_searchExtsyn)
            {
                ret = _ldapHandle.Ldap_Search_Ext_S(
                     basedn,
                     (int)scope,
                     filter,
                     search_attrs,
                     attrsonly,
                     ServerControls,
                     ClientControls,
                     timeout,
                     SizeLimit,
                     out message);
            }
            ret = handleSearchError("SearchExtSynchronous", basedn, ret);

            return ret;
        }


        //Function to query the object list based on paging
        public int PagedSearchSynchronous(
                                 string ObjectDN,
                                 LdapAPI.LDAPSCOPE scope,
                                 string Query,
                                 string[] AttributeList,
                                 out List<LdapEntry> ldapEntries)
        {
            ldapEntries = new List<LdapEntry>();

            //Set up LDAP_OPT_SERVER_CONTROLS option so that server can respond with message that contains page control information            
            LdapTimeVal timeout = new LdapTimeVal(600, 0);
            IntPtr page_cookiePtr = IntPtr.Zero;
            Berval cookie = new Berval(0, IntPtr.Zero);
            IntPtr[] serverCtrls = new IntPtr[2];
            IntPtr pagedControlPtr = IntPtr.Zero;
            IntPtr serverLdapControl = IntPtr.Zero;
            int errcodep = 0;
            ulong pageCount = 0;
            int PageSize = 1000;
            int ret = -1;

            //int ret = Setserver_PageControloption();
            Logger.Log("Before start using paging, enable paging control return " + ret);

            LdapMessage ldapMessage;

            do
            {
                serverCtrls = new IntPtr[2];

                if (PageSize > 0)
                {
                    //Create page control, the initial page control is created using a cookie  with Berval of {0,null}

                    //if the page size is set to be larger than the default page size in server, 
                    //the actual page size will still be the default page size set in server.                
                    int pagedRet = Ldap_Create_Page_Control((int)PageSize, page_cookiePtr, 'T', out pagedControlPtr);
                    Logger.Log("CreatePageControl return is " + pagedRet);
                    if (pagedControlPtr == IntPtr.Zero)
                    {
                        Logger.Log("The created pagedcontrol is null");
                    }

                    serverCtrls[0] = pagedControlPtr;
                    serverCtrls[1] = IntPtr.Zero;

                    ret = SearchExtSynchronous(
                                             ObjectDN,
                                             scope,
                                             Query,
                                             AttributeList,
                                             0,
                                             serverCtrls,
                                             null,
                                             timeout,
                                             0,
                                             out ldapMessage);

                    if (ldapMessage == null)
                    {
                        return ret;
                    }

                    if (serverLdapControl != IntPtr.Zero)
                    {
                        LdapAPI.ldap_control_free(serverLdapControl);
                        serverLdapControl = IntPtr.Zero; ;
                    }

                    ret = Ldap_Parse_Result(ldapMessage.MessagePtr, out errcodep, out serverLdapControl);

                    if (serverLdapControl == IntPtr.Zero)
                    {
                        Logger.Log(" LdapApi.ldap_parse_result" + ret.ToString());
                        return ret;
                    }

                    if (page_cookiePtr != IntPtr.Zero)
                    {
                        LdapAPI.ber_free(page_cookiePtr, 0);
                        page_cookiePtr = IntPtr.Zero;
                    }

                    ret = Ldap_Parse_Page_Control(serverLdapControl, out pageCount, out page_cookiePtr);

                    if (ret != 0)
                    {
                        Logger.Log("ParsePageControl" + ret.ToString());
                        return ret;
                    }

                    if (serverLdapControl != IntPtr.Zero)
                    {
                        LdapAPI.ldap_controls_free(serverLdapControl);
                        serverLdapControl = IntPtr.Zero;
                    }

                    if (pagedControlPtr != IntPtr.Zero)
                    {
                        LdapAPI.ldap_control_free(pagedControlPtr);
                        pagedControlPtr = IntPtr.Zero;
                    }
                }
                else
                {
                    ret = SearchExtSynchronous(
                                           ObjectDN,
                                           scope,
                                           Query,
                                           AttributeList,
                                           0,
                                           null,
                                           null,
                                           timeout,
                                           0,
                                           out ldapMessage);

                    if (ret != 0)
                    {
                        return ret;
                    }
                }

                //process the entries returned in the current page (in ldapMessage)
                List<LdapEntry> onePageldapEntries = (ldapMessage != null ? ldapMessage.Ldap_Get_Entries() : null);
                if (onePageldapEntries != null)
                {
                    foreach (LdapEntry entry in onePageldapEntries)
                    {
                        ldapEntries.Add(entry);
                    }
                }

                if (page_cookiePtr != IntPtr.Zero)
                {
                    cookie = (Berval)Marshal.PtrToStructure(page_cookiePtr, typeof(Berval));
                    Logger.Log("cookie.bv_len is " + cookie.bv_len + "cookie.bv_val is " + cookie.bv_val);
                }

            } while (PageSize > 0 && page_cookiePtr != IntPtr.Zero && cookie.bv_val != IntPtr.Zero && cookie.bv_len > 0);

            ServerCtrls[0] = pagedControlPtr;
            ServerCtrls[1] = IntPtr.Zero;

            return ret;
        }

        #endregion

        #region LDAP wrapper (helper methods)

        private int handleSearchError(string caller, string dn, int ret)
        {

            //error code 1 appears to be spurious, but this needs further investigation.
            if (ret == 1)
            {
                ret = 0;
            }

            //error code 4 appears to be spurious, but this needs further investigation.
            if (ret == 4)
            {
                ret = 0;
            }

            if (ret == -1)
            {
                ret = (int)ErrorCodes.LDAPEnum.LDAP_TIMEOUT;
            }

            if (ret != 0)
            {
                Logger.Log(String.Format(
                    "DirectoryContext.{0} run against {1} returned error {2} ({3})",
                    caller,
                    dn,
                    ret,
                    ErrorCodes.LDAPString(ret)),
                    Logger.LogLevel.Error);
            }

            return ret;
        }

        #endregion 

        #region public methods

        //Function to create domain level dirContext object
        public static DirectoryContext CreateDirectoryContext(
                                        string DomainControllerName,
                                        string rootDN,
                                        string UserName,
                                        string Password,
                                        int portNumber,
                                        bool usingSimpleBind,
                                        out string errorMessage)
        {
            int ret = -1;

            errorMessage = null;

            string sDomainControllerIP = null;
            IPHostEntry domainControllerEntry=null;           

            try
            {
                domainControllerEntry = Dns.GetHostEntry(DomainControllerName);             
            }
            catch(Exception ex)
            {               
                errorMessage =
                    String.Format(
                    "The specified domain either does not exist or DNS could not resolve the address of the domain controller : {0}",
                    DomainControllerName);
                Logger.Log("DirectoryContext.CreateDirectoryContext():" + ex.Message);
                Logger.Log("DirectoryContext.CreateDirectoryContext(): " + errorMessage);
                //Logger.ShowUserError(errorMessage);
                return null;
            }

            if (domainControllerEntry != null && domainControllerEntry.AddressList.Length > 0)
            {
                sDomainControllerIP = domainControllerEntry.AddressList[0].ToString();                
            }
            else
            {
                errorMessage = 
                    String.Format(
                    "DirectoryContext.CreateDirectoryContext(): Could not resolve address of domain controller : {0}",
                    DomainControllerName);
                //Logger.ShowUserError("DirectoryContext.CreateDirectoryContext(): Could not resolve address of domain controller: {0}");
                Logger.Log("DirectoryContext.CreateDirectoryContext(): " + errorMessage);
                errorMessage = "";
                return null;
            }
            
            IntPtr ld = LdapAPI.ldap_open(sDomainControllerIP, portNumber);            
            if (ld == IntPtr.Zero)
            {                
                errorMessage = 
                    "The specified domain either does not exist or could not be contacted.  " + 
                    "Please contact your system administrator to verify that your domain is " +
                    "properly configured and is currently online.";
                Logger.Log("DirectoryContext.CreateDirectoryContext(): " + errorMessage);
                errorMessage = "";
                //Logger.ShowUserError(errorMessage);
                return null; 
            }

            //LdapTimeVal timeout = new LdapTimeVal(1, 0);
            ////IntPtr ptrTimeout = IntPtr.Zero;
            ////ptrTimeout = Marshal.AllocHGlobal(Marshal.SizeOf(timeout));
            ////Marshal.WriteInt32(ptrTimeout, timeout.);
            //ret = LdapAPI.ldap_connect(ld, timeout.ConvertToUM());
            //if (BailOnLdapError("LdapAPI.ldap_connect :", ret, out errorMessage))
            //{
            //    Logger.Log("DirectoryContext.CreateDirectoryContext(): " + errorMessage);
            //    errorMessage = "";
            //    return null;
            //}

            LdapHandle ldapHandle = new LdapHandle(ld);

            string distinguishedName = string.Format("cn={0},cn=Users,{1}", UserName, rootDN);            

            int version = LDAP_VERSION3;
            IntPtr ptrVersion = IntPtr.Zero;
            ptrVersion = Marshal.AllocHGlobal(Marshal.SizeOf(version));
            Marshal.WriteInt32(ptrVersion, version);
            ret = LdapAPI.ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, ptrVersion);
            if(BailOnLdapError("ldap_set_option OPT_PROTOCOL_VERSION :", ret, out errorMessage))
            {
                Logger.Log("DirectoryContext.CreateDirectoryContext(): " + errorMessage);
                errorMessage = "";
                return null;
            }

            ret = LdapAPI.ldap_set_option(ld, LDAP_OPT_REFERRALS, IntPtr.Zero);
            if(BailOnLdapError("ldap_set_option LDAP_OPT_REFERRALS :", ret, out errorMessage))
            {
                Logger.Log("DirectoryContext.CreateDirectoryContext(): " + errorMessage);
                errorMessage = "";
                return null;
            }

            if (!String.IsNullOrEmpty(Password))
            {
                usingSimpleBind = true;
            }

            if (usingSimpleBind)
            {
                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                {
                    ret = LdapAPI.ldap_bind_s(ld, distinguishedName, Password, (ulong)LDAP_AUTHWindows.LDAP_AUTH_SIMPLE);
                }
                else
                {
                    ret = LdapAPI.ldap_bind_s(ld, distinguishedName, Password, (ulong)LDAP_AUTHLinux.LDAP_AUTH_SIMPLE);
                }
                if (BailOnLdapError("", ret, out errorMessage))
                {                    
                    Logger.Log("DirectoryContext.CreateDirectoryContext(): " + errorMessage);
                    return null;
                }
            }
            else  /***GSS-bind***/
            {
                if (Configurations.currentPlatform != LikewiseTargetPlatform.Windows)
                {
                    //set GSS-Remote Principal Name 

                    //char ber_pvt_opt_on;    /* used to get a non-NULL address for *_OPT_ON */
                    ///* option on/off values */
                    //#define LDAP_OPT_ON           ((void *) &ber_pvt_opt_on)
                    //#define LDAP_OPT_OFF    ((void *) 0)
                    //status = ldap_set_option(ld, LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL, LDAP_OPT_ON);   

                    char ber_pvt_opt_on = '1';
                    IntPtr ptrRemotePrincipalflags = IntPtr.Zero;
                    ptrRemotePrincipalflags = Marshal.AllocHGlobal(Marshal.SizeOf(ber_pvt_opt_on));
                    Marshal.WriteInt32(ptrRemotePrincipalflags, ber_pvt_opt_on);
                    ret = LdapAPI.ldap_set_option(ld, LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL, ptrRemotePrincipalflags);
                    if (BailOnLdapError("DirectoryContext GSS-Bind: ldap_set_option LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL, ptrRemotePrincipalflags=" + ptrRemotePrincipalflags,
                    ret, out errorMessage))
                    {
                        Logger.Log("DirectoryContext.CreateDirectoryContext(): " + errorMessage);
                        errorMessage = "";
                        return null;
                    }
                }

                //set GSS-SPNEGO options
                int secflags = ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT;

                IntPtr ptrSecflags = IntPtr.Zero;
                ptrSecflags = Marshal.AllocHGlobal(Marshal.SizeOf(secflags));
                Marshal.WriteInt32(ptrSecflags, secflags);
                ret = LdapAPI.ldap_set_option(ld, LDAP_OPT_SSPI_FLAGS, ptrSecflags);
                if (BailOnLdapError("DirectoryContext GSS-Bind: ldap_set_option LDAP_OPT_SSPI_FLAGS, secflags=" + secflags,
                    ret, out errorMessage))
                {
                    Logger.Log("DirectoryContext.CreateDirectoryContext(): " + errorMessage);
                    errorMessage = "";
                    return null;
                }

                if (Configurations.currentPlatform != LikewiseTargetPlatform.Windows)
                {
                    secflags |= ISC_REQ_INTEGRITY;

                    ptrSecflags = IntPtr.Zero;
                    ptrSecflags = Marshal.AllocHGlobal(Marshal.SizeOf(secflags));
                    Marshal.WriteInt32(ptrSecflags, secflags);
                    ret = LdapAPI.ldap_set_option(ld, LDAP_OPT_SSPI_FLAGS, ptrSecflags);
                    if (BailOnLdapError("DirectoryContext GSS-Bind: ldap_set_option LDAP_OPT_SSPI_FLAGS, secflags=" + secflags,
                        ret, out errorMessage))
                    {
                        Logger.Log("DirectoryContext.CreateDirectoryContext(): " + errorMessage);
                        errorMessage = "";
                        return null;
                    }

                    secflags |= ISC_REQ_CONFIDENTIALITY;

                    ptrSecflags = IntPtr.Zero;
                    ptrSecflags = Marshal.AllocHGlobal(Marshal.SizeOf(secflags));
                    Marshal.WriteInt32(ptrSecflags, secflags);
                    ret = LdapAPI.ldap_set_option(ld, LDAP_OPT_SSPI_FLAGS, ptrSecflags);
                    if (BailOnLdapError("DirectoryContext GSS-Bind: ldap_set_option LDAP_OPT_SSPI_FLAGS, secflags=" + secflags,
                        ret, out errorMessage))
                    {
                        Logger.Log("DirectoryContext.CreateDirectoryContext(): " + errorMessage);
                        errorMessage = "";
                        return null;
                    }
                }
                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                {
                    ret = LdapAPI.ldap_bind_s(ld, null, null, (ulong)LDAP_AUTHWindows.LDAP_AUTH_NEGOTIATE);
                }
                else
                {
                    ret = LdapAPI.ldap_bind_s(ld, null, null, (ulong)LDAP_AUTHLinux.LDAP_AUTH_NEGOTIATE);
                }
                if (BailOnLdapError("DirectoryContext GSS-Bind:ldap_bind_s :", ret, out errorMessage))
                {
                    Logger.Log("DirectoryContext.CreateDirectoryContext(): " + errorMessage);
                    errorMessage = "";
                    return null;
                }
            }  

            //try to figure out the DC that serves as GC            
            string configurationName = null;            
            //searching with baseDn="" allows ldap to access the domain “RootDSE”.  
            //Without passing that, it cannot access the configurationNamingContext
            DirectoryContext dirContext = new DirectoryContext(
                                DomainControllerName,
                                rootDN,
                                distinguishedName,
                                UserName,
                                Password,
                                ldapHandle,
                                portNumber);

            dirContext._domainControllerIP = sDomainControllerIP;
            LdapMessage ldapMessage;
            string gcServer = DomainControllerName;
           
            Logger.Log("root query is started querying ", Logger.ldapLogLevel);
            IntPtr MessagePtr;
            ret = LdapAPI.ldap_search_s(ld,
                                        "",
                                        (int)LdapAPI.LDAPSCOPE.BASE,
                                        "(objectClass=*)",
                                        null,
                                        0,
                                        out MessagePtr);

            ldapMessage = new LdapMessage(ldapHandle, MessagePtr);            
            Logger.Log("root query is finished querying " + ret, Logger.ldapLogLevel);

            List<LdapEntry> ldapEntries = (ldapMessage != null ? ldapMessage.Ldap_Get_Entries() : null);
            Logger.Log("root query is finished querying with ldapEntries count : " + ldapEntries, Logger.ldapLogLevel);

            #region //Obtaining RootDSE attributes
            if (ldapEntries != null && ldapEntries.Count > 0)
            {
                LdapEntry rootDseEntry = ldapEntries[0];

                LdapValue[] values = rootDseEntry.GetAttributeValues("defaultNamingContext", dirContext);

                if (values != null && values.Length > 0)
                {
                    dirContext.DefaultNamingContext = values[0].stringData;
                }
                Logger.Log("defaultNamingContext is " + dirContext.DefaultNamingContext, Logger.ldapLogLevel);

                values = rootDseEntry.GetAttributeValues("schemaNamingContext", dirContext);

                if (values != null && values.Length > 0)
                {
                    dirContext.SchemaNamingContext = values[0].stringData;
                }
                Logger.Log("schemaNamingContext is " + dirContext.SchemaNamingContext, Logger.ldapLogLevel);

                values = rootDseEntry.GetAttributeValues("dnsHostName", dirContext);

                if (values != null && values.Length > 0)
                {
                    dirContext.DnsHostName = values[0].stringData;
                }
                Logger.Log("dnsHostName is " + dirContext.DnsHostName, Logger.ldapLogLevel);

                values = rootDseEntry.GetAttributeValues("rootDomainNamingContext", dirContext);

                if (values != null && values.Length > 0)
                {
                    dirContext.RootDomainNamingContext = values[0].stringData;
                }
                Logger.Log("dnsHostName is " + dirContext.RootDomainNamingContext, Logger.ldapLogLevel);

                values = rootDseEntry.GetAttributeValues("configurationNamingContext", dirContext);

                if (values != null && values.Length > 0)
                {
                    configurationName = values[0].stringData;
                }
                Logger.Log(
                    "configurationNamingContext is " + configurationName, 
                    Logger.ldapLogLevel);

                values = rootDseEntry.GetAttributeValues("SupportedSASLMechanisms", dirContext);

                if (values != null && values.Length > 0)
                {
                    _supportedSASLMechanisms = new string[values.Length];
                    int index = 0;

                    foreach (LdapValue value in values)
                    {
                        _supportedSASLMechanisms[index] = value.stringData;                       

                        Logger.Log(
                                    "SupportedSASLMechanisms is " + value.stringData,
                                    Logger.ldapLogLevel);
                        index++;
                    }
                }   

                dirContext.ConfigurationNamingContext = configurationName;

            #endregion

                ret = LdapAPI.ldap_search_s(ld,
                                        configurationName,
                                        (int)LdapAPI.LDAPSCOPE.BASE,
                                        "(&(objectcategory=ntdsdsa)(options=1))",
                                        new string[] { "distinguishedName", null },
                                        0,
                                        out MessagePtr);

                ldapMessage = new LdapMessage(ldapHandle, MessagePtr);      

                if(BailOnLdapError("ldap_search_s :", ret, out errorMessage))
                {
                    Logger.Log("DirectoryContext.CreateDirectoryContext(): " + errorMessage);
                    errorMessage = "";
                    return null;
                }

                ldapEntries = (ldapMessage != null ? ldapMessage.Ldap_Get_Entries() : null);

                if (ldapEntries != null && ldapEntries.Count > 0)
                {
                    //we only check the first one, then we quit finding others (when we do optimization, this algorithm shall be a lot more complicated
                    LdapEntry ldapNextEntry = ldapEntries[0];

                    values = ldapNextEntry.GetAttributeValues("distinguishedName", dirContext);

                    if (values != null && values.Length > 0)
                    {
                        string dn = values[0].stringData;
                        string[] splits1 = dn.Split(',');
                        string[] splits2 = rootDN.Split(',');

                        gcServer = splits1[1].Substring(3).ToLower();
                        foreach (string str in splits2)
                            gcServer = string.Concat(gcServer, ".", str.Substring(3).ToLower());

                        Logger.Log(
                            "global catelog server is " + gcServer, 
                            Logger.ldapLogLevel);                                           
                    }                   
                }
            }

            dirContext.GCServername = gcServer;
            dirContext.BindMethod = usingSimpleBind;

            ldapHandle.DirectoryContext = dirContext;

            return dirContext;
        }

        // Function to delete the children recursively. If the Dn is a Non-Leaf node then LdapAPI.ldap_delete_s will fail to
        // delete the DnN, to do this check for children then delete the DNs from bottom to top level.
        public int DeleteChildren_Recursive(string distinguishedName)
        {
            bool IsParentEmpty = false;
            int ret = 0;

            LdapMessage ldapMessage = null;

            Logger.Log("Deleting children of " + distinguishedName);

            ret = _ldapHandle.Ldap_Search_S(distinguishedName,
                                           (int)LdapAPI.LDAPSCOPE.ONE_LEVEL,
                                           "(objectClass=*)",
                                           new string[] { null },
                                           0,
                                           out ldapMessage);
            if (ret != 0)
                return ret;

            List<LdapEntry> ldapEntries = (ldapMessage != null ? ldapMessage.Ldap_Get_Entries() : null);

            //delete children
            if (ldapEntries != null && ldapEntries.Count > 0)
            {
                foreach (LdapEntry ldapNextEntry in ldapEntries)
                {
                    string dn = ldapNextEntry.GetDN();
                    ret = DeleteChildren_Recursive(dn);
                    if (ret != 0)
                        return ret;
                }
                IsParentEmpty = true;
            }
            else
            {
                lock (lockThis_delSyn)
                {
                    ret = _ldapHandle.Ldap_Delete_S(distinguishedName);
                    return ret;
                }
            }
            // Delete Parent
            if (IsParentEmpty)
            {
                lock (lockThis_delSyn)
                {
                    ret = _ldapHandle.Ldap_Delete_S(distinguishedName);
                    return ret;
                }
            }
            return ret;
        }

        public int Ldap_CancelSynchronous()
        {
            int ret = -1;

            lock (lockThis_ldapCancel)
            {
                Ldap_AbandonSynchronous();

                ret = _ldapHandle.Ldap_Cancel(
                                    ServerCtrls                                    
                                    );
            }

            ret = handleSearchError("Ldap_Cancel", RootDN, ret);

            return ret;
        }

        public int Ldap_AbandonSynchronous()
        {
            int ret = -1;

            lock (lockThis_ldapAbandon)
            {
                ret = _ldapHandle.Ldap_Abandon(0);
            }

            ret = handleSearchError("Ldap_Abandon", RootDN, ret);

            return ret;
        }

        public int AddSynchronous(string distinguishedName, LDAPMod[] userinfo)
        {
            int ret = 0;

            lock (lockThis_addSyn)
            {
                ret = _ldapHandle.Ldap_Add_S(distinguishedName, userinfo);
            }

            return ret;
        }

        public int DeleteSynchronous(string distinguishedName)
        {
            int ret = 0;

            lock (lockThis_delSyn)
            {
                ret = _ldapHandle.Ldap_Delete_S(distinguishedName);
            }

            return ret;
        }

        public int ModifySynchronous(string distinguishedName, LDAPMod[] userinfo)
        {
            int ret = 0;

            lock (lockThis_modSyn)
            {
                ret = _ldapHandle.Ldap_Modify_S(distinguishedName, userinfo);
            }

            return ret;
        }

        public int RenameSynchronous(string basedn, string newdn, string newSuperior)
        {
            return _ldapHandle.Ldap_Rename_S(basedn, newdn, newSuperior, 1);
        }

        public int Ldap_Create_Page_Control(int pageSize, IntPtr cookie, char isCritical, out IntPtr control)
        {
            int ret = 0;

            lock (lockThis_modSyn)
            {
                ret = _ldapHandle.Ldap_Create_Page_Control(pageSize, cookie, isCritical, out control);
            }

            return ret;
        }

        public int Ldap_Parse_Result(IntPtr ldapMsgRes, out int errcodep, out IntPtr serverLdapControl)
        {
            int ret = 0;

            lock (lockThis_modSyn)
            {
                ret = _ldapHandle.Ldap_Parse_Result(ldapMsgRes, out errcodep, out serverLdapControl);
            }

            return ret;
        }

        public int Ldap_Parse_Page_Control(IntPtr page_control, out ulong totalCount, out IntPtr cookie)
        {
            int ret = 0;

            lock (lockThis_modSyn)
            {
                ret = _ldapHandle.Ldap_Parse_Page_Control(page_control, out totalCount, out cookie);
            }

            return ret;
        }

        public int Setserver_PageControloption()
        {
            return _ldapHandle.setserver_pageControloption();
        }

        public int UnSetserver_PageControloption()
        {
            return _ldapHandle.unsetserver_pageControloption();
        }


        #endregion

        #region synchronization_methods

        private void ldapMsgCallback(LdapMessage ldapMessage)
        {
            Logger.Log("processing call back", Logger.ldapLogLevel);
            if (ldapMessage == null)
            {
                Logger.Log("DirectoryContext.ListChildEntriesSynchronous(): LDAP search returned NULL message", Logger.ldapLogLevel);
                _ListChildEntriesSync = null;
                return;
            }

            Logger.Log("ldapMessage is not null", Logger.ldapLogLevel);
            _ListChildEntriesSync = ldapMessage.Ldap_Get_Entries();

            if (_ListChildEntriesSync != null && _ListChildEntriesSync.Count > 0)
                Logger.Log(string.Format("ldap entry is {0}", _ListChildEntriesSync.Count), Logger.ldapLogLevel);
        }

        public void Search(string basedn,
                                 LdapAPI.LDAPSCOPE scope,
                                 string filter,
                                 string[] search_attrs,
                                 bool attrsonly,
                                 LDAPWrapper.ldapMsgDelegate callback)
        {
            LDAPArguments args = new LDAPArguments();
            args.initialize();

            args.basedn = basedn;
            args.scope = scope;
            args.filter = filter;
            args.search_attrs = search_attrs;
            args.attrsonly = attrsonly;

            args.ldapMsgCallback = callback;

            Logger.Log("Create a new thread to handle SearchSync API call", Logger.ldapLogLevel);

            Thread t = new Thread(new ParameterizedThreadStart(SearchSynchronousWrap));
            t.Start(args);

            t.Join();
        }

        private void SearchSynchronousWrap(object args)
        {
            if (!(args is LDAPArguments))
            {
                return;
            }
            LDAPArguments _args = (LDAPArguments)args;

            int ret = 0;

            LdapMessage result = null;
            ret = SearchSynchronous(
                    _args.basedn, 
                    _args.scope, 
                    _args.filter,
                    _args.search_attrs, 
                    _args.attrsonly,
                    out result);



            if (_args.ldapMsgCallback != null)
            {              
                Logger.Log("pass the result to delegate", Logger.ldapLogLevel);
                _args.ldapMsgCallback(result);
            }
        } 

        #endregion

        #region helper methods

        private static bool BailOnLdapError(string msg, int ret, out string errorMessage)
        {
            string ldapErrMsg = null;
            if (ret != 0)
            {
                if (ret == 48)
                {
                    ldapErrMsg = "Please make sure you have joined the domain," +
                    "and that you have cached kerberos credentials on your system.";
                }
                else if (ret == -10)
                {
                    ldapErrMsg = "The openlap library that is in use does not support GSS binding.  " +
                    "Please choose to provide username/password when you set target machine information.";
                }
                else
                {
                    ldapErrMsg = ErrorCodes.LDAPString(ret);
                }

                errorMessage = String.Format("{0} {1}", msg, ldapErrMsg);
                Logger.Log(errorMessage, Logger.LogLevel.Error);

                return true;
            }
            else
            {
                errorMessage = null;
                return false;
            }

        }
        #endregion

        #region accessors

        public string DomainControllerName
        {
            get
            {
                return _domainControllerName;
            }
        }

        public string ServerName
        {
            get
            {
                return _domainControllerName;
            }
        }

        public string DomainName
        {
            get
            {
                return _domainName;
            }
        }

        public string RootDN
        {
            get
            {
                return _rootDN;
            }
        }

        public string DistinguishedName
        {
            get
            {
                return _distinguishedName;
            }
        }

        public string UserName
        {
            get
            {
                return _userName;
            }
        }

        public string Password
        {
            get
            {
                return _password;
            }
        }

        public LDAPSchemaCache SchemaCache
        {
            get
            {
                return _ldapSchemaCache;
            }

            set
            {
                _ldapSchemaCache = value;
            }
        }

        public int PortNumber
        {
            get
            {
                return _portNumber;
            }
            set
            {
                _portNumber = value;
            }
        }

        public LdapHandle LdapHandle
        {
            get
            {
                return _ldapHandle;
            }
            set
            {
                _ldapHandle = value;
            }
        }

        
        public string GCServername
        {
            get
            {
                return _gcServerName;
            }
            set
            {
                _gcServerName = value;
            }
        }
         
        public bool BindMethod
        {
            get
            {
                return _bindMethod;
            }
            set
            {
                _bindMethod = value;
            }
        }

        public string DomainControllerIP
        {
            get
            {
                return _domainControllerIP;
            }
            set
            {
                _domainControllerIP = value;
            }
        }

        public string ConfigurationNamingContext
        {
            get
            {
                return _configurationNamingContext;
            }
            set
            {
                _configurationNamingContext = value;
            }
        }

        public string DefaultNamingContext
        {
            get
            {
                return _defaultNamingContext;
            }
            set
            {
                _defaultNamingContext = value;
            }
        }

        public string SchemaNamingContext
        {
            get
            {
                return _schemaNamingContext;
            }
            set
            {
                _schemaNamingContext = value;
            }
        }

        public string DnsHostName
        {
            get
            {
                return _dnsHostName;
            }
            set
            {
                _dnsHostName = value;
            }
        }

        public string RootDomainNamingContext
        {
            get
            {
                return _rootDomainNamingContext;
            }
            set
            {
                _rootDomainNamingContext = value;
            }
        }

        public string[] SupportedSASLMechanisms
        {
            get
            {
                return _supportedSASLMechanisms;
            }
            set
            {
                _supportedSASLMechanisms = value;
            }
        }

        #endregion

    }
}

