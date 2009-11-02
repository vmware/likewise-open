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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.ServerControl;
using Likewise.LMC.LDAP;
using System.DirectoryServices;
using Likewise.LMC.NETAPI;


namespace Likewise.LMC.Plugins.ADUCPlugin
{
    public partial class GroupMembersPage : MPPage, IDirectoryPropertiesPage
    {
        #region Class Data

        ADUCPlugin _plugin;
        IPlugInContainer _container;
        List<string> OriginalObjects;
        List<string> ModifiedObjects;

        bool memListchanged = false;
        ADUCDirectoryNode _dirnode = null;

        private ListViewColumnSorter lvwColumnSorter;

        #endregion

        #region Constructors

        public GroupMembersPage(IPlugInContainer container, ADUCPlugin plugin)
        {
            this.pageID = "GroupMembersEditProperities";
            InitializeComponent();

            // Create an instance of a ListView column sorter and assign it
            // to the ListView control.
            lvwColumnSorter = new ListViewColumnSorter();
            this.MemoflistView.ListViewItemSorter = lvwColumnSorter;

            SetPageTitle("Members");
            _plugin = plugin;
            _container = container;
            OriginalObjects = new List<string>();
            ModifiedObjects = new List<string>();
        }

        #endregion

        #region Initialization Methods

        /// <summary>
        /// Queries and initializes the ldapMessage for the selected group
        /// Gets all users and groups those are members for selected group
        /// Fills the list with listview
        /// </summary>
        /// <param name="ce"></param>
        /// <param name="servername"></param>
        /// <param name="name"></param>
        /// <param name="dirnode"></param>
        public void SetData(CredentialEntry ce, string servername, string name, ADUCDirectoryNode dirnode)
        {
            Dictionary<string, string> members = UserGroupUtils.GetGroupMembers(dirnode);

            foreach (string str in members.Keys)
            {
                OriginalObjects.Add(str);
            }
            foreach (string str in members.Keys)
            {
                ModifiedObjects.Add(str.ToLower());
            }

            _dirnode = dirnode;

            MemoflistView.Items.Clear();
            //show a list of group names in the member of page
            Logger.Log("Group member contains: ");

            foreach (string sDN in members.Keys)
            {
                string[] slvItem = null;
                System.DirectoryServices.DirectoryEntry de = new System.DirectoryServices.DirectoryEntry(sDN, _dirnode.LdapContext.UserName, _dirnode.LdapContext.Password);
                if (members[sDN].Equals("foreignSecurityPrincipal"))
                {
                    byte[] objectSid = de.Properties["objectSid"].Value as byte[];
                    string Sid = UserGroupUtils.SIDtoString(objectSid);
                    string cn = UserGroupUtils.GetGroupFromForeignSecurity(Sid, dirnode.LdapContext);
                    if (cn != null)
                    {
                        slvItem = new string[] { cn, "NT AUTHORITY" };
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    slvItem = UserGroupUtils.splitDn(sDN);
                }
                ListViewItem lvItem = new ListViewItem(slvItem);
                lvItem.ImageIndex = MemOfPages.GetIndexForADObject(de);
                MemoflistView.Items.Add(lvItem);
                lvItem.Tag = sDN;
            }

            if (MemoflistView.Items.Count > 0)
            {
                MemoflistView.Items[0].Selected = true;
            }
        }

        #endregion

        #region Helper Methods


        /// <summary>
        /// Modifies the specified attributes for the selected group in AD schema template
        /// </summary>
        /// <returns></returns>
        public bool OnApply()
        {
            bool retVal = true;

            if (!compareLists(ModifiedObjects, OriginalObjects))
            {
                string AdminGroupDN = string.Concat("CN=Administrators,CN=Builtin,", _dirnode.LdapContext.RootDN);
                if (ModifiedObjects.Contains(AdminGroupDN.ToLower()))
                {
                    string userlogonName = string.Empty;
                    DirectoryEntry de = new DirectoryEntry(_dirnode.DistinguishedName, _dirnode.LdapContext.UserName, _dirnode.LdapContext.Password);
                    if (de != null && de.Properties["sAMAccountName"].Value != null)
                    {
                        userlogonName = de.Properties["sAMAccountName"].Value as string;
                    }
                    LUGAPI.NetAddGroupMember(_dirnode.LdapContext.DomainControllerName, "Administrators", userlogonName);
                }

                ModifiedObjects.Remove(AdminGroupDN.ToLower());
                string[] members_values = new string[ModifiedObjects.Count + 1];
                if (ModifiedObjects.Count > 0)
                {
                    ModifiedObjects.CopyTo(members_values);
                }
                members_values[ModifiedObjects.Count] = null;

                LDAPMod memberattr_Info =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "member",
                members_values);

                LDAPMod[] attrinfo = new LDAPMod[] { memberattr_Info };

                if (_dirnode != null)
                {
                    int ret = _dirnode.LdapContext.ModifySynchronous(_dirnode.DistinguishedName, attrinfo);

                    if (ret == 0)
                    {
                        container.ShowMessage("Group Memerbers have been modified successfully!");
                        retVal = true;
                    }
                    else
                    {
                        string sMsg = ErrorCodes.LDAPString(ret);
                        container.ShowError(sMsg);
                        retVal = false;
                    }
                }
            }
            return retVal;
        }

