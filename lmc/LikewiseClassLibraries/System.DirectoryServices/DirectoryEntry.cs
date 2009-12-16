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
using System.Threading;

namespace System.DirectoryServices
{
    public class DirectoryEntry
    {
        private const int GC_PORT = 3268;
        private const int LDAP_PORT = 389;

        private PropertyCollection propertyCollection;
        private object nativeObject;
        private string sLDAPPath;
        private string sName;
        private string sUsername;
        private string sPassword;
        private DirectoryEntries children;
        private ActiveDirectorySecurity objectSecurity;
        private Guid guid;
        private DirectoryEntry parent;

        private DirectoryContext dirContext;
        // crack the path
        private string sProtocol = null;
        private string sServer = null;
        private string sCNs = null;
        private string sDCs = null;
        private string baseDn = null;
        private string rootDN = null; //domain DN

        public static List<DirectoryContext> exisitngDirContext = new List<DirectoryContext>();
        public static List<LDAPSchemaCache> existingSchemaCache = new List<LDAPSchemaCache>();
        private List<string> ldapPaths = null;

        private string objectClassType;

        private bool tobedeleted = false;

        private bool get_baseDnFor_guidOrsid_called = false;


        public DirectoryEntry(string sLDAPPath)
        {
            this.sLDAPPath = sLDAPPath;
            propertyCollection = null;
            nativeObject = null;
            sName = null;
            children = null;
            objectSecurity = null;
            guid = Guid.Empty;
            parent = null;
            objectClassType = null;

            SDSUtils.CrackPath(sLDAPPath, out sProtocol, out sServer, out sCNs, out sDCs);

            /*if (sProtocol != null) Console.WriteLine("sProtocol is " + sProtocol);
            if (sServer != null) Console.WriteLine("sServer is " + sServer);
            if (sCNs != null) Console.WriteLine("sCNs is " + sCNs);
            if (sDCs != null) Console.WriteLine("sDCs is " + sDCs); */

            string[] rootDNcom;

            if (sServer != null)
            {
                rootDNcom = sServer.Split('.');

                rootDN = "";

                foreach (string str in rootDNcom)
                {
                    string temp = string.Concat("dc=", str, ",");
                    rootDN = string.Concat(rootDN, temp);
                }

                rootDN = rootDN.Substring(0, rootDN.Length - 1);
            }
            //beacuse rootDN is nothing but collection of all DC's from DN
            if (sDCs != null)
                rootDN = sDCs;

            baseDn = "";

            //sCNs = RootDSE, Configuration, Schema, Domain
            if (sCNs != null && sDCs == null)
            {
                if (sCNs.Equals("RootDSE", StringComparison.InvariantCultureIgnoreCase))
                    baseDn = "";
                else if (sCNs.Equals("Configuration", StringComparison.InvariantCultureIgnoreCase))
                    baseDn = string.Concat("CN=Configuration,", rootDN);
                else if (sCNs.Equals("Schema", StringComparison.InvariantCultureIgnoreCase))
                    baseDn = string.Concat("CN=Schema,", rootDN);
                else if (sCNs.Equals("Domain", StringComparison.InvariantCultureIgnoreCase) ||
                 sCNs.Equals("", StringComparison.InvariantCultureIgnoreCase) ||
                 sCNs.StartsWith("<"))
                {
                    if (rootDN != null)
                        baseDn = rootDN;
                }
                else baseDn = string.Concat(sCNs, ",", rootDN);

            }

            if (sCNs != null && sDCs != null)
                baseDn = string.Concat(sCNs, ",", sDCs);

            if (sCNs == null && sDCs != null)
                baseDn = sDCs;

            if (sCNs == null && sDCs == null)
                baseDn = rootDN;

            //assign sName value using the dN of this node
            if (baseDn.Equals("", StringComparison.InvariantCultureIgnoreCase))
                sName = "RootDSE";
            else
                sName = baseDn;
        }

        public DirectoryEntry(string sLDAPPath, string sUsername, string sPassword)
            :this(sLDAPPath)
        {
            this.sUsername = sUsername;
            this.sPassword = sPassword;
        }

        public object NativeObject
        {
            get
            {
                return nativeObject;
            }
        }

