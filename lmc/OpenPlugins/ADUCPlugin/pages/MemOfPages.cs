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
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;
using System.Windows.Forms;
using Likewise.LMC.LDAP;
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public class MemOfPages
{
    #region Class Data
    const int ADDING = 1;
    const int REMOVING = 2;
    #endregion


    #region Methods

    public static int GetIndexForADObject(System.DirectoryServices.DirectoryEntry de)
    {
        try
        {
            object[] asProp = de.Properties["objectClass"].Value as object[];
            // poke these in a list for easier reference
            List<string> liClasses = new List<string>();
            foreach (string s in asProp)
            {
                liClasses.Add(s);
            }
            if (liClasses.Contains("user") || liClasses.Contains("computer"))
            {

                string usercontrol = de.Properties["userAccountControl"].Value.ToString();
                int userControl = Convert.ToInt32(usercontrol);
                string userCtrlBinStr = UserGroupUtils.DecimalToBase(userControl, 2);
                if (userCtrlBinStr.Length >= 2)
                {
                    if (liClasses.Contains("computer"))
                    {
                        if (userCtrlBinStr[userCtrlBinStr.Length - 2] == '1')
                        {
                            return (int)ADUCDirectoryNode.GetNodeType("Computer");
                        }
                        else
                        {
                            return (int)ADUCDirectoryNode.GetNodeType("computer");
                        }
                    }
                    if (liClasses.Contains("user"))
                    {
                        if (userCtrlBinStr[userCtrlBinStr.Length - 2] == '1')
                        {
                            return (int)ADUCDirectoryNode.GetNodeType("disabledUser");
                        }
                        else
                        {
                            return (int)ADUCDirectoryNode.GetNodeType("user");
                        }

                    }
                }
            }
            else if (liClasses.Contains("group") || liClasses.Contains("foreignSecurityPrincipal"))
            {
                return (int)ADUCDirectoryNode.GetNodeType("group");
            }
        }

        catch
        {
            return (int)ADUCDirectoryNode.GetNodeType("group");
        }

        return (int)ADUCDirectoryNode.GetNodeType("group");
    }


    /// <summary>
    /// Gets the list of groups those are all of members to the selected node
    /// </summary>
    /// <param name="groupDn"></param>
    /// <param name="_dirnode"></param>
    /// <returns></returns>
    public static List<string> GetMemberAttrofGroup(string groupDn, ADUCDirectoryNode _dirnode)
    {
        string[] search_attrs = { "objectsid", "member", null };
        List<string> member = new List<string>();

        ADUCDirectoryNode newGroupnode = new ADUCDirectoryNode(_dirnode, "group", groupDn);

        List<LdapEntry> ldapEntries = UserGroupUtils.getLdapEntries(false, newGroupnode, search_attrs, "(objectClass=*)", LdapAPI.LDAPSCOPE.BASE);

        if (ldapEntries != null && ldapEntries.Count > 0)
        {
            LdapEntry ldapNextEntry = ldapEntries[0];
            if (ldapNextEntry != null)
            {

                string[] attrsList = ldapNextEntry.GetAttributeNames();


                if (attrsList != null)
                {
                    foreach (string attr in attrsList)
                    {
                        if (attr.Equals("member", StringComparison.InvariantCultureIgnoreCase))
                        {
                            LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, newGroupnode.LdapContext);
                            foreach (LdapValue attrValue in attrValues)
                            {
                                member.Add(attrValue.stringData);
                            }
                        }
                    }
                }
            }
        }
        return member;
    }
    #endregion

    /// <summary>
    /// Modifies the "member" attribute for the selected "user" or "group" in AD schema template
    /// </summary>
    /// <param name="changedGroups"></param>
    /// <param name="_dirnode"></param>
    /// <param name="page"></param>
    /// <param name="operation"></param>
    /// <returns></returns>
    private static bool OnApply_inner(List<string> changedGroups, ADUCDirectoryNode _dirnode, MPPage page, int operation)
    {
        bool retVal = true;
        int ret = -1;
        string AdminGroupDN = string.Concat("CN=Administrators,CN=Builtin,", _dirnode.LdapContext.RootDN);

        if (changedGroups != null && changedGroups.Count > 0)
        {
            foreach (string newGroupname in changedGroups)
            {
                List<string> members = new List<string>();
                members = GetMemberAttrofGroup(newGroupname.Trim(), _dirnode);

                bool existingMember = false;

                //if we want to add, we need check whether it is already a member of the group
                if (operation == ADDING)
                {
                    foreach (string str in members)
                    {
                        if (str.Equals(_dirnode.DistinguishedName, StringComparison.InvariantCultureIgnoreCase))
                        {
                            existingMember = true;
                            break;
                        }
                    }
                }

                if (!existingMember)
                {
                    if (operation == ADDING)
                    {
                        members.Add(_dirnode.DistinguishedName);
                    }
                    if (operation == REMOVING)
                    {
                        members.Remove(_dirnode.DistinguishedName);
                    }

                    if (newGroupname.Trim().ToLower().Equals(AdminGroupDN.Trim().ToLower()))
                    {
                        string userlogonName = OnApply_GetObjectRealmName(_dirnode);
                        LUGAPI.NetAddGroupMember(_dirnode.LdapContext.DomainControllerName, "Administrators", userlogonName);
                    }
                    else
                    {
                        string[] members_values = new string[members.Count + 1];
                        members.CopyTo(members_values);
                        members_values[members.Count] = null;

                        LDAPMod memberattr_Info =
                        new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "member",
                        members_values);

                        LDAPMod[] attrinfo = new LDAPMod[] { memberattr_Info };

                        if (_dirnode != null)
                        {
                            ret = _dirnode.LdapContext.ModifySynchronous(newGroupname.Trim(), attrinfo);

                            if (ret == 0)
                            {
                                retVal = true;
                            }
                            else
                            {
                                string sMsg = ErrorCodes.LDAPString(ret);
                                MessageBox.Show(page, sMsg, "Likewise Management Console",
                                MessageBoxButtons.OK);
                                retVal = false;
                            }
                        }
                    }
                }
            }
            if (ret == 0)
            {
                if (operation == ADDING)
                {
                    MessageBox.Show(
                    page,
                    "User/Computer/Group list is added to new groups!",
                    CommonResources.GetString("Caption_Console"),
                    MessageBoxButtons.OK);
                }
                if (operation == REMOVING)
                {
                    MessageBox.Show(
                    page,
                    "User/Computer/Group list is removed from chose groups!",
                    CommonResources.GetString("Caption_Console"),
                    MessageBoxButtons.OK);
                }
            }
        }
        return retVal;
    }

    private static string OnApply_GetObjectRealmName(ADUCDirectoryNode _dirnode)
    {
        string[] search_attrs = { "sAMAccountName", "userPrincipalName", null };
        string realmName = string.Empty;

        ADUCDirectoryNode newGroupnode = new ADUCDirectoryNode(_dirnode, _dirnode.ObjectClass, _dirnode.DistinguishedName);

        List<LdapEntry> ldapEntries = UserGroupUtils.getLdapEntries(false, newGroupnode, search_attrs, "(objectClass=*)", LdapAPI.LDAPSCOPE.BASE);

        if (ldapEntries != null && ldapEntries.Count > 0)
        {
            LdapEntry ldapNextEntry = ldapEntries[0];
            if (ldapNextEntry != null)
            {

                string[] attrsList = ldapNextEntry.GetAttributeNames();

                if (attrsList != null)
                {
                    foreach (string attr in attrsList)
                    {
                        if (_dirnode.ObjectClass.Equals("group", StringComparison.InvariantCultureIgnoreCase) ||
                            _dirnode.ObjectClass.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
                        {
                            if (attr.Equals("sAMAccountName", StringComparison.InvariantCultureIgnoreCase))
                            {
                                LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, newGroupnode.LdapContext);
                                if (attrValues != null)
                                {
                                    realmName = attrValues[0].stringData;
                                    break;
                                }
                            }
                        }
                        else if (_dirnode.ObjectClass.Equals("user", StringComparison.InvariantCultureIgnoreCase))
                        {
                            if (attr.Equals("userPrincipalName", StringComparison.InvariantCultureIgnoreCase))
                            {
                                LdapValue[] attrValues = ldapNextEntry.GetAttributeValues(attr, newGroupnode.LdapContext);
                                if (attrValues != null)
                                {
                                    realmName = attrValues[0].stringData;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        return realmName;
    }

    #region Helper functions

    public static bool OnApply_helper(List<string> MemofDnList, List<string> AddedGroups, List<string> RemovedGroups, ADUCDirectoryNode _dirnode, MPPage page)
    {
        bool retVal = true;

        if (MemofDnList != null && MemofDnList.Count > 0)
        {
            retVal = OnApply_inner(AddedGroups, _dirnode, page, ADDING);
            if (!retVal)
            {
                return retVal;
            }
            retVal = OnApply_inner(RemovedGroups, _dirnode, page, REMOVING);
            if (AddedGroups != null && AddedGroups.Count > 0)
            {
                AddedGroups.Clear();
            }
            if (RemovedGroups != null && RemovedGroups.Count > 0)
            {
                RemovedGroups.Clear();
            }
        }
        return retVal;
    }
    #endregion

}
}