        private void UpdateApplyButton()
        {
            if (!memListchanged)
            {
                ParentContainer.DataChanged = false;
                ParentContainer.btnApply.Enabled = false;
            }
            else
            {
                ParentContainer.DataChanged = true;
                ParentContainer.btnApply.Enabled = true;
            }
        }

        //compare the contents of two lists, if they contain the same items, return 0
        private bool compareLists(List<string> list1, List<string> list2)
        {
            if (list1.Count != list2.Count)
            {
                return false;
            }
            foreach (string str in list1)
            {
                if (!list2.Contains(str))
                {
                    return false;

                }

            }
            return true;
        }

        /// <summary>
        /// Method to find the duplicate entry from the list
        /// </summary>
        /// <param name="item"></param>
        /// <returns></returns>
        private bool IsItemExists(string DN)
        {
            bool bIsFound = false;
            foreach (ListViewItem lvItem in MemoflistView.Items)
            {
                string Dn = lvItem.Tag as string;
                if (Dn.Trim().Equals(DN))
                {
                    bIsFound = true;
                }
            }
            return bIsFound;
        }

        private string GetgroupType()
        {
            string groupScope = string.Empty;
            string sLdapPath = string.Format("LDAP://{0}/{1}", _dirnode.LdapContext.DomainName, _dirnode.DistinguishedName);
            System.DirectoryServices.DirectoryEntry deLocal = new System.DirectoryServices.DirectoryEntry(sLdapPath, _dirnode.LdapContext.UserName, _dirnode.LdapContext.Password);
            if (deLocal.Properties["groupType"].Value != null)
            {
                groupScope = deLocal.Properties["groupType"].Value.ToString();                
            }
            return groupScope;
        }

        #endregion

        #region Events

        /// <summary>
        /// Shows the AddUserToGroup dialog with the all groups and users
        /// AddUsertoGroup.MEMBERS_PAGE parameter will tell the tree to show both Group and User
        /// On Apply will adds new entry to the "members" attribute for the selected "group"
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Addbutton_Click(object sender, EventArgs e)
        {
            // show picker
            string sLdapPath = string.Format("LDAP://{0}/{1}", _dirnode.LdapContext.DomainName, _dirnode.DistinguishedName);
            string sProtocol;
            string sServer;
            string sCNs;
            string sDCs;

            System.DirectoryServices.SDSUtils.CrackPath(sLdapPath, out sProtocol, out sServer, out sCNs, out sDCs);

            string groupScope = GetgroupType();

            System.DirectoryServices.Misc.DsPicker f = new System.DirectoryServices.Misc.DsPicker(groupScope);
            f.SetData(System.DirectoryServices.Misc.DsPicker.DialogType.SELECT_ONLY_DOMAIN_USERS_OR_GROUPS, sProtocol, sServer, sDCs, true);
            if (f.waitForm != null && f.waitForm.bIsInterrupted)
            {
                return;
            }

            if (f.ShowDialog(this) == DialogResult.OK)
            {
                if (f.ADobjectsArray != null && f.ADobjectsArray.Length != 0)
                {
                    foreach (System.DirectoryServices.Misc.ADObject ado in f.ADobjectsArray)
                    {
                        bool bIsObjectExists = false;
                        string sDN = ado.de.Properties["distinguishedName"].Value as string;
                        string[] slvItem = UserGroupUtils.splitDn(sDN);
                        if (IsItemExists(sDN))
                        {
                            string sMsg =
                            "The object " +
                            slvItem[0] +
                            " is already in the list \nand cannot be added a second time";
                            container.ShowError(sMsg);
                            bIsObjectExists = true;
                        }
                        if (sDN.Equals(_dirnode.DistinguishedName))
                        {
                            container.ShowError("A group cannot be made a member of itself.");
                            continue;
                        }
                        if (!bIsObjectExists)
                        {
                            ListViewItem lvItem = new ListViewItem(slvItem);
                            lvItem.ImageIndex = MemOfPages.GetIndexForADObject(ado.de);
                            MemoflistView.Items.Add(lvItem);
                            lvItem.Tag = sDN;

                            bool found = false;

                            foreach (string str in ModifiedObjects)
                            {
                                if (str.Equals(sDN, StringComparison.InvariantCultureIgnoreCase))
                                {
                                    found = true;
                                    break;
                                }                               
                            }
                            //do not add duplicate objects
                            if (!found)
                            {
                                ModifiedObjects.Add(sDN.ToLower());
                            }

                            memListchanged = true;
                        }
                    }
                }
            }
            else
            {
                memListchanged = false;
            }

            UpdateApplyButton();
        }