        public string Path
        {
            get
            {
                return sLDAPPath;
            }
        }

        public bool ToBeDeleted
        {
            get
            {
                return tobedeleted;
            }
            set
            {
                tobedeleted = value;
            }
        }

        public string Name
        {
            get
            {
                Assign_dirContext();

                if (dirContext == null)
                    return sName;

                if (!get_baseDnFor_guidOrsid_called)
                    Get_baseDn_Guid_Or_sid();

                return sName;
            }
        }

        public PropertyCollection Properties
        {
            get
            {
                if (propertyCollection == null)
                {
                    Assign_dirContext();

                    if (dirContext == null)
                        return null;

                    if (!get_baseDnFor_guidOrsid_called)
                        Get_baseDn_Guid_Or_sid();

                    if (baseDn == null)
                        return null;

                    //do the ldap search here to obtain the value of _properties

                    //searching with baseDn="" allows ldap to access the domain “RootDSE”.
                    //First get base which returns the information of this node
                    List<string> allowedAttributes = SDSUtils.InitLdapMessageFilterForProperties(dirContext, baseDn);
                    string[] search_attrs;

                    if (allowedAttributes != null && allowedAttributes.Count > 0)
                    {
                        search_attrs = new string[allowedAttributes.Count + 3];
                        int i;
                        search_attrs[0] = "dummy";
                        for (i = 0; i < allowedAttributes.Count; i++)
                            search_attrs[i + 1] = allowedAttributes[i];
                        search_attrs[i + 1] = "groupType";
                        search_attrs[i + 2] = null;
                    }
                    else
                    {
                        search_attrs = new string[] { null };
                    }

                    LdapMessage ldapMessage = dirContext.SearchSynchronous(
                                                baseDn,
                                                LdapAPI.LDAPSCOPE.BASE,
                                                "ObjectClass=*",
                                                search_attrs,
                                                false);

                    List<LdapEntry> ldapEntries = (ldapMessage != null ? ldapMessage.Ldap_Get_Entries() : null);

                    if (ldapEntries != null && ldapEntries.Count > 0)
                    {
                        LdapEntry entry = ldapEntries[0];
                        //construct and initialize propertyCollection with the information from entry

                        if (allowedAttributes != null && allowedAttributes.Count > 0)
                        {
                            string[] attrsFullList = new string[allowedAttributes.Count];
                            allowedAttributes.CopyTo(attrsFullList);
                            propertyCollection = new PropertyCollection(entry, dirContext, attrsFullList);
                        }
                        else
                        {
                            propertyCollection = new PropertyCollection(entry, dirContext);
                        }

                        #region commentedCode
                        /*//the following portion of code is addede to include "objectClass" property in the current node's properties
                            LdapMessage ldapMessage_objClass = dirContext.SearchSynchronous(
                                                        baseDn,
                                                        LdapAPI.LDAPSCOPE.BASE,
                                                        "(objectClass=*)",
                                                        new string[] { "objectClass", null },
                                                        false);

                            List<LdapEntry> ldapEntries_objClass = (ldapMessage_objClass != null ? ldapMessage_objClass.Ldap_Get_Entries() : null);
                            if (ldapEntries_objClass != null && ldapEntries_objClass.Count > 0)
                            {
                                LdapEntry objclassentry = ldapEntries_objClass[0];
                                if (objclassentry != null)
                                {
                                    LdapValue[] values = objclassentry.GetAttributeValues("objectClass", dirContext);
                                    PropertyValueCollection propertyValue = new PropertyValueCollection(values);
                                    propertyCollection.Add("objectClass", propertyValue);
                                }
                            } */
                        #endregion

                    }
                    else propertyCollection = new PropertyCollection();
                }

                return propertyCollection;
            }
        }

        public string Username
        {
            get
            {
                return sUsername;
            }
            set
            {
                sUsername = value;
            }
        }

        public string Password
        {
            get
            {
                return sPassword;
            }
            set
            {
                sPassword = value;
            }
        }

