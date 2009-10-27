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

namespace System.DirectoryServices
{
    public class DirectoryEntries: List<DirectoryEntry>
    {
        private string parentDN;
        private string sServer;
        private DirectoryContext dirContext;

        public DirectoryEntries()
            :base()
        {

        }

        public DirectoryEntries(string parentDN, string sServer, DirectoryContext dirContext)
            : base()
        {
            this.parentDN = parentDN;
            this.sServer = sServer;
            this.dirContext = dirContext;        
        }

        public DirectoryEntry Add(string sName, string sClass)
        {
            //for instance: sName is CN=***  sClass is "container"
            string newChildLdapPath = string.Concat("LDAP://" + sServer);
            string newChildDN = string.Concat(sName,",", parentDN);
            newChildLdapPath = string.Concat(newChildLdapPath, "/", newChildDN);

            //Console.WriteLine("In DirectoryEntries:Add: newChildLdapPath is " + newChildLdapPath);      
            
          //  DirectoryEntry parent = new DirectoryEntry(string.Format("LDAP://{0}/{1}",sServer,parentDN));

          //  Console.WriteLine("parent's DN is " + parent.Name);

         //   Console.WriteLine("grab parent node info " + parent.Properties["distinguishedName"].Value.ToString());

            DirectoryEntry newChild = new DirectoryEntry(newChildLdapPath);

            newChild.SchemaClassName = sClass;

            //Console.WriteLine("dirContext is " + dirContext.DomainName + " dirContext portNumber is " + dirContext.PortNumber + " sClass is " + sClass
            //    + " newchildDN is " + newChildDN );            

            

            int ret = SDSUtils.AddNewObj(dirContext, sClass, newChildDN);

            //System.Threading.Thread.Sleep(5000);

            //Console.WriteLine("addnew obj returned " + ret);        

            if (ret == 0)
            {
                this.Add(newChild);
                return newChild;
            }
            else
            {
                //Console.WriteLine("Failed to add one child " + newChildDN);
                return null;
            }

           // return newChild;
        }

        //Deletes a child DirectoryEntry from this collection.
        public new void Remove(DirectoryEntry entryTodelete)
        {
            foreach (DirectoryEntry entry in this)
            {
                if (entryTodelete.Path.Equals(entry.Path, StringComparison.InvariantCultureIgnoreCase))
                {
                    entry.ToBeDeleted = true;
                    //Console.WriteLine("In DirectoryEntries:Remove: removedChildLpath is " + entryTodelete.Path); 
                    break;
                }
            }
        }

        public DirectoryEntry Find(string sName)
        {
           //for instance, when CN=$LikewiseIdentityCell we need match the first portion
            foreach (DirectoryEntry entry in this)
            {
                string sProtocol, sServer, sCNs, sDCs = null;

                SDSUtils.CrackPath(entry.Path, out sProtocol, out sServer, out sCNs, out sDCs);

                if (sCNs != null)
                {
                    string[] splits = sCNs.Split(',');
                    if (splits[0].Equals(sName, StringComparison.InvariantCultureIgnoreCase))
                        return entry;
                }
            }

            return null;
        }
    }
}