        /// <summary>
        /// Removes the selected user/group from the list
        /// On Apply will removes the selected listview item from "members" attribute for the selected "group"
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void RemoveButton_Click(object sender, EventArgs e)
        {
            if (MemoflistView.SelectedItems.Count > 0)
            {
                DialogResult dlg =
                MessageBox.Show(this, "Do you want to remove the selected member(s) from the group?",
                CommonResources.GetString("Caption_Console"), MessageBoxButtons.YesNo,
                MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button2);
                if (dlg == DialogResult.Yes)
                {
                    foreach (ListViewItem item in MemoflistView.SelectedItems)
                    {
                        string sDn = item.Tag as string;
                        string sLdapPath = string.Format("LDAP://{0}/{1}", _dirnode.LdapContext.DomainName, sDn);
                        DirectoryEntry entry = new DirectoryEntry(sLdapPath, _dirnode.LdapContext.UserName, _dirnode.LdapContext.Password);
                        if (entry == null)
                        {
                            return;
                        }
                        ADUCDirectoryNode dn = new ADUCDirectoryNode(
                        sDn,
                        _dirnode.LdapContext,                       
                        _dirnode.ObjectClass,
                        Properties.Resources.computer,
                        _dirnode.t,
                        _dirnode.Plugin,
                        _dirnode.IsDisabled);
                        string primaryDn = UserGroupUtils.GetPrimaryGroup(dn);
                        if (primaryDn != null && primaryDn.Trim().Equals(_dirnode.DistinguishedName, StringComparison.InvariantCultureIgnoreCase))
                        {
                            string Msg =
                            "This is the member's primary group, so the member cannot be removed. " +
                            "Go to the Member Of tab of \n" +
                            "the member's property sheet and set another group " +
                            "as primary. You can then remove the member from this group";
                            container.ShowError(Msg);
                            return;
                        }

                        ListViewItem[] lvItemArr = new ListViewItem[MemoflistView.Items.Count - 1];
                        List<ListViewItem> lvItemList = new List<ListViewItem>();
                        int i = 0;

                        foreach (ListViewItem lvitem in MemoflistView.Items)
                        {
                            if (!lvitem.Equals(item))
                            {
                                lvItemList.Add(lvitem);
                            }
                        }
                        foreach (ListViewItem lvitem in lvItemList)
                        {
                            lvItemArr[i++] = lvitem;
                        }

                        MemoflistView.Items.Clear();
                        MemoflistView.Items.AddRange(lvItemArr);

                        ModifiedObjects.Remove(item.Tag as string);
                        memListchanged = true;
                    }
                    RemoveButton.Enabled = false;
                }
            }
            else
            {
                memListchanged = false;
            }

            UpdateApplyButton();
        }

        private void MemoflistView_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (MemoflistView.SelectedItems.Count > 0)
            {
                RemoveButton.Enabled = true;
            }
            else
            {
                RemoveButton.Enabled = false;
            }
        }

        private void MemoflistView_ColumnClick(object sender, ColumnClickEventArgs e)
        {
            // Determine if clicked column is already the column that is being sorted.
            if (e.Column == lvwColumnSorter.SortColumn)
            {
                // Reverse the current sort direction for this column.
                if (lvwColumnSorter.Order == SortOrder.Ascending)
                {
                    lvwColumnSorter.Order = SortOrder.Descending;
                }
                else
                {
                    lvwColumnSorter.Order = SortOrder.Ascending;
                }
            }
            else
            {
                // Set the column number that is to be sorted; default to ascending.
                lvwColumnSorter.SortColumn = e.Column;
                lvwColumnSorter.Order = SortOrder.Ascending;
            }

            // Perform the sort with these new sort options.
            this.MemoflistView.Sort();
        }