        public DirectoryEntries Children
        {
            get
            {
                if (children == null)
                {
                    Assign_dirContext();

                    if (dirContext == null)
                        return null;

                    if (!get_baseDnFor_guidOrsid_called)
                        Get_baseDn_Guid_Or_sid();

                    children = new DirectoryEntries(sName, sServer, dirContext);

                    string[] search_attrs = { null };
                    //second get "one-level" returns the information of this node's children
                    LdapMessage ldapMessage = dirContext.SearchSynchronous(
                                                baseDn,
                                                LdapAPI.LDAPSCOPE.ONE_LEVEL,
                                                "(objectClass=*)",
                                                search_attrs,
                                                false);
                    List<LdapEntry> ldapEntries = (ldapMessage != null ? ldapMessage.Ldap_Get_Entries() : null);

                    if (ldapEntries != null && ldapEntries.Count > 0)
                    {
                        foreach (LdapEntry entry in ldapEntries)
                        {
                             //obtain the distinguishedName so that to construct DirectoryEntry for this child
                            LdapValue[] dN = entry.GetAttributeValues("distinguishedName", dirContext);
                            if (dN != null && dN.Length > 0)
                            {
                                string childLdapPath = string.Concat("LDAP://", sServer, "/", dN[0].stringData);
                                //Console.WriteLine("child Path is " + childLdapPath);
                                children.Add(new DirectoryEntry(childLdapPath));
                            }
                        }
                    }
                }

                return children;
            }
        }

        public DirectoryEntry Parent
        {
            get
            {
                if (parent == null)
                {
                    Assign_dirContext();

                    if (dirContext == null)
                        return null;

                    //e.g: RootDSE,configuration,Schema, Domain, then parent is rootDN
                    //sLdapPath should be something like LDAP://corpqa.centeris.com/
                    string parentLdapPath;
                    if (sDCs == null)
                    {
                        parentLdapPath = string.Concat("LDAP://", sServer, "/");
                    }
                    else
                    {
                        int firstCNend = sCNs.IndexOf(',', 0);
                        if (firstCNend > 0)
                        {
                            string parentsCNs = sCNs.Substring(firstCNend + 1);
                            parentLdapPath = string.Concat("LDAP://", sServer, "/", parentsCNs, ",", sDCs);
                        }
                        else
                        {
                           parentLdapPath = string.Concat("LDAP://", sServer, "/", sDCs);
                        }
                    }
                    parent = new DirectoryEntry(parentLdapPath, this.sUsername, this.sPassword);
                }

                return parent;
            }
        }

        public Guid Guid
        {
            get
            {
                if (guid == Guid.Empty)
                {
                    Assign_dirContext();

                    if (dirContext == null)
                        return Guid.Empty;

                    if (!get_baseDnFor_guidOrsid_called)
                        Get_baseDn_Guid_Or_sid();

                    LdapMessage ldapMessage = dirContext.SearchSynchronous(
                                                baseDn,
                                                LdapAPI.LDAPSCOPE.BASE,
                                                "(objectClass=*)",
                                                new string[] { "objectGUID", null },
                                                false);

                    List<LdapEntry> ldapEntries_Guid = (ldapMessage != null ? ldapMessage.Ldap_Get_Entries() : null);
                    if (ldapEntries_Guid != null && ldapEntries_Guid.Count > 0)
                    {
                        LdapEntry entry = ldapEntries_Guid[0];
                        if (entry != null)
                        {
                            LdapValue[] values = entry.GetAttributeValues("objectGUID", dirContext);
                            if (values != null && values.Length > 0)
                                guid = new Guid(values[0].byteData);
                        }
                    }
                }

                return guid;
            }
        }

        public ActiveDirectorySecurity ObjectSecurity
        {
            get
            {
                return objectSecurity;
            }
        }



