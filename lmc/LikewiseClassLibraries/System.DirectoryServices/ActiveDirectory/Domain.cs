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

namespace System.DirectoryServices.ActiveDirectory
{
    public enum ADTrustType
    {
        TYPE_DOWNLEVEL = 1, //downlevel trust (to a windows NT domain)
        TYPE_UPLEVEL = 2,   //uplevel Windows 2003 (with an AD domain)
        TYPE_MIT = 3,   //with MIT kerberos v5 realm
        TYPE_DCE = 4   //with DCE realm
    }

    public class Domain
    {
        private string dName;
        private DirectoryContext dc;
        private TrustRelationshipInformationCollection trustCollection;
        private Domain parent;
        private DomainCollection children;

        public Domain()
            : base()
        {
            trustCollection = null;
            parent = null;
            children = new DomainCollection();            
        }

        public Domain(string dName)
            : this()
        {
            this.dName = dName;
        }

        public static Domain GetDomain(DirectoryContext dc)
        {
            Domain domain = new Domain();

            domain.dName = dc.Name;
            domain.DC = dc;

            return domain;           

        }

        //Retrieves a DirectoryEntry object that represents the default naming context of the domain.         
        public DirectoryEntry GetDirectoryEntry()
        {  
            return new DirectoryEntry(string.Format("LDAP://{0}/Domain", this.dName));
        }

        public TrustRelationshipInformationCollection GetAllTrustRelationships()
        {
            if (trustCollection == null)
            {
                try
                {
                    DirectoryEntry rootDse = new DirectoryEntry(string.Format("LDAP://{0}/RootDSE", dName), dc.UserName, dc.Password);

                    string defaultName = rootDse.DirContext.DefaultNamingContext;

                    if (defaultName == null || defaultName == "")
                    {
                        trustCollection = null;
                        return trustCollection;
                    }

                    DirectoryEntry sys = new DirectoryEntry(string.Format("LDAP://{0}/CN=System,{1}", SDSUtils.DNToDomainName(defaultName), defaultName), dc.UserName, dc.Password);

                    DirectorySearcher ds = new DirectorySearcher(sys);
                    ds.Filter = "(objectClass=trustedDomain)";
                    ds.SearchScope = SearchScope.Subtree;

                    SearchResultCollection src = ds.FindAll();

                    if (src != null && src.Count > 0)
                    {
                        trustCollection = new TrustRelationshipInformationCollection();

                        foreach (SearchResult sr in src)
                        {
                            string sProtocol, sServer, sCNs, sDCs;
                            SDSUtils.CrackPath(sr.Path, out sProtocol, out sServer, out sCNs, out sDCs);
                            /*Console.WriteLine("sProtocol " + sProtocol);
                            Console.WriteLine("sServer " + sServer);
                            Console.WriteLine("sCNs " + sCNs);
                            Console.WriteLine("sDCs " + sDCs);*/                          

                            string sourcename, targetname;
                            TrustDirection trustdirection;
                            TrustType trusttype = TrustType.Unknown;

                            DirectoryEntry trustEntry = new DirectoryEntry(sr.Path, dc.UserName, dc.Password);

                            int trustdir = (int)trustEntry.Properties["trustDirection"].Value;

                            string trustDn = trustEntry.Properties["distinguishedName"].Value.ToString();
                            string[] splits = trustDn.Split(',');
                            trustDn = splits[0].Substring(3);

                            int trustattr = (int)trustEntry.Properties["trustAttributes"].Value;

                            int trusttp = (int)trustEntry.Properties["trustType"].Value;

                            //Note:the following implementation of how to determine the TrustType is still under investigation
                            if (trusttp == (int)ADTrustType.TYPE_UPLEVEL) //windows 2003 trust
                            {
                                switch (trustattr)
                                {
                                    case 0:
                                        trusttype = TrustType.External; //this trust is non-transitive
                                        break;
                                    case 1:   //ATTRIBUTES_NON_TRANSITIVE
                                        break;
                                    case 2: //ATTRIBUTES_UPLEVEL_ONLY
                                        break;

                                    case 4: //ATTRIBUTES_QUARANTINED_DOMAIN
                                        trusttype = TrustType.External;
                                        break;

                                    case 8: //ATTRIBUTES_FOREST_TRANSITIVE
                                        trusttype = TrustType.Forest; //and this trust is transitive
                                        break;

                                    case 16: //ATTRIBUTES_CROSS_ORGANIZATION
                                        trusttype = TrustType.CrossLink;
                                        break;

                                    case 32://ATTRIBUTES_WITHIN_FOREST
                                        if (trustDn.ToLower().Contains(dName.ToLower()))
                                            trusttype = TrustType.ParentChild;
                                        else
                                            trusttype = TrustType.External;  //this trust is non-transitive
                                        break;

                                    case 64: //ATTRIBUTES_TREAT_AS_EXTERNAL
                                        trusttype = TrustType.External;
                                        break;

                                    default:
                                        trusttype = TrustType.Unknown;
                                        break;
                                }
                            }
                            else if (trusttp == (int)ADTrustType.TYPE_MIT)
                                trusttype = TrustType.Kerberos;

                            switch (trustdir)
                            {
                                case 1:
                                    trustdirection = TrustDirection.Inbound;
                                    sourcename = dName;
                                    targetname = trustDn;
                                    break;
                                case 2:
                                    trustdirection = TrustDirection.Outbound;
                                    sourcename = trustDn;
                                    targetname = dName;
                                    break;
                                case 3:
                                    trustdirection = TrustDirection.Bidirectional;
                                    sourcename = dName;
                                    targetname = trustDn;
                                    break;
                                default:
                                    trustdirection = TrustDirection.Disabled;
                                    sourcename = targetname = "";
                                    break;
                            }

                            TrustRelationshipInformation trustinfo = new TrustRelationshipInformation(sourcename, targetname, trusttype, trustdirection);                      
                            trustCollection.Add(trustinfo);
                        }
                    }
                }
                catch
                {
                    return null;
                }
            }
            
            return trustCollection;            
        }

