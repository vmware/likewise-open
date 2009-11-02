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
using Likewise.LMC.LDAP.Interop;

namespace Likewise.LMC.LDAP
{
    public class LdapUtils
    {
        //find the DN of given the groupsid
        public static string SearchByGuid(string objectGuid, DirectoryContext dirContext)
        {
            int ret = -1;
            string searchFilter = objectGuid;
            searchFilter = string.Concat("(objectGuid=", objectGuid, ")");
            List<LdapEntry> ldapEntries = null;

            ret = dirContext.ListChildEntriesSynchronous(
                               dirContext.RootDN,
                               LdapAPI.LDAPSCOPE.SUB_TREE,
                               searchFilter,
                               null,
                               false,
                               out ldapEntries);

            if (ldapEntries == null || ldapEntries.Count == 0)
            {
                Logger.Log("ldapEntries.Count == 0");
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


        //find the DN of given the sid
        public static string SearchBySid(string sid, DirectoryContext dirContext)
        {
            int ret = -1;
            string searchFilter = string.Concat("(objectSid=", sid, ")");
            List<LdapEntry> ldapEntries = null;

            ret = dirContext.ListChildEntriesSynchronous(
                                        dirContext.RootDN,
                                        LdapAPI.LDAPSCOPE.SUB_TREE,
                                        searchFilter,
                                        null,
                                        false,
                                        out ldapEntries);
            
            if (ldapEntries == null || ldapEntries.Count == 0)
            {
                Logger.Log("ldapEntries.Count == 0");
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
}