        public string SchemaClassName
        {
            get
            {
                if (objectClassType == null)
                {
                    Assign_dirContext();

                    if (dirContext == null)
                        return null;

                    if (!get_baseDnFor_guidOrsid_called)
                        Get_baseDn_Guid_Or_sid();

                    LdapMessage ldapMessage_objClass = dirContext.SearchSynchronous(
                                                        baseDn,
                                                        LdapAPI.LDAPSCOPE.BASE,
                                                        "(objectClass=*)",
                                                        new string[] { "objectClass", null },
                                                        false);

                    List<LdapEntry> ldapEntries_objClass = (ldapMessage_objClass != null ? ldapMessage_objClass.Ldap_Get_Entries() : null);
                    if (ldapEntries_objClass != null && ldapEntries_objClass.Count > 0)
                    {
                        LdapEntry objclassentry = ldapEntries_objClass[0];
                        if (objclassentry != null)
                        {
                            LdapValue[] values = objclassentry.GetAttributeValues("objectClass", dirContext);
                            if (values != null && values.Length > 0)
                                objectClassType = values[values.Length - 1].stringData;
                        }
                    }
                }

                return objectClassType;
            }

            set
            {
                objectClassType = value;
            }
        }

        public DirectoryContext DirContext
        {
            get
            {
                Assign_dirContext();

                return dirContext;
            }
        }

        public void CommitChanges()
        {
            Assign_dirContext();

            if (dirContext == null)
                return;

            if (!get_baseDnFor_guidOrsid_called)
                Get_baseDn_Guid_Or_sid();

            string[] search_attrs = { null };
            LdapMessage ldapMessage = dirContext.SearchSynchronous(
                                        baseDn,
                                        LdapAPI.LDAPSCOPE.BASE,
                                        "(objectClass=*)",
                                        search_attrs,
                                        false);
            List<LdapEntry> ldapEntries = (ldapMessage != null ? ldapMessage.Ldap_Get_Entries() : null);
            //if this object does not exist in AD, we need create it first
            if (ldapEntries == null || ldapEntries.Count == 0)
            {
                int ret = SDSUtils.AddNewObj(dirContext, objectClassType, baseDn);
                if (ret != 0)
                {
                    //Console.WriteLine("Create new object failed!");
                    return;
                }
            }

            //go through the properties to check whether there is PropertyValueCollection has been modified
            //PropertyCollection: Dictionary<string, PropertyValueCollection>
            if (propertyCollection != null && propertyCollection.Count > 0)
            {
                foreach (KeyValuePair<string, PropertyValueCollection> kvp in propertyCollection)
                {
                    if (kvp.Value.Modified)
                    {
                        //Console.WriteLine("BaseDN is " + baseDn + " Modified key value pair: " + kvp.Key );
                        int ret = SDSUtils.ModifyProperty(dirContext, baseDn, kvp.Key, kvp.Value);
                        //if (ret != 0) ; Console.WriteLine("Modify a property failed");
                    }
                }
            }

            //go through its children to see whether this is any children marked needed be deleted
            if (children != null && children.Count > 0)
            {
                DirectoryEntries modifiedChildren = new DirectoryEntries();

                foreach (DirectoryEntry child in children)
                {
                    if (child.ToBeDeleted) //delete this DE
                    {
                        int ret = SDSUtils.DeleteObj(dirContext, child.Name);
                    }
                }

                //reflect the changes to children collection
                foreach (DirectoryEntry child in children)
                {
                    if (!child.ToBeDeleted)
                        modifiedChildren.Add(child);
                }

                children = modifiedChildren;
            }
        }

        //The DeleteTree method deletes this entry and its entire subtree from the Active Directory hierarchy.
        public void DeleteTree()
        {
            if (!get_baseDnFor_guidOrsid_called)
                Get_baseDn_Guid_Or_sid();

            DirectoryEntries oneLevel_children = this.Children;

            if (oneLevel_children != null && oneLevel_children.Count > 0)
            {
                foreach (DirectoryEntry entry in oneLevel_children)
                    entry.DeleteTree();
            }

            int ret = SDSUtils.DeleteObj(dirContext, baseDn);
        }

        public void RefreshCache()
        {
            throw new NotImplementedException("DirectoryEntry.RefreshCache");
        }

