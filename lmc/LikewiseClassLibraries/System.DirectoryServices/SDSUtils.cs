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
using Likewise.LMC.LDAP;
using Likewise.LMC.LDAP.Interop;
using System.DirectoryServices.ActiveDirectory;

namespace System.DirectoryServices
{
    public class SDSUtils
    {

        public static bool CrackPath(string sPath, out string sProtocol, out string sServer, out string sCNs, out string sDCs)
        {
            // get this out of the way
            sProtocol =
            sServer =
            sCNs =
            sDCs = null;

            // set the protocol
            sProtocol = "LDAP";

            // start scanning after that
            int ich = 0;
            int ichNext = 0;

            ichNext = sPath.IndexOf("://");
            if (ichNext >= 0)
            {
                sProtocol = sPath.Substring(0, ichNext);
                ich = ichNext + 3;
            }
            else
            {
                sProtocol = "LDAP";
            }

            // look for the first slash
            ichNext = sPath.IndexOf("/", ich);
            if (ichNext >= 0)
            {
                // grab the server
                sServer = sPath.Substring(ich, ichNext - ich);

                // skip the server
                ich = ichNext + 1;
            }
            else
            {
                // have no server
                ichNext = ich;
            }

            // look for the first "dc="
            ichNext = sPath.ToLower().IndexOf("dc=");
            if (ichNext < 0)
            {
                // got none so take everything to the end
                sCNs = sPath.Substring(ich);
                return true;
            }

            // set the cn part

            // Only grab CNs if there's anything to grab
            if (ichNext > ich)
            {
                // ignore the trailing blank
                sCNs = sPath.Substring(ich, (ichNext - 1) - ich);
            }

            // skip the cns
            ich = ichNext;

            // ignore anything after the DCs
            ichNext = sPath.IndexOf("/", ich);

            if (ichNext < 0)
                sDCs = sPath.Substring(ich);
            else
                sDCs = sPath.Substring(ich, ichNext - ich);

            return true;
        }

        public static string MakePath(string sProtocol, string sServer, string sCNs, string sDCs)
        {
            string sPath;

            if (!string.IsNullOrEmpty(sProtocol))
                sPath = sProtocol;
            else
                sPath = "LDAP";

            sPath += "://";

            if (!string.IsNullOrEmpty(sServer))
                sPath += sServer + "/";
            else
            {
                // synthesize a server name
                if (sDCs != null && sDCs != "")
                {
                    sPath += DNToDomainName(sDCs) + "/";
                }
            }

            if (!string.IsNullOrEmpty(sCNs))
                sPath += sCNs;

            if (!string.IsNullOrEmpty(sDCs))
            {
                if (sCNs != null && sCNs != "")
                    sPath += ",";
                sPath += sDCs;
            }

            return sPath;
        }

        public static string DNToDomainName(string DomainDN)
        {
            DomainDN = DomainDN.ToUpper();

            // skip anything before the first DC=
            int iPos = DomainDN.IndexOf("DC=");
            if (iPos > 0)
                DomainDN = DomainDN.Substring(iPos);

            DomainDN = DomainDN.Replace("DC=", "");
            DomainDN = DomainDN.Replace(',', '.');

            DomainDN = DomainDN.ToLower();

            return DomainDN;
        }

        /// <summary>
        /// Convert dot separated domain name to LDAP distinguished name.
        /// </summary>
        /// <param name="domainName">dot separated domain name</param>
        /// <returns>LDAP distinguished name for the domain</returns>
        public static string DomainNameToDN(string domainName)
        {
            string dn;
            string[] parts = domainName.Split(new char[] { '.' });

            if (parts.Length <= 0)
                return "";

            dn = "DC=" + parts[0];
            for (int i = 1; i < parts.Length; i++)
                dn += ",DC=" + parts[i];

            return dn;
        }

        public static int AddNewObj(Likewise.LMC.LDAP.DirectoryContext dirContext, string choosenclass, string nodeDN)
        {
            if (dirContext != null)
            {
                LDAPMod[] info = new LDAPMod[1];

                string[] objectClass_values = new string[] { choosenclass, null };
                info[0] = new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "ObjectClass", objectClass_values);

                return dirContext.AddSynchronous(nodeDN, info);
            }

