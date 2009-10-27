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
using System.DirectoryServices;
using System.IO;
using Likewise.LMC.LDAP;
using Likewise.LMC.LDAP.Interop;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Krb5;
using Likewise.LMC.AuthUtils;

namespace Likewise.LMC
{
    public partial class ObjectsListPage : Form
    {
        #region Class Data

        private DirectoryContext dirContext = null;
        private List<ListViewItem> lvItems = new List<ListViewItem>();
        private List<LACTreeNode> Items = null;
        private IPlugIn plugin = null;
        private IPlugInContainer container = null;

        private string sDistinguishedName = null;
        private string sGPODisplayName = null;
        private string sNewGPODN = string.Empty;
        private bool bIsLabelEdited = false;
        private bool bIsRenameEdited = false;
        private bool bCreateGPO = false;
        private bool bRenameGPO = false;
        private string sParentDN = string.Empty;

        private List<string> navigationHistory = new List<string>();
        private int navigationPosition = 0;

        #endregion

        #region Constructors

        public ObjectsListPage()
        {
            InitializeComponent();
        }

        #endregion        

        #region Helper functions

        public void SetData(DirectoryContext dirContext, IPlugIn plugin, IPlugInContainer container)
        {
            this.dirContext = dirContext;
            this.plugin = plugin;
            this.container = container;

            lvDomainOUs.Items.Clear();
            cbDomainOUs.Items.Add(dirContext.DomainName);
            cbDomainOUs.SelectedIndex = 0;
            ListOUChildren(dirContext.RootDN);
            sParentDN = dirContext.RootDN;
            SetNavState(false);
            RefreshlvItems();
        }

        /// <summary>
        /// List the all children for the selected distinguished name 
        /// Adds the all children to the node
        /// </summary>
        public void ListOUChildren(string distinguishedName)
        {
            Items = new List<LACTreeNode>();            
            Logger.Log("ObjectsListPage.ListOUChildren() called", Logger.ldapLogLevel);
            int ret = -1;

            List<LdapEntry> ldapEntries = null;
            ret = dirContext.ListChildEntriesSynchronous
                                      (distinguishedName,
                                      LdapAPI.LDAPSCOPE.ONE_LEVEL,
                                      "(objectClass=organizationalUnit)",
                                      new string[] { "dummy", "objectClass", "distinguishedName", "userAccountControl", null },
                                      false,
                                      out ldapEntries);

            if (ldapEntries == null || ldapEntries.Count == 0)
            {
                if (ret == (int)Likewise.LMC.LMConsoleUtils.ErrorCodes.LDAPEnum.LDAP_SERVER_DOWN ||
                   ret == (int)Likewise.LMC.LMConsoleUtils.ErrorCodes.LDAPEnum.LDAP_CONNECT_ERROR ||
                   ret == -1)
                {
                    if (ret == -1)
                    {
                        ret = (int)ErrorCodes.LDAPEnum.LDAP_TIMEOUT;
                        Logger.LogMsgBox(ErrorCodes.LDAPString(ret));
                    }
                }               
            }   

            foreach (LdapEntry ldapNextEntry in ldapEntries)
            {
                string currentDN = ldapNextEntry.GetDN();

                if (!String.IsNullOrEmpty(currentDN))
                {
                    LACTreeNode newNode = new LACTreeNode(currentDN, Properties.Resources.addserver.ToBitmap(), null, plugin);                   
                    newNode.Tag = currentDN;

                    Logger.Log(String.Format("new Entry: {0}", currentDN), Logger.ldapLogLevel);

                    Items.Add(newNode); 
                }
            }
            DirectoryEntry rootDE = new DirectoryEntry(distinguishedName);

            if (rootDE != null)
            {
                string gpLinkEntries = null;

                if (rootDE.Properties["gpLink"].Value != null)
                    gpLinkEntries = rootDE.Properties["gpLink"].Value.ToString();

                if (gpLinkEntries != null)
                {
                    string[] gplinkTokens = gpLinkEntries.Split(']');
                    foreach (string gpLink in gplinkTokens)
                    {
                        if (gpLink.Trim().Length == 0)
                            continue;

                        string gpoLink = gpLink.Substring(1, gpLink.IndexOf(';') - 1);

                        DirectoryEntry de = new DirectoryEntry(gpoLink);

                        if (de != null && de.Parent != null && de.Properties != null)
                        {
                            if (de.Properties["distinguishedName"].Value != null)
                            {
                                string sDN = string.Empty;
                                string name = string.Empty;

                                sDN = de.Properties["distinguishedName"].Value.ToString();
                                if (de.Properties["displayName"].Value != null)
                                    name = de.Properties["displayName"].Value.ToString();

                                LACTreeNode newNode = new LACTreeNode(name, Properties.Resources.addserver.ToBitmap(), null, plugin);
                                newNode.Tag = sDN;
                                Items.Add(newNode);
                            }
                        }
                    }
                }
            }           
        }