        //return the found entry's LdapPath
        public string FindFirstChild(string filter, SearchScope searchScope, string[] propertiesToLoad)
        {
            Assign_dirContext();

            if (dirContext == null)
                return null;

            if (!get_baseDnFor_guidOrsid_called)
                Get_baseDn_Guid_Or_sid();

            LdapAPI.LDAPSCOPE ldapscope = LdapAPI.LDAPSCOPE.ONE_LEVEL;

            if (searchScope == SearchScope.Base) ldapscope = LdapAPI.LDAPSCOPE.BASE;
            else if (searchScope == SearchScope.OneLevel) ldapscope = LdapAPI.LDAPSCOPE.ONE_LEVEL;
            else if (searchScope == SearchScope.Subtree) ldapscope = LdapAPI.LDAPSCOPE.SUB_TREE;

            LdapMessage ldapMessage = dirContext.SearchSynchronous(
                                                baseDn,
                                                ldapscope,
                                                filter,
                                                //new string[] { "distinguishedName", null },
                                                Getsearch_attrs(propertiesToLoad),
                                                false);

            List<LdapEntry> ldapEntries = (ldapMessage != null ? ldapMessage.Ldap_Get_Entries() : null);
            if (ldapEntries != null && ldapEntries.Count > 0)
            {
                LdapEntry entry = ldapEntries[0];

                string[] attrsList = entry.GetAttributeNames();

                if (attrsList != null && attrsList.Length > 0)
                {
                    if (entry != null)
                    {
                        LdapValue[] values = entry.GetAttributeValues("distinguishedName", dirContext);
                        if (values != null && values.Length > 0)
                        {
                            return string.Concat("LDAP://", sServer, "/", values[0].stringData);
                        }
                    }
                }
                else return null;
            }

            return null;
        }


        //return an array of ldapPath strings
        public List<string> FindAllChild(string filter, SearchScope searchScope, string[] propertiesToLoad)
        {
            Assign_dirContext();

            if (dirContext == null)
                return null;

            if (!get_baseDnFor_guidOrsid_called)
                Get_baseDn_Guid_Or_sid();

            LdapAPI.LDAPSCOPE ldapscope = LdapAPI.LDAPSCOPE.ONE_LEVEL;

            if (searchScope == SearchScope.Base) ldapscope = LdapAPI.LDAPSCOPE.BASE;
            else if (searchScope == SearchScope.OneLevel) ldapscope = LdapAPI.LDAPSCOPE.ONE_LEVEL;
            else if (searchScope == SearchScope.Subtree) ldapscope = LdapAPI.LDAPSCOPE.SUB_TREE;

            LdapMessage ldapMessage = dirContext.SearchSynchronous(
                                                baseDn,
                                                ldapscope,
                                                filter,
                                                //new string[] { "distinguishedName", null },
                                                Getsearch_attrs(propertiesToLoad),
                                                false);

            List<LdapEntry> ldapEntries = (ldapMessage != null ? ldapMessage.Ldap_Get_Entries() : null);
            if (ldapEntries != null && ldapEntries.Count > 0)
            {
                ldapPaths = new List<string>();

                foreach (LdapEntry entry in ldapEntries)
                {
                    Thread thread = new Thread(new ParameterizedThreadStart(GetObjectLdapPath));
                    thread.Start(entry);
                    thread.Join();
                }

                return ldapPaths;
            }

            return null;
        }


        #region helper_funcs

        private void GetObjectLdapPath(object args)
        {
            if (!(args is LdapEntry))
            {
                return;
            }

            if (args != null)
            {
                LdapEntry entry = args as LdapEntry;

                string[] attrsList = entry.GetAttributeNames();

                if (attrsList != null && attrsList.Length > 0)
                {
                    if (entry != null)
                    {
                        LdapValue[] values = entry.GetAttributeValues("distinguishedName", dirContext);
                        if (values != null && values.Length > 0)
                        {
                            ldapPaths.Add(string.Concat("LDAP://", sServer, "/", values[0].stringData));
                        }
                    }
                }
                //else return null;
            }
        }

        private bool HasCreds()
        {
            return !(this.Username == null || this.Username == "" || this.Password == null || this.Password == "");
        }