            return -1;
        }

        public static int DeleteObj(Likewise.LMC.LDAP.DirectoryContext dirContext, string nodeDN)
        {
            if (dirContext != null)
            {
                return dirContext.DeleteSynchronous(nodeDN);
            }

            return -1;
        }

        public static int ModifyProperty(Likewise.LMC.LDAP.DirectoryContext dirContext, string nodeDN, string propertyName, PropertyValueCollection propertyValue)
        {
            List<object> valueObjects = propertyValue.ValueCollection;

            string[] values;

            if (valueObjects == null || valueObjects.Count == 0)
                values = new string[] { null };
            else if (valueObjects.Count == 1)
            {
                values = new string[] { ParsingValueObj(valueObjects[0]), null };
                //Console.WriteLine("In SDSUtils::modifyPropertyvalue is " + ParsingValueObj(valueObjects[0]));
            }
            else
            {
                values = new string[valueObjects.Count + 1];
                int i;
                for (i = 0; i < valueObjects.Count; i++)
                {
                    values[i] = ParsingValueObj(valueObjects[i]);
                    //Console.WriteLine("In SDSUtils::modifyPropertyvalue " + i + "is " + values[i]);
                }
                values[i] = null;
            }

            LDAPMod[] attrinfo = new LDAPMod[] { new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, propertyName, values) };