        public string Name
        {
            get
            {
                return dName;
            }
            set
            {
                dName = value;
            }
        }

        public DirectoryContext DC
        {
            get
            {
                return dc;
            }
            set
            {
                dc = value;
            }
        }


        public Domain Parent
        {
            get
            {
                if (parent == null)
                {
                    FindParentDomain();                   
                }                                               

                return parent;
            }
        }


        public DomainCollection Children
        {
            get
            {
                if (children.Count == 0)
                {
                    FindChildrenDomains();
                }

                return children;
            }
        }

        #region //helper functions

        private void FindParentDomain()
        {   
            DirectoryEntry rootDse = new DirectoryEntry(string.Format("LDAP://{0}/RootDSE", dName), dc.UserName, dc.Password);

            string configureName = rootDse.DirContext.ConfigurationNamingContext;

            if (configureName == null || configureName == "")
            {
                parent = null;
                return;
            }

            DirectoryEntry sys = new DirectoryEntry(string.Format("LDAP://{0}/CN=Partitions,{1}", SDSUtils.DNToDomainName(configureName), configureName), dc.UserName, dc.Password);
            
            DirectorySearcher ds = new DirectorySearcher(sys);

            ds.Filter = "(objectClass=crossRef)";
            ds.SearchScope = SearchScope.OneLevel;

            SearchResultCollection src = ds.FindAll();

            if (src != null && src.Count > 0)
            {
                foreach (SearchResult sr in src)
                {

                    string sProtocol, sServer, sCNs, sDCs;
                    SDSUtils.CrackPath(sr.Path, out sProtocol, out sServer, out sCNs, out sDCs);

                    DirectoryEntry partEntry = new DirectoryEntry(sr.Path, dc.UserName, dc.Password);

                    string partName = partEntry.Properties["nCName"].Value as string;

                    if (dName.Equals(SDSUtils.DNToDomainName(partName), StringComparison.InvariantCultureIgnoreCase))
                    {
                        string parentDomainDN = partEntry.Properties["trustParent"].Value as string;

                        if (parentDomainDN != null && parentDomainDN != "")
                        {
                            parent = new Domain(SDSUtils.DNToDomainName(parentDomainDN));

                            break;
                        }
                    }
                }
            }

            return;
        }

        private void FindChildrenDomains()
        {               

            DirectoryEntry rootDse = new DirectoryEntry(string.Format("LDAP://{0}/RootDSE", dName), dc.UserName, dc.Password);

            string configureName = rootDse.DirContext.ConfigurationNamingContext;

            if (configureName == null || configureName == "")
            {                   
                return;
            }

            DirectoryEntry sys = new DirectoryEntry(string.Format("LDAP://{0}/CN=Partitions,{1}", SDSUtils.DNToDomainName(configureName), configureName), dc.UserName, dc.Password);

            DirectorySearcher ds = new DirectorySearcher(sys);

            ds.Filter = "(objectClass=crossRef)";
            ds.SearchScope = SearchScope.OneLevel;

            SearchResultCollection src = ds.FindAll();

            if (src != null && src.Count > 0)
            {
                foreach (SearchResult sr in src)
                {

                    string sProtocol, sServer, sCNs, sDCs;
                    SDSUtils.CrackPath(sr.Path, out sProtocol, out sServer, out sCNs, out sDCs);

                    DirectoryEntry partEntry = new DirectoryEntry(sr.Path, dc.UserName, dc.Password);

                    string parentDomainDN = partEntry.Properties["trustParent"].Value as string;

                    if (parentDomainDN != null && parentDomainDN != "" &&
                        dName.Equals(SDSUtils.DNToDomainName(parentDomainDN), StringComparison.InvariantCultureIgnoreCase))
                    {
                        children.Add(new Domain(SDSUtils.DNToDomainName(partEntry.Properties["nCName"].Value as string)));                        
                    }
                }
            }

            return;
        }

        #endregion
    }
}