        private bool findDircontext(int portNumber)
        {
            bool FindDirContext = false;

            if (exisitngDirContext.Count > 0)
            {
                foreach (DirectoryContext context in exisitngDirContext)
                {
                    if (HasCreds())
                    {
                        if (context.RootDN.Equals(rootDN, StringComparison.InvariantCultureIgnoreCase)
                         && context.PortNumber == portNumber
                         && context.UserName == this.Username
                         && context.Password == this.Password)
                        {
                            dirContext = context;
                            FindDirContext = true;
                            break;
                        }
                    }
                    else
                    {
                        if (context.RootDN.Equals(rootDN, StringComparison.InvariantCultureIgnoreCase)
                         && context.PortNumber == portNumber)
                        {
                            dirContext = context;
                            FindDirContext = true;
                            break;
                        }
                    }
                }
            }

            return FindDirContext;
        }

        //look throught current dirContext to see whether we can find an existing one, the existing one shall carry the GC servername information
        private string findGCServer()
        {
            if (exisitngDirContext.Count > 0)
            {
                foreach (DirectoryContext context in exisitngDirContext)
                {
                    if (context.RootDN.Equals(rootDN, StringComparison.InvariantCultureIgnoreCase))
                        return context.GCServername;
                }
            }

            return null;
        }

        private bool findBindingMethod()
        {
            if (exisitngDirContext.Count > 0)
            {
                foreach (DirectoryContext context in exisitngDirContext)
                {
                    if (context.RootDN.Equals(rootDN, StringComparison.InvariantCultureIgnoreCase))
                        return context.BindMethod;
                }
            }

            return false;
        }


        public static bool FindDirectoryContext(int portNumber, string rdn, out DirectoryContext dc)
        {
            if (exisitngDirContext.Count > 0)
            {
                foreach (DirectoryContext context in exisitngDirContext)
                {
                    if (context.RootDN.Equals(rdn, StringComparison.InvariantCultureIgnoreCase)
                        && (context.PortNumber == portNumber))
                    {
                        dc = context;
                        return true;
                    }
                }
                dc = null;
                return false;
            }
            else
            {
                dc = null;
                return false;
            }
        }

        private string[] Getsearch_attrs(string[] propertiesToLoad)
        {
            string[] search_attr;

            if (propertiesToLoad == null || propertiesToLoad.Length == 0)
            {
                search_attr = new string[] { null };
            }
            else
            {
                search_attr = new string[propertiesToLoad.Length + 1];
                int i;
                for (i = 0; i < propertiesToLoad.Length; i++)
                    search_attr[i] = propertiesToLoad[i];
                search_attr[i] = null;
            }

            return search_attr;
        }

        private void Get_baseDn_Guid_Or_sid()
        {
            if (sCNs != null && sCNs.StartsWith("<")) //for instance, LDAP://corpqa.centeris.com/<GUID=***> <GUID...> part will be used as filter to search the whole domain
            {
                //GUID=\XX\XX\XX...
                if (sCNs.Substring(1, 4).Equals("GUID", StringComparison.InvariantCultureIgnoreCase))
                {
                    string guidstr = sCNs.Substring(6);
                    guidstr = guidstr.Substring(0, guidstr.Length - 1);

                    Guid myguid = new Guid(guidstr);

                    byte[] guidbytes = myguid.ToByteArray();
                    System.Text.Encoding enc = System.Text.Encoding.ASCII;
                    string guidbytestr = BitConverter.ToString(guidbytes);
                    guidbytestr = guidbytestr.Replace("-", "\\");
                    guidbytestr = string.Concat("\\", guidbytestr);

                    string dN = SDSUtils.SearchByGuid(guidbytestr, dirContext);

                    if (dN != null) baseDn = dN;

                    get_baseDnFor_guidOrsid_called = true;
                }
                //SID=S-1-... Or SID=\XX\XX\XX....
                if (sCNs.Substring(1, 3).Equals("SID", StringComparison.InvariantCultureIgnoreCase))
                {
                    string sid = sCNs.Substring(5);
                    sid = sid.Substring(0, sid.Length - 1);

                    if (!sid.StartsWith("S-"))
                    {

                        char[] ldapsid = new char[sid.Length / 2 * 3];

                        int j = 0;

                        for (int i = 0; i < ldapsid.Length; i++)
                        {
                            if (i % 3 == 0) ldapsid[i] = '\\';
                            else
                            {
                                ldapsid[i] = sid[j];
                                j++;
                            }
                        }

                        sid = new string(ldapsid);
                    }

                    string dN = SDSUtils.SearchBySid(sid, dirContext);

                    baseDn = dN;

                    get_baseDnFor_guidOrsid_called = true;
                }

            }

            //assign sName value using the dN of this node
            if (baseDn.Equals("", StringComparison.InvariantCultureIgnoreCase))
                sName = "RootDSE";
            else
                sName = baseDn;
        }