        private void MemoflistView_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            if (MemoflistView.SelectedItems.Count > 0)
            {
                ListViewItem selectedItem = MemoflistView.SelectedItems[0];
                if (selectedItem.Tag != null)
                {
                    string distinguishedName = selectedItem.Tag.ToString();
                    if (distinguishedName.Equals(_dirnode.DistinguishedName, StringComparison.InvariantCultureIgnoreCase))
                    {
                        return;
                    }
                    ADUCPlugin plugin = _dirnode.Plugin as ADUCPlugin;
                    if (plugin.Propertywindowhandles.ContainsKey(distinguishedName))
                    {
                        Form f = plugin.Propertywindowhandles[distinguishedName] as Form;
                        f.BringToFront();
                        return;
                    }
                    string sLDAPPath = string.Format("LDAP://{0}/{1}", _dirnode.LdapContext.DomainName, distinguishedName);
                    DirectoryEntry entry = new DirectoryEntry(sLDAPPath, _dirnode.LdapContext.UserName, _dirnode.LdapContext.Password);
                    if (entry == null)
                    {
                        return;
                    }

                    object[] asProp = entry.Properties["objectClass"].Value as object[];

                    // poke these in a list for easier reference
                    List<string> liClasses = new List<string>();
                    foreach (string s in asProp)
                    {
                        liClasses.Add(s);
                    }

                    if (liClasses.Contains("computer"))
                    {
                        ADUCDirectoryNode dirnode = new ADUCDirectoryNode(distinguishedName,
                        _dirnode.LdapContext, "computer",
                        Properties.Resources.computer, _dirnode.t, _dirnode.Plugin, _dirnode.IsDisabled);
                        ADComputerPropertiesDlg f = new ADComputerPropertiesDlg(base.container, _dirnode.PluginPage as ADUCPage, _plugin);
                        if (_dirnode.LdapContext.SchemaCache != null)
                        {
                            f.SetData(_plugin.HostInfo.creds, _plugin.HostInfo.hostName, dirnode.Text.Substring(3), dirnode, _dirnode.LdapContext.SchemaCache);
                            f.Show();
                        }
                    }
                    else if (liClasses.Contains("group"))
                    {
                        ADUCDirectoryNode dirnode = new ADUCDirectoryNode(distinguishedName,
                        _dirnode.LdapContext, "group",
                        Properties.Resources.Group_48, _dirnode.t, _dirnode.Plugin, _dirnode.IsDisabled);
                        ADGroupPropertiesDlg f = new ADGroupPropertiesDlg(base.container, _dirnode.PluginPage as ADUCPage, _plugin, dirnode);
                        if (_dirnode.LdapContext.SchemaCache != null)
                        {
                            f.SetData(_plugin.HostInfo.creds, _plugin.HostInfo.hostName, dirnode.Text.Substring(3), dirnode, _dirnode.LdapContext.SchemaCache);
                            f.Show();
                        }
                    }
                    else if (liClasses.Contains("user"))
                    {
                        ADUCDirectoryNode dirnode = new ADUCDirectoryNode(distinguishedName,
                        _dirnode.LdapContext, "user",
                        Properties.Resources.User, _dirnode.t, _dirnode.Plugin, _dirnode.IsDisabled);

                        List<object> dirnodes = new List<object>();
                        dirnodes.Add(dirnode);
                        ADUserPropertiesDlg f = new ADUserPropertiesDlg(base.container, _dirnode.PluginPage as ADUCPage, _plugin, dirnodes);
                        if (_dirnode.LdapContext.SchemaCache != null)
                        {
                            f.SetData(_plugin.HostInfo.creds, _plugin.HostInfo.hostName, dirnode.Text.Substring(3), dirnode, _dirnode.LdapContext.SchemaCache);
                            f.Show();
                        }
                    }
                    else if (liClasses.Contains("foreignSecurityPrincipal"))
                    {
                        ADUCDirectoryNode dirnode = new ADUCDirectoryNode(distinguishedName,
                        _dirnode.LdapContext, "foreignSecurityPrincipal",
                        Properties.Resources.Group_48, _dirnode.t, _dirnode.Plugin, _dirnode.IsDisabled);
                        ADGroupPropertiesDlg f = new ADGroupPropertiesDlg(base.container, _dirnode.PluginPage as ADUCPage, _plugin, dirnode);
                        if (_dirnode.LdapContext.SchemaCache != null)
                        {
                            f.SetData(_plugin.HostInfo.creds, _plugin.HostInfo.hostName, dirnode.Text.Substring(3), dirnode, _dirnode.LdapContext.SchemaCache);
                            f.Show();
                        }
                    }
                }
            }
        }

        #endregion
    }
}
