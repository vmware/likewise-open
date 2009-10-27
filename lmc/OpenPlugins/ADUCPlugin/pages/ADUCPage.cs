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
using System.Drawing;
using System.Windows.Forms;
using System.Diagnostics;
using System.Collections;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.LDAP;
using Likewise.LMC.Plugins.ADUCPlugin.Properties;
using Likewise.LMC.ServerControl;
using System.Collections.Generic;
using System.DirectoryServices;
using Likewise.LMC.NETAPI;
using System.Threading;
using System.Text;
using System.ComponentModel;
using Likewise.LMC.AuthUtils;

namespace Likewise.LMC.Plugins.ADUCPlugin
{

public partial class ADUCPage : StandardPage
{
    #region Class Data
    public static bool promptCredDlg = true;
    public static bool IsMultiselect = false;
    private const string CN_PREFIX = "cn=";
    private const string OU_PREFIX = "ou=";
    private const string DN_SEPARATOR = ",";
    private const string CN_COMPUTER = "cn=computer,";
    
    private ListViewColumnSorter lvwColumnSorter;
    
    private ADUCDirectoryNode[] ChildNodes = null;
    public int Count = 0;
    public static string ADObjectType = string.Empty;
       
    #endregion
    
    #region Constructors
    
    public ADUCPage()
    {
        InitializeComponent();
        // Create an instance of a ListView column sorter and assign it
        // to the ListView control.
        lvChildNodes.Scrollable = Configurations.useListScrolling;        
        lvwColumnSorter = new ListViewColumnSorter();
        this.lvChildNodes.ListViewItemSorter = lvwColumnSorter;
        lvwColumnSorter.Order = SortOrder.Ascending;
    }
    
    #endregion
    
    #region overrides
    
    /// <summary>
    /// Sets the Plugin information to base
    /// List the all child nodes, then refreshes the listview with the all child treenodes.
    /// </summary>
    /// <param name="ccontainer"></param>
    /// <param name="pi"></param>
    /// <param name="treeNode"></param>
    /// <param name="lmctreeview"></param>
    public override void SetPlugInInfo(IPlugInContainer ccontainer, IPlugIn pi, LACTreeNode treeNode, LWTreeView lmctreeview, CServerControl sc)
    {
        
        base.SetPlugInInfo(ccontainer, pi, treeNode, lmctreeview, sc);
        bEnableActionMenu = false;
        ADUCDirectoryNode node = treeNode as ADUCDirectoryNode;
        
        ADUCPlugin plugin = pi as ADUCPlugin;
        
        plugin.aducPagelvChildNodes = lvChildNodes;
        
        ctx = (IContext)plugin.HostInfo;
        
            /*
            if (plugin.currentDomain != null && plugin.currentDomain.adContext != null)
            {
                plugin.currentDomain.adContext.lmctreeView = lmctreeview;
                plugin.currentDomain.adContext.lvChildNodes = lvChildNodes;
            }
            */
        
        if (node != null)
        {
            SetCaption(node.DistinguishedName);
            
            if (!node.haveRetrievedChildren || node.IsModified)
            {
                if (Configurations.useWaitForm)
                {                   
                    try
                    {
                        backgroundWorker.RunWorkerAsync(node);
                        
                        if (ChildNodes != null)
                        {
                            node.Nodes.AddRange(ChildNodes);
                        }
                    }
                    catch (Exception ex)
                    {
                        Logger.LogException("ADUCPage.SetPluginInfo", ex);
                    }
                }
                else
                {
                    ChildNodes = node.ListChildren(node);
                    if (ChildNodes != null)
                    {
                        node.Nodes.AddRange(ChildNodes);
                    }
                }
            }            
            RefreshlvChildNodes(node);
        }
        else
        {
            RefreshlvChildNodes(treeNode);
            string sADUC = "Active Directory Users && Computers";
            SetCaption(sADUC);
        }
    }
    
    /// <summary>
    /// Method to change the listview view style.
    /// </summary>
    public override void Refresh()
    {
        Logger.Log("ADUCPage.Refresh");
        
        //don't actually refresh if the only thing that has changed is the view style.
        if (currentViewStyleChanged)
        {
            switch (currentViewStyle)
            {
                case ViewStyle.LARGE_ICONS:
                lvChildNodes.View = View.LargeIcon;
                break;
                case ViewStyle.SMALL_ICONS:
                lvChildNodes.View = View.SmallIcon;
                break;
                case ViewStyle.LIST_VIEW:
                lvChildNodes.View = View.List;
                break;
                case ViewStyle.DETAIL_VIEW:
                lvChildNodes.View = View.Details;
                break;
            }
            currentViewStyleChanged = false;
        }
        else if (treeNode is ADUCDirectoryNode)
        {
            Logger.Log("ADUCPage.Refresh: refreshing directory node");
            
            ADUCDirectoryNode dn = treeNode as ADUCDirectoryNode;
            if (dn != null)
            {
                dn.Refresh();
                RefreshlvChildNodes(dn);
            }
        }
        else
        {
            base.Refresh();
        }
    }
    
    #endregion
    
    #region events
    
    /// <summary>
    /// Get initializes the contextmenu for selected treenode on right click of mouse
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void lvChildNodes_MouseUp(object sender, MouseEventArgs e)
    {
        try
        {
            ListView lvSender = sender as ListView;
            if (lvSender != null && e.Button == MouseButtons.Right)
            {
                ListViewHitTestInfo hti = lvSender.HitTest(e.X, e.Y);
                if (hti != null && hti.Item != null)
                {
                    ListViewItem lvItem = hti.Item;

                    if (!lvItem.Selected)
                    {
                        lvItem.Selected = true;
                    }
                    if (lvItem.Tag != null)
                    {
                        if (lvItem.Tag is ADUCDirectoryNode)
                        {
                            ADUCDirectoryNode dirnode = lvItem.Tag as ADUCDirectoryNode;

                            if (dirnode != null)
                            {
                                ContextMenu menu = GetTreeContextMenu_curr(dirnode);
                                if (menu != null)
                                {
                                    menu.Show(lvSender, new Point(e.X, e.Y));
                                }
                                else
                                {
                                    Logger.Log(
                                    "ADUCPage::lvChildNodes_MouseUp, menu == null",
                                    Logger.ldapLogLevel);
                                }
                            }
                        }
                        else if(lvItem.Tag is LACTreeNode)
                        {
                            LACTreeNode lacNode = lvItem.Tag as LACTreeNode;
                            if (lacNode != null)
                            {
                                if (treeNode.TreeView == null)
                                {
                                    return;
                                }

                                ContextMenu menu = lacNode.Plugin.GetTreeContextMenu(lacNode);

                                if (menu != null)
                                {
                                    menu.Show(lvSender, new Point(e.X, e.Y));
                                }
                                else
                                {
                                    Logger.Log(
                                    "ADUCPage::lvChildNodes_MouseUp, menu == null",
                                    Logger.ldapLogLevel);
                                }
                            }
                        }                       
                    }
                }
                else
                {
                    ContextMenu menu = null;
                    if (treeNode != null && treeNode is ADUCDirectoryNode)
                    {
                        menu = GetTreeContextMenu_curr(treeNode as ADUCDirectoryNode);
                    }
                    else
                    {
                        menu = GetListViewWhitespaceMenu();
                    }
                    if (menu != null)
                    {
                        menu.Show(lvSender, new Point(e.X, e.Y));
                    }
                }
            }
        }

        catch (Exception ex)
        {
            MessageBox.Show(ex.Message.ToString());
        }
    }
    