            return dirContext.ModifySynchronous(nodeDN, attrinfo);
        }

        private static string ParsingValueObj(object val)
        {
            if (val.GetType() == typeof(int))
            {
                int ival = (int)val;
                return val.ToString();
            }

            if (val.GetType() == typeof(long))
            {
                long lval = (long)val;
                return val.ToString();
            }

            if (val.GetType() == typeof(bool))
            {
                bool bval = (bool)val;
                return val.ToString();
            }

            if (val is string)
            {
                return val as string;
            }

            return val as string;
            //Todo: if val is byte[] needs use \XX\XX formatting rule Wei think
        }

        //this function will return a ldapMessage that contains all the attributes that are available for an object
        //use this to populate DirectoryEntry's properties
        public static List<string> InitLdapMessageFilterForProperties(Likewise.LMC.LDAP.DirectoryContext dirContext, string nodeDN)
        {
            LdapMessage ldapMessagetemp = null;

            string[] attrs = { "name", "allowedAttributes", null };

            if (ldapMessagetemp == null)
            {
                ldapMessagetemp = dirContext.SearchSynchronous(
                    nodeDN,
                    LdapAPI.LDAPSCOPE.BASE,
                    "(objectClass=*)",
                    attrs,
                    false);
            }

            if (ldapMessagetemp == null)
                return null;

            List<LdapEntry> ldapEntries = ldapMessagetemp.Ldap_Get_Entries();

            if (ldapEntries == null || ldapEntries.Count == 0)
            {
                return null;
            }

            LdapEntry ldapNextEntry = ldapEntries[0];

            List<string> allowedAttributes = new List<string>();

            LdapValue[] attrValues = ldapNextEntry.GetAttributeValues("allowedAttributes", dirContext);
            if (attrValues != null && attrValues.Length > 0)
                foreach (LdapValue attrValue in attrValues)
                    allowedAttributes.Add(attrValue.stringData);

            return allowedAttributes;


        }

        //find the DN of given the groupsid
        public static string SearchByGuid(string objectGuid, Likewise.LMC.LDAP.DirectoryContext dirContext)
        {
            string searchFilter = objectGuid;
            searchFilter = string.Concat("(objectGuid=", objectGuid, ")");

            LdapMessage ldapMessage = dirContext.SearchSynchronous(
                       dirContext.RootDN,
                       LdapAPI.LDAPSCOPE.SUB_TREE,
                       searchFilter,
                       null,
                       false);

            if (ldapMessage == null)
            {
                return null;
            }
            else
            {
                List<LdapEntry> ldapEntries = ldapMessage.Ldap_Get_Entries();
                if (ldapEntries == null || ldapEntries.Count == 0)
                {
                    return null;
                }

                LdapEntry ldapNextEntry = ldapEntries[0];

                if (ldapNextEntry != null)
                {
                    string[] attrsList = ldapNextEntry.GetAttributeNames();

                    if (attrsList != null)
                    {
                        foreach (string attr in attrsList)
                        {
                            if (attr.Equals("distinguishedName", StringComparison.InvariantCultureIgnoreCase))
                            {
                                LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirContext);

                                if (attrValues != null && attrValues.Length > 0)
                                {
                                    return attrValues[0].stringData;
                                }
                            }
                        }
                    }
                }
                return null;
            }
        }


        //find the DN of given the sid
        public static string SearchBySid(string sid, Likewise.LMC.LDAP.DirectoryContext dirContext)
        {
            string searchFilter = string.Concat("(objectSid=", sid, ")");

            LdapMessage ldapMessage = dirContext.SearchSynchronous(
                       dirContext.RootDN,
                       LdapAPI.LDAPSCOPE.SUB_TREE,
                       searchFilter,
                       null,
                       false);

            if (ldapMessage == null)
            {
               // Logger.Log("ldapMessage = null");
                return null;
            }
            else
            {
                List<LdapEntry> ldapEntries = ldapMessage.Ldap_Get_Entries();
                if (ldapEntries == null || ldapEntries.Count == 0)
                {
                   // Logger.Log("ldapEntries.Count == 0");
                    return null;
                }

                LdapEntry ldapNextEntry = ldapEntries[0];

                if (ldapNextEntry != null)
                {
                    string[] attrsList = ldapNextEntry.GetAttributeNames();

                    if (attrsList != null)
                    {
                        foreach (string attr in attrsList)
                        {
                            if (attr.Equals("distinguishedName", StringComparison.InvariantCultureIgnoreCase))
                            {
                                LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, dirContext);

                                if (attrValues != null && attrValues.Length > 0)
                                {
                                    return attrValues[0].stringData;
                                }
                            }
                        }
                    }
                }
                return null;
            }
        }


        //make this searching the GC
        public static bool LookUpAcctSid(string contextSystem, byte[] abSID, StringBuilder sbDomain)
        {
            SecurityIdentifier sid = new SecurityIdentifier(abSID, 0);

            string[] splits = contextSystem.Split('.');

            string sDCs = "";

            foreach (string split in splits)
                sDCs = string.Concat(sDCs, "DC=", split, ",");

            sDCs = sDCs.Substring(0, sDCs.Length - 1);

            //some hack to obtain the creds to establish a GC dirContext [Wei]
            string username = string.Empty;
            string password = string.Empty;

            DirectoryEntry.ObtainCreds(out username, out password, contextSystem.ToLower());

            GlobalCatalog gc = GlobalCatalog.GetGlobalCatalog(
                new System.DirectoryServices.ActiveDirectory.DirectoryContext(DirectoryContextType.Domain, contextSystem.ToLower(),
                                                                             username, password));

            if (gc == null) //cannot talk to GC
            {
                string contextldapPath = string.Concat("LDAP://", contextSystem.ToLower(), "/", sDCs);

                DirectoryEntry context = new DirectoryEntry(contextldapPath);

                string filter = string.Concat("(objectSid=", sid.ToString(), ")");

                DirectorySearcher ds = new DirectorySearcher(context, filter);

                ds.SearchScope = SearchScope.Subtree;

                SearchResult de = ds.FindOne();

                if (de == null)
                {
                    //Console.WriteLine("GetSidDomain::LookUpAcctSid (Not Found!)");
                    return false;
                }
                else
                {
                    //Console.WriteLine("GetSidDomain::LookUpAcctSid (Found!)");
                    sbDomain.Append(contextSystem);
                    return true;
                }
            }
            else //search in GC
            {
                DirectorySearcher ds = gc.GetDirectorySearcher();
                ds.Filter = string.Concat("(objectSid=", sid.ToString(), ")");
                ds.SearchScope = SearchScope.Subtree;
                SearchResult sr = ds.FindOne();
                if (sr == null)
                {
                    //Console.WriteLine("GetSidDomain::LookUpAcctSid (Not Found!) (in GC)");
                    return false;
                }
                else
                {
                    //Console.WriteLine("GetSidDomain::LookUpAcctSid (Found!) (in GC)");
                    sbDomain.Append(contextSystem);
                    return true;
                }
            }
        }




    }
}