        /// <summary>
        /// List the all children for the selected distinguished name 
        /// Adds the all children to the node
        /// </summary>
        public void ListGPOChildren(string distinguishedName)
        {
            Items = new List<LACTreeNode>();
            Logger.Log("ObjectsListPage.ListGPOChildren() called", Logger.ldapLogLevel);
            int ret = -1;

            List<LdapEntry> ldapEntries = null;
            ret = dirContext.ListChildEntriesSynchronous
                                      (distinguishedName,
                                      LdapAPI.LDAPSCOPE.ONE_LEVEL,
                                      "(objectClass=groupPolicyContainer)",
                                      new string[] { null },
                                      false,
                                      out ldapEntries);

            if (ldapEntries == null || ldapEntries.Count == 0)
            {
                if (ret == (int)Likewise.LMC.LMConsoleUtils.ErrorCodes.LDAPEnum.LDAP_SERVER_DOWN ||
                   ret == (int)Likewise.LMC.LMConsoleUtils.ErrorCodes.LDAPEnum.LDAP_CONNECT_ERROR ||
                   ret == -1)
                {
                    if (ret == -1)
                    {
                        ret = (int)ErrorCodes.LDAPEnum.LDAP_TIMEOUT;
                        Logger.LogMsgBox(ErrorCodes.LDAPString(ret));
                    }                   
                }            
            }         

            foreach (LdapEntry ldapNextEntry in ldapEntries)
            {
                string currentDN = ldapNextEntry.GetDN();

                if (!String.IsNullOrEmpty(currentDN))
                {
                    LdapValue[] values = ldapNextEntry.GetAttributeValues("distinguishedName", dirContext);
                    string sDN = "";
                    if (values != null && values.Length > 0)
                    {
                        sDN = values[values.Length - 1].stringData;
                    }

                    values = ldapNextEntry.GetAttributeValues("displayName", dirContext);
                    string name = "";
                    if (values != null && values.Length > 0)
                    {
                        name = values[values.Length - 1].stringData;
                    }

                    if (sDN != null && sDN != "" && name != null && name != "")
                    {
                        LACTreeNode newNode = new LACTreeNode(name, Properties.Resources.addserver.ToBitmap(), null, plugin);
                        newNode.Tag = sDN;
                        Items.Add(newNode);
                        Logger.Log(String.Format("new Entry: {0}", currentDN), Logger.ldapLogLevel);
                    }
                }
            } 
        }

        private void RefreshlvItems()
        {
            lvItems.Clear();
            if (Items != null && Items.Count != 0)
            {                
                foreach (LACTreeNode node in Items)
                {
                    string sName = "";
                    string[] items = null;

                    string DN = node.Tag as string;

                    if (!DN.StartsWith("DC=", StringComparison.InvariantCultureIgnoreCase))
                    {
                        string[] parts = DN.Split(',');
                        sName = parts[0];
                    }
                    DirectoryEntry de = new DirectoryEntry(DN);
                    int imageindex = 0;
                    if (de != null)
                    {
                        object[] asProp = de.Properties["objectClass"].Value as object[];
                        if (asProp != null)
                        {
                            // poke these in a list for easier reference
                            List<string> liClasses = new List<string>();
                            foreach (string s in asProp)
                                liClasses.Add(s);

                            if (liClasses.Contains("groupPolicyContainer"))
                            {
                                items = new string[] { node.Text, dirContext.DomainName };
                                imageindex = (int)Manage.ManageImageType.GPO;
                            }
                            else
                            {
                                items = new string[] { sName, "" };

                                imageindex = (int)Manage.ManageImageType.OrganizationalUnit;
                            }
                        }
                        ListViewItem lvItem = new ListViewItem(items);
                        lvItem.Tag = DN;
                        lvItem.ImageIndex = imageindex;
                        lvItems.Add(lvItem);
                        
                    }
                }
                ListViewItem[] lvItemArry = new ListViewItem[lvItems.Count];
                lvItems.CopyTo(lvItemArry);
                if (tabControl.SelectedTab == tabPageDomainOUs)
                {
                    lvDomainOUs.Items.Clear();
                    lvDomainOUs.Items.AddRange(lvItemArry);
                }
                else
                {
                    lvGPOs.Items.Clear();
                    lvGPOs.Items.AddRange(lvItemArry);
                }
            }
        }

        public bool AddNewGPO(string DN, bool bIsLinkable)
        {
            string baseDn = "CN=Policies,CN=System,";
            baseDn = string.Concat(baseDn, dirContext.RootDN);           

            List<LDAPMod> listattr = new List<LDAPMod>();          
            Guid guid = Guid.NewGuid();

            string[] attr_values = { "groupPolicyContainer", null };
            LDAPMod ouinfo_attr =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "objectClass", attr_values);
            listattr.Add(ouinfo_attr);

            attr_values = new string[] { DN, null };
            ouinfo_attr =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "displayName", attr_values);
            listattr.Add(ouinfo_attr);