    /// <summary>
    /// Expands the selected node with all its chidren on mouse left click
    /// Else gets the contextmenu and initializes the menu items for selected object
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void lvChildNodes_MouseDoubleClick(object sender, MouseEventArgs e)
    {
        if (lvChildNodes.SelectedItems.Count == 0)
            return;

        ListView lvSender = sender as ListView;
        if (lvSender != null)
        {
            ListViewHitTestInfo hti = lvSender.HitTest(e.X, e.Y);
            if (hti != null && hti.Item != null)
            {
                ListViewItem lvItem = hti.Item;
                
                if (!lvItem.Selected)
                {
                    lvItem.Selected = true;
                }
                
                if (lvItem.Tag != null)
                {
                    if (e.Button == MouseButtons.Right)
                    {
                        MouseEventContextMenu(lvSender, e);
                    }
                    else if (e.Button == MouseButtons.Left)
                    {
                        if (lvItem.Tag is ADUCDirectoryNode)
                        {
                            ADUCDirectoryNode dirnode = lvItem.Tag as ADUCDirectoryNode;
                            if (dirnode != null)
                            {
                                if (Configurations.useWaitForm)
                                {
                                    try
                                    {
                                        if (!dirnode.haveRetrievedChildren || dirnode.IsModified)
                                        { 
                                            ChildNodes = dirnode.ListChildren(dirnode);
                                        }
                                        if (ChildNodes != null)
                                        {
                                            dirnode.Nodes.AddRange(ChildNodes);
                                        }                                        
                                    }
                                    catch (Exception ex)
                                    {
                                        Logger.LogException("ADUCPage.MouseDoubleClick", ex);
                                    }
                                }
                                else
                                {
                                    ChildNodes = dirnode.ListChildren(dirnode);
                                    if (ChildNodes != null)
                                    {
                                        dirnode.Nodes.AddRange(ChildNodes);
                                    }
                                }                                
                               
                                if (treeNode.TreeView == null)
                                {
                                    return;
                                }
                                treeNode.TreeView.SelectedNode = dirnode;
                                treeNode = dirnode;
                                
                                if (dirnode.Nodes.Count > 0)
                                {
                                    dirnode.Expand();
                                    Refresh();
                                }
                                else
                                {
                                    DoPropertyPagesWork(dirnode);
                                }
                            }
                        }
                        else if (lvItem.Tag is LACTreeNode)
                        {
                            LACTreeNode lacNode = lvItem.Tag as LACTreeNode;
                            if (lacNode != null)
                            {
                                if (treeNode.TreeView == null)
                                {
                                    return;
                                }
                                treeNode.TreeView.SelectedNode = lacNode;
                                treeNode = lacNode;

                                if (lacNode.Nodes.Count > 0)
                                {                                    
                                    lacNode.sc.ShowControl(lacNode);                               
                                }
                                else
                                {
                                    ContextMenu cm = lacNode.Plugin.GetTreeContextMenu(lacNode);

                                    if (cm != null)
                                    {
                                        cm.Show(lvChildNodes, new Point(e.X, e.Y));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }   
    
    private void lvChildNodes_ColumnClick(object sender, ColumnClickEventArgs e)
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
        this.lvChildNodes.Sort();
    }
    
    
    #endregion
    
    #region context menus
    
    /// <summary>
    /// Method to call building contextmenus for each selected AD Objects
    /// </summary>
    /// <returns></returns>
    public ContextMenu GetTreeContextMenu()
    {
        if (lvChildNodes.SelectedItems.Count > 1)
        {
            bool bIsMultiUser = false;
            bool bIsMultiGroup = false;
            bool bIsMultiComp = false;
            bool bIsOthers = false;
            bool bIsServiceConnectionPoint = false;
            foreach (ListViewItem item in lvChildNodes.SelectedItems)
            {
                ADUCDirectoryNode dn = item.Tag as ADUCDirectoryNode;
                if (dn != null)
                {
                    if (dn.ObjectClass.Trim().Equals("user", StringComparison.InvariantCultureIgnoreCase))
                    {
                        bIsMultiUser = true;
                    }
                    else if (dn.ObjectClass.Trim().Equals("group", StringComparison.InvariantCultureIgnoreCase))
                    {
                        bIsMultiGroup = true;
                    }
                    else if (dn.ObjectClass.Trim().Equals("computer", StringComparison.InvariantCultureIgnoreCase))
                    {
                        bIsMultiComp = true;
                    }
                    else if (dn.ObjectClass.Trim().Equals("serviceConnectionPoint", StringComparison.InvariantCultureIgnoreCase))
                    {
                        bIsServiceConnectionPoint = true;
                    }
                    else
                    {
                        bIsOthers = true;
                        bIsMultiUser = bIsMultiGroup = bIsMultiComp = bIsServiceConnectionPoint = false;
                        break;
                    }
                }
            }

            if (bIsMultiUser && bIsMultiGroup && bIsMultiComp && !bIsOthers)
            {
                return buildMultiGroupContextMenu();
            }
            if (bIsMultiUser && !bIsOthers)
            {
                if (bIsMultiUser && bIsMultiComp)
                {
                    return buildMultiComputerContextMenu();
                }
                else if (bIsMultiUser && bIsMultiGroup)
                {
                    return buildMultiGroupContextMenu();
                }
                else
                    return buildMultiUserContextMenu();
            }
            if (bIsMultiGroup && !bIsOthers)
            {
                return buildMultiGroupContextMenu();
            }
            if (bIsMultiComp && !bIsOthers)
            {
                return buildMultiComputerContextMenu();
            }
            if (bIsServiceConnectionPoint && !bIsOthers)
            {
                return buildMultiGroupContextMenu();
            }
            if (bIsOthers)
            {                
                return buildMultiGroupContextMenu();
            }
        }
        else
        {
            ADUCDirectoryNode dirnode = TreeNode as ADUCDirectoryNode;

            if (dirnode != null)
            {
                string obj_type = dirnode.ObjectClass;

                if (dirnode.DistinguishedName == dirnode.LdapContext.RootDN)
                {
                    return buildDomainContextMenu(dirnode);
                }
                else
                {
                    if ((obj_type.Equals("user", StringComparison.InvariantCultureIgnoreCase)
                    || obj_type.Equals("top", StringComparison.InvariantCultureIgnoreCase)))
                    {
                        if (dirnode.IsDisabled)
                        {
                            return buildUserContextMenu(dirnode, "Enable Account");
                        }
                        else
                        {
                            return buildUserContextMenu(dirnode, "Disable Account");
                        }
                    }

                    else if (obj_type.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
                    {
                        if (dirnode.IsDisabled)
                        {
                            return buildComputerContextMenu(dirnode, "Enable Account");
                        }
                        else
                        {
                            return buildComputerContextMenu(dirnode, "Disable Account");
                        }
                    }

                    else if ((obj_type.Equals("group", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("serviceConnectionPoint", StringComparison.InvariantCultureIgnoreCase)))
                    {
                        return buildGroupContextMenu(dirnode);
                    }

                    else if (obj_type.Equals("organizationalUnit", StringComparison.InvariantCultureIgnoreCase))
                    {
                        return buildOUContextMenu(dirnode);
                    }                   

                    else if ((obj_type.Equals("container", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("lostAndFound", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("builtinDomain", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("rpcContainer", StringComparison.InvariantCultureIgnoreCase)))
                    {
                        return buildContainerContextMenu(dirnode);
                    }

                    else if ((obj_type.Equals("classSchema", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("subSchema", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("attributeSchema", StringComparison.InvariantCultureIgnoreCase)))
                    {
                        return buildSchemaAttrContextMenu(dirnode);
                    }

                    else if ((obj_type.Equals("configuration", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("dMD", StringComparison.InvariantCultureIgnoreCase)))
                    {
                        return buildconfigurationdMDContextMenu(dirnode);
                    }

                    else if ((obj_type.Equals("crossRefContainer", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("physicalLocation", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("sitesContainer", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("msDS-QuotaContainer", StringComparison.InvariantCultureIgnoreCase)))
                    {
                        return buildContainerContextMenu(dirnode);
                    }
                    else
                    {
                        return GetContextMenuForADObject(dirnode);
                    }
                }
            }
        }
        return null;
    }
    
    /// <summary>
    /// Method to call building contextmenus for each selected AD Objects
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    public ContextMenu GetTreeContextMenu_curr(ADUCDirectoryNode dirnode)
    {
        if (lvChildNodes.SelectedItems.Count > 1)
        {
            bool bIsMultiUser = false;
            bool bIsMultiGroup = false;
            bool bIsMultiComp = false;
            bool bIsOthers = false;
            bool bIsServiceConnectionPoint = false;
            foreach (ListViewItem item in lvChildNodes.SelectedItems)
            {
                ADUCDirectoryNode dn = item.Tag as ADUCDirectoryNode;
                if (dn != null)
                {
                    if (dn.ObjectClass.Trim().Equals("user", StringComparison.InvariantCultureIgnoreCase))
                    {
                        bIsMultiUser = true;
                    }
                    else if (dn.ObjectClass.Trim().Equals("group", StringComparison.InvariantCultureIgnoreCase))
                    {
                        bIsMultiGroup = true;
                    }
                    else if (dn.ObjectClass.Trim().Equals("computer", StringComparison.InvariantCultureIgnoreCase))
                    {
                        bIsMultiComp = true;
                    }
                    else if (dn.ObjectClass.Trim().Equals("serviceConnectionPoint", StringComparison.InvariantCultureIgnoreCase))
                    {
                        bIsServiceConnectionPoint = true;
                    }
                    else
                    {
                        bIsOthers = true;
                        bIsMultiUser = bIsMultiGroup = bIsMultiComp = bIsServiceConnectionPoint = false;
                        break;
                    }
                }
            }

            if (bIsMultiUser && bIsMultiGroup && bIsMultiComp && !bIsOthers)
            {
                return buildMultiGroupContextMenu();
            }

            if (bIsMultiUser && !bIsOthers)
            {
                if (bIsMultiUser && bIsMultiComp)
                {
                    return buildMultiComputerContextMenu();
                }
                else if (bIsMultiUser && bIsMultiGroup)
                {
                    return buildMultiGroupContextMenu();
                }
                else
                    return buildMultiUserContextMenu();
            }
            if (bIsMultiGroup && !bIsOthers)
            {
                return buildMultiGroupContextMenu();
            }
            if (bIsMultiComp && !bIsOthers)
            {
                return buildMultiComputerContextMenu();
            }
            if (bIsServiceConnectionPoint && !bIsOthers)
            {
                return buildMultiGroupContextMenu();
            }

            if (bIsOthers)
            {
                return buildMultiGroupContextMenu();
            }
        }
        else
        {
            if (dirnode != null)
            {
                string obj_type = dirnode.ObjectClass;

                if (dirnode.DistinguishedName == dirnode.LdapContext.RootDN)
                {
                    return buildDomainContextMenu(dirnode);
                }
                else
                {
                    if ((obj_type.Equals("user", StringComparison.InvariantCultureIgnoreCase)
                    || obj_type.Equals("top", StringComparison.InvariantCultureIgnoreCase)))
                    {
                        if (dirnode.IsDisabled)
                        {
                            return buildUserContextMenu(dirnode, "Enable Account");
                        }
                        else
                        {
                            return buildUserContextMenu(dirnode, "Disable Account");
                        }
                    }
                    //  return buildOUContextMenu(dirnode);
                    else if (obj_type.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
                    {
                        if (dirnode.IsDisabled)
                        {
                            return buildComputerContextMenu(dirnode, "Enable Account");
                        }
                        else
                        {
                            return buildComputerContextMenu(dirnode, "Disable Account");
                        }
                    }

                    else if ((obj_type.Equals("group", StringComparison.InvariantCultureIgnoreCase)) ||
                   (obj_type.Equals("serviceConnectionPoint", StringComparison.InvariantCultureIgnoreCase)))
                    {
                        return buildGroupContextMenu(dirnode);
                    }

                    else if (obj_type.Equals("organizationalUnit", StringComparison.InvariantCultureIgnoreCase))
                    {
                        return buildOUContextMenu(dirnode);
                    }

                    else if ((obj_type.Equals("container", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("lostAndFound", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("builtinDomain", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("rpcContainer", StringComparison.InvariantCultureIgnoreCase)))
                    {
                        return buildContainerContextMenu(dirnode);
                    }

                    else if ((obj_type.Equals("classSchema", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("subSchema", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("attributeSchema", StringComparison.InvariantCultureIgnoreCase)))
                   
                    {
                        return buildSchemaAttrContextMenu(dirnode);
                    }

                    else if ((obj_type.Equals("configuration", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("dMD", StringComparison.InvariantCultureIgnoreCase)))
                    {
                        return buildconfigurationdMDContextMenu(dirnode);
                    }

                    else if ((obj_type.Equals("crossRefContainer", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("physicalLocation", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("sitesContainer", StringComparison.InvariantCultureIgnoreCase)) ||
                    (obj_type.Equals("msDS-QuotaContainer", StringComparison.InvariantCultureIgnoreCase)))
                    {
                        return buildContainerContextMenu(dirnode);
                    }
                    else
                    {
                        return GetContextMenuForADObject(dirnode);
                    }
                }
            }
        }

        return null;
    }

    /// <summary>
    /// Check whether the selected AD object is from the allowedchild objects list, if So build the context menu 
    /// according to that object
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    private ContextMenu GetContextMenuForADObject(ADUCDirectoryNode dirnode)
    {
        List<LdapEntry> ldapEntries = null;
        ADUCDirectoryNode dn = dirnode.Parent as ADUCDirectoryNode;

        if (dn != null)
        {
            string[] objectClasses = null;
            string[] attrs = { "name", "allowedAttributes", "allowedChildClasses", null };
            int ret = dirnode.LdapContext.ListChildEntriesSynchronous
            (dn.DistinguishedName,
            LDAP.Interop.LdapAPI.LDAPSCOPE.BASE,
            "(objectClass=*)",
            attrs,
            false,
            out ldapEntries);

            if (ldapEntries != null && ldapEntries.Count != 0)
            {
                LdapEntry ldapNextEntry = ldapEntries[0];

                LdapValue[] ldapValues = ldapNextEntry.GetAttributeValues("allowedChildClasses", dirnode.LdapContext);
                if (ldapValues != null && ldapValues.Length > 0)
                {
                    objectClasses = new string[ldapValues.Length];
                    int index = 0;
                    foreach (LdapValue Oclass in ldapValues)
                    {
                        objectClasses[index] = Oclass.stringData;
                        index++;
                    }
                }
                if (objectClasses != null && objectClasses.Length != 0)
                {
                    foreach (string objectclass in objectClasses)
                    {
                        if (dirnode.ObjectClass.Trim().Equals(objectclass.Trim(), StringComparison.InvariantCultureIgnoreCase))
                        {
                            return buildSchemaAttrContextMenu(dirnode);
                        }
                    }
                }               
            }           
        }        

        return buildGenericContextMenu(dirnode);
    }
    

    /// <summary>
    /// Builds the contextmenu for the Multi Group objects
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    private ContextMenu buildMultiGroupContextMenu()
    {
        ContextMenu cm = new ContextMenu();

        MenuItem m_item = new MenuItem("Move...", new EventHandler(cm_OnMenuClick));
        cm.MenuItems.Add(m_item);

        m_item = new MenuItem("Delete", new EventHandler(cm_OnMenuClick));
        cm.MenuItems.Add(m_item);

        cm.MenuItems.Add(new MenuItem("-"));

        m_item = new MenuItem("Properties", new EventHandler(cm_OnMenuClick));
        cm.MenuItems.Add(m_item);

        cm.MenuItems.Add(new MenuItem("-"));

        m_item = new MenuItem("Help", new EventHandler(cm_OnMenuClick));
        cm.MenuItems.Add(m_item);

        return cm;
    }

    /// <summary>
    /// Builds the contextmenu for the Multi Group objects
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    private ContextMenu buildMultiServiceConnectionPointContextMenu()
    {
        ContextMenu cm = new ContextMenu();

        MenuItem m_item = new MenuItem("Delete", new EventHandler(cm_OnMenuClick));
        cm.MenuItems.Add(m_item);

        cm.MenuItems.Add(new MenuItem("-"));       

        m_item = new MenuItem("Help", new EventHandler(cm_OnMenuClick));
        cm.MenuItems.Add(m_item);

        return cm;
    }

    /// <summary>
    /// Builds the contextmenu with items "Disbale","Enable","Move","Delete","Properties","Help" for each selected object
    /// that will be shown when we do right mouse click
    /// </summary>
    /// <returns></returns>
    private ContextMenu buildMultiComputerContextMenu()
    {
        ContextMenu cm = new ContextMenu();

        MenuItem m_item = new MenuItem("Enable Account", new EventHandler(cm_OnMenuClick));
        cm.MenuItems.Add(m_item);

        m_item = new MenuItem("Disable Account", new EventHandler(cm_OnMenuClick));
        cm.MenuItems.Add(m_item);

        m_item = new MenuItem("Move...", new EventHandler(cm_OnMenuClick));
        cm.MenuItems.Add(m_item);

        m_item = new MenuItem("Delete", new EventHandler(cm_OnMenuClick));
        cm.MenuItems.Add(m_item);

        cm.MenuItems.Add(new MenuItem("-"));

        m_item = new MenuItem("Properties", new EventHandler(cm_OnMenuClick));
        cm.MenuItems.Add(m_item);

        cm.MenuItems.Add(new MenuItem("-"));

        m_item = new MenuItem("Help", new EventHandler(cm_OnMenuClick));
        cm.MenuItems.Add(m_item);

        return cm;
    }
    
    /// <summary>
    /// Builds genereic contextmenu with items "Refresh", "Properties" for each selected object
    /// that will be shown when we mouse right clicked on any object in left hand pane of the ADUC Plugin tree in LMC
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    private ContextMenu buildGenericContextMenu(ADUCDirectoryNode dirnode)
    {
        ContextMenu cm = new ContextMenu();
        
        MenuItem m_item = new MenuItem("Refresh", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Properties", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        return cm;
    }
    
    /// <summary>
    /// Builds the contextmenu for the computer objectclass
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    private ContextMenu buildComputerContextMenu(ADUCDirectoryNode dirnode, string enableOrdisable)
    {
        ContextMenu cm = new ContextMenu();
        MenuItem m_item;

        if (!dirnode.IsDomainController)
        {
            m_item = new MenuItem(enableOrdisable, new EventHandler(cm_OnMenuClick));
            m_item.Tag = dirnode;
            cm.MenuItems.Add(m_item);
        }
        
        //Commenting this option for 4.1 release as per Glenn Suggestion
        m_item = new MenuItem("Reset Account", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        m_item = new MenuItem("Move...", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        m_item = new MenuItem("Delete", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Refresh", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Properties", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Help", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        return cm;
    }
    
    /// <summary>
    /// Builds the contextmenu for the configuration and dMD objectclass
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    private ContextMenu buildconfigurationdMDContextMenu(ADUCDirectoryNode dirnode)
    {
        ContextMenu cm = new ContextMenu();
        
        MenuItem m_item = new MenuItem("Delete", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        m_item = new MenuItem("Rename", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Refresh", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        return cm;
    }
    
    /// <summary>
    /// Builds the contextmenu for the computer objectclass
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    private ContextMenu buildSchemaAttrContextMenu(ADUCDirectoryNode dirnode)
    {
        ContextMenu cm = new ContextMenu();
        
        MenuItem m_item = new MenuItem("Move...", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        m_item = new MenuItem("Delete", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Rename", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Properties", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        return cm;
    }
    
    /// <summary>
    /// Builds the contextmenu for the user objectclass
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    private ContextMenu buildUserContextMenu(ADUCDirectoryNode dirnode, string enableOrdisable)
    {
        ContextMenu cm = new ContextMenu();
        
        MenuItem m_item = new MenuItem("Add to Group", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        m_item = new MenuItem("Copy...", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        m_item = new MenuItem("Move...", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        m_item = new MenuItem(enableOrdisable, new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        m_item = new MenuItem("Reset Password", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        m_item = new MenuItem("Delete", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        m_item = new MenuItem("Rename", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Refresh", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Properties", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Help", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        return cm;
    }
    /// <summary>
    /// Builds the context menu for multiple items selected for User object
    /// </summary>
    /// <param name="dirnode"></param>
    /// <param name="enableOrdisable"></param>
    /// <returns></returns>
    private ContextMenu buildMultiUserContextMenu()
    {
        ContextMenu cm = new ContextMenu();
        MenuItem m_item = new MenuItem("Add to Group", new EventHandler(cm_OnMenuClick));       
        cm.MenuItems.Add(m_item);

        m_item = new MenuItem("Move...", new EventHandler(cm_OnMenuClick));      
        cm.MenuItems.Add(m_item);

        m_item = new MenuItem("Enable Account", new EventHandler(cm_OnMenuClick));       
        cm.MenuItems.Add(m_item);

        m_item = new MenuItem("Disable Account", new EventHandler(cm_OnMenuClick));        
        cm.MenuItems.Add(m_item);

        m_item = new MenuItem("Delete", new EventHandler(cm_OnMenuClick));      
        cm.MenuItems.Add(m_item);

        m_item = new MenuItem("Properties", new EventHandler(cm_OnMenuClick));
        cm.MenuItems.Add(m_item);

        cm.MenuItems.Add(new MenuItem("-"));

        m_item = new MenuItem("Help", new EventHandler(cm_OnMenuClick));       
        cm.MenuItems.Add(m_item);
        return cm;

    }
    
    
    /// <summary>
    /// Builds the contextmenu for the OU objectclass
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    private ContextMenu buildOUContextMenu(ADUCDirectoryNode dirnode)
    {
        ContextMenu cm = new ContextMenu();
        
        MenuItem m_item = new MenuItem("New", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        MenuItem newSubMenuItem = new MenuItem("User", new EventHandler(cm_OnMenuClick));
        newSubMenuItem.Tag = dirnode;
        m_item.MenuItems.Add(newSubMenuItem);
        
        newSubMenuItem = new MenuItem("Group", new EventHandler(cm_OnMenuClick));
        newSubMenuItem.Tag = dirnode;
        m_item.MenuItems.Add(newSubMenuItem);
        
        newSubMenuItem = new MenuItem("Computer", new EventHandler(cm_OnMenuClick));
        newSubMenuItem.Tag = dirnode;
        m_item.MenuItems.Add(newSubMenuItem);
        
        newSubMenuItem = new MenuItem("Organizational Unit", new EventHandler(cm_OnMenuClick));
        newSubMenuItem.Tag = dirnode;
        m_item.MenuItems.Add(newSubMenuItem);
        
        m_item.MenuItems.Add(new MenuItem("-"));
        
        newSubMenuItem = new MenuItem("Other", new EventHandler(cm_OnMenuClick));
        newSubMenuItem.Tag = dirnode;
        m_item.MenuItems.Add(newSubMenuItem);
        
        m_item = new MenuItem("Move...", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);

        if (!lvChildNodes.Focused || lvChildNodes.SelectedItems.Count != 0)
        {
            m_item = new MenuItem("Delete", new EventHandler(cm_OnMenuClick));
            m_item.Tag = dirnode;
            cm.MenuItems.Add(m_item);

            m_item = new MenuItem("Rename", new EventHandler(cm_OnMenuClick));
            m_item.Tag = dirnode;
            cm.MenuItems.Add(m_item);
        }
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Refresh", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Properties", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Help", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        return cm;
    }
    
    /// <summary>
    /// Builds the contextmenu for the Conatiner objectclass
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    private ContextMenu buildContainerContextMenu(ADUCDirectoryNode dirnode)
    {
        ContextMenu cm = new ContextMenu();
        if (dirnode != null)
        {
            string obj_type = dirnode.ObjectClass;
            
            MenuItem m_item = new MenuItem("Move...", new EventHandler(cm_OnMenuClick));
            m_item.Tag = dirnode;
            cm.MenuItems.Add(m_item);
            
            cm.MenuItems.Add(new MenuItem("-"));
            
            m_item = new MenuItem("New", new EventHandler(cm_OnMenuClick));
            m_item.Tag = dirnode;
            cm.MenuItems.Add(m_item);
            
            MenuItem newSubMenuItem = null;
            
            if ((obj_type.Equals("container", StringComparison.InvariantCultureIgnoreCase)) ||
            (obj_type.Equals("lostAndFound", StringComparison.InvariantCultureIgnoreCase)) ||
            (obj_type.Equals("builtinDomain", StringComparison.InvariantCultureIgnoreCase)) ||
            (obj_type.Equals("rpcContainer", StringComparison.InvariantCultureIgnoreCase)))
            {
                
                newSubMenuItem = new MenuItem("User", new EventHandler(cm_OnMenuClick));
                newSubMenuItem.Tag = dirnode;
                m_item.MenuItems.Add(newSubMenuItem);
                
                newSubMenuItem = new MenuItem("Group", new EventHandler(cm_OnMenuClick));
                newSubMenuItem.Tag = dirnode;
                m_item.MenuItems.Add(newSubMenuItem);
                
                newSubMenuItem = new MenuItem("Computer", new EventHandler(cm_OnMenuClick));
                newSubMenuItem.Tag = dirnode;
                m_item.MenuItems.Add(newSubMenuItem);
                
                m_item.MenuItems.Add(new MenuItem("-"));
            }
            
            newSubMenuItem = new MenuItem("Other", new EventHandler(cm_OnMenuClick));
            newSubMenuItem.Tag = dirnode;
            m_item.MenuItems.Add(newSubMenuItem);
            
            cm.MenuItems.Add(new MenuItem("-"));
            
            m_item = new MenuItem("Delete", new EventHandler(cm_OnMenuClick));
            m_item.Tag = dirnode;
            cm.MenuItems.Add(m_item);
            
            m_item = new MenuItem("Rename", new EventHandler(cm_OnMenuClick));
            m_item.Tag = dirnode;
            cm.MenuItems.Add(m_item);
            
            m_item = new MenuItem("Refresh", new EventHandler(cm_OnMenuClick));
            m_item.Tag = dirnode;
            cm.MenuItems.Add(m_item);
            
            cm.MenuItems.Add(new MenuItem("-"));
            
            m_item = new MenuItem("Properties", new EventHandler(cm_OnMenuClick));
            m_item.Tag = dirnode;
            cm.MenuItems.Add(m_item);
            
            cm.MenuItems.Add(new MenuItem("-"));
            
            m_item = new MenuItem("Help", new EventHandler(cm_OnMenuClick));
            m_item.Tag = dirnode;
            cm.MenuItems.Add(m_item);
        }
        return cm;
    }
    
    /// <summary>
    /// Builds the contextmenu for the Configuration objectclass
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    private ContextMenu buildConfigurationContextMenu(ADUCDirectoryNode dirnode)
    {
        ContextMenu cm = new ContextMenu();
        
        MenuItem m_item = new MenuItem("New", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        MenuItem newSubMenuItem = new MenuItem("Other", new EventHandler(cm_OnMenuClick));
        newSubMenuItem.Tag = dirnode;
        m_item.MenuItems.Add(newSubMenuItem);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Refresh", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Properties", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        return cm;
    }
    
    
    /// <summary>
    /// Builds the contextmenu for the domain
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    private ContextMenu buildDomainContextMenu(ADUCDirectoryNode dirnode)
    {
        ContextMenu cm = new ContextMenu();
        
        MenuItem m_item = new MenuItem("New", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        MenuItem newSubMenuItem = new MenuItem("User", new EventHandler(cm_OnMenuClick));
        newSubMenuItem.Tag = dirnode;
        m_item.MenuItems.Add(newSubMenuItem);
        
        newSubMenuItem = new MenuItem("Group", new EventHandler(cm_OnMenuClick));
        newSubMenuItem.Tag = dirnode;
        m_item.MenuItems.Add(newSubMenuItem);
        
        newSubMenuItem = new MenuItem("Computer", new EventHandler(cm_OnMenuClick));
        newSubMenuItem.Tag = dirnode;
        m_item.MenuItems.Add(newSubMenuItem);
        
        newSubMenuItem = new MenuItem("Organizational Unit", new EventHandler(cm_OnMenuClick));
        newSubMenuItem.Tag = dirnode;
        m_item.MenuItems.Add(newSubMenuItem);
        
        m_item.MenuItems.Add(new MenuItem("-"));
        
        newSubMenuItem = new MenuItem("Other", new EventHandler(cm_OnMenuClick));
        newSubMenuItem.Tag = dirnode;
        m_item.MenuItems.Add(newSubMenuItem);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Refresh", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Properties", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Help", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        return cm;
        
    }
    
    /// <summary>
    /// Builds the contextmenu for the group objectclass
    /// </summary>
    /// <param name="dirnode"></param>
    /// <returns></returns>
    private ContextMenu buildGroupContextMenu(ADUCDirectoryNode dirnode)
    {
        ContextMenu cm = new ContextMenu();
        
        MenuItem m_item = new MenuItem("Move...", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        m_item = new MenuItem("Delete", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        m_item = new MenuItem("Rename", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Refresh", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Properties", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        cm.MenuItems.Add(new MenuItem("-"));
        
        m_item = new MenuItem("Help", new EventHandler(cm_OnMenuClick));
        m_item.Tag = dirnode;
        cm.MenuItems.Add(m_item);
        
        return cm;
    }
    
    /// <summary>
    /// Method that refreshes the selected node when we click on 'refresh' menu item
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    public override void cm_OnRefresh(object sender, EventArgs e)
    {
        // assure that the sender is a MenuItem
        MenuItem mi = sender as MenuItem;
        
        //Refresh the dirnode (redo a ldap query at the current dirnode)
        if (mi != null && mi.Text.Equals("Refresh"))
        {
            ADUCDirectoryNode dirnode = treeNode as ADUCDirectoryNode;
            if (dirnode != null)
            {
                dirnode.Refresh();
                RefreshlvChildNodes(dirnode);
            }
        }
    }
    
    /// <summary>
    /// Event raises when we click on any contextmenu item
    /// And then performs the specified action
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    public void cm_OnMenuClick(object sender, EventArgs e)
    {
        // assure that the sender is a MenuItem
        MenuItem mi = sender as MenuItem;
        
        //Help
        if (mi != null && mi.Text.Equals("Help"))
        {
            ProcessStartInfo psi = new ProcessStartInfo();
            psi.UseShellExecute = true;
            psi.FileName = CommonResources.GetString("LAC_Help");
            psi.Verb = "open";
            psi.WindowStyle = ProcessWindowStyle.Normal;
            Process.Start(psi);
            return;
        }
        
        ADUCDirectoryNode dirnode = mi.Tag as ADUCDirectoryNode;
        
        if (dirnode == null && lvChildNodes.SelectedItems.Count == 1)
        {
            return;
        }
        
        //if (CheckLdapTimedOut(treeNode as DirectoryNode))
        //{
        //    return;
        //}
        
        int ret = -1;

        if (dirnode != null)
        {
            string[] attrs = { null };

            List<LdapEntry> ldapEntries = null;

            ret = dirnode.LdapContext.ListChildEntriesSynchronous(
            dirnode.DistinguishedName,
            Likewise.LMC.LDAP.Interop.LdapAPI.LDAPSCOPE.ONE_LEVEL,
            "(objectClass=*)",
            attrs,
            false,
            out ldapEntries);

            if (ret == (int)Likewise.LMC.LMConsoleUtils.ErrorCodes.LDAPEnum.LDAP_NO_SUCH_OBJECT)
            {
                return;
            }
        }
        
        //Refresh the dirnode (redo a ldap query at the current dirnode)
        if (mi != null && mi.Text.Equals("Refresh"))
        {
            if (dirnode != null)
            {
                dirnode.Refresh();
                if (dirnode.Nodes.Count == 0)
                {
                    return;
                }
                dirnode.IsModified = true;
                base.treeNode = dirnode;
            }
        }
        
        //Add New User
        if (mi != null && mi.Text.Equals("User"))
        {
            if (dirnode != null)
            {
                AddNewUser(dirnode);
            }
        }
        //Add new computer
        if (mi != null && mi.Text.Equals("Computer"))
        {
            if (dirnode != null)
            {
                AddNewComputer(dirnode, null);
            }
        }
        //Add new group
        if (mi != null && mi.Text.Equals("Group"))
        {
            if (dirnode != null)
            {
                AddNewGroup(dirnode);
            }
        }
        //Add new OU
        if (mi != null && mi.Text.Equals("Organizational Unit"))
        {
            if (dirnode != null)
            {
                AddNewOU(dirnode);
            }
        }
        
        //Add a generic Object
        if (mi != null && mi.Text.Equals("Other"))
        {
            if (dirnode != null)
            {
                AddNewObject(dirnode);
            }
        }
        
        //Delete an object (user, computer, group and OU
        if (mi != null && mi.Text.Equals("Delete"))
        {
            LACTreeNode parentNode = null;
            ret = -1;

            DialogResult dlg = container.ShowMessage(
           "Are you sure you want to delete this object(s)?",
           MessageBoxButtons.YesNo,
           MessageBoxIcon.Exclamation);

            if (dlg == DialogResult.Yes)
            {
                if (mi.Tag != null)
                {
                    parentNode = dirnode.Parent as ADUCDirectoryNode;

                    ret = DODeleteADObjects(dirnode);
                }
                else
                {
                    parentNode = (LACTreeNode)treeNode;

                    foreach (ListViewItem item in lvChildNodes.SelectedItems)
                    {
                        ADUCDirectoryNode dn = item.Tag as ADUCDirectoryNode;
                        if (dn != null)
                        {
                            ret = DODeleteADObjects(dn);
                            if (ret != 0)
                            {
                                break;
                            }
                        }
                    }
                }

                if (ret == 0)
                {
                    int length = CommonResources.GetString("Caption_Console").Length + 24;
                    string msgToDisplay = "Object(s) is deleted!";
                    if (length > msgToDisplay.Length)
                    {
                        msgToDisplay = msgToDisplay.PadRight(length - msgToDisplay.Length, ' ');
                    }
                    container.ShowError(msgToDisplay);

                    if ((parentNode != null) && (parentNode is ADUCDirectoryNode))
                    {
                        ADUCPlugin plugin = treeNode.Plugin as ADUCPlugin;
                        ADUCDirectoryNode parentdirnode = parentNode as ADUCDirectoryNode;
                        parentdirnode.Refresh();
                        parentdirnode.IsModified = true;
                        base.treeNode = parentdirnode;
                    }
                }
            }
        }
        
        //Copy ...
        if (mi != null && mi.Text.Equals("Copy..."))
        {
            if (dirnode != null)
            {
                string obj_type = dirnode.ObjectClass;

                if (obj_type.Equals("top", StringComparison.InvariantCultureIgnoreCase) ||
                obj_type.Equals("user", StringComparison.InvariantCultureIgnoreCase))
                {
                    string sText = "Copy Object - User";
                    DirectoryContext dirContext = dirnode.LdapContext;

                    try
                    {
                        //first obtain the current userAccountControl value
                        DirectoryEntry de = new DirectoryEntry(string.Format("LDAP://{0}/{1}", dirContext.DomainName, dirnode.DistinguishedName));
                        int userCtrlInt = Convert.ToInt32(de.Properties["userAccountControl"].Value.ToString());
                        long pwdLastSet = Convert.ToInt64(de.Properties["pwdLastSet"].Value.ToString());
                        string copyfrom = "Copy from: " + de.Properties["name"].Value.ToString();

                        bool bAcountDisable = false, bNeverExpiresPwd = false, bMustChangePwd = false, bUserCannotChange = false;

                        string userCtrlBinStr = UserGroupUtils.DecimalToBase(userCtrlInt, 2);

                        if (userCtrlBinStr.Length >= 10 && userCtrlBinStr[userCtrlBinStr.Length - 10] == '1'
                            && pwdLastSet == 0)
                        {
                            bMustChangePwd = true;
                        }
                        if (userCtrlBinStr.Length >= 17 && userCtrlBinStr[userCtrlBinStr.Length - 17] == '1')
                        {
                            bNeverExpiresPwd = true;
                            bMustChangePwd = false;
                        }
                        if (userCtrlBinStr.Length >= 10 && userCtrlBinStr[userCtrlBinStr.Length - 7] == '1'
                        && pwdLastSet != 0)
                        {
                            bUserCannotChange = true;
                        }
                        if (userCtrlBinStr.Length >= 10 && userCtrlBinStr[userCtrlBinStr.Length - 2] == '1')                         
                        {
                            bAcountDisable = true;
                        } 

                        //ADUserAddDlg f;
                        //ADUCPlugin plugin = dirnode.Plugin as ADUCPlugin;
                        //if (plugin != null)
                        //    f = new ADUserAddDlg(base.container, this, sText, plugin.HostInfo.domainName, bAcountDisable, bNeverExpiresPwd, bMustChangePwd, copyfrom);
                        //else
                        //    f = new ADUserAddDlg(base.container, this, sText, null, bAcountDisable, bNeverExpiresPwd, bMustChangePwd, copyfrom);

                        ADUserAddDlg f = new ADUserAddDlg(base.container, this, sText, dirnode.Parent as ADUCDirectoryNode, bAcountDisable, bNeverExpiresPwd, bMustChangePwd, bUserCannotChange, copyfrom);

                        f.ShowDialog(this);
                        //the user information is gather in f.userInfo before "finish" button is clicked
                        if (f.userInfo.commit == true)
                        {
                            Hashtable htUserInfo = new Hashtable();
                            if (f.userInfo.fName != "")
                            {
                                htUserInfo.Add("givenName", f.userInfo.fName);
                            }
                            if (f.userInfo.initials != "")
                            {
                                htUserInfo.Add("initials", f.userInfo.initials);
                            }
                            if (f.userInfo.lName != "")
                            {
                                htUserInfo.Add("sn", f.userInfo.lName);
                            }
                            if (f.userInfo.fullName != "")
                            {
                                htUserInfo.Add("cn", f.userInfo.fullName);
                                htUserInfo.Add("displayName", f.userInfo.fullName);
                                htUserInfo.Add("name", f.userInfo.fullName);
                            }
                            if (f.userInfo.logonName != "")
                            {
                                htUserInfo.Add("userPrincipalName", f.userInfo.logonName);
                            }
                            if (f.userInfo.userPrelogonname != "")
                            {
                                htUserInfo.Add("sAMAccountName", f.userInfo.userPrelogonname);
                            }
                            //use logon name to set "sAMAaccountname"
                            AddNewObj_User(dirnode.Parent as ADUCDirectoryNode, htUserInfo, false, f.userInfo.passWord, f.userInfo.bAcountDisable, f.userInfo.bNeverExpiresPwd, f.userInfo.bMustChangePwd, f.userInfo.bCannotChangePwd);
                        }
                    }
                    catch (Exception ex)
                    {
                        Logger.LogException("ADUCPage.cm_OnMenuClick", ex);
                    }
                }
            }
        }

        //Move...
        if (mi != null && mi.Text.Equals("Move..."))
        {
            ADUCDirectoryNode oldparentdirnode = null;
            ret = -1;

            ADMoveObjectPage f = new ADMoveObjectPage(base.container, this, base.pi as ADUCPlugin, lmctreeview);
            if (f.ShowDialog(this) == DialogResult.OK)
            {
                if (mi.Tag != null)
                {
                    oldparentdirnode = dirnode.Parent as ADUCDirectoryNode;
                    ret = DoMoveADObject(dirnode, f.moveInfo.newParentDn);
                }
                else
                {
                    oldparentdirnode = (ADUCDirectoryNode)treeNode;

                    foreach (ListViewItem item in lvChildNodes.SelectedItems)
                    {
                        ADUCDirectoryNode dn = item.Tag as ADUCDirectoryNode;
                        if (dn != null)
                        {
                            ret = DoMoveADObject(dn, f.moveInfo.newParentDn);
                            if (ret != 0)
                            {
                                break;
                            }
                        }
                    }
                }
                //refresh both old parent and new parent
                if (ret == 0)
                {
                    container.ShowMessage("Object(s) was moved successfully!");

                    ADUCPlugin plugin = treeNode.Plugin as ADUCPlugin;
                    ADUCDirectoryNode newParentDirnode = f.moveInfo.newParentDirnode;
                    newParentDirnode.DistinguishedName = f.moveInfo.newParentDn;

                    oldparentdirnode.Refresh();
                    RefreshModifiedNode(plugin._pluginNode, newParentDirnode);
                    oldparentdirnode.IsModified = true;
                    base.treeNode = oldparentdirnode;
                }
                else if (ret != 0)
                {
                    if (ret == 64)
                    {
                        container.ShowError(this, "The object cannot be added because the parent is not on the list of possible superirors");
                    }
                    else if (ret == 53)
                    {
                        string sMsg = string.Format("Windows cannot move object {0} because:\nIllegal modify operation. Some aspect of the modification is not permitted", dirnode.Text);
                        container.ShowError(this, sMsg);
                    }
                    else
                        container.ShowError(ErrorCodes.LDAPString(ret));
                    return;
                }
            }
        }
        
        //Rename
        if (mi != null && mi.Text.Equals("Rename"))
        {
            if (dirnode != null)
            {
                DirectoryContext dirContext = dirnode.LdapContext;
                string obj_type = dirnode.ObjectClass;

                if (obj_type.Equals("top", StringComparison.InvariantCultureIgnoreCase) ||
                    obj_type.Equals("user", StringComparison.InvariantCultureIgnoreCase))
                {
                    ADRenameUserDlg f = new ADRenameUserDlg(dirnode);
                    f.ShowDialog(this);

                    if (f.renameUserInfo.commit == true)
                    {
                        string basedn = dirnode.DistinguishedName;
                        LACTreeNode parentnode = (LACTreeNode)dirnode.Parent;

                        ADUCDirectoryNode parentdirnode = parentnode as ADUCDirectoryNode;
                        string newdn = f.renameUserInfo.fullName;

                        newdn = string.Concat(CN_PREFIX, newdn); 

                        ret = dirContext.RenameSynchronous(basedn, newdn, null);

                        if (ret == 0)
                        {
                            Hashtable htUserInfo = new Hashtable();
                            if (f.renameUserInfo.fName != "")
                            {
                                htUserInfo.Add("givenName", f.renameUserInfo.fName);
                            }
                            if (f.renameUserInfo.initials != "")
                            {
                                htUserInfo.Add("initials", f.renameUserInfo.initials);
                            }
                            if (f.renameUserInfo.lName != "")
                            {
                                htUserInfo.Add("sn", f.renameUserInfo.lName);
                            }                          
                            if (f.renameUserInfo.displayName != "")
                            {
                                htUserInfo.Add("displayName", f.renameUserInfo.displayName);
                            }
                            if (f.renameUserInfo.logonName != "")
                            {
                                htUserInfo.Add("userPrincipalName", f.renameUserInfo.logonName);
                            }
                            if (f.renameUserInfo.userPrelogonname != "")
                            {
                                htUserInfo.Add("sAMAccountName", f.renameUserInfo.userPrelogonname);
                            }

                            string cn = string.Empty;
                            string smamAccount = string.Empty;
                            List<LDAPMod> userinfo = new List<LDAPMod>();

                            foreach (string key in htUserInfo.Keys)
                            {
                                string[] objectClass_values = new string[] { htUserInfo[key].ToString(), null };

                                LDAPMod ldapMod_Info =
                                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, key, objectClass_values);

                                userinfo.Add(ldapMod_Info);
                            }

                            LDAPMod[] renameinfo = new LDAPMod[userinfo.Count];
                            userinfo.CopyTo(renameinfo);

                            string dn = string.Concat(newdn, DN_SEPARATOR);
                            string ou = parentdirnode.DistinguishedName;
                            dn = string.Concat(dn, ou);

                            ret = dirContext.ModifySynchronous(dn, renameinfo);

                            if (ret == 0)
                            {
                                int length = CommonResources.GetString("Caption_Console").Length + 24;
                                string msgToDisplay = "Object is renamed!";
                                if (length > msgToDisplay.Length)
                                {
                                    msgToDisplay = msgToDisplay.PadRight(length - msgToDisplay.Length, ' ');
                                }
                                container.ShowMessage(msgToDisplay);
                            }
                            else
                            {
                                container.ShowError(ErrorCodes.LDAPString(ret));
                                return;
                            }
                        }
                        else
                        {
                            container.ShowError(ErrorCodes.LDAPString(ret));
                            return;
                        }

                        if (parentdirnode != null)
                        {
                            parentdirnode.Refresh();
                            parentdirnode.IsModified = true;
                            base.treeNode = parentdirnode;
                        }
                    }
                }
                else
                {
                    ADRenameDlg f = new ADRenameDlg(base.container, this, dirnode.Text, dirnode.ObjectClass);
                    f.ShowDialog(this);

                    //OK clicked
                    if (f.rename != null)
                    {
                        //the following portion of code uses openldap "ldap_rename_s"
                        string basedn = dirnode.DistinguishedName;
                        LACTreeNode parentnode = (LACTreeNode)dirnode.Parent;

                        ADUCDirectoryNode parentdirnode = parentnode as ADUCDirectoryNode;
                        string newdn = f.rename;

                        //rename an object
                        if (obj_type.Equals("organizationalUnit", StringComparison.InvariantCultureIgnoreCase))
                        {
                            newdn = string.Concat(OU_PREFIX, newdn);
                        }
                        else
                        {
                            newdn = string.Concat(CN_PREFIX, newdn); //Renaming any object(user, group, computer, container etc...)
                        }

                        ret = dirContext.RenameSynchronous(basedn, newdn, null);

                        if (ret == 0)
                        {
                            int length = CommonResources.GetString("Caption_Console").Length + 24;
                            string msgToDisplay = "Object is renamed!";
                            if (length > msgToDisplay.Length)
                            {
                                msgToDisplay = msgToDisplay.PadRight(length - msgToDisplay.Length, ' ');
                            }
                            container.ShowMessage(msgToDisplay);
                        }

                        else
                        {
                            container.ShowError(ErrorCodes.LDAPString(ret));
                            return;
                        }

                        if (parentdirnode != null)
                        {
                            parentdirnode.Refresh();
                            parentdirnode.IsModified = true;
                            base.treeNode = parentdirnode;
                        }
                    }
                }//if f.name == null, there is no rename performed
            }
        }        

        //in Computer Or user
        if (mi != null && mi.Text.Equals("Disable Account"))
        {
            ret = -1;
            ADUCDirectoryNode parentNode = null;
            if (mi.Tag != null)
            {
                parentNode = dirnode.Parent as ADUCDirectoryNode;
                string obj_type = dirnode.ObjectClass;
                DialogResult dlg =
                MessageBox.Show(this, "Are you sure you want to disable this account?",
                CommonResources.GetString("Caption_Console"), MessageBoxButtons.YesNo,
                MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button2);
                if (dlg == DialogResult.Yes)
                {
                    if (obj_type.Equals("user", StringComparison.InvariantCultureIgnoreCase))
                    {
                        dlg = DialogResult.Yes;
                    }
                    if (obj_type.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
                    {
                        dlg = MessageBox.Show(this, "Disabling the computer account " + dirnode.Text + " prevents users on that computer from logging on to this domain\n " +
                                              "Do you want to disable this computer account?",
                        CommonResources.GetString("Caption_Console"), MessageBoxButtons.YesNo,
                        MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button2);
                    }
                    if (dlg == DialogResult.Yes || dlg == DialogResult.OK)
                    {                        
                        ret = DoDisableADObjects(dirnode);
                    }
                    if (ret == 0)
                    {
                        MessageBox.Show(this, string.Format("Object {0} has been disabled.", dirnode.Text),
                        CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK,
                        MessageBoxIcon.Information);
                    }
                }
            }
            else
            {
                bool bAlertMsg = true;
                parentNode = treeNode as ADUCDirectoryNode;
                foreach (ListViewItem item in lvChildNodes.SelectedItems)
                {
                    ADUCDirectoryNode dn = item.Tag as ADUCDirectoryNode;
                    if (dn != null)
                    {
                        if (!dn.IsDisabled)
                        {
                            bAlertMsg = false;
                            break;
                        }
                    }
                }
                if (bAlertMsg)
                {
                    MessageBox.Show(this, "All of the selected objects are disabled",
                    CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                    return;
                }
                foreach (ListViewItem item in lvChildNodes.SelectedItems)
                {
                    ADUCDirectoryNode dn = item.Tag as ADUCDirectoryNode;
                    if (dn != null)
                    {
                        ret = DoDisableADObjects(dn);
                        if (ret != 0)
                        {
                            return;
                        }
                    }
                }
                if (ret == 0)
                {
                    MessageBox.Show(this, string.Format("All of the selected objects have been disabled."),
                    CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                }
            }
            if (ret == 0)
            {
                if (parentNode != null)
                {
                    parentNode.Refresh();
                    parentNode.IsModified = true;
                    base.treeNode = parentNode;
                }
            }
            else if (ret != -1)
            {
                string sMsg = ErrorCodes.LDAPString(ret);
                container.ShowMessage(sMsg);
            }
        }

        //Enable Account
        if (mi != null && mi.Text.Equals("Enable Account"))
        {
            ret = -1;
            ADUCDirectoryNode parentNode = null;
            if (mi.Tag != null)
            {
                parentNode = dirnode.Parent as ADUCDirectoryNode;
                DialogResult dlg =
                MessageBox.Show(this, "Are you sure you want to enable this account?",
                CommonResources.GetString("Caption_Console"), MessageBoxButtons.YesNo,
                MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button2);
                if (dlg == DialogResult.Yes)
                {
                    ret = DoEnableAccount(dirnode);
                    if (ret == 0)
                    {
                        MessageBox.Show(this, string.Format("Object {0} has been enabled.", dirnode.Text),
                        CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK,
                        MessageBoxIcon.Information);
                    }
                }
            }
            else
            {
                bool bAlertMsg = true;
                parentNode = treeNode as ADUCDirectoryNode;
                foreach (ListViewItem item in lvChildNodes.SelectedItems)
                {
                    ADUCDirectoryNode dn = item.Tag as ADUCDirectoryNode;
                    if (dn != null)
                    {
                        if (dn.IsDisabled)
                        {
                            bAlertMsg = false;
                            break;
                        }
                    }
                }
                if (bAlertMsg)
                {
                    MessageBox.Show(this, "All of the selected objects are enabled",
                    CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                    return;
                }
                foreach (ListViewItem item in lvChildNodes.SelectedItems)
                {
                    ADUCDirectoryNode dn = item.Tag as ADUCDirectoryNode;
                    if (dn != null)
                    {
                        ret = DoEnableAccount(dn);
                        if (ret != 0)
                        {
                            return;
                        }
                    }
                }
                if (ret == 0)
                {
                    MessageBox.Show(this, string.Format("All of the selected objects have been enabled."),
                    CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                }
            }
            if (ret == 0)
            {
                if (parentNode != null)
                {
                    parentNode.Refresh();
                    parentNode.IsModified = true;
                    base.treeNode = parentNode;
                }
            }            
            else if (ret == -2)
            {
                string sMsg = string.Format("Windows cannot enable object {0} becuase\n" +
                              "Unanle to update the password. The value provided for the new passoword does not meeth\n" +
                              "length, complexity, or history requirement of the domain", dirnode.Text);
                MessageBox.Show(this, sMsg, CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else if (ret != -1)
            {
                string sMsg = ErrorCodes.LDAPString(ret);
                container.ShowMessage(sMsg);
            }
        }
        
        //in User only
        if (mi != null && mi.Text.Equals("Reset Password"))
        {
            if (dirnode != null)
            {
                DirectoryContext dirContext = dirnode.LdapContext;
                
                //first obtain the current userAccountControl value
                DirectoryEntry de = new DirectoryEntry(string.Format("LDAP://{0}/{1}", dirContext.DomainName, dirnode.DistinguishedName));
                int userCtrlInt = int.Parse(de.Properties["userAccountControl"].Value.ToString());
                int useraccountCtr_decimalVal = 512;
                bool bNeverExpiresPwd = false;
                ret = 0;

                string userCtrlBinStr = UserGroupUtils.DecimalToBase(userCtrlInt, 2);

                if ((userCtrlBinStr.Length >= 17 && userCtrlBinStr[userCtrlBinStr.Length - 17] == '1') ||
                    (userCtrlBinStr.Length >= 10 && userCtrlBinStr[userCtrlBinStr.Length - 7] == '1'))
                {
                    bNeverExpiresPwd = true;
                }                            
                
                ResetPassword f = new ResetPassword(base.container, this, bNeverExpiresPwd, dirnode.Text);
                if (f.ShowDialog(this) == DialogResult.OK)
                {
                    //first, change the password
                    //first obtain the current userAccountControl value
                    //DirectoryEntry de = new DirectoryEntry(string.Format("LDAP://{0}/{1}", dirContext.DomainName, dirnode.DistinguishedName));
                    string username = de.Properties["sAMAccountName"].Value.ToString();
                    if (String.IsNullOrEmpty(f.passwordinfo.password))
                    {
                        return;
                    }
                    bool retValue = false;
                    try
                    {
                        ADUCPlugin plugin = pi as ADUCPlugin;
                        Hostinfo hn = ctx as Hostinfo;

                        if (!plugin.bIsNetInitCalled)
                        {                            
                            plugin.bIsNetInitCalled = LUGAPI.NetInitMemory(hn.creds, hn.domainControllerName);                            
                        }
                        CredentialEntry creds = new CredentialEntry(dirContext.UserName, dirContext.Password, dirContext.DomainName);
                        retValue = !Convert.ToBoolean(LUGAPI.NetChangePassword(creds, hn.domainName, username, f.passwordinfo.password));
                    }
                    catch (Exception)
                    {
                        retValue = false;
                    }                   
                    List<LDAPMod> attrlist = new List<LDAPMod>();

                    //second, we need to set up to make sure that user shall change his password next time he logs in.
                    if (f.passwordinfo.MustChangePwNextLogon && !bNeverExpiresPwd && retValue)
                    {                       
                        //int userCtrlInt = (int)de.Properties["msDS-User-Account-Control-Computed"].Value;
                        //userCtrlInt = Convert.ToInt32(de.Properties["userAccountControl"].Value.ToString());   
                        if (userCtrlInt != 546)
                        {
                            string newUserCtrl_val = Convert.ToString(useraccountCtr_decimalVal + 8388608);

                            string[] userControl_values = { newUserCtrl_val, null };
                            LDAPMod userControl_Info =
                            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "userAccountControl", userControl_values);
                            attrlist.Add(userControl_Info);
                        }

                        string[] pwdLastSet_values = { "0", null };
                        LDAPMod pwdLastSet_attr = new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "pwdLastSet", pwdLastSet_values);
                        attrlist.Add(pwdLastSet_attr);                       
                    }                  

                    if (attrlist.Count != 0)
                    {
                        LDAPMod[] attrinfo = new LDAPMod[attrlist.Count];
                        attrlist.CopyTo(attrinfo);

                        ret = dirnode.LdapContext.ModifySynchronous(dirnode.DistinguishedName, attrinfo);                        
                    }

                    if (retValue && ret == 0)
                    {
                        int length = CommonResources.GetString("Caption_Console").Length + 28;
                        string msgToDisplay = "Password has been reset successfully.";
                        if (length > msgToDisplay.Length)
                        {
                            msgToDisplay = msgToDisplay.PadRight(length - msgToDisplay.Length, ' ');
                        }
                        container.ShowMessage(msgToDisplay);
                        return;
                    }
                    else
                    {
                        container.ShowError("Failed to reset password.");
                        return;
                    }
                }
                else
                {
                    return;
                }
            }
        }
        
        //Add to Group
        if (mi != null && mi.Text.Equals("Add to Group"))
        {
            if (dirnode == null)
            {
                dirnode = lvChildNodes.SelectedItems[0].Tag as ADUCDirectoryNode;
            }
            string sLdapPath = string.Format("LDAP://{0}/{1}", dirnode.LdapContext.DomainName, dirnode.DistinguishedName);
            string sProtocol;
            string sServer;
            string sCNs;
            string sDCs;

            System.DirectoryServices.SDSUtils.CrackPath(sLdapPath, out sProtocol, out sServer, out sCNs, out sDCs);
            System.DirectoryServices.Misc.DsPicker f = new System.DirectoryServices.Misc.DsPicker();
            f.SetData(System.DirectoryServices.Misc.DsPicker.DialogType.SELECT_GROUPS, sProtocol, sServer, sDCs, false);

            if (f.waitForm != null && f.waitForm.bIsInterrupted)
            {
                return;
            }

            if (f.ShowDialog(this) == DialogResult.OK)
            {
                List<ListViewItem> itemlist = new List<ListViewItem>();
                ADUCDirectoryNode parentNode = null;
                string sDN = f.ADobjectsArray[0].de.Properties["distinguishedName"].Value as string;

                if (mi.Tag != null)
                {
                    parentNode = dirnode.Parent as ADUCDirectoryNode;
                    ret = DoAddtoGroup(dirnode, sDN, true, f.ADobjectsArray[0].Name, ref itemlist);
                    if (ret == 0)
                    {
                        container.ShowMessage("The Add to Group operation was successfully completed");
                    }
                }
                else
                {
                    parentNode = treeNode as ADUCDirectoryNode;
                    foreach (ListViewItem item in lvChildNodes.SelectedItems)
                    {
                        ADUCDirectoryNode dn = item.Tag as ADUCDirectoryNode;
                        if (dn != null)
                        {
                            ret = DoAddtoGroup(dn, sDN, false,null, ref itemlist);
                        }
                    }
                    if (itemlist != null && itemlist.Count != 0)
                    {
                        ADAddtoGroupErrorEditor errorDailog = new ADAddtoGroupErrorEditor(itemlist);
                        errorDailog.ShowDialog(this);
                    }
                    else if (ret == 0)
                    {
                        container.ShowMessage("The Add to Group operation was successfully completed");
                    }
                }
                if (ret != 0 && ret != -1)
                {
                    string sMsg = ErrorCodes.LDAPString(ret);
                    container.ShowError(sMsg);
                    return;
                }
            }
        }
        
        //Reset Computer Account
        
        if (mi != null && mi.Text.Equals("Reset Account"))
        {
            DirectoryContext dirContext = dirnode.LdapContext;
            DirectoryEntry de = new DirectoryEntry(string.Format("LDAP://{0}/{1}", dirContext.DomainName, dirnode.DistinguishedName));
            int iPrimaryGroupID = Convert.ToInt32(de.Properties["primaryGroupID"].Value.ToString());
            //  string DCResetacc = de.Properties["cn"].Value.ToString() + "$";
            if (iPrimaryGroupID == 516)
            {
                string sMsg = string.Format(
                "Server {0} is a domain controller.You cannot reset " +
                "the password on this object.",
                dirnode.Text);
                MessageBox.Show(sMsg);
                return;
            }
            if (dirnode != null)
            {
                DialogResult dlg =
                MessageBox.Show(this, "Are you sure you want to reset this computer account?",
                CommonResources.GetString("Caption_Console"), MessageBoxButtons.YesNo,
                MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button2);
                if (dlg == DialogResult.Yes)
                {
                    string computername = de.Properties["sAMAccountName"].Value.ToString();
                    //It will give the DistinguishedName for the computer is nothing but getting 'cn' attribute
                    string compResetpawd = de.Properties["cn"].Value.ToString() + "$"; //dirnode.DistinguishedName.Split(',')[0].Substring(3) + "$";
                    try
                    {
                        ADUCPlugin plugin = pi as ADUCPlugin;
                        Hostinfo hn = ctx as Hostinfo;

                        if (!plugin.bIsNetInitCalled)
                        {
                            plugin.bIsNetInitCalled = LUGAPI.NetInitMemory(hn.creds, hn.domainControllerName);
                        }

                        bool retValue = !Convert.ToBoolean(LUGAPI.NetChangePassword(hn.creds, hn.domainName, computername, compResetpawd));
                        if (retValue)
                        {
                            string sMsg = string.Format(
                            "Account {0} was successfully reset.",
                            dirnode.Text);
                            container.ShowMessage(sMsg);
                            return;
                        }
                        else
                        {
                            string sMsg = string.Format(
                            "Resetting account for {0} was unsuccessful.",
                            dirnode.Text);
                            container.ShowMessage(sMsg);
                            return;
                        }
                    }
                    catch (Exception ex)
                    {
                        string sMsg = string.Format(
                        "Resetting account for {0} was unsuccessful. Becuase {1}",
                        dirnode.Text,
                        ex.Message);
                        container.ShowError(sMsg);
                        Logger.LogException("ADUCPage", ex);
                        return;
                    }
                }
            }
        }
		RefreshPluginPage();      
        //Properties...
        if (mi != null && mi.Text.Equals("Properties"))
        {
            //Thread newThread = new Thread(this.DoPropertyPagesWork);
            //newThread.Start(mi);
            
            //Not using threading

            if (lvChildNodes.SelectedItems.Count > 1)
            {
                ADUCPlugin plugin = treeNode.Plugin as ADUCPlugin;
                ADUCDirectoryNode dnNode = treeNode as ADUCDirectoryNode;

                List<object> dirnodes = new List<object>();
                bool bIsuserType = false;
                bool bIsgroupType = false;               
                bool bIsOUType = false;
                bool bIsOthers = false;
                ADObjectType = string.Empty;

                foreach (ListViewItem item in lvChildNodes.SelectedItems)
                {
                    ADUCDirectoryNode dn = item.Tag as ADUCDirectoryNode;
                    if (dn != null)
                    {
                        if (dn.ObjectClass.Trim().Equals("user", StringComparison.InvariantCultureIgnoreCase))
                        {
                            bIsuserType = true;
                        }
                        else if (dn.ObjectClass.Trim().Equals("group", StringComparison.InvariantCultureIgnoreCase))
                        {
                            bIsgroupType = true;
                        }
                        else if (dn.ObjectClass.Trim().Equals("organizationalUnit", StringComparison.InvariantCultureIgnoreCase))
                        {
                            bIsOUType = true;
                        }
                        else
                        {
                            bIsOthers = true;
                            ADObjectType = string.Empty;
                        }
                        dirnodes.Add(dn);
                    }
                }
                if (bIsgroupType && !bIsOthers && !bIsuserType && !bIsOUType)
                {
                    ADObjectType = "group";
                }
                else if (bIsuserType && !bIsOthers && !bIsgroupType && !bIsOUType)
                {
                    ADObjectType = "user";
                }
                else if (bIsOUType && !bIsOthers && !bIsuserType && !bIsgroupType)
                {
                    ADObjectType = "organizationalUnit";
                }              
                MultiItemPropertiesDlg propertiesDlg = new MultiItemPropertiesDlg(base.container, this, base.pi as ADUCPlugin, dirnodes);
                propertiesDlg.SetData(plugin.HostInfo.creds, plugin.HostInfo.hostName, "", dnNode, dnNode.LdapContext.SchemaCache);
                propertiesDlg.Show();
            }
            else
            {
                DoPropertyPagesWork(dirnode);
            }
        } 
    }
    
    #endregion
    
    #region helper_functions

    private int DoAddtoGroup(ADUCDirectoryNode dirnode, 
                             string sDN, 
                             bool IsSingleObject, 
                             string group,
                             ref List<ListViewItem> itemsList)
    {
        int ret = -1;
        string errorMsg = "The specified account name is already a member of the local group";
        
        string[] groupDns = UserGroupUtils.GetGroupsforUser(dirnode);

        foreach (string dn in groupDns)
        {
            string aPartName = string.Empty;                      
            if (dn.Trim().ToLower().Equals(sDN.Trim().ToLower()))
            {
                if (IsSingleObject)
                {
                    string sMsg = string.Format(
                    "Object {0} cannot be added to group {1} becuase:\n " +
                    "The specified user is already member to that group",
                    dirnode.Text,
                    group);
                    container.ShowError(sMsg);
                    return ret;
                }
                else
                {
                    string[] values = new string[] { dirnode.Text, errorMsg };
                    ListViewItem item = new ListViewItem(values);
                    itemsList.Add(item);
                    return ret;
                }
            }
        }
        string AdminGroupDN = string.Concat("CN=Administrators,CN=Builtin,", dirnode.LdapContext.RootDN);
        if (sDN.Trim().ToLower().Equals(AdminGroupDN.Trim().ToLower()))
        {
            string userlogonName = string.Empty;
            Hostinfo hn = ctx as Hostinfo;
            DirectoryEntry de = new DirectoryEntry(dirnode.DistinguishedName);

            if (de != null && de.Properties["userPrincipalName"].Value != null)
            {
                userlogonName = de.Properties["userPrincipalName"].Value as string;
            }
            LUGAPI.NetAddGroupMember(hn.creds, dirnode.LdapContext.DomainControllerName, "Administrators", userlogonName);
        }
        else
        {
            List<string> members = new List<string>();

            members = MemOfPages.GetMemberAttrofGroup(sDN, dirnode);
            members.Add(dirnode.DistinguishedName);

            string[] members_values = new string[members.Count + 1];
            members.CopyTo(members_values);
            members_values[members.Count] = null;

            LDAPMod memberattr_Info =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "member",
            members_values);

            LDAPMod[] attrinfo = new LDAPMod[] { memberattr_Info };
            ret = dirnode.LdapContext.ModifySynchronous(sDN, attrinfo);
        }

        return ret;
    }

    private int DoEnableAccount(ADUCDirectoryNode dirnode)
    {
        int ret = -1;

        if (dirnode != null && dirnode.IsDisabled)
        {
            DirectoryContext dirContext = dirnode.LdapContext;
            string obj_type = dirnode.ObjectClass;

            if (obj_type.Equals("user", StringComparison.InvariantCultureIgnoreCase)
            || obj_type.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
            {
                //first obtain the current userAccountControl value
                DirectoryEntry de = new DirectoryEntry(string.Format("LDAP://{0}/{1}", dirContext.DomainName, dirnode.DistinguishedName));
                int userCtrlInt = Convert.ToInt32(de.Properties["userAccountControl"].Value.ToString());              

                string newUserCtrl_val = Convert.ToString(userCtrlInt - 2);
                //in order to disable an user, we need to add 2 to the existing userControl value
                string[] userControl_values = { newUserCtrl_val, null };
                LDAPMod userControl_Info =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "userAccountControl",
                userControl_values);

                LDAPMod[] attrinfo = new LDAPMod[] { userControl_Info };

                ret = dirnode.LdapContext.ModifySynchronous(dirnode.DistinguishedName, attrinfo);

                return ret;
            }
        }
        else
        {
            return ret;
        }

        return ret;
    }

    private int DoDisableADObjects(ADUCDirectoryNode dirnode)
    {
        int ret = -1;
        if (dirnode != null && !dirnode.IsDisabled)
        {
            DirectoryContext dirContext = dirnode.LdapContext;
            string obj_type = dirnode.ObjectClass;

            //first obtain the current userAccountControl value
            DirectoryEntry de = new DirectoryEntry(string.Format("LDAP://{0}/{1}", dirContext.DomainName, dirnode.DistinguishedName));
            int userCtrlInt = Convert.ToInt32(de.Properties["userAccountControl"].Value.ToString());

            string newUserCtrl_val = Convert.ToString(userCtrlInt + 2);
            //in order to disable an user, we need to add 2 to the existing userControl value
            string[] userControl_values = { newUserCtrl_val, null };
            LDAPMod userControl_Info =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "userAccountControl",
            userControl_values);

            LDAPMod[] attrinfo = new LDAPMod[] { userControl_Info };

            ret = dirnode.LdapContext.ModifySynchronous(dirnode.DistinguishedName, attrinfo);
        }
        else
        {
            return ret;
        }
        return ret;
    }


    private int DODeleteADObjects(ADUCDirectoryNode dirnode)
    {
        int ret = -1;
        if (dirnode != null)
        {
            DialogResult dlg;
            string obj_type = dirnode.ObjectClass;
            DirectoryContext dirContext = dirnode.LdapContext;

            if (obj_type.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
            {
                //first obtain the current primaryGroupID value
                DirectoryEntry de = new DirectoryEntry(string.Format("LDAP://{0}/{1}", dirContext.DomainName, dirnode.DistinguishedName));
                int iPrimaryGroupID = Convert.ToInt32(de.Properties["primaryGroupID"].Value.ToString());

                if (iPrimaryGroupID == 516)
                {
                    ADDeleteComputerConfirmDialog confirmDialog = new ADDeleteComputerConfirmDialog(dirnode);
                    if (confirmDialog.ShowDialog(this) == DialogResult.OK)
                    {
                        if (confirmDialog.bRb1Checked || confirmDialog.bRb2Checked)
                        {
                            return ret;
                        }
                    }
                    else
                    {
                        return ret;
                    }
                }
            }
            else if (obj_type.Equals("organizationalUnit", StringComparison.InvariantCultureIgnoreCase))
            {                
                string Msg = "The object {0} is a conatiner and contains other objects.\n" +
                "Are you sure want to delete the object {1} and the objects it contains?\n" +
                "This operation could take a long time if {2} contains a large number of objects.";
                Msg = string.Format(Msg, dirnode.Text, dirnode.Text, dirnode.Text);

                dlg = MessageBox.Show(this, Msg,
                CommonResources.GetString("Caption_Console"), MessageBoxButtons.YesNo,
                MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button2);
                if (dlg == DialogResult.Yes)
                {
                    //first obtain the current primaryGroupID value
                    DirectoryEntry de = new DirectoryEntry(string.Format("LDAP://{0}/{1}", dirContext.DomainName, dirnode.DistinguishedName));
                    string sChilds = string.Empty;
                    foreach (DirectoryEntry dechild in de.Children)
                    {
                        if (dechild != null)
                        {
                            object[] asProp = dechild.Properties["objectClass"].Value as object[];
                            // poke these in a list for easier reference
                            List<string> liClasses = new List<string>();
                            foreach (string s in asProp)
                            {
                                liClasses.Add(s);
                            }
                            if (liClasses.Contains("computer"))
                            {
                                int iPrimaryGroupID = Convert.ToInt32(dechild.Properties["primaryGroupID"].Value);
                                if (iPrimaryGroupID == 516)
                                {
                                    sChilds = "," + dechild.Properties["name"].Value.ToString();
                                }
                            }
                        }
                    }
                    if (sChilds.StartsWith(","))
                    {
                        sChilds = sChilds.Substring(1);
                    }
                    if (sChilds != "")
                    {
                        Msg = "Likewise cannot delete {0} because it contains the critical system object(s)\n" +
                        "{1}. If you want to delete this container, you must first use Find to locate critical object(s) and \n" +
                        "delete it or move it to another container. If objects are still not available for deletion go to the \n" +
                        "view menu and check 'Users, Groups, Computers as containers and 'Advanced Features'";
                        Msg = string.Format(Msg, dirnode.Text, sChilds);
                        container.ShowError(Msg);
                        return ret;
                    }
                }
                else
                {
                    return ret;
                }
            }
            ret = dirnode.LdapContext.DeleteSynchronous(dirnode.DistinguishedName);
            
            if(ret!=0)
            {
                if (ret == (int)0x42)
                {
                    dlg = MessageBox.Show(
                    this,
                    "Do you want to delete this container and everything in it?",
                    CommonResources.GetString("Caption_Console"),
                    MessageBoxButtons.YesNo,
                    MessageBoxIcon.Exclamation,
                    MessageBoxDefaultButton.Button2);
                    if (dlg == DialogResult.Yes)
                    {
                        ret = dirnode.LdapContext.DeleteChildren_Recursive(dirnode.DistinguishedName);
                        if (ret == 0)
                        {
                            return ret;
                        }
                    }
                    else
                    {
                        return ret;
                    }
                }
                else
                {
                    if (Manage.bSSOFailed == false)
                    {
                        container.ShowError("You dont have administrator privilages to delete");
                    }
                    else
                        container.ShowError("Delete Object Failed !!");
                    return ret;
                }
            }
        }
        else
        {
            return ret;
        }
        return ret;
    }

    private int DoMoveADObject(ADUCDirectoryNode dirnode, string newparent)
    {
        int ret = -1;

        if (dirnode != null)
        {
            string oldDN = dirnode.DistinguishedName;
            DirectoryContext dirContext = dirnode.LdapContext;

            //the following portion of code uses openldap "ldap_rename_s"
            string fullDn = dirnode.DistinguishedName;
            LACTreeNode oldparentnode = dirnode.Parent as LACTreeNode;
            ADUCDirectoryNode oldparentdirnode = oldparentnode as ADUCDirectoryNode;
            string parentDn = oldparentdirnode.DistinguishedName;

            string rDn = fullDn.Substring(0, fullDn.Length - parentDn.Length - 1);

            //move an object
            if (newparent != null)
            {
                ret = dirContext.RenameSynchronous(fullDn, rDn, newparent);

                Logger.Log("fullDN is " + fullDn + "rDN is " + rDn + "newParent is " + newparent);
            }
        }

        return ret;
    }   

    public void DoPropertyPagesWork(ADUCDirectoryNode dirnode)
    {
        string newDn = null;

        if (dirnode != null)
        {
            string obj_type = dirnode.ObjectClass;
            DirectoryContext dirContext = dirnode.LdapContext;
            ADUCPlugin plugin = dirnode.Plugin as ADUCPlugin;

            if (plugin.Propertywindowhandles.ContainsKey(dirnode.DistinguishedName))
            {
                Form f = plugin.Propertywindowhandles[dirnode.DistinguishedName] as Form;
                f.BringToFront();
                return;
            }

            if (obj_type.Equals("top", StringComparison.InvariantCultureIgnoreCase) ||
            obj_type.Equals("user", StringComparison.InvariantCultureIgnoreCase))
            {
                List<object> dirnodes = new List<object>();
                dirnode.hn = plugin.HostInfo;
                dirnodes.Add(dirnode);
                ADUserPropertiesDlg f = new ADUserPropertiesDlg(base.container, this, plugin, dirnodes);
                if (dirContext.SchemaCache != null)
                {
                    f.SetData(
                    plugin.HostInfo.creds,
                    plugin.HostInfo.hostName,
                    dirnode.Text,
                    dirnode,
                    dirContext.SchemaCache);     
                    f.Show();
                    if (f.AttrModified && f.newDn != null)
                    {
                        newDn = f.newDn;
                    }
                }
            }
            else if (obj_type.Equals("group", StringComparison.InvariantCultureIgnoreCase) ||
                    (obj_type.Equals("foreignSecurityPrincipal", StringComparison.InvariantCultureIgnoreCase)))
            {
                dirnode.hn = plugin.HostInfo;
                ADGroupPropertiesDlg f = new ADGroupPropertiesDlg(base.container, this, base.pi as ADUCPlugin, dirnode);
                if (dirContext.SchemaCache != null)
                {
                    f.SetData(plugin.HostInfo.creds, plugin.HostInfo.hostName, dirnode.Text, dirnode, dirContext.SchemaCache);
                    f.Show();
                    if (f.AttrModified && f.newDn != null)
                    {
                        newDn = f.newDn;
                    }
                }
            }
            else if (obj_type.Equals("organizationalUnit", StringComparison.InvariantCultureIgnoreCase))
            {
                ADOUPropertiesDlg f = new ADOUPropertiesDlg(base.container, this, base.pi as ADUCPlugin);
                if (dirContext.SchemaCache != null)
                {
                    f.SetData(plugin.HostInfo.creds, plugin.HostInfo.hostName, dirnode.Text, dirnode, dirContext.SchemaCache);
                    f.Show();
                    if (f.AttrModified && f.newDn != null)
                    {
                        newDn = f.newDn;
                    }
                }
            }
            else if (obj_type.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
            {
                ADComputerPropertiesDlg f = new ADComputerPropertiesDlg(base.container, this, base.pi as ADUCPlugin);
                if (dirContext.SchemaCache != null)
                {
                    dirnode.hn = plugin.HostInfo;
                    f.SetData(plugin.HostInfo.creds, plugin.HostInfo.hostName, dirnode.Text, dirnode, dirContext.SchemaCache);
                    f.Show();
                    if (f.AttrModified && f.newDn != null)
                    {
                        newDn = f.newDn;
                    }
                }
            }
            else if (obj_type.Equals("serviceConnectionPoint", StringComparison.InvariantCultureIgnoreCase))
            {
                GenericPropertiesDlg f = new GenericPropertiesDlg(base.container, this, false);
                if (dirContext.SchemaCache != null)
                {
                    f.SetData(plugin.HostInfo.creds, plugin.HostInfo.hostName, dirnode.Text, dirnode, dirContext.SchemaCache);
                    f.Show();
                }
            }
            else if (dirnode.DistinguishedName == dirnode.LdapContext.RootDN)
            {
                DomainPropertiesDlg f = new DomainPropertiesDlg(base.container, this, base.pi as ADUCPlugin);
                if (dirContext.SchemaCache != null)
                {
                    f.SetData(plugin.HostInfo.creds, plugin.HostInfo.hostName, plugin.adContext.DomainName, dirnode, dirContext.SchemaCache);
                    f.Show();
                }
            }
            else
            {
                GenericPropertiesDlg f = new GenericPropertiesDlg(base.container, this, true);
                if (dirContext.SchemaCache != null)
                {
                    f.SetData(plugin.HostInfo.creds, plugin.HostInfo.hostName, dirnode.Text, dirnode, dirContext.SchemaCache);
                    f.Show();
                }
            }

            if (newDn != null)
            {
                dirnode.DistinguishedName = newDn;
                dirnode.Refresh();
                dirnode.IsModified = true;
                base.treeNode = dirnode;
            }
        }
    }
    
    
    
    
    /// <summary>
    /// Refreshes the child nodes when we click of refresh menu item on any selected node
    /// </summary>
    /// <param name="dn"></param>
    private void RefreshlvChildNodes(LACTreeNode dn)
    {

        lvChildNodes.Items.Clear();

        ListViewItem[] lviArr = new ListViewItem[dn.Nodes.Count];
        int i = 0;

        foreach (TreeNode tn in dn.Nodes)
        {
            ADUCDirectoryNode dtn = tn as ADUCDirectoryNode;
            if (dtn != null)
            {
                string[] values;

                if (dtn.DistinguishedName.Equals(dtn.LdapContext.RootDN, StringComparison.InvariantCultureIgnoreCase))
                {
                    values = new string[] { dtn.LdapContext.DomainName, dtn.ObjectClass, dtn.DistinguishedName };
                }
                else
                {
                    string[] parts = dtn.DistinguishedName.Split(',');
                    values = new string[] { parts.Length > 0 ? parts[0].Substring(3) : "", dtn.ObjectClass, dtn.DistinguishedName };
                }

                lviArr[i] = new ListViewItem(values);
                lviArr[i].Tag = dtn;
                lviArr[i].ImageIndex = (int)ADUCDirectoryNode.GetNodeType(dtn);
                i++;
            }
            else
            {
                lviArr[i] = new ListViewItem(tn.Text);
                lviArr[i].Tag = tn;
                lviArr[i].ImageIndex = (int)tn.ImageIndex;
                i++;
            }
        }
        if (dn.Nodes.Count == 0)
        {
            lblNoitemstodisplay.Visible = true;
        }
        else
        {
            lvChildNodes.Items.AddRange(lviArr);
            lvChildNodes.Sort();
            AutoResizePage();
        }
    }
    /// <summary>
    /// Method to resize the listview columns based on column content
    /// It will resizes the columns by setting the column width for each selected node
    /// </summary>
    private void AutoResizePage()
    {
        if (lvChildNodes == null || lvChildNodes.Columns == null || lvChildNodes.Columns.Count < 2)
        {
            return;
        }
        
        //tells the no.of columns that the listview contains
        const int NUM_COLUMNS = 3;
        
        //pixels to use to give a visible margin between columns
        const int MARGIN = 10;
        
        int minColumnWidth = (lvChildNodes.Width / NUM_COLUMNS);
        
        lvChildNodes.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
        
        foreach (ColumnHeader ch in lvChildNodes.Columns)
        {
            if (ch.Width + MARGIN < minColumnWidth)
            {
                ch.Width = minColumnWidth;
            }
            else
            {
                ch.Width += MARGIN;
            }
        }
    }
    
    /// <summary>
    /// Resets the contextmenu on mouse right click of any selected node
    /// </summary>
    /// <param name="lvSender"></param>
    /// <param name="e"></param>
    private void MouseEventContextMenu(ListView lvSender, MouseEventArgs e)
    {
        ContextMenu menu = GetTreeContextMenu();
        if (menu != null)
        {
            menu.Show(lvSender, new Point(e.X, e.Y));
        }
        else
        {
            Logger.Log("ADUCPage::lvChildNodes_MouseUp, menu == null",
            Logger.LogLevel.Error);
        }
        
    }
    
    /// <summary>
    /// Helper function that is used to add new user when we click of 'New - User' menu item
    /// </summary>
    /// <param name="dirnode"></param>
    private void AddNewUser(ADUCDirectoryNode dirnode)
    {
        if (dirnode != null)
        {
            string sText = "New Object - User";
            ADUserAddDlg f = new ADUserAddDlg(base.container, this, sText, dirnode, false, false, true, false, "");
            
            f.ShowDialog(this);
            //the user information is gather in f.userInfo before "finish" button is clicked
            if (f.userInfo.commit == true)
            {
                Hashtable htUserInfo = new Hashtable();
                if (f.userInfo.fName != "")
                {
                    htUserInfo.Add("givenName", f.userInfo.fName);
                }
                if (f.userInfo.initials != "")
                {
                    htUserInfo.Add("initials", f.userInfo.initials);
                }
                if (f.userInfo.lName != "")
                {
                    htUserInfo.Add("sn", f.userInfo.lName);
                }
                if (f.userInfo.fullName != "")
                {
                    htUserInfo.Add("cn", f.userInfo.fullName);
                    htUserInfo.Add("displayName", f.userInfo.fullName);
                    htUserInfo.Add("name", f.userInfo.fullName);
                }
                if (f.userInfo.logonName != "")
                {
                    htUserInfo.Add("userPrincipalName", f.userInfo.logonName);
                }
                if (f.userInfo.userPrelogonname != "")
                {
                    htUserInfo.Add("sAMAccountName", f.userInfo.userPrelogonname);
                }
                //use logon name to set "sAMAaccountname"
                AddNewObj_User(dirnode, htUserInfo, false, f.userInfo.passWord, f.userInfo.bAcountDisable, f.userInfo.bNeverExpiresPwd, f.userInfo.bMustChangePwd, f.userInfo.bCannotChangePwd);
            }
        }
    }
    
    /// <summary>
    /// Helper function that is used to add new computer when we click of 'New - Computer' menu item
    /// </summary>
    /// <param name="dirnode"></param>
    /// <param name="smamAccount"></param>
    private void AddNewComputer(ADUCDirectoryNode dirnode, string smamAccount)
    {
        if (dirnode != null)
        {
            string obj_type = dirnode.ObjectClass;
            
            ADComputerAddDlg f = new ADComputerAddDlg(base.container, this, dirnode);
            f.ShowDialog(this);
            
            if (f.computerInfo.commit == true)
            {
                Hashtable htCompInfo = new Hashtable();
                if (f.computerInfo.ComputerName != "")
                {
                    htCompInfo.Add("cn", f.computerInfo.ComputerName);
                    htCompInfo.Add("name", f.computerInfo.ComputerName);
                }
                if (f.computerInfo.PreWindowsCName != "")
                {
                    htCompInfo.Add("sAMAccountName", f.computerInfo.PreWindowsCName);
                }
                string compuserAccountControl = Convert.ToString(f.computerInfo.UserAccountControl);
                htCompInfo.Add("userAccountControl", compuserAccountControl);
                AddNewObj_Computer(dirnode, htCompInfo, false, f.computerInfo.IsBackUpDomainComputer, f.computerInfo.IsPreWindowsComputer);
            }
        }
    }
    
    /// <summary>
    /// Helper function that is used to add new group when we click of 'New - Group' menu item
    /// </summary>
    /// <param name="dirnode"></param>
    private void AddNewGroup(ADUCDirectoryNode dirnode)
    {
        if (dirnode != null)
        {
            ADGroupAddDlg f = new ADGroupAddDlg(base.container, this,dirnode);
            f.ShowDialog(this);
            
            if (f.groupInfo.commit == true)
            {
                Hashtable htgroupInfo = new Hashtable();
                if (f.groupInfo.GroupName != "")
                {
                    htgroupInfo.Add("cn", f.groupInfo.GroupName);
                    htgroupInfo.Add("name", f.groupInfo.GroupName);
                }
                if (f.groupInfo.PreWindowsgroupname != "")
                {
                    htgroupInfo.Add("sAMAccountName", f.groupInfo.PreWindowsgroupname);
                }
                if (f.groupInfo.groupType!="")
                {
                    htgroupInfo.Add("grouptype", f.groupInfo.groupType);
                }
                AddNewObj_Group(dirnode, htgroupInfo, false);
            }
        }
    }
    
    /// <summary>
    /// Helper function that is used to add new OU when we click of 'New - Organizational Unit' menu item
    /// </summary>
    /// <param name="dirnode"></param>
    private void AddNewOU(ADUCDirectoryNode dirnode)
    {
        if (dirnode != null)
        {
            ADOUAddDlg f = new ADOUAddDlg(base.container, this, dirnode);
            f.ShowDialog(this);
            
            if (f.ouInfo.commit == true)
            {
                AddNewObj_OU(dirnode, f.ouInfo.OUName, null, false);
            }
        }
    }
    
    /// <summary>
    /// Helper function that is used to add new object when we click of 'New - Object' menu item
    /// </summary>
    /// <param name="dirnode"></param>
    private void AddNewObject(ADUCDirectoryNode dirnode)
    {
        if (dirnode != null)
        {
            DirectoryContext dirContext = dirnode.LdapContext;
            string sText = "Create Object";
            LDAPSchemaCache schemaCache = dirContext.SchemaCache;


            ADObjectAddDlg f = new ADObjectAddDlg(base.container, this, sText, schemaCache, dirnode);
            f.ShowDialog(this);
            
            if (f.objectInfo.commit)
            {
                //create an new object
                if (f.choosenClass.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
                {
                    AddNewObj_Computer(dirnode, f.objectInfo.htMandatoryAttrList, true, false, false);
                }
                else if (f.choosenClass.Equals("user", StringComparison.InvariantCultureIgnoreCase))
                {
                    AddNewObj_User(dirnode, f.objectInfo.htMandatoryAttrList, true, null, true, false, false, false);
                }
                else if (f.choosenClass.Equals("group", StringComparison.InvariantCultureIgnoreCase))
                {
                    AddNewObj_Group(dirnode, f.objectInfo.htMandatoryAttrList, true);
                }
                else if (f.choosenClass.Equals("organizationalUnit", StringComparison.InvariantCultureIgnoreCase))
                {
                    AddNewObj_OU(dirnode, null, f.objectInfo.htMandatoryAttrList, true);
                }
                else
                {
                    AddNewObj(dirnode, f.choosenClass, null, f.objectInfo.htMandatoryAttrList);
                }
            }
        }
    }
    
    /// <summary>
    /// Helper function that saves the new computer information in AD by
    /// defining the attributes "cn", "ObjectClass" as computer for the new object
    /// Here we can say that ObjectClass is mandatory attribute that has to be set while creating the new computer
    /// </summary>
    /// <param name="dirnode"></param>
    /// <param name="cn"></param>
    /// <param name="smamAccount"></param>
    /// <param name="htAttributesList"></param>
    /// <param name="IsObjectItem"></param>
    private void AddNewObj_Computer(ADUCDirectoryNode dirnode, Hashtable htAttributesList, bool IsObjectItem, bool IsBackupDC,bool IsPreWindowsComp)
    {
        if (dirnode != null)
        {
            string cn = string.Empty;
            List<LDAPMod> ldapattrlst = new List<LDAPMod>();
            DirectoryContext dirContext = dirnode.LdapContext;
            string[] objectClass_values = { "computer", null };
            int ret = 0;
            
            if (IsObjectItem)
            {
                foreach (string key in htAttributesList.Keys)
                {
                    if (key.Trim().ToLower().Equals("cn"))
                    {
                        cn = htAttributesList[key].ToString();
                    }
                    else if (key.Trim().ToLower().Equals("objectclass") ||
                    key.Trim().ToLower().Equals("useraccountcontrol"))
                    {
                        continue;
                    }
                    
                    objectClass_values = new string[] { htAttributesList[key].ToString(), null };
                    LDAPMod ouinfo_attr1 =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, key, objectClass_values);
                    ldapattrlst.Add(ouinfo_attr1);
                }
                
                objectClass_values = new string[] { "computer", null };
                LDAPMod ouinfo_attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "ObjectClass", objectClass_values);
                ldapattrlst.Add(ouinfo_attr);
                
                string compuserAccountControl = Convert.ToString(546);
                objectClass_values = new string[] { compuserAccountControl, null };
                ouinfo_attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "userAccountControl", objectClass_values);
                ldapattrlst.Add(ouinfo_attr);
            }
            else
            {
                LDAPMod userinfo_attr1 =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "ObjectClass", objectClass_values);
                ldapattrlst.Add(userinfo_attr1);
                
                foreach (string key in htAttributesList.Keys)
                {
                    if (key.Trim().ToLower().Equals("cn"))
                    {
                        cn = htAttributesList[key].ToString();
                    }
                    objectClass_values = new string[] { htAttributesList[key].ToString(), null };
                    
                    userinfo_attr1 =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, key, objectClass_values);
                    ldapattrlst.Add(userinfo_attr1);
                }
            }
            
            string dn = string.Concat(CN_PREFIX, cn, DN_SEPARATOR);
            string ou = dirnode.DistinguishedName;
            dn = string.Concat(dn, ou);
            
            LDAPMod[] computerinfo = new LDAPMod[ldapattrlst.Count];
            ldapattrlst.CopyTo(computerinfo);
            
            //returns ret=0 if adding computer is successfull
            ret = dirContext.AddSynchronous(dn, computerinfo);
            if (ret == 0)
            {
                container.ShowMessage("New Computer Object is added!");
                
                dirnode.Refresh();
                dirnode.IsModified = true;
                base.treeNode = dirnode;
            }
            else if (ret == (int)ErrorCodes.LDAPEnum.LDAP_INSUFFICIENT_RIGHTS)
            {
                string showmsg = "You do not have sufficient access rights";
                container.ShowError(showmsg);
            }
            else
            {
                container.ShowError(ErrorCodes.LDAPString(ret));
            }
        }
    }


    /// <summary>
    /// Helper function that saves the new user information in AD by
    /// defining the attributes "cn", "sAMAccountName", "ObjectClass" as user for the new object
    /// Here we can say that ObjectClass,sAMAccountName is mandatory attribute that has to be set while creating the new user
    /// </summary>
    /// <param name="dirnode"></param>
    /// <param name="cn"></param>
    /// <param name="smamAccount"></param>
    /// <param name="htAttributesList"></param>
    /// <param name="IsObjectItem"></param>
    /// <param name="password"></param>
    /// <param name="IsDisabled"></param>
    private void AddNewObj_User(ADUCDirectoryNode dirnode, Hashtable htAttributesList, bool IsObjectItem, string password, bool IsDisabled, bool NeverExpiredPW, bool MustChangePWNxtLogon, bool CannotChangePW)
    {
        if (dirnode != null)
        {
            string cn = string.Empty;
            string smamAccount = string.Empty;
            List<LDAPMod> userinfo = new List<LDAPMod>();
            DirectoryContext dirContext = dirnode.LdapContext;
            string[] objectClass_values = { "user", null };

            if (IsObjectItem)
            {
                foreach (string key in htAttributesList.Keys)
                {
                    if (key.Trim().ToLower().Equals("cn"))
                    {
                        cn = htAttributesList[key].ToString();
                    }
                    else if (key.Trim().ToLower().Equals("sAMAccountName".Trim().ToLower()))
                    {
                        smamAccount = htAttributesList[key].ToString();
                    }
                    else if (key.Trim().ToLower().Equals("objectclass"))
                    {
                        continue;
                    }
                    objectClass_values = new string[] { htAttributesList[key].ToString(), null };

                    LDAPMod userinfo_attr1 =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, key, objectClass_values);
                    userinfo.Add(userinfo_attr1);
                }

                objectClass_values = new string[] { "user", null };
                LDAPMod userinfo_attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "ObjectClass", objectClass_values);
                userinfo.Add(userinfo_attr);
            }
            else
            {
                LDAPMod userinfo_attr1 =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "ObjectClass", objectClass_values);
                userinfo.Add(userinfo_attr1);

                foreach (string key in htAttributesList.Keys)
                {
                    if (key.Trim().ToLower().Equals("cn"))
                    {
                        cn = htAttributesList[key].ToString();
                    }
                    else if (key.Trim().ToLower().Equals("sAMAccountName".ToLower()))
                    {
                        smamAccount = htAttributesList[key].ToString();
                    }
                    objectClass_values = new string[] { htAttributesList[key].ToString(), null };

                    userinfo_attr1 =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, key, objectClass_values);
                    userinfo.Add(userinfo_attr1);
                }
            }
            
            int useraccountCtr_decimalVal = 512;

            if (IsObjectItem)
            {
                useraccountCtr_decimalVal = 546;
            }
            else
            {
                if (IsDisabled)
                {
                    useraccountCtr_decimalVal += 2;
                }

                if (NeverExpiredPW)
                {
                    useraccountCtr_decimalVal += 65536;
                }
                ////user password needs to be set in another way instead of using "userPassword" attribute
                //if (MustChangePWNxtLogon)
                //{
                //    useraccountCtr_decimalVal += 8388608;                   
                //}
                if (CannotChangePW)
                {
                    useraccountCtr_decimalVal += 64;
                }
            }

            string useraccountCtr_decimalStr = Convert.ToString(useraccountCtr_decimalVal);

            string[] useraccountCtr_values = { useraccountCtr_decimalStr, null };
           
            LDAPMod[] userinfos = new LDAPMod[userinfo.Count];
            userinfo.CopyTo(userinfos);

            string dn = string.Concat(CN_PREFIX, cn, DN_SEPARATOR);
            string ou = dirnode.DistinguishedName;
            dn = string.Concat(dn, ou);

            Logger.Log(string.Format("New user DN :" + dn));
            foreach (LDAPMod key in userinfos)
            {
                Logger.Log(string.Format(string.Format("New user keys {0}, {1} :", key.ldapmod.mod_type, key.modv_strvals[0])));
            }

            //returns ret=0 if adding user is successfull
            int ret = dirContext.AddSynchronous(dn, userinfos);

            if (ret == 0)
            {
                bool retValue = false;
                if (!IsDisabled)
                {
                    container.ShowMessage("New User Object is added!");
                }
                else
                {
                    container.ShowMessage("New (disabled) User Object is added!");
                }

                //need to make sure the interop is working correctly, comment it for now
                try
                {
                    if (!String.IsNullOrEmpty(password))
                    {
                        ADUCPlugin plugin = pi as ADUCPlugin;
                        Hostinfo hn = ctx as Hostinfo;

                        if (!plugin.bIsNetInitCalled)
                        {
                            plugin.bIsNetInitCalled = LUGAPI.NetInitMemory(hn.creds, hn.domainControllerName);
                        }
                        retValue = !Convert.ToBoolean(LUGAPI.NetChangePassword(hn.creds, hn.domainName, smamAccount, password));
                        Logger.Log(string.Format("hn.domainName is {0} : userName is {1} and password is {2}", hn.domainName, smamAccount, password));
                    }
                }
                catch (Exception ex)
                {
                    container.ShowError("Filed to set the password for this new user because " + ex.Message);
                }
            }
            else if (ret == (int)ErrorCodes.LDAPEnum.LDAP_UNWILLING_TO_PERFORM)
            {
                string accessDenied = "Indicates that the Active Directory server cannot process the request because of server-defined restrictions. \n This error is returned for the following reasons:\n  - The add entry request violates the server's structure rules.\n  - The modify attribute request specifies attributes that users cannot modify.\n  - Password restrictions prevent the action.\n  - Connection restrictions prevent the action.";

                string returnedErrMsg = ErrorCodes.LDAPString(ret);

                if (accessDenied.Equals(returnedErrMsg, StringComparison.InvariantCultureIgnoreCase))
                {
                    returnedErrMsg = "The password could not be set for the new user object.\n\nThe the Active Directory server cannot process the request because of server-defined restrictions.\nThis error is returned for the following reasons:\n  - The add entry request violates the server's structure rules.\n  - The modify attribute request specifies attributes that users cannot modify.\n  - Password restrictions prevent the action.\n  - Connection restrictions prevent the action.\n\nLikewise Console will attempt to disable this account.\nBefore this user can log on, the password should be set and the account must be enabled.";

                    container.ShowError(returnedErrMsg);
                }
                //returns ret=0 if adding user is successfull
                ret = dirContext.AddSynchronous(dn, userinfos);

                if (!IsDisabled)
                {
                    container.ShowMessage("New User Object is added!");
                }
                else
                {
                    container.ShowMessage("New (disabled) User Object is added!");
                }
            }
            else if (ret == (int)ErrorCodes.LDAPEnum.LDAP_INSUFFICIENT_RIGHTS)
            {
                string showmsg = "You do not have sufficient access rights";
                container.ShowError(showmsg);
            }
            else
            {
                container.ShowError(ErrorCodes.LDAPString(ret));
            }

            if (ret == 0)
            {
                DirectoryEntry de = new DirectoryEntry(dn, dirContext.UserName, dirContext.Password);
                int userCtrlInt = Convert.ToInt32(de.Properties["userAccountControl"].Value.ToString());
                de.CommitChanges();

                //string newUserCtrl_val = Convert.ToString(userCtrlInt - 2);
                //in order to disable an user, we need to add 2 to the existing userControl value
                //string[] userControl_values = { newUserCtrl_val, null };
                LDAPMod userControl_Info =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_REPLACE, "userAccountControl",
                useraccountCtr_values);
                LDAPMod[] attrinfo = new LDAPMod[] { userControl_Info };
                ret = dirContext.ModifySynchronous(dn, attrinfo);

                if (ret.ToString() == ((int)ErrorCodes.LDAPEnum.LDAP_UNWILLING_TO_PERFORM).ToString())
                {
                    //string accessDenied = "Indicates that the Active Directory server cannot process the request because of server-defined restrictions. \n This error is returned for the following reasons:\n  - The add entry request violates the server's structure rules.\n  - The modify attribute request specifies attributes that users cannot modify.\n  - Password restrictions prevent the action.\n  - Connection restrictions prevent the action.";
                    //string returnedErrMsg = ErrorCodes.LDAPString(ret);
                    //if (accessDenied.Equals(returnedErrMsg, StringComparison.InvariantCultureIgnoreCase))
                    //{
                    string returnedErrMsg = "The password could not be set for the new user object.\n\nThe the Active Directory server cannot process the request because of server-defined restrictions.\nThis error is returned for the following reasons:\n  - The add entry request violates the server's structure rules.\n  - The modify attribute request specifies attributes that users cannot modify.\n  - Password restrictions prevent the action.\n  - Connection restrictions prevent the action.\n\nLikewise Console will attempt to disable this account.\nBefore this user can log on, the password should be set and the account must be enabled.";

                    container.ShowError(returnedErrMsg);
                }

                if (MustChangePWNxtLogon)
                {
                    de.Properties["pwdLastSet"].Value = "0";
                    de.CommitChanges();
                }
                else
                {
                    de.Properties["pwdLastSet"].Value = ConvertToUnixTimestamp(DateTime.Now).ToString();
                    de.CommitChanges();                    
                }               

                dirnode.Refresh();
                dirnode.IsModified = true;
                base.treeNode = dirnode;
            }
        }
    }

    private UInt64 ConvertToUnixTimestamp(DateTime date)
    {
        UInt64 unixTimestamp;
        try
        {
            unixTimestamp = (UInt64)((TimeSpan)(date - new DateTime(1601, 1, 1, 0, 0, 0))).TotalSeconds;
        }
        catch (Exception ex)
        {
            unixTimestamp = 128765605339375000;
            Logger.LogException("ADUCPage.ConvertToUnixTimestamp : " + unixTimestamp.ToString(), ex);
        }
        return unixTimestamp;
    }
    
    /// <summary>
    /// Helper function that saves the new group information in AD by
    /// defining the attributes "cn", "grouptype", "ObjectClass" as user for the new object
    /// Here we can say that ObjectClass,grouptype is mandatory attributes that has to be set while creating the new group
    /// </summary>
    /// <param name="dirnode"></param>
    /// <param name="cn"></param>
    /// <param name="smamAccount"></param>
    /// <param name="groupType"></param>
    /// <param name="htAttributesList"></param>
    /// <param name="IsObjectItem"></param>
    private void AddNewObj_Group(ADUCDirectoryNode dirnode,Hashtable htAttributesList,bool IsObjectItem)
    {
        if (dirnode != null)
        {
            string cn = string.Empty;
            DirectoryContext dirContext = dirnode.LdapContext;
            string[] objectClass_values = { "group", null };
            List<LDAPMod> ldapAttrlist = new List<LDAPMod>();
            if (IsObjectItem)
            {
                foreach (string key in htAttributesList.Keys)
                {
                    if (key.Trim().ToLower().Equals("cn"))
                    {
                        cn = htAttributesList[key].ToString();
                    }
                    else if (key.Trim().ToLower().Equals("objectclass"))
                    {
                        continue;
                    }
                    
                    objectClass_values = new string[] { htAttributesList[key].ToString(), null };
                    
                    LDAPMod groupinfo_attr1 =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, key, objectClass_values);
                    ldapAttrlist.Add(groupinfo_attr1);
                }
                
                objectClass_values = new string[] { "group", null };
                LDAPMod groupinfo_attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "ObjectClass", objectClass_values);
                ldapAttrlist.Add(groupinfo_attr);
            }
            else
            {
                LDAPMod groupinfo_attr1 =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "ObjectClass", objectClass_values);
                ldapAttrlist.Add(groupinfo_attr1);
                
                foreach (string key in htAttributesList.Keys)
                {
                    if (key.Trim().ToLower().Equals("cn"))
                    {
                        cn = htAttributesList[key].ToString();
                    }
                    
                    objectClass_values = new string[] { htAttributesList[key].ToString(), null };
                    
                    groupinfo_attr1 =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, key, objectClass_values);
                    ldapAttrlist.Add(groupinfo_attr1);
                }
            }           

            LDAPMod[] groupinfo = new LDAPMod[ldapAttrlist.Count];
            ldapAttrlist.CopyTo(groupinfo);
            
            string dn = string.Concat(CN_PREFIX, cn, DN_SEPARATOR);
            string ou = dirnode.DistinguishedName;
            dn = string.Concat(dn, ou);
            
            //Returns ret=0 if adding group is successfull
            int ret = dirContext.AddSynchronous(dn, groupinfo);
            
            if (ret == 0)
            {
                container.ShowMessage("New Group Object is added!");
                dirnode.Refresh();
                dirnode.IsModified = true;
                base.treeNode = dirnode;
            }
            else if (ret == (int)ErrorCodes.LDAPEnum.LDAP_INSUFFICIENT_RIGHTS)
            {
                string showmsg = "You do not have sufficient access rights";
                container.ShowError(showmsg);
            }
            else
            {
                container.ShowError(ErrorCodes.LDAPString(ret));
            }
        }
    }
    
    /// <summary>
    /// Helper function that saves the new OU information in AD by
    /// defining the attributes "ou", "ObjectClass" as user for the new object
    /// Here we can say that ObjectClass,cn is mandatory attributes that has to be set while creating the new OU
    /// </summary>
    /// <param name="dirnode"></param>
    /// <param name="cn"></param>
    /// <param name="htAttributesList"></param>
    /// <param name="IsObjectItem"></param>
    private void AddNewObj_OU(ADUCDirectoryNode dirnode, string cn,Hashtable htAttributesList,bool IsObjectItem)
    {
        if (dirnode != null)
        {
            DirectoryContext dirContext = dirnode.LdapContext;
            List<LDAPMod> listattr = new List<LDAPMod>();
            string[] objectClass_values = { "organizationalUnit", null };
            
            if (IsObjectItem)
            {
                foreach (string key in htAttributesList.Keys)
                {
                    if (key.Trim().ToLower().Equals("ou") || key.Trim().ToLower().Equals("cn"))
                    {
                        cn = htAttributesList[key].ToString();
                    }
                    if (key.Trim().ToLower().Equals("objectclass"))
                    {
                        continue;
                    }
                    
                    objectClass_values = new string[] { htAttributesList[key].ToString(), null };
                    LDAPMod ouinfo_attr1 =
                    new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, key, objectClass_values);
                    listattr.Add(ouinfo_attr1);
                }
                objectClass_values = new string[] { "organizationalUnit", null };
                LDAPMod ouinfo_attr =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "ObjectClass", objectClass_values);
                listattr.Add(ouinfo_attr);
            }
            else
            {
                objectClass_values = new string[] { "organizationalUnit", null };
                LDAPMod ouinfo_attr1 =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "ObjectClass", objectClass_values);
                listattr.Add(ouinfo_attr1);
            }
            
            LDAPMod[] ouinfo = new LDAPMod[listattr.Count];
            listattr.CopyTo(ouinfo);
            
            string dn = string.Concat(OU_PREFIX, cn, DN_SEPARATOR);
            string ou = dirnode.DistinguishedName;
            dn = string.Concat(dn, ou);
            
            //Returns the ret=0 if adding new OU is successfull
            int ret = dirContext.AddSynchronous(dn, ouinfo);
            
            if (ret == 0)
            {
                //int length = CommonResources.GetString("Caption_Console").Length + 24;
                string msgToDisplay = "New OrganizationalUnit Object is added!";
                MessageBox.Show(this, msgToDisplay, CommonResources.GetString("Caption_Console"), MessageBoxButtons.OK);
                //if (length > msgToDisplay.Length)
                //{
                //    msgToDisplay = msgToDisplay.PadRight(length - msgToDisplay.Length, ' ');
                //}
                //container.ShowMessage(msgToDisplay);
                
                dirnode.Refresh();
                dirnode.IsModified = true;
                base.treeNode = dirnode;
            }
            else
            {
                container.ShowError(ErrorCodes.LDAPString(ret));
            }
        }
    }
    
    
    /// <summary>
    /// Helper function that saves the new object information in AD by
    /// defining the attributes "ObjectClass" as selected object type for the new object
    /// Here we can say that the values getting from "systemMustContain" attribute have to be set while creating the new object
    /// New object may be any of the class schema attribute from AD Schema template
    /// </summary>
    /// <param name="dirnode"></param>
    /// <param name="choosenclass"></param>
    /// <param name="cn"></param>
    /// <param name="htAttributesList"></param>
    private void AddNewObj(ADUCDirectoryNode dirnode, string choosenclass, string cn, Hashtable htAttributesList)
    {
        if (dirnode != null)
        {
            LDAPMod[] info = null;
            
            DirectoryContext dirContext = dirnode.LdapContext;
            
            string[] objectClass_values = { choosenclass, null };
            
            if (htAttributesList != null)
            {
                info = new LDAPMod[htAttributesList.Count];
            }
            
            int i = 0;
            foreach (string key in htAttributesList.Keys)
            {
                if (key.Trim().ToLower().Equals("cn"))
                {
                    cn = htAttributesList[key].ToString();
                    continue;
                }
                objectClass_values = new string[] { htAttributesList[key].ToString(), null };
                
                LDAPMod info_attr1 =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, key, objectClass_values);
                
                info[i] = info_attr1;
                i++;
            }
            if (!htAttributesList.Contains("ObjectClass"))
            {
                objectClass_values = new string[] { choosenclass, null };
                LDAPMod info_attr1 =
                new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "ObjectClass", objectClass_values);
                
                info[i] = info_attr1;
            }
            
            string dn = string.Concat(CN_PREFIX, cn, DN_SEPARATOR);
            string ou = dirnode.DistinguishedName;
            dn = string.Concat(dn, ou);
            
            int ret = dirContext.AddSynchronous(dn, info);
            
            if (ret == 0)
            {
                container.ShowMessage("New Object is added!");
                dirnode.Refresh();
                dirnode.IsModified = true;
                base.treeNode = dirnode;
            }
            else if (ret == (int)ErrorCodes.LDAPEnum.LDAP_INSUFFICIENT_RIGHTS)
            {
                string showmsg = "You do not have sufficient access rights";
                container.ShowError(showmsg);
            }
            else
            {
                container.ShowError(ErrorCodes.LDAPString(ret));
            }
        }
    }
    
    /// <summary>
    /// Updates the listview with newly added/modified/deleted object on right hand side of the ADUCPlugin
    /// </summary>
    /// <param name="objectClass"></param>
    /// <param name="dn"></param>
    /// <param name="dirContext"></param>
    /// <param name="dirnode"></param>
    private void listViewhelper(string objectClass, string dn, DirectoryContext dirContext, ADUCDirectoryNode dirnode)
    {
        ADUCDirectoryNode dtn = new ADUCDirectoryNode(dn, dirContext, objectClass,
        Resources.Group_16, dirnode.NodeType, this.pi, dirnode.IsDisabled);
        
        ListViewItem lviArr;
        
        string[] parts = dtn.DistinguishedName.Split(',');
        string[] values = { parts.Length > 0 ? parts[0] : "", dtn.ObjectClass, dtn.DistinguishedName };
        
        lviArr = new ListViewItem(values);
        lviArr.Tag = dtn;
        lviArr.ImageIndex = (int)ADUCDirectoryNode.GetNodeType(dtn);
        
        lvChildNodes.Items.Add(lviArr);
        
    }
    
    /// <summary>
    /// takes a leaf node as input, perform a rename operation (delete and then add)
    /// </summary>
    /// <param name="dirnode"></param>
    /// <param name="newName"></param>
    private void DelAndAddLeaf(ADUCDirectoryNode dirnode, string newName)
    {
        int ret;
        
        LACTreeNode parentnode = (LACTreeNode) dirnode.Parent;
        
        ADUCDirectoryNode parentdirnode = parentnode as ADUCDirectoryNode;
        
        string newDn = parentdirnode.DistinguishedName;
        
        string obj_type = dirnode.ObjectClass;
        
        DirectoryContext dirContext = dirnode.LdapContext;
        //rename leaf user
        if (obj_type.Equals("top", StringComparison.InvariantCultureIgnoreCase) ||
        obj_type.Equals("user", StringComparison.InvariantCultureIgnoreCase))
        {
            //User
        }
        {
            ret = dirContext.DeleteSynchronous(dirnode.DistinguishedName);
            
            if (ret!=0)
            {
                
                container.ShowError(ErrorCodes.LDAPString(ret));
                return;
            }
            
            string[] objectClass_values = { "user", null };
            
            LDAPMod userinfo_attr1 =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "ObjectClass", objectClass_values);
            
            LDAPMod[] userinfo = new LDAPMod[] { userinfo_attr1 };
            
            newDn = string.Concat(CN_PREFIX, newName, DN_SEPARATOR, newDn);
            
            ret = dirContext.AddSynchronous(newDn, userinfo);
            
            if (ret != 0)
            {
                container.ShowError(ErrorCodes.LDAPString(ret));
            }
        }
        
        //rename leaf computer
        if (obj_type.Equals("computer", StringComparison.InvariantCultureIgnoreCase))
        {
            ret = dirContext.DeleteSynchronous(dirnode.DistinguishedName);
            
            if (ret != 0)
            {
                container.ShowError(ErrorCodes.LDAPString(ret));
                return;
            }
            
            string[] objectClass_values = { "user", null }; //need to be fixed, computer is now treated as user (person)
            
            LDAPMod userinfo_attr1 =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "ObjectClass", objectClass_values);
            
            LDAPMod[] userinfo = new LDAPMod[] { userinfo_attr1 };
            
            newDn = string.Concat(CN_PREFIX, newName, DN_SEPARATOR, newDn);
            
            ret = dirContext.AddSynchronous(newDn, userinfo);
            
            if (ret != 0)
            {
                container.ShowError(ErrorCodes.LDAPString(ret));
            }
        }
        
        //rename leaf ou
        if (obj_type.Equals("organizationalUnit", StringComparison.InvariantCultureIgnoreCase))
        {
            
        }
        {
            ret = dirContext.DeleteSynchronous(dirnode.DistinguishedName);
            
            if (ret != 0)
            {
                container.ShowError(ErrorCodes.LDAPString(ret));
                return;
            }
            
            string[] objectClass_values = { "organizationalUnit", null };
            
            LDAPMod userinfo_attr1 =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "ObjectClass", objectClass_values);
            
            LDAPMod[] userinfo = new LDAPMod[] { userinfo_attr1 };
            
            newDn = string.Concat(OU_PREFIX, newName, DN_SEPARATOR, newDn);
            
            ret = dirContext.AddSynchronous(newDn, userinfo);
            
            if (ret != 0)
            {
                container.ShowError(ErrorCodes.LDAPString(ret));
            }
        }
        
        //rename a group
        if (obj_type.Equals("group", StringComparison.InvariantCultureIgnoreCase))
        {
            ret = dirContext.DeleteSynchronous(dirnode.DistinguishedName);
            
            if (ret != 0)
            {
                container.ShowError(ErrorCodes.LDAPString(ret));
                return;
            }
            
            string[] objectClass_values = { "group", null };
            
            LDAPMod userinfo_attr1 =
            new LDAPMod((int)LDAPMod.mod_ops.LDAP_MOD_ADD, "ObjectClass", objectClass_values);
            
            LDAPMod[] userinfo = new LDAPMod[] { userinfo_attr1 };
            
            newDn = string.Concat(CN_PREFIX, newName, DN_SEPARATOR, newDn);
            
            ret = dirContext.AddSynchronous(newDn, userinfo);
            
            if (ret != 0)
            {
                container.ShowError(ErrorCodes.LDAPString(ret));
            }
        }
        
        parentdirnode.Refresh();
        
    }
    
    private void RefreshModifiedNode(LACTreeNode dirnode, ADUCDirectoryNode newParentDirnode)
    {
        ADUCDirectoryNode newParentnode = newParentDirnode as ADUCDirectoryNode;
        foreach (LACTreeNode node in dirnode.Nodes)
        {
            if (node is ADUCDirectoryNode)
            {
                ADUCDirectoryNode dn = node as ADUCDirectoryNode;
                if (dn.DistinguishedName.Equals(newParentnode.DistinguishedName, StringComparison.InvariantCultureIgnoreCase))
                {
                    if (dn.DistinguishedName.Trim().ToLower().Equals(dn.LdapContext.RootDN.ToLower()))
                    {
                        dn.Refresh();
                        //RefreshlvChildNodes(dn);
                    }
                    else
                    {
                        dn.haveRetrievedChildren = false;
                    }
                    break;
                }
                else if (node.Nodes.Count != 0)
                {
                    RefreshModifiedNode(node, newParentnode);
                }
            }
        }
    }

    private bool CheckLdapTimedOut(ADUCDirectoryNode node)
    {
        int ret = -1;
        ADUCPlugin plugin = node.Plugin as ADUCPlugin;
        LdapMessage message = null;
        ret = node.LdapContext.SearchSynchronous(
        node.DistinguishedName,
        Likewise.LMC.LDAP.Interop.LdapAPI.LDAPSCOPE.BASE,
        "(objectClass=*)",
        new string[]
        {
            "objectClass", "distinguishedName", null
        }
        ,
        false,
        out message);
        if (ret == (int)Likewise.LMC.LMConsoleUtils.ErrorCodes.LDAPEnum.LDAP_SERVER_DOWN ||
        ret == (int)Likewise.LMC.LMConsoleUtils.ErrorCodes.LDAPEnum.LDAP_CONNECT_ERROR ||
        ret == -1 || ret == 85)
        {
            if (ret == -1 || ret == 85)
            {
                ret = (int)ErrorCodes.LDAPEnum.LDAP_TIMEOUT;
                Logger.ShowUserError(ErrorCodes.LDAPString(ret));
            }           

            string sPluginName = plugin._pluginNode.Text.Trim();
            if (sPluginName.Contains("["))
            {
                sPluginName = sPluginName.Substring(0, sPluginName.IndexOf('['));
                plugin._pluginNode.Text = sPluginName;
            }

            plugin._pluginNode.Nodes.Clear();
            node.sc.ShowControl(plugin._pluginNode);

            return true;
        }
        return false;
    }
    
    #endregion
    

}

    //Pack up the entire object info before sending the information to the backgroundworker thread.
    public class ObjectPropertyInfo
    {
        public CredentialEntry ce;
        public string servername;
        public string objectName;
        public ADUCDirectoryNode dirnode;
        public MPPage page;

        public ObjectPropertyInfo(CredentialEntry ce,
                                  string server,
                                  string name,
                                  ADUCDirectoryNode node,
                                  MPPage pp
                                  )
        {
            this.ce = ce;
            servername = server;
            objectName = name;
            dirnode = node;
            page = pp;
        }      
    }
}