        private bool FindSchemaCache()
        {
            if (existingSchemaCache.Count > 0)
            {
                foreach (LDAPSchemaCache schemaCache in existingSchemaCache)
                {
                    if (schemaCache.rootDN.Equals(rootDN,StringComparison.InvariantCultureIgnoreCase))
                    {
                        dirContext.SchemaCache = schemaCache;

                        return true;
                    }
                }
                return false;
            }
            else return false;

        }

        private DirectoryContext Assign_dirContext()
        {
            if (dirContext == null)
            {
                if (sProtocol.Equals("GC", StringComparison.InvariantCultureIgnoreCase))
                {
                    if (!findDircontext(GC_PORT))
                    {
                        //Console.WriteLine("****************Creating GC directoryContext*******************");

                        if (findGCServer() != null)
                            sServer = findGCServer();

                        string errorMessage = null;

                        dirContext = DirectoryContext.CreateDirectoryContext(
                                        sServer,
                                        rootDN,
                                        this.sUsername,
                                        this.sPassword,
                                        GC_PORT,
                                        findBindingMethod(),
                                        out errorMessage);

                        if (dirContext != null)
                        {
                            //Console.WriteLine("-------------Global catalog DirectoryContext is created.--------------------");
                            exisitngDirContext.Add(dirContext);
                            if (!FindSchemaCache())
                            {
                                dirContext.SchemaCache = LDAPSchemaCache.Build(dirContext);
                                existingSchemaCache.Add(dirContext.SchemaCache);
                            }
                        }
                    }
                }
                else if (sProtocol.Equals("LDAP", StringComparison.InvariantCultureIgnoreCase))
                {
                    if (!findDircontext(LDAP_PORT))
                    {
                        string errorMessage = null;

                        //Console.WriteLine("****************Creating Normal Ldap directoryContext*******************");
                        dirContext = DirectoryContext.CreateDirectoryContext(
                                        sServer,
                                        rootDN,
                                        this.sUsername,
                                        this.sPassword,
                                        LDAP_PORT,
                                        findBindingMethod(),
                                        out errorMessage);

                        if (dirContext != null)
                        {
                            //Console.WriteLine("-------------------Normal LDAP DirectoryContext is created.--------------------");
                            exisitngDirContext.Add(dirContext);
                            //Console.WriteLine("dircontext portnumber is " + dirContext.PortNumber + dirContext.LdapHandle);
                            if (!FindSchemaCache())
                            {
                                dirContext.SchemaCache = LDAPSchemaCache.Build(dirContext);
                                existingSchemaCache.Add(dirContext.SchemaCache);
                            }
                        }
                    }
                }
            }

            if (dirContext == null)
                throw new Exception("Create DirectoryContext Failed - logon failure: Unknown Username or Bad password.");
            else
                return dirContext;
        }

        private void unbind_dirContext(string rootDN)
        {
            List<int> removeIndex = new List<int>();

            if (exisitngDirContext != null && exisitngDirContext.Count > 0)
            {
                foreach (DirectoryContext dirCon in exisitngDirContext)
                {
                    if (dirCon.RootDN.Equals(rootDN, StringComparison.InvariantCultureIgnoreCase))
                    {
                        int ret = dirCon.LdapHandle.Ldap_Unbind_S();// DirContext_unbind();
                        if (ret == 0)
                        {
                            //Console.WriteLine("Unbind existing dirContext " + dirCon.PortNumber);
                            removeIndex.Add(exisitngDirContext.IndexOf(dirCon));
                        }
                    }
                }

                foreach (int index in removeIndex)
                  exisitngDirContext.RemoveAt(index);
            }
        }

