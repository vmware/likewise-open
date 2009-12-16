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
using System.Net;

namespace System.DirectoryServices.ActiveDirectory
{
    public class GlobalCatalog
    {

        private DirectoryContext dc; //the corresponding directoryContext for this GC
        private string ipaddr;
        private string gcName;
        //private bool gcEnabled = false;

        public GlobalCatalog()
            :base()
        {
        }

        public string IPAddress
        {
            get
            {
                return ipaddr;
            }
            set
            {
                ipaddr = value;
            }
        }

        public string Name
        {
            get
            {
                return gcName;
            }
            set
            {
                gcName = value;
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

        //before this gets called, the dc should be assigned, hence name, username, password should be set.
        public DirectorySearcher GetDirectorySearcher()
        {
            string gcLdapPath = string.Format("GC://{0}/", dc.Name);

            DirectoryEntry deSearchRoot = new DirectoryEntry(gcLdapPath, dc.UserName, dc.Password);

            return new DirectorySearcher(deSearchRoot);
        }

        public DirectoryEntry GetDirectoryEntry()
        {
            string gcLdapPath = string.Format("GC://{0}/", dc.Name);

            return new DirectoryEntry(gcLdapPath, dc.UserName, dc.Password);
        }


        //if we can find a GC to talk to, return that GC, if not, return null
        public static GlobalCatalog GetGlobalCatalog(DirectoryContext dc)
        {
            GlobalCatalog gc;

            bool gcEnabled = true;

            //using dc.Name to query RootDSE
           /* string sPath = string.Format("LDAP://{0}/RootDSE", dc.Name);
            DirectoryEntry rootDse = new DirectoryEntry(sPath, dc.UserName,dc.Password);
            bool gcEnabled = false;
            //gcEnabled = (bool)rootDse.Properties["isGlobalCatalogReady"].Value;
            string ntdsdsaPath = string.Format("LDAP://{0}/{1}/", dc.Name, rootDse.Properties["configurationNamingContext"].Value.ToString());
            Console.WriteLine("ntdsdsaPath is " + ntdsdsaPath);
            DirectorySearcher ds = new DirectorySearcher(new DirectoryEntry(ntdsdsaPath, dc.UserName, dc.Password));
            ds.Filter = "(&(objectcategory=ntdsdsa)(options=1))";
            ds.SearchScope = SearchScope.Subtree;
            SearchResultCollection src = ds.FindAll();
            if (src != null && src.Count > 0)
                gcEnabled = true;                */

            if (gcEnabled)
            {
                gc = new GlobalCatalog();
                gc.DC = dc;
                gc.Name = dc.Name;
                IPHostEntry ipHostEntry = Dns.GetHostEntry(gc.Name);
                gc.IPAddress = ipHostEntry.AddressList[0].ToString();
            }
            else
            {
                gc = null; //can't find a global catalog to talk to
                //Console.WriteLine("Cannot find a GC to talk to.");
            }

            return gc;
        }

        public void Print()
        {
            Console.WriteLine(string.Format("GC name is {0}, IpAddr is {1}.", gcName, ipaddr));
        }
    }
}
