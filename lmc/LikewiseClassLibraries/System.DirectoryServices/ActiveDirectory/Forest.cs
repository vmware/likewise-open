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
using System.Collections.Specialized;

namespace System.DirectoryServices.ActiveDirectory
{
    public class Forest
    {
        private string fName; //name of the forest         
        private DirectoryContext dc;

        private DomainCollection domains = null;
        private GlobalCatalogCollection globalcatalogs = null;
        private Domain rootdomain = null;



        public Forest()
            : base()
        {
        }
        
        public static Forest GetForest(DirectoryContext dc)
        {
            Forest fr = new Forest();

            fr.fName = dc.Name;
            fr.DC = dc;

            return fr;
        }

        public GlobalCatalogCollection GlobalCatalogs
        {
            get
            {
                if (globalcatalogs == null)
                {
                    DirectoryEntry rootDse = new DirectoryEntry(string.Format("LDAP://{0}/RootDSE", fName), dc.UserName, dc.Password);

                    string confName = string.Empty;
                    if (rootDse.Properties["configurationNamingContext"].Value != null)
                    {
                        confName = rootDse.Properties["configurationNamingContext"].Value.ToString();
                    }
                    else
                    {                        
                        confName = string.Concat("CN=Configuration,", DomainNameToDN(fName));
                    }

                    //Console.WriteLine("confName is " + confName);

                    DirectoryEntry confde = new DirectoryEntry(string.Format("LDAP://{0}/{1}/",fName,confName), dc.UserName, dc.Password);

                    DirectorySearcher confds = new DirectorySearcher(confde);
                    confds.Filter = "(&(objectcategory=ntdsdsa)(options=1))";
                    confds.SearchScope = SearchScope.Subtree;
                    StringCollection attrs = new StringCollection();
                   // attrs.Add("distinguishedName");
                   // confds.PropertiesToLoad = attrs;

                    SearchResultCollection src = confds.FindAll();

                    if (src!=null && src.Count >0)
                    {
                        globalcatalogs = new GlobalCatalogCollection();

                        foreach (SearchResult sr in src)
                        {      
                            string sProtocol, sServer, sCNs, sDCs;
                            SDSUtils.CrackPath(sr.Path, out sProtocol, out sServer, out sCNs, out sDCs);

                            /*Console.WriteLine("sProtocol " + sProtocol);
                            Console.WriteLine("sServer " + sServer);
                            Console.WriteLine("sCNs " + sCNs);
                            Console.WriteLine("sDCs " + sDCs);*/
                            //sCNs  CN=NTDS Settings, CN=CORPQA-DC2,CN=Servers,CN=Default-First-Site-Name,CN=Sites,CN=Configuration
                            //sDCs is DC=corpqa,DC=centeris,DC=com
                            string gcName = sServer;
                          /*  if (sCNs != null)
                            {
                                string[] splits = sCNs.Split(',');
                                gcName = string.Format("{0}.{1}", splits[1].Substring(3).ToLower(), sServer);                                
                            }*/
                            GlobalCatalog gc = GlobalCatalog.GetGlobalCatalog(
                                new DirectoryContext(DirectoryContextType.DirectoryServer, gcName, dc.UserName, dc.Password));

                            globalcatalogs.Add(gc);
                        }
                    }
                }

                return globalcatalogs;
            }
        }

        public TrustRelationshipInformationCollection GetAllTrustRelationships()
        {
            TrustRelationshipInformationCollection trustcollection = new TrustRelationshipInformationCollection();

            DomainCollection dmc = this.Domains;

            if (dmc != null && dmc.Count > 0)
            {
                foreach (Domain dm in dmc)
                {
                    TrustRelationshipInformationCollection dmtrusts = dm.GetAllTrustRelationships();
                    if (dmtrusts != null && dmtrusts.Count > 0)
                    {
                        foreach (TrustRelationshipInformation trust in dmtrusts)
                            trustcollection.Add(trust);
                    }
                }
            }

            return trustcollection;
        }

        public DomainCollection Domains
        {
            get
            {
                if (domains == null)
                {
                    string sPath = string.Format("LDAP://{0}/RootDSE", fName);
                    DirectoryEntry rootDse = new DirectoryEntry(sPath, dc.UserName, dc.Password);                

                    string rootDomainName = rootDse.Properties["rootDomainNamingContext"].Value as string;

                    if (rootDomainName == null)
                    {
                        sPath = string.Format("LDAP://{0}/{1}", fName, SDSUtils.DomainNameToDN(fName));
                        rootDse = new DirectoryEntry(sPath, dc.UserName, dc.Password);

                        rootDomainName = rootDse.Properties["distinguishedName"].Value as string;
                    }

                    GlobalCatalog gc = GlobalCatalog.GetGlobalCatalog(new DirectoryContext(DirectoryContextType.Forest, SDSUtils.DNToDomainName(rootDomainName).ToLower(), dc.UserName, dc.Password));                    
                    DirectorySearcher gc_ds = gc.GetDirectorySearcher();

                    gc_ds.Filter = "(objectcategory=domainDNS)";
                    gc_ds.SearchScope = SearchScope.Subtree;
                    
                    StringCollection attrs = new StringCollection();
                    attrs.Add("name");
                    attrs.Add("distinguishedName");

                    gc_ds.PropertiesToLoad = attrs;

                    SearchResultCollection src = gc_ds.FindAll();

                    if (src != null && src.Count > 0)
                    {
                        domains = new DomainCollection();

                        foreach (SearchResult sr in src)
                        {
                            string sProtocol, sServer, sCNs, sDCs;
                                                                                                                         
                            SDSUtils.CrackPath(sr.Path, out sProtocol, out sServer, out sCNs, out sDCs);
                            /*Console.WriteLine("sProtocol " + sProtocol);
                            Console.WriteLine("sServer " + sServer);
                            Console.WriteLine("sCNs " + sCNs);
                            Console.WriteLine("sDCs " + sDCs);*/ //sDCs is DC=AREKDOMAIN, DC=corpqa,DC=centeris,DC=com

                            string domainName = sServer;
                            if (sDCs != null)
                                domainName = SDSUtils.DNToDomainName(sDCs).ToLower();

                            Domain domain = Domain.GetDomain(new DirectoryContext(DirectoryContextType.Domain, domainName, dc.UserName, dc.Password));

                            domains.Add(domain);
                        }
                    }
                }

                if (domains == null)
                {
                    domains = new DomainCollection();

                    Domain domain = Domain.GetDomain(new DirectoryContext(DirectoryContextType.Domain, fName, dc.UserName, dc.Password));

                    domains.Add(domain);
                }

                return domains;
            }
        }

        public Domain RootDomain
        {
            get
            {
                if (rootdomain == null)
                {
                    DirectoryEntry rootDse = new DirectoryEntry(string.Format("LDAP://{0}/RootDSE", fName), dc.UserName, dc.Password);

                    string rootDomainName = rootDse.Properties["rootDomainNamingContext"].Value.ToString();

                    rootdomain = Domain.GetDomain(new DirectoryContext(DirectoryContextType.Domain, 
                        SDSUtils.DNToDomainName(rootDomainName), dc.UserName, dc.Password));
                }

                return rootdomain;
            }
        }

        /// <summary>
        /// Convert dot separated domain name to LDAP distinguished name.
        /// </summary>
        /// <param name="domainName">dot separated domain name</param>
        /// <returns>LDAP distinguished name for the domain</returns>
        private static string DomainNameToDN(string domainName)
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

        public string Name
        {
            get
            {
                return fName;
            }
            set
            {
                fName = value;
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

    }
}