        public static void ObtainCreds(out string username, out string password, string rootDN)
        {
            username = string.Empty;
            password = string.Empty;
            if (exisitngDirContext != null && exisitngDirContext.Count > 0)
            {
                foreach (DirectoryContext context in exisitngDirContext)
                {
                    //if (context.RootDN.Equals(rootDN, StringComparison.InvariantCultureIgnoreCase))
                    if (context.DomainName.Equals(rootDN, StringComparison.InvariantCultureIgnoreCase))
                    {
                        username = context.UserName;
                        password = context.Password;
                        break;
                    }
                }
            }
        }

        public static int CheckLdapTimedOut(string sDN, string sRootDN)
        {
            int ret = -1;
            LdapMessage message = null;
            DirectoryContext dircontext = null;

            if (exisitngDirContext != null && exisitngDirContext.Count > 0)
            {
                foreach (DirectoryContext context in exisitngDirContext)
                {
                    if (context.DomainName.Equals(sRootDN, StringComparison.InvariantCultureIgnoreCase))
                    {
                        dircontext = context;
                    }
                }
            }

            ret = dircontext.SearchSynchronous(
                             sDN,
                             Likewise.LMC.LDAP.Interop.LdapAPI.LDAPSCOPE.BASE,
                             "(objectClass=*)",
                             new string[]
                             {
                             "objectClass", "distinguishedName", null
                             },
                             false,
                             out message);

            return ret;
        }

        //- RootDSE -> configurationNamingContext
        //- cn=partition, "configurationNamingContext" -> netBIOSName
        //Using the value of "netBIOSName" as shortDomain name.
        public string GetnetBiosName(string domain)
        {
            int ret = -1;
            string configurationName = null;
            string netbiosName = null;
            string baseDn = "";
            string[] search_attrs = { null };

            DirectoryContext dircontext = null;

            if (exisitngDirContext != null && exisitngDirContext.Count > 0)
            {
                foreach (DirectoryContext context in exisitngDirContext)
                {
                    if (context.DomainName.Equals(domain, StringComparison.InvariantCultureIgnoreCase))
                    {
                        dircontext = context;
                    }
                }
            }

            //searching with baseDn="" allows ldap to access the domain “RootDSE”.
            //Without passing that, it cannot access the configurationNamingContext
            List<LdapEntry> ldapEntries = null;
            ret = dircontext.ListChildEntriesSynchronous
            (baseDn,
            LdapAPI.LDAPSCOPE.BASE,
            "(objectClass=*)",
            search_attrs,
            false,
            out ldapEntries);

            if (ldapEntries != null && ldapEntries.Count > 0)
            {
                LdapEntry ldapNextEntry = ldapEntries[0];

                LdapValue[] values = ldapNextEntry.GetAttributeValues("configurationNamingContext", dircontext);

                if (values != null && values.Length > 0)
                {
                    configurationName = values[0].stringData;
                }
            }
            //by default, if we couldn't find configurateName we use CN=configuration + rootDN as one
            if (configurationName == null)
            {
                configurationName = "CN=configuration,";
                configurationName = string.Concat(configurationName, dircontext.RootDN);
            }

            string partitionDn = "CN=Partitions,";

            partitionDn = string.Concat(partitionDn, configurationName);

            string sFilter = "(&(objectcategory=Crossref)(dnsRoot=" + domain.ToLower() + ")(netBIOSName=*))";

            List<LdapEntry> ldapEntries1 = null;
            ret = dircontext.ListChildEntriesSynchronous
            (partitionDn,
            LdapAPI.LDAPSCOPE.SUB_TREE,
            sFilter,
            search_attrs,
            false,
            out ldapEntries1);

            if (ldapEntries1 != null && ldapEntries1.Count > 0)
            {
                LdapEntry ldapNextEntry = ldapEntries1[0];

                LdapValue[] values = ldapNextEntry.GetAttributeValues("netBIOSName", dircontext);
                netbiosName = values[0].stringData;
            }

            //by default, if we couldn't find netbiosName we use the first portion of rootDn as one
            if (netbiosName == null)
            {
                string[] rootDns = dircontext.RootDN.Split(',');
                netbiosName = rootDns[0].Substring(3).ToUpper();
            }

            return netbiosName;

        }

        #endregion
    }
}