            attr_values = new string[] { "0", null };
            ouinfo_attr =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "flags", attr_values);
            listattr.Add(ouinfo_attr);

            attr_values = new string[] { "2", null };
            ouinfo_attr =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "gPCFunctionalityVersion", attr_values);
            listattr.Add(ouinfo_attr);

            attr_values = new string[] { "0", null };
            ouinfo_attr =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "versionNumber", attr_values);
            listattr.Add(ouinfo_attr);

            string gPCFileSysPath = @"\\" + dirContext.DomainName + @"\SysVol\" + dirContext.DomainName + @"\Policies\" + "{" + guid.ToString().ToUpper() + "}";
            attr_values = new string[] { gPCFileSysPath, null };
            ouinfo_attr =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "gPCFileSysPath", attr_values);
            listattr.Add(ouinfo_attr);
           
            string ldapDN = "CN={" + guid.ToString() + "}," + baseDn;

            if (bIsLinkable)
            {
                lvDomainOUs.Items[lvDomainOUs.Items.Count - 1].Tag = ldapDN;
            }
            else
            {
                lvGPOs.Items[lvGPOs.Items.Count - 1].Tag = ldapDN;
            }

            LDAPMod[] gpoinfo = new LDAPMod[listattr.Count];
            listattr.CopyTo(gpoinfo);

            //Returns the ret=0 if adding new GPO is successfull
            int ret = dirContext.AddSynchronous(ldapDN.ToUpper(), gpoinfo);

            if (ret == 0)
            {
                int length = CommonResources.GetString("Caption_Console").Length + 24;
                string msgToDisplay = "New Group Policy Object is added!";
                if (length > msgToDisplay.Length)
                {
                    msgToDisplay = msgToDisplay.PadRight(length - msgToDisplay.Length, ' ');
                }
                container.ShowMessage(msgToDisplay);

                //call of CreateGPOFolderInSysVol() function to make Folder generation in SysVol
                CreateGPOFolderInSysVol(guid.ToString().ToUpper(), DN);

                listattr.Clear();
                string machineDN = string.Concat("CN=Machine,", ldapDN.ToUpper());
                attr_values = new string[] { "container", null };
                ouinfo_attr =
                        new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "objectClass", attr_values);
                listattr.Add(ouinfo_attr);
                gpoinfo = new LDAPMod[listattr.Count];
                listattr.CopyTo(gpoinfo);
                //Returns the ret=0 if adding new Machine is successfull
                ret = dirContext.AddSynchronous(machineDN, gpoinfo);

                listattr.Clear();
                string UserDN = string.Concat("CN=User,", ldapDN.ToUpper());
                attr_values = new string[] { "container", null };
                ouinfo_attr =
                        new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "objectClass", attr_values);
                listattr.Add(ouinfo_attr);
                gpoinfo = new LDAPMod[listattr.Count];
                listattr.CopyTo(gpoinfo);
                //Returns the ret=0 if adding new User is successfull
                ret = dirContext.AddSynchronous(UserDN, gpoinfo);

                #region Linking to slected OU
                if (bIsLinkable)
                {
                    DirectoryEntry rootDE = new DirectoryEntry(sParentDN);                  
                    if (rootDE != null)
                    {
                        Dictionary<string, string> gpoLinksList = new Dictionary<string, string>();
                        string gpLinkEntries = null;
                        if (rootDE.Properties["gpLink"].Value != null)
                            gpLinkEntries = rootDE.Properties["gpLink"].Value.ToString();

                        if (gpLinkEntries != null)
                        {
                            string[] gplinkTokens = gpLinkEntries.Split(']');
                            foreach (string gpLink in gplinkTokens)
                            {
                                if (gpLink.Trim().Length == 0)
                                    continue;

                                string gpoLink = gpLink.Substring(1, gpLink.IndexOf(';') - 1);

                                gpoLinksList.Add(gpoLink.Trim().ToLower(), gpLink + "]");
                            }
                        }                        
                        if (!gpoLinksList.ContainsKey(ldapDN.Trim().ToLower()))
                        {
                            string newgpoLink = "[LDAP://" + ldapDN.ToUpper() + ";1]";
                            gpoLinksList.Add(ldapDN, newgpoLink);
                        }

                        if (gpoLinksList.Count != 0)
                        {                                                   
                            string gpLink_Value = "";
                            foreach (string key in gpoLinksList.Keys)
                                gpLink_Value += gpoLinksList[key];

                            string[] gpLink_Values = new string[] { gpLink_Value, null };

                            LDAPMod gpLink_Info =
                            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "gpLink",
                                     gpLink_Values);
                            LDAPMod[] attrinfo = new LDAPMod[] { gpLink_Info };

                            ret = dirContext.ModifySynchronous(sParentDN, attrinfo);

                            if (ret == 0)
                            {
                                container.ShowMessage("Linked GPO object successful");
                                return true;
                            }
                            else
                            {
                                string sMsg = ErrorCodes.LDAPString(ret);
                                container.ShowMessage(sMsg);
                                return false;
                            }
                        }
                    }
                }
                #endregion
            }
                
            else
            {
                container.ShowMessage(LMConsoleUtils.ErrorCodes.LDAPString(ret));
            }
            return true;
        }

        /// <summary>
        /// Method to create the Folder in Sysvol for created GPO object with the corresponding guid ID
        /// </summary>
        /// <param name="sGuid"></param>
        /// <returns></returns>
        private bool CreateGPOFolderInSysVol(string sGuid, string displayname)
        {
            Logger.Log("CreateGPOFolderInSysVol", Logger.GPMCLogLevel);

            if (Configurations.currentPlatform == LikewiseTargetPlatform.UnixOrLinux ||
            Configurations.currentPlatform == LikewiseTargetPlatform.Darwin)
            {

                try
                {
                    string sTempPath = Configurations.tempDirectory;
                    string sDomainPath = Path.Combine(sTempPath, dirContext.DomainName.ToLower());
                    string sLocalMountPath = Path.Combine(sDomainPath, @"sysvol");
                    sLocalMountPath = Path.Combine(sLocalMountPath, "policies");

                    //Replace the '\' with the '\\'
                    if (sLocalMountPath.IndexOf(@"\") >= 0)
                        sLocalMountPath = sLocalMountPath.Replace(@"\", @"\\");

                    if (!Directory.Exists(sLocalMountPath))
                    {
                        Configurations.MakeDirectoryRecursive(sLocalMountPath);
                    }

                    //create the Folder with the Guid given to the Group Policy Object
                    string sGPOSysVolPath = Path.Combine(sLocalMountPath, "{" + sGuid + "}");

                    if (!Directory.Exists(sGPOSysVolPath))
                    {
                        try
                        {
                            string sMachinepath = Path.Combine(sGPOSysVolPath, "Machine");
                            string sUserPath = Path.Combine(sGPOSysVolPath, "User");
                            string sGPOGPTPath = Path.Combine(sGPOSysVolPath, "GPT.INI");

                            Configurations.MakeDirectoryRecursive(sGPOSysVolPath);
                            Configurations.MakeDirectoryRecursive(sMachinepath);
                            Configurations.MakeDirectoryRecursive(sUserPath);

                            if (!File.Exists(sGPOGPTPath))
                            {
                                string sTePath = Path.Combine(System.IO.Path.GetTempPath(), "GPT.INI");

                                FileStream fStream = new FileStream(sTePath, FileMode.OpenOrCreate, FileAccess.ReadWrite);

                                StreamWriter writer = new StreamWriter(fStream);

                                writer.WriteLine("[General]");
                                writer.WriteLine("Version=0");
                                writer.WriteLine(string.Format("displayName={0}", displayname));

                                writer.Flush();
                                writer.Close();
                                fStream.Close();

                                File.Move(sTePath, sGPOGPTPath.Replace(@"\\", @"\"));
                            }
                        }
                        catch (Exception ex)
                        {
                            Logger.LogException("objectsListPage:CreateGPOFolderInSysVol", ex);
                        }
                    }

                    sLocalMountPath = sGPOSysVolPath;

                    //Make the remote path to access the SysVol from the Server
                    string sRemoteMountPath = String.Format(@"//{0}/sysvol", dirContext.DomainControllerName);
                    sRemoteMountPath = String.Concat(sRemoteMountPath, "/", dirContext.DomainName, @"/policies");

                    CredentialEntry creds = new CredentialEntry(
                    dirContext.UserName,
                    dirContext.Password,
                    dirContext.DomainName);

                    //string changeDirectoryPath = String.Format(@"{0}\\Policies", dirContext.DomainName);
                    //SMBClient sysVolHandle = new SMBClient(creds, sLocalMountPath, sRemoteMountPath, changeDirectoryPath);
                    LwioCopy sysVolHandle = new LwioCopy(creds, sLocalMountPath, sRemoteMountPath);

                    sysVolHandle.UploadFile("{" + sGuid + "}");
                }
                catch (Exception ex)
                {
                    Logger.LogException("objectsListPage:CreateGPOFolderInSysVol", ex);
                }
            }

            return true;
        }

        /// <summary>
        /// Build Context menu for OU/GPOs in listview 
        /// </summary>
        /// <param name="node"></param>
        /// <returns></returns>
        private ContextMenu GetListViewMouseUpContextMenu(string distinguishedName)
        {
            ContextMenu cm = new ContextMenu();           

            MenuItem m_item = new MenuItem("&Delete", new EventHandler(cm_OnMenuClick));
            cm.Tag = distinguishedName;
            cm.MenuItems.Add(m_item);

            m_item = new MenuItem("&Rename", new EventHandler(cm_OnMenuClick));
            cm.Tag = distinguishedName;
            cm.MenuItems.Add(m_item);

            m_item = new MenuItem("Re&fresh", new EventHandler(cm_OnMenuClick));
            cm.Tag = distinguishedName;
            cm.MenuItems.Add(m_item);

            return cm;
        }

        /// <summary>
        /// Event raises when we click on any contextmenu item
        /// And then performs the specified action
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_OnMenuClick(object sender, EventArgs e)
        {
            // assure that the sender is a MenuItem
            MenuItem mi = sender as MenuItem;

            //Rena&me
            if (mi != null && mi.Text.Equals("&Rename"))
            {
                bIsRenameEdited = false;
                bRenameGPO = true;
                if (tabControl.SelectedTab == tabPageDomainOUs)
                {
                    if (lvDomainOUs.SelectedItems[0] != null)
                    {
                        lvDomainOUs.LabelEdit = true;
                        lvDomainOUs.SelectedItems[0].BeginEdit();
                    }
                }
                else if (tabControl.SelectedTab == tabPageAll)
                {
                    if (lvGPOs.SelectedItems[0] != null)
                    {
                        lvGPOs.LabelEdit = true;
                        lvGPOs.SelectedItems[0].BeginEdit();
                    }
                }                
            }

             //Refresh
            if (mi != null && mi.Text.Equals("Re&fresh"))
            {
                if (tabControl.SelectedTab == tabPageDomainOUs)
                {
                    ListOUChildren(sParentDN);
                    RefreshlvItems();
                }
                else if (tabControl.SelectedTab == tabPageAll)
                {
                    ListGPOChildren(sParentDN);
                    RefreshlvItems();
                }
            }

            //Delete
            if (mi != null && mi.Text.Equals("&Delete"))
            {
                if (tabControl.SelectedTab == tabPageDomainOUs)
                {
                    if (lvDomainOUs.SelectedItems.Count == 0 && lvDomainOUs.SelectedItems[0].Tag == null)
                        return;

                    ListViewItem selectedItem = lvDomainOUs.SelectedItems[0];

                    string DistinguishedName = selectedItem.Tag as string;

                    if (DistinguishedName != null)
                    {
                        string sMessage = string.Format("Are you sure wish to permanently delete {0}?", selectedItem.Text);
                        DialogResult dlg = MessageBox.Show(this,
                                                            sMessage,
                                                            CommonResources.GetString("Caption_Console"),
                                                            MessageBoxButtons.OKCancel,
                                                            MessageBoxIcon.Exclamation,
                                                            MessageBoxDefaultButton.Button1);
                        if (dlg == DialogResult.OK)
                        {
                            DeleteGPOLink(DistinguishedName);
                        }
                    } 
                }
                else if (tabControl.SelectedTab == tabPageAll)
                {
                    if (lvGPOs.SelectedItems.Count == 0 && lvGPOs.SelectedItems[0].Tag == null)
                        return;

                    ListViewItem selectedItem = lvGPOs.SelectedItems[0];
                    
                    string DistinguishedName = selectedItem.Tag as string;

                    if (DistinguishedName != null)
                    {
                        string sMessage = string.Format("Are you sure wish to permanently delete {0}?", selectedItem.Text);
                        DialogResult dlg = container.ShowMessage(
                                            sMessage, 
                                            MessageBoxButtons.OKCancel, 
                                            MessageBoxIcon.Exclamation); 

                        if (dlg == DialogResult.OK)
                        {
                            DeleteGPO(DistinguishedName);
                        }
                    }
                }
            }
        }

        private void DeleteGPO(string DistinguishedName)
        {
            List<LdapEntry> ldapEntries = new List<LdapEntry>();
            int ret = dirContext.ListChildEntriesSynchronous
                                     (dirContext.RootDN,
                                     LdapAPI.LDAPSCOPE.SUB_TREE,
                                     "(objectClass=organizationalUnit)",
                                     new string[] { "dummy", "objectClass", "distinguishedName", "userAccountControl", null },
                                     false,
                                     out ldapEntries);

            if (ldapEntries != null && ldapEntries.Count != 0)
            {
                List<LdapEntry> rootldapEntries = new List<LdapEntry>();
                ret = dirContext.ListChildEntriesSynchronous
                                       (dirContext.RootDN,
                                       LdapAPI.LDAPSCOPE.BASE,
                                       "(objectClass=*)",
                                       new string[] { "dummy", "objectClass", "distinguishedName", "userAccountControl", null },
                                       false,
                                       out rootldapEntries);

                if (rootldapEntries != null && rootldapEntries.Count != 0)
                {
                    foreach (LdapEntry ldapentry in rootldapEntries)
                        ldapEntries.Add(ldapentry);
                }

                foreach (LdapEntry ldapNextEntry in ldapEntries)
                {
                    string currentDN = ldapNextEntry.GetDN();

                    DirectoryEntry deEntry = new DirectoryEntry(currentDN);

                    if (deEntry != null)
                    {
                        string gpLinkEntries = null;

                        if (deEntry.Properties["gpLink"].Value != null)
                            gpLinkEntries = deEntry.Properties["gpLink"].Value.ToString();

                        if (gpLinkEntries != null)
                        {
                            string[] gplinkTokens = gpLinkEntries.Split(']');
                            if (gplinkTokens == null)
                                return;
                            List<string> gpoLinksList = new List<string>();
                            bool IsLinkExists = false;

                            foreach (string gpLink in gplinkTokens)
                            {
                                if (gpLink.Trim().Length == 0)
                                    continue;

                                string gpoLink = gpLink.Substring(1, gpLink.IndexOf(';') - 1);
                                string currentGPO = "LDAP://" + DistinguishedName;

                                if (gpoLink.Trim().Equals(currentGPO.Trim(), StringComparison.InvariantCultureIgnoreCase))
                                    IsLinkExists = true;
                                else
                                {
                                    gpoLinksList.Add(gpLink + "]");
                                }
                            }
                            if (IsLinkExists)
                            {
                                string gpLink_Value = "";
                                foreach (string value in gpoLinksList)
                                {
                                    gpLink_Value += value;
                                }
                                string[] gpLink_values = { gpLink_Value, null };

                                LDAPMod gpLink_Info =
                                   new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "gpLink",
                                               gpLink_values);
                                LDAPMod[] attrinfo = new LDAPMod[] { gpLink_Info };

                                ret = dirContext.ModifySynchronous(currentDN, attrinfo);
                            }
                        }
                    }
                }
            }
            ret = dirContext.DeleteChildren_Recursive(DistinguishedName);

            if (ret == 0)
            {
                string baseDn = "CN=Policies,CN=System,";
                baseDn = string.Concat(baseDn, dirContext.RootDN);
                ListGPOChildren(baseDn);
                RefreshlvItems();
            }
            else
            {
                string sMsg = ErrorCodes.LDAPString(ret);
                container.ShowMessage(sMsg);
                return;
            }
        }

        private void DeleteGPOLink(string DistinguishedName)
        {
            List<string> attrValues = new List<string>();

            DirectoryEntry parentDE = new DirectoryEntry(sParentDN);
            if (parentDE != null)
            {
                string gpLinkEntries = null;
                string[] gplinkTokens = null;

                if (parentDE.Properties["gpLink"].Value != null)
                    gpLinkEntries = parentDE.Properties["gpLink"].Value.ToString();

                if (gpLinkEntries != null)
                    gplinkTokens = gpLinkEntries.Split(']');

                if (gplinkTokens == null)
                    return;

                foreach (string gpLink in gplinkTokens)
                {
                    if (gpLink.Trim().Length == 0)
                        continue;
                    string gpoLink = gpLink.Substring(1, gpLink.IndexOf(';') - 1);
                    gpoLink = gpoLink.Substring(gpoLink.LastIndexOf('/') + 1);
                    if (!gpoLink.Trim().Equals(DistinguishedName, StringComparison.InvariantCultureIgnoreCase))
                        attrValues.Add(gpLink + "]");
                }

                string gpLink_Value = "";
                foreach (string value in attrValues)
                {
                    gpLink_Value += value;
                }

                string[] gpLink_values = null;
                if (String.IsNullOrEmpty(gpLink_Value))
                {
                    gpLink_values = new string[] { null };
                }
                else
                {
                    gpLink_values = new string[] { gpLink_Value, null };
                }

                LDAPMod gpLink_Info =
                   new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "gpLink",
                               gpLink_values);

                LDAPMod[] attrinfo = new LDAPMod[] { gpLink_Info };

                int ret = dirContext.ModifySynchronous(sParentDN, attrinfo);

                if (ret == 0)
                {
                    ListOUChildren(sParentDN);
                    RefreshlvItems();
                }
                else
                {
                    string sMsg = ErrorCodes.LDAPString(ret);
                    container.ShowMessage(sMsg);
                    return;
                }
            }
        }        

        public void SetNavState(bool bCanBack)
        {
            Logger.Log(String.Format(
                "SetNavState(canBack={0}",
                bCanBack),
                Logger.manageLogLevel);
           
            btnBack.Enabled = bCanBack;         

            for (int i = 0; i < navigationHistory.Count; i++)
            {
                if (navigationPosition == i)
                {
                    Logger.Log(String.Format(
                        "navigationHistory[{0}]: {1}  (SELECTED!)",
                        i,
                        navigationHistory[i].ToString()),
                        Logger.manageLogLevel);
                }
                else
                {
                    Logger.Log(String.Format(
                        "navigationHistory[{0}]: {1}",
                        i,
                        navigationHistory[i].ToString()),
                        Logger.manageLogLevel);
                }
            }
        }
        
        #endregion

        #region event handlers

        private void tabControl_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (tabControl.SelectedTab == tabPageDomainOUs)
            {
                cbDomainOUs.Items.Clear();
                cbDomainOUs.Items.Add(dirContext.DomainName);
                cbDomainOUs.SelectedIndex = 0;
                sParentDN = dirContext.RootDN;
                ListOUChildren(dirContext.RootDN);
            }
            else if (tabControl.SelectedTab == tabPageAll)
            {
                cbDomainAll.Items.Clear();
                cbDomainAll.Items.Add(dirContext.DomainName);
                cbDomainAll.SelectedIndex = 0;
                string baseDn = "CN=Policies,CN=System,";
                baseDn = string.Concat(baseDn, dirContext.RootDN);
                sParentDN = baseDn;
                ListGPOChildren(baseDn);
            }
            RefreshlvItems();
        }

        private void lvGPOs_DoubleClick(object sender, EventArgs e)
        {
            if (lvGPOs.SelectedItems.Count == 0)
                return;

            ListViewItem selectedItem = lvGPOs.SelectedItems[0];
            sDistinguishedName = selectedItem.Tag as string;
            sGPODisplayName = selectedItem.Text;
           
            btnOk_Click(sender, e);
        }

        private void lvDomainOUs_DoubleClick(object sender, EventArgs e)
        {
            if (lvDomainOUs.SelectedItems.Count == 0)
                return;

            ListViewItem selectedItem = lvDomainOUs.SelectedItems[0];
            sDistinguishedName = selectedItem.Tag as string;
            string sDN = sDistinguishedName.Substring(sDistinguishedName.IndexOf(',') + 1);
            
            //only modify the nav history if the selected node is newly selected
            if (!navigationHistory.Contains(sDN))
            {
                navigationHistory.Add(sDN);
                navigationPosition = navigationHistory.Count;
                SetNavState(true);
            }

            DirectoryEntry de = new DirectoryEntry(sDistinguishedName, dirContext.UserName, dirContext.Password);
            
            if (de != null)
            {
                object[] asProp = de.Properties["objectClass"].Value as object[];
                if (asProp != null)
                {
                    // poke these in a list for easier reference
                    List<string> liClasses = new List<string>();
                    foreach (string s in asProp)
                        liClasses.Add(s);

                    if (liClasses.Contains("groupPolicyContainer"))
                    {
                        sGPODisplayName = selectedItem.Text;
                        btnOk_Click(sender, e);
                    }
                    else
                    {
                        ListOUChildren(sDistinguishedName);
                        sParentDN = sDistinguishedName;
                        RefreshlvItems();
                    }
                }
            }
        }

        private void btnOk_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.OK;
            Close();            
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            Close();
        }        

        private void lvDomainOUs_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lvDomainOUs.SelectedItems.Count == 0)
                return;

            ListViewItem selectedItem = lvDomainOUs.SelectedItems[0];
            if (selectedItem.Tag == null)
                return;
            sDistinguishedName = selectedItem.Tag as string;

            DirectoryEntry de = new DirectoryEntry(sDistinguishedName);
            if (de != null)
            {
                object[] asProp = de.Properties["objectClass"].Value as object[];
                if (asProp != null)
                {
                    // poke these in a list for easier reference
                    List<string> liClasses = new List<string>();
                    foreach (string s in asProp)
                        liClasses.Add(s);

                    if (liClasses.Contains("groupPolicyContainer"))
                    {
                        sGPODisplayName = selectedItem.Text;
                        btnOk.Enabled = true;
                    }
                    else
                        btnOk.Enabled = false;
                }
            }
        }       

        private void lvGPOs_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (lvGPOs.SelectedItems.Count == 0)
                return;

            ListViewItem selectedItem = lvGPOs.SelectedItems[0];
            if (selectedItem.Tag == null)
                return;
            sDistinguishedName = selectedItem.Tag as string;
            sGPODisplayName = selectedItem.Text;

            btnOk.Enabled = true;
        }        

        private void lvDomainOUs_AfterLabelEdit(object sender, LabelEditEventArgs e)
        {
            lvDomainOUs.LabelEdit = false;

            if (bCreateGPO)
            {
                bCreateGPO = false;
                bIsLabelEdited = true;
                lvDomainOUs.Items[lvDomainOUs.Items.Count - 1].Selected = true;
            }
            else if (bRenameGPO)
            {
                bRenameGPO = false;
                bIsRenameEdited = true;
            }
        }

        private void lvDomainOUs_MouseUp(object sender, MouseEventArgs e)
        {
            ListView lvSender = sender as ListView;
            if (lvSender != null && e.Button == MouseButtons.Right)
            {
                ListViewHitTestInfo hti = lvSender.HitTest(e.X, e.Y);
                if (hti != null && hti.Item != null)
                {
                    ListViewItem lvItem = hti.Item;
                    ContextMenu menu = null;
                    if (!lvItem.Selected)
                    {
                        lvItem.Selected = true;
                    }
                    if (lvItem.Tag != null)
                    {
                        DirectoryEntry de = new DirectoryEntry(lvItem.Tag as string);
                        if (de != null)
                        {
                            object[] asProp = de.Properties["objectClass"].Value as object[];
                            if (asProp != null)
                            {
                                List<string> liClasses = new List<string>();
                                foreach (string s in asProp)
                                    liClasses.Add(s);

                                if (liClasses.Contains("groupPolicyContainer"))
                                    menu = GetListViewMouseUpContextMenu(lvItem.Tag as string);
                            }
                        }
                    }
                    if (menu != null)
                    {
                        menu.Show(lvSender, new Point(e.X, e.Y));
                    }
                    else
                    {
                        Logger.Log(
                            "ObjectsListPage::lvDomainOUs_MouseUp, menu == null",
                            Logger.manageLogLevel);
                    }
                }
            }
            else if (lvSender != null && e.Button == MouseButtons.Left)
            {
                if (bIsLabelEdited && lvDomainOUs.Items.Count != 0)
                {
                    if (lvDomainOUs.Items[lvDomainOUs.Items.Count - 1].Text.Trim().Equals(string.Empty))
                        lvDomainOUs.Items[lvDomainOUs.Items.Count - 1].Text = "New Group Policy Object";

                    sNewGPODN = lvDomainOUs.Items[lvDomainOUs.Items.Count - 1].Text;                    

                    if (!AddNewGPO(sNewGPODN, true))
                    {
                        lvDomainOUs.Items.RemoveAt(lvDomainOUs.Items.Count - 1);
                        return;
                    }

                    bIsLabelEdited = false;

                    if (lvDomainOUs.SelectedItems.Count == 0)
                        return;

                    ListViewItem selectedItem = lvDomainOUs.SelectedItems[0];                   
                    sDistinguishedName = selectedItem.Tag as string;
                    sGPODisplayName = selectedItem.Text;

                    btnOk.Enabled = true;                   
                }
                else if (bIsRenameEdited && lvDomainOUs.Items.Count != 0)
                {
                    if (lvDomainOUs.SelectedItems.Count == 0)
                        return;

                    if (lvDomainOUs.SelectedItems[0].Text.Trim().Equals(string.Empty))
                    {
                        lvDomainOUs.SelectedItems[0].Text = sGPODisplayName;
                        return;
                    }

                    string rename = lvDomainOUs.SelectedItems[0].Text;
                    bIsRenameEdited = false;

                    ListViewItem selectedItem = lvDomainOUs.SelectedItems[0];
                    sDistinguishedName = selectedItem.Tag as string;
                    sGPODisplayName = selectedItem.Text;

                    string[] displayname_values = new string[] { rename, null };

                    LDAPMod displayname_Info =
                       new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "displayName",
                                   displayname_values);
                    LDAPMod[] attrinfo = new LDAPMod[] { displayname_Info };

                    int ret = dirContext.ModifySynchronous(sDistinguishedName, attrinfo);

                    btnOk.Enabled = true;
                }
            }            
        }

        private void lvGPOs_MouseUp(object sender, MouseEventArgs e)
        {
            ListView lvSender = sender as ListView;
            if (lvSender != null && e.Button == MouseButtons.Right)
            {
                ListViewHitTestInfo hti = lvSender.HitTest(e.X, e.Y);
                if (hti != null && hti.Item != null)
                {
                    ListViewItem lvItem = hti.Item;
                    ContextMenu menu = null;
                    if (!lvItem.Selected)
                    {
                        lvItem.Selected = true;
                    }
                    if (lvItem.Tag != null)
                    {
                        menu = GetListViewMouseUpContextMenu(lvItem.Tag as string);
                    }
                    if (menu != null)
                    {
                        menu.Show(lvSender, new Point(e.X, e.Y));
                    }
                    else
                    {
                        Logger.Log(
                            "ObjectsListPage::lvGPOs_MouseUp, menu == null",
                            Logger.manageLogLevel);
                    }
                }
            }
            else if (lvSender != null && e.Button == MouseButtons.Left)
            {
                if (bIsLabelEdited && lvGPOs.Items.Count != 0)
                {
                    if (lvGPOs.Items[lvGPOs.Items.Count - 1].Text.Trim().Equals(string.Empty))
                        lvGPOs.Items[lvGPOs.Items.Count - 1].Text = "New Group Policy Object";

                    sNewGPODN = lvGPOs.Items[lvGPOs.Items.Count - 1].Text;
                    if (!AddNewGPO(sNewGPODN, false))
                        bIsLabelEdited = false;

                    if (lvGPOs.SelectedItems.Count == 0)
                        return;

                    ListViewItem selectedItem = lvGPOs.SelectedItems[0];
                    sDistinguishedName = selectedItem.Tag as string;
                    sGPODisplayName = selectedItem.Text;

                    btnOk.Enabled = true;  
                }
                else if (bIsRenameEdited && lvGPOs.Items.Count != 0)
                {
                    if (lvGPOs.SelectedItems.Count == 0)
                        return;

                    if (lvGPOs.SelectedItems[0].Text.Trim().Equals(string.Empty))
                    {
                        lvGPOs.SelectedItems[0].Text = sGPODisplayName;
                        return;
                    }

                    string rename = lvGPOs.SelectedItems[0].Text;
                    bIsRenameEdited = false;

                    ListViewItem selectedItem = lvGPOs.SelectedItems[0];
                    sDistinguishedName = selectedItem.Tag as string;
                    sGPODisplayName = selectedItem.Text;

                    string[] displayname_values = new string[] { rename, null };

                    LDAPMod displayname_Info =
                       new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "displayName",
                                   displayname_values);
                    LDAPMod[] attrinfo = new LDAPMod[] { displayname_Info };

                    int ret = dirContext.ModifySynchronous(sDistinguishedName, attrinfo);

                    btnOk.Enabled = true;
                }
            }
        }

        private void lvGPOs_AfterLabelEdit(object sender, LabelEditEventArgs e)
        {
            lvGPOs.LabelEdit = false;

            if (bCreateGPO)
            {
                bCreateGPO = false;
                bIsLabelEdited = true;
                lvGPOs.Items[lvGPOs.Items.Count - 1].Selected = true;             
            }
            else if (bRenameGPO)
            {
                bRenameGPO = false;
                bIsRenameEdited = true;             
            }
        }

        private void btnCreateGPO_Click(object sender, EventArgs e)
        {
            string[] items = new string[] { "New Group Policy Object", dirContext.DomainName };
            ListViewItem lvItem = new ListViewItem(items);
            lvItem.ImageIndex = (int)Manage.ManageImageType.GPO;
            lvGPOs.LabelEdit = true;
            bIsLabelEdited = false;
            bCreateGPO = true;
            lvGPOs.Items.Add(lvItem);
            lvGPOs.Items[lvGPOs.Items.Count - 1].BeginEdit();
        }

        private void btnCreateLinkGPO_Click(object sender, EventArgs e)
        {
            string[] items = new string[] { "New Group Policy Object", dirContext.DomainName };
            ListViewItem lvItem = new ListViewItem(items);
            lvItem.ImageIndex = (int)Manage.ManageImageType.GPO;
            lvDomainOUs.LabelEdit = true;
            bIsLabelEdited = false;
            bCreateGPO = true;
            lvDomainOUs.Items.Add(lvItem);
            lvDomainOUs.Items[lvDomainOUs.Items.Count - 1].BeginEdit();
        }

        private void btnBack_Click(object sender, EventArgs e)
        {
            if (navigationPosition > 0)
            {
                navigationPosition--;

                string distinguishedName = navigationHistory[navigationPosition];

                if (distinguishedName != null && distinguishedName != "")
                {
                    navigationHistory.RemoveAt(navigationPosition);
                    ListOUChildren(distinguishedName);
                    RefreshlvItems();
                }
            }
            SetNavState((navigationPosition > 0) ? true : false);
        }

        #endregion

        #region Accessors

        public string DistinguishedName
        {
            get
            {
                return sDistinguishedName;
            }
        }

        public string GPODisplayName
        {
            get
            {
                return sGPODisplayName;
            }
        }

        #endregion 
    }
}