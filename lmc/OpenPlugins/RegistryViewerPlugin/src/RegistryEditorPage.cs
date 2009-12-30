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
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using Likewise.LMC.Registry;
using Likewise.LMC.UtilityUIElements;
using Likewise.LMC.SecurityDesriptor;

using Microsoft.Win32;

namespace Likewise.LMC.Plugins.RegistryViewerPlugin
{
    public partial class RegistryEditorPage : StandardPage
    {

        #region Class data

        private ListViewColumnSorter lvwColumnSorter;
        private RegistryViewerPlugin.NodeType nodeType;
        private RegistryViewerPlugin plugin;

        //Set columns to Listview as per the node type
        private int numColumns = 0;
        private string[] columnLabels = null;
        private ColumnHeader[] columnHeaders = null;

        #endregion

        #region Constructor

        public RegistryEditorPage()
        {
            InitializeComponent();

            // Create an instance of a ListView column sorter and assign it
            // to the ListView control.
            lvwColumnSorter = new ListViewColumnSorter();
            //this.lvRegistryPage.ListViewItemSorter = lvwColumnSorter;

            nodeType = RegistryViewerPlugin.NodeType.NONE;
        }

        public RegistryEditorPage(RegistryViewerPlugin.NodeType nodetype)
        {
            InitializeComponent();

            // Create an instance of a ListView column sorter and assign it
            // to the ListView control.
            lvwColumnSorter = new ListViewColumnSorter();
            //this.lvRegistryPage.ListViewItemSorter = lvwColumnSorter;

            //this.picture.Image = (Image)CommonResources.GetIcon("agent_lgr");
            this.nodeType = nodetype;
        }
        #endregion

        #region IPlugInPage Members

        public override void SetPlugInInfo(IPlugInContainer container, IPlugIn pi, LACTreeNode treeNode, LWTreeView lmctreeview, CServerControl sc)
        {
            base.SetPlugInInfo(container, pi, treeNode, lmctreeview, sc);
            bEnableActionMenu = false;
            ShowHeaderPane(true);

            plugin = pi as RegistryViewerPlugin;

            SetListviewColumns();

            Refresh();
        }

        public override void Refresh()
        {
            base.Refresh();
            HKEY hKey = HKEY.HEKY_CURRENT_USER;
            IntPtr pRootKey = IntPtr.Zero;
            RegistryKey sSubKey = null;
            RegistryEnumKeyInfo KeyInfo = null;

            if (plugin.IsConnectionSuccess)
            {
                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                {
                    if (!plugin.Do_LogonUserSet())
                    {
                        Logger.Log("RegistryEditorPage.Refresh(): Failed to authenticate the specified user");
                        return;
                    }
                }
                else
                {
                    if (plugin != null && (plugin.handle == null || plugin.handle.Handle == IntPtr.Zero))
                    {
                        Logger.Log("Failed to get the Registry handle");
                        return;
                    }
                }
            }
            else
                return;

            switch (nodeType)
            {
                case RegistryViewerPlugin.NodeType.HKEY_CURRENT_CONFIG:

                    hKey = HKEY.HKEY_CURRENT_CONFIG;
                    plugin.RegRootKeySelected = treeNode.Text.Trim();
                    RegistryInteropWrapperWindows.Win32RegOpenRemoteBaseKey(
                                                        hKey,
                                                        out sSubKey);
                    break;

                case RegistryViewerPlugin.NodeType.HKEY_CURRENT_USER:

                    hKey = HKEY.HEKY_CURRENT_USER;
                    plugin.RegRootKeySelected = treeNode.Text.Trim();
                    RegistryInteropWrapperWindows.Win32RegOpenRemoteBaseKey(hKey,
                                                        out sSubKey);

                    //IntPtr pSD = RegistryInteropWrapperWindows.ApiRegGetKeySecurity(
                    //            RegistryInteropWrapperWindows.GetRegistryHive(HKEY.HEKY_CURRENT_USER),
                    //            "AppEvents1");
                    //SecurityDescriptor SecurityDescriptor = new SecurityDescriptor();
                    //SecurityDescriptorWrapper.ReadSecurityDescriptor(pSD, ref SecurityDescriptor);

                    //if (SecurityDescriptor.IsAccessDenied)
                    //{
                    //    MessageBox.Show("Access Denied", "Likewise Administrative Console", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    //    return;
                    //}
                    //PermissionsControlDlg dlg = new PermissionsControlDlg(SecurityDescriptor, "AppEvent1");
                    //if (dlg.ShowDialog(this) == DialogResult.OK)
                    //{
                    //    RegistryInteropWrapperWindows.ApiRegSetKeySecurity(
                    //            RegistryInteropWrapperWindows.GetRegistryHive(HKEY.HEKY_CURRENT_USER),
                    //            "AppEvents1",
                    //            SecurityDescriptor.pSecurityDescriptorOut);
                    //}
                    break;

                case RegistryViewerPlugin.NodeType.HKEY_LOCAL_MACHINE:

                    hKey = HKEY.HKEY_LOCAL_MACHINE;
                    plugin.RegRootKeySelected = treeNode.Text.Trim();
                    RegistryInteropWrapperWindows.Win32RegOpenRemoteBaseKey(hKey,
                                                        out sSubKey);
                    break;

                case RegistryViewerPlugin.NodeType.HKEY_USERS:

                    hKey = HKEY.HKEY_USERS;
                    plugin.RegRootKeySelected = treeNode.Text.Trim();
                    RegistryInteropWrapperWindows.Win32RegOpenRemoteBaseKey(hKey,
                                                        out sSubKey);
                    break;

                case RegistryViewerPlugin.NodeType.HKEY_CLASSES_ROOT:

                    hKey = HKEY.HKEY_CLASSES_ROOT;
                    plugin.RegRootKeySelected = treeNode.Text.Trim();
                    RegistryInteropWrapperWindows.Win32RegOpenRemoteBaseKey(hKey,
                                                        out sSubKey);
                    break;

                case RegistryViewerPlugin.NodeType.HKEY_LIKEWISE:
                    plugin.RegRootKeySelected = treeNode.Text.Trim();
                    hKey = HKEY.HKEY_LIKEWISE;
                    KeyInfo = treeNode.Tag as RegistryEnumKeyInfo;

					if(plugin.pRootHandle != IntPtr.Zero)
						RegistryInteropWrapper.ApiRegCloseKey(plugin.handle.Handle, plugin.pRootHandle);

                    RegistryInteropWrapper.ApiRegOpenKeyExW(plugin.handle.Handle,
                                                            IntPtr.Zero,
                                                            plugin.RegRootKeySelected,
                                                            out pRootKey);

					plugin.pRootHandle = pRootKey;
                    KeyInfo = new RegistryEnumKeyInfo();
                    KeyInfo.pRootKey = pRootKey;
                    KeyInfo.sKeyname = plugin.RegRootKeySelected;
                    treeNode.Tag = KeyInfo;
                    KeyInfo.OrigKey = plugin.RegRootKeySelected;

                    break;

                case RegistryViewerPlugin.NodeType.HKEY_LIKEWISE_SUBKEY:
                    hKey = HKEY.HKEY_LIKEWISE_SUBKEY;
                    KeyInfo = treeNode.Tag as RegistryEnumKeyInfo;
                    if (KeyInfo != null)
                    {
                        RegistryInteropWrapper.ApiRegOpenKeyExW(plugin.handle.Handle,
                                                            plugin.pRootHandle,
                                                            KeyInfo.sKeyname,
                                                            out pRootKey);
                        treeNode.Tag = KeyInfo;
                    }
					else
					{
						MessageBox.Show("Invalid key ");
					}
                    break;

                case RegistryViewerPlugin.NodeType.HKEY_SUBKEY:
                    SubKeyInfo subkeyInfo = treeNode.Tag as SubKeyInfo;
                    if (subkeyInfo != null)
                    {
                        sSubKey = subkeyInfo.sSubKey;
                        hKey = subkeyInfo.hKey;
                    }
                    break;

                case RegistryViewerPlugin.NodeType.NONE:
                default:
                    //For default displays
                    ListViewItem[] itemlist = new ListViewItem[treeNode.Nodes.Count];
                    int index = 0;

                    foreach (LACTreeNode node in treeNode.Nodes)
                    {
                        ListViewItem item = new ListViewItem(new string[] { node.Text });
                        itemlist[index] = item;
                        index++;
                    }
                    if (itemlist.Length != 0)
                        lvRegistryPage.Items.AddRange(itemlist);
                    break;
            }

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
            {
                EnumChildNodes(sSubKey, hKey);
                plugin.Do_LogonUserHandleClose();
            }
            else
            {
                Do_RegEnumKeys(pRootKey, KeyInfo);

				if(nodeType != RegistryViewerPlugin.NodeType.HKEY_LIKEWISE && pRootKey != IntPtr.Zero)
					RegistryInteropWrapper.ApiRegCloseKey(plugin.handle.Handle, pRootKey);
            }
        }

        #endregion

        #region Private helper functions

        private void Do_RegEnumKeys(IntPtr pRootKey, RegistryEnumKeyInfo KeyInfo)
        {
            if (pRootKey == IntPtr.Zero)
                return;

            List<RegistryEnumKeyInfo> keys = null;
            List<RegistryValueInfo> values = null;

            int ret = RegistryInteropWrapper.ApiRegQueryInfoEx(
                                    plugin.handle.Handle,
                                    pRootKey,
                                    out KeyInfo.sSubKeyCount,
                                    out KeyInfo.sValueCount);

            if (KeyInfo.sSubKeyCount != 0)
            {
                ret = RegistryInteropWrapper.ApiRegEnumKeyEx(
                                        plugin.handle.Handle,
                                        pRootKey,
                                        KeyInfo.sSubKeyCount,
                                        out keys);

            }

            if (KeyInfo.sValueCount != 0)
            {
                ret = RegistryInteropWrapper.ApiRegEnumValues(
                                       plugin.handle.Handle,
                                       pRootKey,
                                       KeyInfo.sValueCount,
                                       out values);
            }

            EnumChildNodes(keys, values);
		}

        private void EnumChildNodes(List<RegistryEnumKeyInfo> keys,
                                    List<RegistryValueInfo> values)
        {
			if(treeNode.IsModified)
			{
				treeNode.Nodes.Clear();
		treeNode.IsModified = false;
			}

            Dictionary<string, LACTreeNode> nodesAdded = new Dictionary<string, LACTreeNode>();
            Dictionary<string, LACTreeNode> nodesToAdd = new Dictionary<string, LACTreeNode>();

            foreach (LACTreeNode n in treeNode.Nodes) {
				RegistryEnumKeyInfo keyInfo = n.Tag as RegistryEnumKeyInfo;
				if(keyInfo != null)
				{
			nodesAdded.Add(keyInfo.sKeyname, n);
				}
			}

            if (keys != null && keys.Count != 0)
            {
                foreach (RegistryEnumKeyInfo key in keys)
                {
                    if (key == null)
                        continue;

					string keyname = key.sKeyname.LastIndexOf(@"\") <0 ?  key.sKeyname : key.sKeyname.Substring(key.sKeyname.LastIndexOf(@"\")+1).Trim();

					Icon ic = Properties.Resources.Reports;
                    LACTreeNode node = Manage.CreateIconNode(keyname,
                                      ic,
                                      typeof(RegistryViewerLikewiseSubKeyPage),
                                      plugin);

                    node.sc = treeNode.sc;
                    node.Tag = key;
                    if (!nodesAdded.ContainsKey(key.sKeyname))
                        nodesToAdd.Add(key.sKeyname, node);
                }
            }

            if (values != null && values.Count != 0)
            {
                foreach (RegistryValueInfo value in values)
                {
                    if (value == null)
                        continue;

                    value.sKeyname = treeNode.Text;
                    string sValuename = (String.IsNullOrEmpty(value.pValueName) ||
                                        (value.bIsDefault)) ?
                                                            "(Default)"
                                                            : value.pValueName;

                    ListViewItem lvItem = new ListViewItem(new string[] {
                                sValuename,
                                GetRegLinuxValueKindType(value.pType),
                                ((value.bDataBuf == null) || (String.IsNullOrEmpty(value.bDataBuf.ToString()))) ? "(value not set)" :  value.bDataBuf.ToString() });

                    lvItem.Tag = value;

                    if (value.bIsDefault)
                        lvRegistryPage.Items.Insert(0, lvItem);
                    else
                        lvRegistryPage.Items.Add(lvItem);
                }
            }

            LACTreeNode[] nodestoAddedRe = new LACTreeNode[nodesToAdd.Count];
            int idx = 0;
            if (nodesToAdd != null && nodesToAdd.Count != 0)
            {
                foreach (string key in nodesToAdd.Keys)
                {
                    nodestoAddedRe[idx] = nodesToAdd[key];
                    idx++;
                }
            }

            if (nodestoAddedRe != null && nodestoAddedRe.Length != 0)
                treeNode.Nodes.AddRange(nodestoAddedRe);

            lvRegistryPage.Sorting = SortOrder.None;
        }

        private void EnumChildNodes(RegistryKey sSubKey, HKEY hKey)
        {
            Array sSubKeys = null;
            Array sValues = null;
            Dictionary<string, LACTreeNode> nodesAdded = new Dictionary<string, LACTreeNode>();
            Dictionary<string, LACTreeNode> nodesToAdd = new Dictionary<string, LACTreeNode>();

            foreach (LACTreeNode n in treeNode.Nodes)
                nodesAdded.Add(n.Text.Trim(), n);

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
            {
                if (sSubKey != null)
                {
                    if (nodeType != RegistryViewerPlugin.NodeType.HKEY_SUBKEY)
                    {
                        SubKeyInfo subKeyInfo = new SubKeyInfo();
                        subKeyInfo.sKey = sSubKey.Name;
                        subKeyInfo.hKey = hKey;
                        subKeyInfo.sSubKey = sSubKey;
                        treeNode.Tag = subKeyInfo;
                    }
                    RegistryInteropWrapperWindows.Win32RegSubKeyList(sSubKey, out sSubKeys);

                    if (sSubKeys != null && sSubKeys.Length != 0)
                    {
                        foreach (string key in sSubKeys)
                        {
                            RegistryKey subKey = RegistryInteropWrapperWindows.Win32RegOpenRemoteSubKey(sSubKey, key);

                            Icon ic = Properties.Resources.Reports;
                            LACTreeNode node = Manage.CreateIconNode(key,
                                              ic,
                                              typeof(RegistryViewerKeyPage),
                                              plugin);
                            node.sc = plugin.GetPlugInNode().sc;

                            SubKeyInfo subKeyInfo = new SubKeyInfo();
                            subKeyInfo.sKey = key;
                            subKeyInfo.hKey = hKey;
                            subKeyInfo.sSubKey = subKey;

                            node.Tag = subKeyInfo;

                            if (!nodesAdded.ContainsKey(key.Trim()))
                                nodesToAdd.Add(key, node);
                        }
                    }

                    RegistryInteropWrapperWindows.Win32RegSubKeyValueList(sSubKey, out sValues);

                    ListViewItem lvItem = new ListViewItem(new string[] { "(Default)", "REG_SZ", "(value not set)" });
                    SubKeyValueInfo valueInfo = new SubKeyValueInfo();
                    valueInfo.hKey = hKey;
                    valueInfo.sParentKey = sSubKey;
                    valueInfo.sValue = "";
                    valueInfo.sData = "(value not set)";
                    valueInfo.RegDataType = LWRegistryValueKind.REG_SZ;
                    valueInfo.IsDefaultValue = true;

                    lvItem.Tag = valueInfo;
                    lvRegistryPage.Items.Add(lvItem);

                    if (sValues != null && sValues.Length != 0)
                    {
                        foreach (string value in sValues)
                        {
                            valueInfo = new SubKeyValueInfo();
                            RegistryInteropWrapperWindows.Win32RegValueKind(sSubKey, value, valueInfo);
                            valueInfo.hKey = hKey;
                            valueInfo.sParentKey = sSubKey;
                            valueInfo.sValue = value;
                            valueInfo.IsDefaultValue = false;

                            if (String.IsNullOrEmpty(value))
                            {
                                valueInfo.IsDefaultValue = true;
                                lvItem = new ListViewItem(new string[] { "(Default)", GetRegValueStringType(valueInfo.RegDataType), valueInfo.sData });
                                lvRegistryPage.Items.RemoveAt(0);
                            }
                            else
                                lvItem = new ListViewItem(new string[] { value, GetRegValueStringType(valueInfo.RegDataType), valueInfo.sData });

                            lvItem.Tag = valueInfo;
                            lvRegistryPage.Items.Add(lvItem);
                        }
                    }
                }
            }

            LACTreeNode[] nodestoAddedRe = new LACTreeNode[nodesToAdd.Count];
            int idx = 0;
            if (nodesToAdd != null && nodesToAdd.Count != 0)
            {
                foreach (string key in nodesToAdd.Keys) {
                    nodestoAddedRe[idx] = nodesToAdd[key];
                    idx++;
                }
            }

            if (nodestoAddedRe != null && nodestoAddedRe.Length != 0)
                treeNode.Nodes.AddRange(nodestoAddedRe);
        }

        private void AutoResize()
        {
            const int NUM_COLUMNS = 3;

            //pixels to use to give a visible margin between columns
            const int MARGIN = 10;

            lvRegistryPage.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);

            int minColumnWidth = (this.Width / NUM_COLUMNS) / 2;
            foreach (ColumnHeader ch in lvRegistryPage.Columns)
            {
                if (ch.Index != 0)
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

            //HACK: make sure that rightmost column is always covers
            //any remaining space on the right side of the list view
            lvRegistryPage.Columns[NUM_COLUMNS - 1].Width = this.Width;
        }

        private void SetListviewColumns()
        {
            if (lvRegistryPage.Columns.Count != 0)
            {
                lvRegistryPage.Columns.Clear();
            }
            this.lblCaption.Text = string.Concat(this.lblCaption.Text, " on ", System.Environment.MachineName);

            switch (nodeType)
            {
                case RegistryViewerPlugin.NodeType.HKEY_CURRENT_CONFIG:
                case RegistryViewerPlugin.NodeType.HKEY_CURRENT_USER:
                case RegistryViewerPlugin.NodeType.HKEY_LOCAL_MACHINE:
                case RegistryViewerPlugin.NodeType.HKEY_USERS:
                case RegistryViewerPlugin.NodeType.HKEY_CLASSES_ROOT:
                case RegistryViewerPlugin.NodeType.HKEY_SUBKEY:
                case RegistryViewerPlugin.NodeType.HKEY_LIKEWISE:
                case RegistryViewerPlugin.NodeType.HKEY_LIKEWISE_SUBKEY:
                case RegistryViewerPlugin.NodeType.NONE:
                    columnLabels = new string[] { "Name", "Type", "Data" };
                    break;
            }

            numColumns = columnLabels.Length;
            columnHeaders = new ColumnHeader[numColumns];
            for (int i = 0; i < numColumns; i++)
            {
                columnHeaders[i] = new ColumnHeader();
                columnHeaders[i].Text = columnLabels[i];
                columnHeaders[i].Width = 200;
            }

            this.lvRegistryPage.Columns.AddRange(columnHeaders);
        }

        public ContextMenu GetTreeContextMenu()
        {
            ContextMenu cm = new ContextMenu();

            MenuItem m_item = new MenuItem("Modify", new EventHandler(On_MenuClick));
            m_item.Tag = lvRegistryPage.SelectedItems[0].Tag;
            cm.MenuItems.Add(0, m_item);

            m_item = new MenuItem("Modify Binary Data", new EventHandler(On_MenuClick));
            m_item.Tag = lvRegistryPage.SelectedItems[0].Tag;
            cm.MenuItems.Add(1, m_item);

            m_item = new MenuItem("-");
            cm.MenuItems.Add(2, m_item);

            m_item = new MenuItem("&Delete", new EventHandler(On_MenuClick));
            m_item.Tag = lvRegistryPage.SelectedItems[0].Tag;
            cm.MenuItems.Add(3, m_item);

            m_item = new MenuItem("&Rename", new EventHandler(On_MenuClick));
            m_item.Tag = lvRegistryPage.SelectedItems[0].Tag;
            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                m_item.Enabled = !((SubKeyValueInfo)lvRegistryPage.SelectedItems[0].Tag).IsDefaultValue;
            else
                m_item.Enabled = !((RegistryValueInfo)lvRegistryPage.SelectedItems[0].Tag).bIsDefault;

            cm.MenuItems.Add(4, m_item);

            return cm;
        }

        public ContextMenu GetListViewContextMenu()
        {
            ContextMenu cm = new ContextMenu();

            MenuItem m_item = new MenuItem("&New", new EventHandler(On_MenuClick));
            m_item.Tag = treeNode;

            MenuItem m_Inneritem = new MenuItem("Key", new EventHandler(On_MenuClick));
            m_Inneritem.Tag = treeNode;
            m_item.MenuItems.Add(m_Inneritem);

            m_Inneritem = new MenuItem("-");
            m_item.MenuItems.Add(m_Inneritem);

            m_Inneritem = new MenuItem("String Value", new EventHandler(On_MenuClick));
            m_Inneritem.Tag = treeNode;
            m_item.MenuItems.Add(m_Inneritem);

            m_Inneritem = new MenuItem("Binary Value", new EventHandler(On_MenuClick));
            m_Inneritem.Tag = treeNode;
            m_item.MenuItems.Add(m_Inneritem);

            m_Inneritem = new MenuItem("DWORD Value", new EventHandler(On_MenuClick));
            m_Inneritem.Tag = treeNode;
            m_item.MenuItems.Add(m_Inneritem);

            m_Inneritem = new MenuItem("Multi-String Value", new EventHandler(On_MenuClick));
            m_Inneritem.Tag = treeNode;
            m_item.MenuItems.Add(m_Inneritem);

            m_Inneritem = new MenuItem("Expandable String Value", new EventHandler(On_MenuClick));
            m_Inneritem.Enabled = Configurations.currentPlatform == LikewiseTargetPlatform.Windows;
            m_Inneritem.Tag = treeNode;
            m_item.MenuItems.Add(m_Inneritem);

            cm.MenuItems.Add(m_item);

            return cm;
        }

        public void On_MenuClick(object sender, EventArgs e)
        {
            MenuItem mi = sender as MenuItem;
            LACTreeNode node = mi.Tag as LACTreeNode;

            //Since both are having the different set up proreties each. made two seperate objects.
            //For windows supported registry
            SubKeyValueInfo valueInfo = null;
            SubKeyInfo keyInfo = null;

            //For linux supported registry
            RegistryEnumKeyInfo regKeyInfo = null;
            RegistryValueInfo regValueInfo = null;

            if (mi != null)
            {
				if(mi.Text.Trim().Equals("&Refresh"))
				{
					 treeNode.IsModified = true;
                     treeNode.sc.ShowControl(treeNode);
                     return;
				}
				else if(mi.Text.Trim().Equals("&Refresh"))
				{
                     ProcessStartInfo psi = new ProcessStartInfo();
                     psi.UseShellExecute = true;
                     psi.FileName = CommonResources.GetString("LAC_Help");
                     psi.Verb = "open";
                     psi.WindowStyle = ProcessWindowStyle.Normal;
                     Process.Start(psi);
                     return;
				}
				else
				{
	                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
	                {
	                    keyInfo = node == null ? null : node.Tag as SubKeyInfo;
	                    valueInfo = node == null ? mi.Tag as SubKeyValueInfo : null;
	                }
	                else
					{
		                regKeyInfo = node == null ? null : node.Tag as RegistryEnumKeyInfo;
		                regValueInfo = node == null ? mi.Tag as RegistryValueInfo : null;

                        if (!node.Text.Trim().Equals(Properties.Resources.HKEY_THIS_MACHINE, StringComparison.InvariantCultureIgnoreCase))
                        {
			                if (regKeyInfo != null)
			                {
			                    RegistryInteropWrapper.ApiRegOpenKeyExW(plugin.handle.Handle,
			                                           plugin.pRootHandle,
			                                           regKeyInfo.sKeyname,
			                                           out regKeyInfo.pKey);
		                    }
							else  if (regValueInfo != null)
		                    {
								RegistryEnumKeyInfo valueKeyInfo = treeNode.Tag as RegistryEnumKeyInfo;

		                        RegistryInteropWrapper.ApiRegOpenKeyExW(plugin.handle.Handle,
	                                               plugin.pRootHandle,
	                                               valueKeyInfo.sKeyname,
	                                               out regValueInfo.pParentKey);
							}
						}
						else
						{
							if(regKeyInfo != null)
								regKeyInfo.pKey = plugin.pRootHandle;
							else if(regValueInfo != null)
								regValueInfo.pParentKey = plugin.pRootHandle;
						}
					}
				}

                switch (mi.Text.Trim())
                {
					//Modify
                    case "Modify":
                        if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                        {
                            valueInfo = mi.Tag is SubKeyValueInfo ? mi.Tag as SubKeyValueInfo : lvRegistryPage.SelectedItems[0].Tag as SubKeyValueInfo;
                            if (valueInfo != null)
                            {
                                DoEditorWork(valueInfo, false, (ulong)valueInfo.RegDataType);
                            }
                        }
                        else if (regValueInfo != null)
                            DoEditorWork(regValueInfo, false, regValueInfo.pType);

                        break;

                    //Modify Binary Data
                    case "Modify Binary Data":
                        valueInfo = mi.Tag is SubKeyValueInfo ? mi.Tag as SubKeyValueInfo : lvRegistryPage.SelectedItems[0].Tag as SubKeyValueInfo;
                        if (valueInfo != null)
                        {
                            SubKeyValueInfo tempValueInfo = new SubKeyValueInfo();
                            tempValueInfo.hKey = valueInfo.hKey;
                            tempValueInfo.RegDataType = LWRegistryValueKind.REG_BINARY;
                            tempValueInfo.sData = valueInfo.sData;
                            tempValueInfo.sDataBuf = valueInfo.sDataBuf;
                            tempValueInfo.sParentKey = valueInfo.sParentKey;
                            tempValueInfo.sValue = valueInfo.sValue;
                            DoEditorWork(tempValueInfo, false, (ulong)tempValueInfo.RegDataType);
                        }
                        else if (regValueInfo != null)
                            DoEditorWork(regValueInfo, false, (ulong)RegistryApi.REG_BINARY);
                        break;

                    //Delete
                    case "&Delete":
                        if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                        {
                            if (keyInfo != null) Do_DeleteKey(keyInfo, node);
                            else if (valueInfo != null) Do_DeleteKeyValue(valueInfo);
                        }
                        else
                        {
                            if (regKeyInfo != null) Do_DeleteKey(regKeyInfo, node);
                            else if (regValueInfo != null) Do_DeleteKeyValue(regValueInfo);
                        }
						break;

                    //Rename
                    case "&Rename":
                        if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                        {
                            if (keyInfo != null) Do_RenameKey(keyInfo, node);
                            else if (valueInfo != null) Do_RenameKeyValue(valueInfo, treeNode);
                        }
                        else
                        {
                            if (regKeyInfo != null) Do_RenameKey(regKeyInfo, node);
                            else if (regValueInfo != null) Do_RenameKeyValue(regValueInfo, treeNode);
                        }
						break;

                    //Key
                    case "Key":
                        if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                            Do_CreateKey(keyInfo, node);
                        else
                            Do_CreateKey(regKeyInfo, node);
                        break;

                    //&Import...
                    case "&Import...":
                        Do_ImportRegistry();
                        break;

                    //&Export...
                    case "&Export...":
                        Do_ExportRegistry();
                        break;

                    //String Value
                    case "String Value":
                        if (keyInfo != null)
                        {
                            valueInfo = new SubKeyValueInfo();
                            valueInfo.hKey = keyInfo.hKey;
                            valueInfo.IsDefaultValue = false;
                            valueInfo.RegDataType = LWRegistryValueKind.REG_SZ;
                            valueInfo.sParentKey = keyInfo.sSubKey;
                            DoEditorWork(valueInfo, true, (ulong)valueInfo.RegDataType);
                        }
                        else if (regKeyInfo != null)
                        {
                            regValueInfo = new RegistryValueInfo();
                            regValueInfo.pType = (ulong)RegistryApi.REG_SZ;
                            regValueInfo.pParentKey = regKeyInfo.pKey;
                            DoEditorWork(regValueInfo, true, regValueInfo.pType);
                        }
                        break;

                    //DWORD Value
                    case "DWORD Value":
                        if (keyInfo != null)
                        {
                            valueInfo = new SubKeyValueInfo();
                            valueInfo.hKey = keyInfo.hKey;
                            valueInfo.IsDefaultValue = false;
                            valueInfo.RegDataType = LWRegistryValueKind.REG_DWORD;
                            valueInfo.sParentKey = keyInfo.sSubKey;
                            DoEditorWork(valueInfo, true, (ulong)valueInfo.RegDataType);
                        }
                        else if (regKeyInfo != null)
                        {
                            regValueInfo = new RegistryValueInfo();
                            regValueInfo.pType = (ulong)RegistryApi.REG_DWORD;
                            regValueInfo.pParentKey = regKeyInfo.pKey;
                            DoEditorWork(regValueInfo, true, regValueInfo.pType);
                        }
                        break;

                    //Binary Value
                    case "Binary Value":
                        if (keyInfo != null)
                        {
                            valueInfo = new SubKeyValueInfo();
                            valueInfo.hKey = keyInfo.hKey;
                            valueInfo.IsDefaultValue = false;
                            valueInfo.RegDataType = LWRegistryValueKind.REG_BINARY;
                            valueInfo.sParentKey = keyInfo.sSubKey;
                            DoEditorWork(valueInfo, true, (ulong)valueInfo.RegDataType);
                        }
                        else if (regKeyInfo != null)
                        {
                            regValueInfo = new RegistryValueInfo();
                            regValueInfo.pType = (ulong)RegistryApi.REG_BINARY;
                            regValueInfo.pParentKey = regKeyInfo.pKey;
                            DoEditorWork(regValueInfo, true, regValueInfo.pType);
                        }
                        break;

                    //Multi-String Value
                    case "Multi-String Value":
                        if (keyInfo != null)
                        {
                            valueInfo = new SubKeyValueInfo();
                            valueInfo.hKey = keyInfo.hKey;
                            valueInfo.IsDefaultValue = false;
                            valueInfo.RegDataType = LWRegistryValueKind.REG_MULTI_SZ;
                            valueInfo.sParentKey = keyInfo.sSubKey;
                            DoEditorWork(valueInfo, true, (ulong)valueInfo.RegDataType);
                        }
                        else if (regKeyInfo != null)
                        {
                            regValueInfo = new RegistryValueInfo();
                            regValueInfo.pType = (ulong)RegistryApi.REG_MULTI_SZ;
                            regValueInfo.pParentKey = regKeyInfo.pKey;
                            DoEditorWork(regValueInfo, true, regValueInfo.pType);
                        }
                        break;

                    //Expandable String Value
                    case "Expandable String Value":
                        if (keyInfo != null)
                        {
                            valueInfo = new SubKeyValueInfo();
                            valueInfo.hKey = keyInfo.hKey;
                            valueInfo.IsDefaultValue = false;
                            valueInfo.RegDataType = LWRegistryValueKind.REG_EXPAND_SZ;
                            valueInfo.sParentKey = keyInfo.sSubKey;
                            DoEditorWork(valueInfo, true, (ulong)valueInfo.RegDataType);
                        }
                        else if (regKeyInfo != null)
                        {
                            regValueInfo = new RegistryValueInfo();
                            regValueInfo.pType = (ulong)RegistryApi.REG_EXPAND_SZ;
                            regValueInfo.pParentKey = regKeyInfo.pKey;
                            DoEditorWork(regValueInfo, true, regValueInfo.pType);
                        }
                        break;

                    default:
                        break;
                }
            }
        }

        private string GetRegValueStringType(LWRegistryValueKind valuekind)
        {
            string sRegType = string.Empty;

            switch (valuekind)
            {
                case LWRegistryValueKind.REG_BINARY:
                    sRegType = "REG_BINARY";
                    break;

                case LWRegistryValueKind.REG_DWORD:
                    sRegType = "REG_DWORD";
                    break;

                case LWRegistryValueKind.REG_EXPAND_SZ:
                    sRegType = "REG_EXPAND_SZ";
                    break;

                case LWRegistryValueKind.REG_MULTI_SZ:
                    sRegType = "REG_MULTI_SZ";
                    break;

                case LWRegistryValueKind.REG_NONE:
                    sRegType = "REG_NONE";
                    break;

                case LWRegistryValueKind.REG_QUADWORD:
                    sRegType = "REG_QWORD";
                    break;

                case LWRegistryValueKind.REG_RESOURCE_LIST:
                    sRegType = "REG_RESOURCE_LIST";
                    break;

                case LWRegistryValueKind.REG_SZ:
                    sRegType = "REG_SZ";
                    break;

                default :
                    sRegType = "REG_SZ";
                    break;
            }

            return sRegType;
        }

        private string GetRegLinuxValueKindType(ulong valuekind)
        {
            string sRegType = string.Empty;

            switch (valuekind)
            {
                case (ulong)RegistryApi.REG_BINARY:
                    sRegType = "REG_BINARY";
                    break;

                case (ulong)RegistryApi.REG_DWORD:
                    sRegType = "REG_DWORD";
                    break;

                case (ulong)RegistryApi.REG_EXPAND_SZ:
                    sRegType = "REG_EXPAND_SZ";
                    break;

                case (ulong)RegistryApi.REG_MULTI_SZ:
                    sRegType = "REG_MULTI_SZ";
                    break;

                case (ulong)RegistryApi.REG_NONE:
                    sRegType = "REG_NONE";
                    break;

                case (ulong)RegistryApi.REG_QWORD:
                    sRegType = "REG_QWORD";
                    break;

                case (ulong)RegistryApi.REG_RESOURCE_LIST:
                    sRegType = "REG_RESOURCE_LIST";
                    break;

                case (ulong)RegistryApi.REG_SZ:
                    sRegType = "REG_SZ";
                    break;

                case (ulong)RegistryApi.REG_RESOURCE_REQUIREMENTS_LIST:
                    sRegType = "REG_RESOURCE_REQUIREMENTS_LIST";
                    break;

                case (ulong)RegistryApi.REG_PLAIN_TEXT:
                    sRegType = "REG_PLAIN_TEXT";
                    break;

                case (ulong)RegistryApi.REG_FULL_RESOURCE_DESCRIPTOR:
                    sRegType = "REG_FULL_RESOURCE_DESCRIPTOR";
                    break;

                case (ulong)RegistryApi.REG_UNKNOWN:
                    sRegType = "REG_UNKNOWN";
                    break;

                default:
                    sRegType = "REG_UNKNOWN";
                    break;
            }

            return sRegType;
        }

        private void DoEditorWork(object valueInfo, bool bIsAdd, ulong dwType)
        {
            DialogResult dlg;

            try
            {
                if ((dwType == (ulong)LWRegistryValueKind.REG_BINARY) ||
                   (dwType == (ulong)LWRegistryValueKind.REG_NONE) ||
                   (dwType == (ulong)RegistryApi.REG_BINARY) ||
                   (dwType == (ulong)RegistryApi.REG_NONE))
                {
                    BinaryValueEditorDialog BinaryDialog = new BinaryValueEditorDialog(valueInfo, bIsAdd, plugin, treeNode.Text);
                    dlg = BinaryDialog.ShowDialog(this);
                }
                else if ((dwType == (ulong)LWRegistryValueKind.REG_DWORD) ||
                   (dwType == (ulong)RegistryApi.REG_DWORD))
                {
                    DWORDValueEditorDialog DwordDialog = new DWORDValueEditorDialog(valueInfo, bIsAdd);
                    dlg = DwordDialog.ShowDialog(this);
                }
                else if ((dwType == (ulong)LWRegistryValueKind.REG_QUADWORD) ||
                        (dwType == (ulong)RegistryApi.REG_QWORD))
                {
                    if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows && Environment.OSVersion.Version.Major <= 5)
                    {
                        BinaryValueEditorDialog BinaryDialog = new BinaryValueEditorDialog(valueInfo, bIsAdd, plugin, treeNode.Text);
                        dlg = BinaryDialog.ShowDialog(this);
                    }
                    else
                    {
                        DWORDValueEditorDialog DwordDialog = new DWORDValueEditorDialog(valueInfo, bIsAdd);
                        dlg = DwordDialog.ShowDialog(this);
                    }
                }
                else if ((dwType == (ulong)LWRegistryValueKind.REG_EXPAND_SZ) ||
                   (dwType == (ulong)LWRegistryValueKind.REG_SZ) ||
                   (dwType == (ulong)RegistryApi.REG_PLAIN_TEXT) ||
                   (dwType == (ulong)RegistryApi.REG_SZ) ||
                   (dwType == (ulong)RegistryApi.REG_EXPAND_SZ))
                {
                    StringEditorDialog StringDialog = new StringEditorDialog(valueInfo, bIsAdd);
                    dlg = StringDialog.ShowDialog(this);
                }
                else if ((dwType == (ulong)LWRegistryValueKind.REG_MULTI_SZ) ||
                    (dwType == (ulong)RegistryApi.REG_MULTI_SZ))
                {
                    MultiStringValueEditorDialog MultiStringDialog = new MultiStringValueEditorDialog(valueInfo, bIsAdd, plugin);
                    dlg = MultiStringDialog.ShowDialog(this);
                }
                else if ((dwType == (ulong)LWRegistryValueKind.REG_RESOURCE_LIST) ||
                  (dwType == (ulong)RegistryApi.REG_RESOURCE_LIST) ||
                  (dwType == (ulong)RegistryApi.REG_RESOURCE_REQUIREMENTS_LIST) ||
                  (dwType == (ulong)RegistryApi.REG_FULL_RESOURCE_DESCRIPTOR))
                {
                    ResourceListDialog ResListDialog = new ResourceListDialog(valueInfo, bIsAdd);
                    dlg = ResListDialog.ShowDialog(this);
                }
                else
                {
                    StringEditorDialog stringDialog = new StringEditorDialog(valueInfo, bIsAdd);
                    dlg = stringDialog.ShowDialog(this);
                }

                if (dlg == DialogResult.OK)
                {
                    if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                    {
                        SubKeyValueInfo ValueInfo = valueInfo as SubKeyValueInfo;
                        if (!Do_ModifyKeyValue(ValueInfo))
                        {
                            string sMsg = string.Format("The Registry Editor cannot rename or create name specified {0}.\n The specified value name already exists. Type another name and try again", ValueInfo.sValue);
                            container.ShowError(this, sMsg);
                            return;
                        }
                    }
                    else
                    {
                        RegistryValueInfo regValueInfo = valueInfo as RegistryValueInfo;
                        if (!Do_ModifyKeyValue(regValueInfo))
                        {
                            string sMsg = string.Format("The Registry Editor cannot rename or create name specified {0}.\n The specified value name already exists. Type another name and try again", regValueInfo.pValueName);
                            container.ShowError(this, sMsg);
                            return;
                        }
                    }

                    treeNode.IsModified = true;
                    treeNode.sc.ShowControl(treeNode);
                }
            }
            catch (Exception e)
            {
                Logger.LogException("DoEditorWork() :", e);
            }
        }

        private void Do_DeleteKeyValue(object valueInfo)
        {
            DialogResult dlg = MessageBox.Show(this, "Are you sure you want delete this key value?", "Likewise Aministrative Console",
                                            MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1);
            if (dlg == DialogResult.Yes)
            {
                if (valueInfo != null && valueInfo is SubKeyValueInfo)
                {
                    SubKeyValueInfo subKeyValueInfo = valueInfo as SubKeyValueInfo;
                    RegistryInteropWrapperWindows.Win32DeleteSubKeyValue(subKeyValueInfo.sParentKey, subKeyValueInfo.sValue);
                    lvRegistryPage.Items.Remove(lvRegistryPage.SelectedItems[0]);
                }
                else if (valueInfo != null && valueInfo is RegistryValueInfo)
                {
                    RegistryValueInfo regValueInfo = valueInfo as RegistryValueInfo;
                    int ret = RegistryInteropWrapper.ApiRegDeleteValue(plugin.handle.Handle, regValueInfo.pParentKey, regValueInfo.pValueName);
                    if (ret == 0)
                    {
                       if(!regValueInfo.sKeyname.Trim().Equals(Properties.Resources.HKEY_THIS_MACHINE))
					RegistryInteropWrapper.ApiRegCloseKey(plugin.handle.Handle, regValueInfo.pParentKey);

                        treeNode.IsModified = true;
                        treeNode.sc.ShowControl(treeNode);
                    }
                }
            }


        }

        private void Do_RenameKeyValue(object ValueInfo, LACTreeNode node)
        {
            try
            {
                if (ValueInfo != null && ValueInfo is SubKeyValueInfo)
                {
                    SubKeyValueInfo valueInfo = ValueInfo as SubKeyValueInfo;
                    RegistryAddSubKeyDlg renameDlg = new RegistryAddSubKeyDlg(valueInfo.sValue, false, valueInfo.sParentKey);
                    if (renameDlg.ShowDialog(this) == DialogResult.OK)
                    {
                        if (!string.IsNullOrEmpty(renameDlg.KeyName))
                        {
                            RegistryInteropWrapperWindows.Win32RegKeyValueData(valueInfo.sParentKey, valueInfo.sValue, out valueInfo.sDataBuf);
                            RegistryInteropWrapperWindows.Win32DeleteSubKeyValue(valueInfo.sParentKey, valueInfo.sValue);
                            valueInfo.sValue = renameDlg.KeyName;
                            RegistryInteropWrapperWindows.Win32ModifySubKeyValue(valueInfo);
                        }
                    }
                }
                else
                {
                    RegistryValueInfo regValueInfo = ValueInfo as RegistryValueInfo;

					try{

	                    RegistryAddSubKeyDlg renameDlg = new RegistryAddSubKeyDlg(regValueInfo.pValueName, false, regValueInfo.pParentKey);
	                    if (renameDlg.ShowDialog(this) == DialogResult.OK)
	                    {
	                        if (!string.IsNullOrEmpty(renameDlg.KeyName))
	                        {
	                            RegistryInteropWrapper.GetRegGetValueW(plugin.handle.Handle, regValueInfo);

	                            RegistryInteropWrapper.ApiRegDeleteValue(plugin.handle.Handle,
	                                            regValueInfo.pParentKey,
	                                            regValueInfo.pValueName);

	                            regValueInfo.pValueName = renameDlg.KeyName;
	                            RegistryInteropWrapper.ApiRegSetValueEx(plugin.handle.Handle,
	                                            regValueInfo.pParentKey,
	                                            regValueInfo.pValueName,
	                                            (uint)regValueInfo.pType,
	                                            regValueInfo.bDataBuf as byte[]);
	                        }
						}
					}catch{}

                    if (!regValueInfo.sKeyname.Trim().Equals(Properties.Resources.HKEY_THIS_MACHINE))
				RegistryInteropWrapper.ApiRegCloseKey(plugin.handle.Handle, regValueInfo.pParentKey);
                }

                treeNode.IsModified = true;
				treeNode.sc.ShowControl(treeNode);
            }
            catch (Exception ex)
            {
                Logger.LogException("Do_RenameKeyValue :", ex);
            }
        }

        private void Do_DeleteKey(object KeyInfo, LACTreeNode node)
        {
            DialogResult dlg = MessageBox.Show(this, "Are you sure you want delete this key and all of its subkeys?", "Likewise Aministrative Console",
                                             MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1);
            if (dlg == DialogResult.Yes)
            {
				// For windows support
                if (KeyInfo != null && KeyInfo is SubKeyInfo)
                {
                    SubKeyInfo keyInfo = KeyInfo as SubKeyInfo;
                    //Get the parent key to add the new key
                    RegistryKey parentKey = ((SubKeyInfo)(node.Parent as LACTreeNode).Tag).sSubKey;

                    if (keyInfo.sSubKey.SubKeyCount > 0)
                        RegistryInteropWrapperWindows.Win32DeleteSubKeyTree(parentKey, keyInfo.sKey);
                    else
                        RegistryInteropWrapperWindows.Win32DeleteSubKey(parentKey, keyInfo.sKey);
                }
				//for linux support
                else if (KeyInfo != null && KeyInfo is RegistryEnumKeyInfo)
                {
                    RegistryEnumKeyInfo keyInfo = KeyInfo as RegistryEnumKeyInfo;

					string keyname = keyInfo.sKeyname.LastIndexOf(@"\") < 0 ? keyInfo.sKeyname : keyInfo.sKeyname.Substring(keyInfo.sKeyname.LastIndexOf(@"\")+1);

					RegistryEnumKeyInfo parentKeyinfo = node.Parent.Tag as RegistryEnumKeyInfo;
					if(parentKeyinfo != null)
					{
                        if (!parentKeyinfo.sKeyname.Trim().Equals(Properties.Resources.HKEY_THIS_MACHINE))
						{
							RegistryInteropWrapper.ApiRegOpenKeyExW(plugin.handle.Handle,
							                                        plugin.pRootHandle,
							                                        parentKeyinfo.sKeyname,
							                                        out keyInfo.pRootKey);
						}
						else
						{
							keyInfo.pRootKey = plugin.pRootHandle;
						}

					}
                    if (keyInfo.sSubKeyCount != 0) {
                        RegistryInteropWrapper.ApiRegDeleteTree(plugin.handle.Handle, keyInfo.pRootKey, keyInfo.pKey, keyname);
                    }
                    else
                        RegistryInteropWrapper.ApiRegDeleteKey(plugin.handle.Handle, keyInfo.pRootKey, keyInfo.pKey, keyname);

                    if (!parentKeyinfo.sKeyname.Trim().Equals(Properties.Resources.HKEY_THIS_MACHINE))
			RegistryInteropWrapper.ApiRegCloseKey(plugin.handle.Handle, keyInfo.pRootKey);
                }

                LACTreeNode parentNode = node.Parent as LACTreeNode;
                parentNode.IsModified = true;
                parentNode.sc.ShowControl(parentNode);
            }
        }

        private void Do_RenameKey(object ObjInfo, LACTreeNode node)
        {
            if (ObjInfo is SubKeyInfo)
            {
                SubKeyInfo keyInfo = ObjInfo as SubKeyInfo;
                if (keyInfo != null)
                {
                    //Get the parent key to add the new key
                    RegistryKey parentKey = ((SubKeyInfo)(node.Parent as LACTreeNode).Tag).sSubKey;

                    RegistryAddSubKeyDlg renameDlg = new RegistryAddSubKeyDlg(keyInfo.sKey, true, parentKey);
                    if (renameDlg.ShowDialog(this) == DialogResult.OK)
                    {
                        if (!string.IsNullOrEmpty(renameDlg.KeyName))
                        {
                            // Create a key with the new Name
                            RegistryKey newKey = RegistryInteropWrapperWindows.Win32CreateSubKey(parentKey, renameDlg.KeyName);

                            // Move the old key values & subkeys to new key
                            Do_MoveSubKeys(keyInfo.sSubKey, newKey);

                            // Delete oldKey
                            if (parentKey.SubKeyCount > 0)
                                RegistryInteropWrapperWindows.Win32DeleteSubKeyTree(parentKey, keyInfo.sKey);
                            else
                                RegistryInteropWrapperWindows.Win32DeleteSubKey(parentKey, keyInfo.sKey);
                        }
                    }
                }
            }
            else if (ObjInfo != null && ObjInfo is RegistryEnumKeyInfo)
            {
                RegistryEnumKeyInfo keyInfo = ObjInfo as RegistryEnumKeyInfo;

                RegistryAddSubKeyDlg renameDlg = new RegistryAddSubKeyDlg(keyInfo.sKeyname, true, keyInfo.pKey);
                if (renameDlg.ShowDialog(this) == DialogResult.OK)
                {
                    if (!string.IsNullOrEmpty(renameDlg.KeyName))
                    {
						//get the parent key handle
						RegistryEnumKeyInfo parentKeyinfo = node.Parent.Tag as RegistryEnumKeyInfo;
						if(parentKeyinfo != null)
						{
                            if (!parentKeyinfo.sKeyname.Trim().Equals(Properties.Resources.HKEY_THIS_MACHINE))
							{
								RegistryInteropWrapper.ApiRegOpenKeyExW(plugin.handle.Handle,
								                                        plugin.pRootHandle,
								                                        parentKeyinfo.sKeyname,
								                                        out keyInfo.pRootKey);
							}
							else
							{
								keyInfo.pRootKey = plugin.pRootHandle;
							}
						}

						renameDlg.KeyName = renameDlg.KeyName.LastIndexOf(@"\") < 0 ? renameDlg.KeyName : renameDlg.KeyName.Substring(renameDlg.KeyName.LastIndexOf(@"\")+1);

                        // Create a key with the new Name
                        IntPtr phkResult = IntPtr.Zero;
                        RegistryInteropWrapper.ApiRegCreateKeyEx(plugin.handle.Handle, keyInfo.pRootKey, Marshal.StringToHGlobalUni(renameDlg.KeyName), out phkResult);

                        // Move the old key values & subkeys to new key
                        Do_MoveSubKeys(keyInfo, phkResult);

                        if (phkResult != IntPtr.Zero)
                            RegistryInteropWrapper.ApiRegCloseKey(plugin.handle.Handle, phkResult);

						keyInfo.sKeyname = keyInfo.sKeyname.LastIndexOf(@"\") < 0 ? keyInfo.sKeyname : keyInfo.sKeyname.Substring(keyInfo.sKeyname.LastIndexOf(@"\")+1);

                        // Delete oldKey
                        if (keyInfo.sSubKeyCount > 0) {
                            RegistryInteropWrapper.ApiRegDeleteTree(plugin.handle.Handle, keyInfo.pRootKey, keyInfo.pKey, keyInfo.sKeyname);
                        }
                        else
                            RegistryInteropWrapper.ApiRegDeleteKey(plugin.handle.Handle, keyInfo.pRootKey, keyInfo.pKey, keyInfo.sKeyname);

                        if (!parentKeyinfo.sKeyname.Trim().Equals(Properties.Resources.HKEY_THIS_MACHINE))
				RegistryInteropWrapper.ApiRegCloseKey(plugin.handle.Handle, keyInfo.pRootKey);
					}
                }
            }

            LACTreeNode parentnode = node.Parent as LACTreeNode;
            parentnode.IsModified = true;
            parentnode.sc.ShowControl(parentnode);
        }

        private bool Do_ModifyKeyValue(object objInfo)
        {
            bool bSuccess = false;

            if (objInfo != null && objInfo is SubKeyValueInfo)
            {
                SubKeyValueInfo valueInfo = objInfo as SubKeyValueInfo;
                valueInfo.sValue = valueInfo.IsDefaultValue ? "" : valueInfo.sValue;
                bSuccess = RegistryInteropWrapperWindows.Win32ModifySubKeyValue(valueInfo);
            }
            else if (objInfo != null && objInfo is RegistryValueInfo)
            {
                byte[] pData = null;
                RegistryValueInfo valueInfo = objInfo as RegistryValueInfo;

                RegistryInteropWrapper.RegModifyKeyValue(valueInfo, out pData);
                int ret = RegistryInteropWrapper.ApiRegSetValueEx(plugin.handle.Handle,
                                        valueInfo.pParentKey,
                                        valueInfo.pValueName,
                                        (uint)valueInfo.pType,
                                        pData);
				if(ret==(int)40158 && container != null )
				{
					container.ShowError(this, "Access denied");
				}

                bSuccess = (ret == 0);
            }

            return bSuccess;
        }

        private void Do_ImportRegistry()
        {
            FileStream fStream = null;
            StreamReader reader = null;

            try
            {
                string _fileName = string.Empty;
                string _fullPath = string.Empty;

                if (!plugin.IsConnectionSuccess)
                {
                    container.ShowMessage(string.Format("Registry plugin has not connected to any machine. " +
                                                        "Please connect to the host and try again"),
                                                        MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return;
                }

                OpenFileDialog openFileDialog = new OpenFileDialog();
                openFileDialog.AddExtension = true;
                openFileDialog.Filter = @"Registration Files(*.reg)|*.reg";
                openFileDialog.ValidateNames = true;
                if (openFileDialog.ShowDialog(this) == DialogResult.OK)
                {
                    _fileName = openFileDialog.FileName;
                    _fullPath = Path.GetFullPath(_fileName);

                    fStream = new FileStream(_fullPath, FileMode.OpenOrCreate, FileAccess.ReadWrite);
                    reader = new StreamReader(fStream);

                    Do_RecursiveImportKey(reader);

                    container.ShowMessage(string.Format("Information in {0} has been successfully entered into the registry", _fullPath),
                                          MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            }
            catch (Exception e)
            {
                Logger.LogException("RegistryEditorPage.Do_ImportRegistry", e);
            }
            finally
            {
                if (reader != null)
                    reader.Close();
                if (fStream != null)
                    fStream.Close();
            }
        }

        private void Do_ExportRegistry()
        {
            FileStream fStream = null;
            StreamWriter writer = null;

            try
            {
                Logger.Log("Exporting the key " + treeNode.Text.Trim() + "starts", Logger.RegistryViewerLoglevel);

                string _fileName = string.Empty;
                string _fullPath = string.Empty;

                SaveFileDialog savefileDailog = new SaveFileDialog();
                savefileDailog.AddExtension = true;
                savefileDailog.Filter = @"Registration Files(*.reg)|*.reg";
                savefileDailog.OverwritePrompt = true;
                savefileDailog.ValidateNames = true;
                if (savefileDailog.ShowDialog(this) == DialogResult.OK)
                {
                    _fileName = savefileDailog.FileName;
                    _fullPath = Path.GetFullPath(_fileName);

                    fStream = new FileStream(_fullPath, FileMode.OpenOrCreate, FileAccess.ReadWrite);
                    writer = new StreamWriter(fStream);

                    if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                    {
                        writer.WriteLine("Windows Registry Editor Version 5.00");
                        RegistryKey key = (treeNode.Tag as SubKeyInfo).sSubKey;
                        Do_RecursiveExportKey(writer, key);
                    }
                    else
                    {
                        writer.WriteLine("Likewise Registry Version");
                        Do_RecursiveExportKey(writer, treeNode.Tag as RegistryEnumKeyInfo);
                    }
                    writer.Flush();
                }
            }
            catch (Exception e)
            {
                Logger.LogException("RegistryEditorPage.Do_ExportRegistry", e);
            }
            finally
            {
                if (writer != null)
                    writer.Close();
                if (fStream != null)
                    fStream.Close();
            }
        }

        private void Do_RecursiveImportKey(StreamReader reader)
        {
            HKEY hKey = HKEY.HEKY_CURRENT_USER;
            RegistryKey sSubKey = null;
            List<LACTreeNode> KeyNodes = new List<LACTreeNode>();
            Hostinfo hn = ctx as Hostinfo;
            IntPtr pRootKey = IntPtr.Zero;
            LACTreeNode pluginNode = plugin.GetPlugInNode();

            if (pluginNode.Nodes.Count != 0 && Configurations.currentPlatform != LikewiseTargetPlatform.Windows)
            {
                Do_CloseRegKeyHandles(pluginNode.Nodes[1] as LACTreeNode);
                pluginNode.Nodes[1].Nodes.Clear();
            }

            while (!reader.EndOfStream)
            {
                string currentLine = reader.ReadLine();
                if (currentLine != null && currentLine.StartsWith("["))
                {
                    hKey = HKEY.HEKY_CURRENT_USER;
                    sSubKey = null;
                    LACTreeNode KeyNode = null;

                    string[] splits = currentLine.Split('\\');
                    if (splits != null && splits.Length != 0)
                    {
                        if (pRootKey != IntPtr.Zero)
                            RegistryInteropWrapper.ApiRegCloseKey(plugin.handle.Handle, pRootKey);

                        foreach (string sName in splits)
                        {
                            string sKeyName = sName.EndsWith("]") ? sName.Substring(0, sName.Length - 1) : sName;
                            sKeyName = sKeyName.StartsWith("[") ? sKeyName.Substring(1) : sKeyName;

                            //Check for the Win defined the HKEY
                            if (sName.StartsWith("["))
                            {
                                Icon ic = Properties.Resources.Reports;

                                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                                {
                                    switch (sKeyName.Trim().ToUpper())
                                    {
                                        case "HKEY_CLASSES_ROOT":
                                            hKey = HKEY.HKEY_CLASSES_ROOT;
                                            KeyNode = Manage.CreateIconNode(Properties.Resources.HKEY_CLASSES_ROOT,
                                                                             ic,
                                                                             typeof(RegistryViewerClassesPage),
                                                                             plugin);
                                            KeyNode.Tag = HKEY.HKEY_CLASSES_ROOT;
                                            KeyNodes.Add(KeyNode);
                                            break;

                                        case "HKEY_CURRENT_CONFIG":
                                            hKey = HKEY.HKEY_CURRENT_CONFIG;
                                            KeyNode = Manage.CreateIconNode(Properties.Resources.HKEY_CURRENT_CONFIG,
                                                                             ic,
                                                                             typeof(RegistryViewerConfigPage),
                                                                             plugin);
                                            KeyNode.Tag = HKEY.HKEY_CURRENT_CONFIG;
                                            KeyNodes.Add(KeyNode);
                                            break;

                                        case "HKEY_CURRENT_USER":
                                            hKey = HKEY.HEKY_CURRENT_USER;
                                            KeyNode = Manage.CreateIconNode(Properties.Resources.HKEY_CURRENT_USER,
                                                                             ic,
                                                                             typeof(RegistryViewerUserPage),
                                                                             plugin);
                                            KeyNode.Tag = HKEY.HEKY_CURRENT_USER;
                                            KeyNodes.Add(KeyNode);
                                            break;

                                        case "HKEY_LOCAL_MACHINE":
                                            hKey = HKEY.HKEY_LOCAL_MACHINE;
                                            KeyNode = Manage.CreateIconNode(Properties.Resources.HKEY_LOCAL_MACHINE,
                                                                             ic,
                                                                             typeof(RegistryViewerMachinePage),
                                                                             plugin);
                                            KeyNode.Tag = HKEY.HKEY_LOCAL_MACHINE;
                                            KeyNodes.Add(KeyNode);
                                            break;

                                        case "HKEY_USERS":
                                            hKey = HKEY.HKEY_USERS;
                                            KeyNode = Manage.CreateIconNode(Properties.Resources.HKEY_USERS,
                                                                             ic,
                                                                             typeof(RegistryViewerUsersPage),
                                                                             plugin);
                                            KeyNode.Tag = HKEY.HKEY_USERS;
                                            KeyNodes.Add(KeyNode);
                                            break;
                                    }
                                    RegistryInteropWrapperWindows.Win32RegOpenRemoteBaseKey(hKey,
                                          out sSubKey);
                                }
                                else
                                {
                                    plugin.RegRootKeySelected = Properties.Resources.HKEY_LIKEWISE_IMPORT;
                                    hKey = HKEY.HKEY_LIKEWISE;
                                    KeyNode = Manage.CreateIconNode(Properties.Resources.HKEY_LIKEWISE_IMPORT,
                                                                     ic,
                                                                     typeof(RegistryViewerLikewisePage),
                                                                     plugin);
                                    KeyNode.Tag = HKEY.HKEY_LIKEWISE;
                                    KeyNodes.Add(KeyNode);

                                    if (pluginNode.Nodes[1].Tag != null && pluginNode.Nodes[1].Tag is RegistryEnumKeyInfo)
                                    {
                                        RegistryEnumKeyInfo rootKeyInfo = pluginNode.Nodes[1].Tag as RegistryEnumKeyInfo;
                                        if (rootKeyInfo.pKey != IntPtr.Zero)
                                            pRootKey = rootKeyInfo.pKey;
                                        else
                                            RegistryInteropWrapper.ApiRegOpenKeyExW(plugin.handle.Handle,
                                                                                    IntPtr.Zero,
                                                                                    plugin.RegRootKeySelected,
                                                                                    out pRootKey);
                                    }
                                    else
                                    {
                                        RegistryInteropWrapper.ApiRegOpenKeyExW(plugin.handle.Handle,
                                                                                IntPtr.Zero,
                                                                                plugin.RegRootKeySelected,
                                                                                out pRootKey);
                                    }
                                    //Create the keys under Likewise_Import
                                    if (!sKeyName.Trim().ToUpper().Equals(plugin.RegRootKeySelected))
                                        RegistryInteropWrapper.ApiRegOpenKeyExW(plugin.handle.Handle, pRootKey,
                                            sKeyName, out pRootKey);
                                }
                            }
                            else if (sSubKey != null)
                            {
                                sSubKey = RegistryInteropWrapperWindows.Win32CreateSubKey(sSubKey, sKeyName);
                            }
                            else if (pRootKey != IntPtr.Zero)
                            {
                                IntPtr pSubKey = IntPtr.Zero;
                                RegistryInteropWrapper.ApiRegOpenKeyExW(plugin.handle.Handle, pRootKey, sKeyName, out pSubKey);
                                RegistryInteropWrapper.ApiRegCloseKey(plugin.handle.Handle, pRootKey);
                                pRootKey = pSubKey;
                            }
                        }
                    }
                }
                else if (currentLine != null && currentLine.Contains("="))
                {
                    string[] splits = currentLine.Split('=');
                    if (splits != null && splits.Length != 0)
                    {
                        if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                        {
                            SubKeyValueInfo valueInfo = new SubKeyValueInfo();
                            valueInfo.hKey = hKey;
                            valueInfo.IsDefaultValue = false;

                            if (splits[0].StartsWith("\""))
                                valueInfo.sValue = splits[0].Substring(1, splits[0].Length - 2);
                            else if (splits[0].Equals("@"))
                                valueInfo.sValue = "";
                            else
                                valueInfo.sValue = splits[0];

                            valueInfo.sParentKey = sSubKey;
                            valueInfo.hKey = hKey;
                            RegistryHelper.GetValueKind(valueInfo, splits[1]);

                            if ((valueInfo.RegDataType == LWRegistryValueKind.REG_BINARY) ||
                              (valueInfo.RegDataType == LWRegistryValueKind.REG_MULTI_SZ) ||
                              (valueInfo.RegDataType == LWRegistryValueKind.REG_EXPAND_SZ) ||
                              (valueInfo.RegDataType == LWRegistryValueKind.REG_RESOURCE_LIST))
                            {
                                while (!reader.EndOfStream && currentLine.Contains(@"\"))
                                {
                                    splits[1] += currentLine;
                                    currentLine = reader.ReadLine();
                                }
                                splits[1] += currentLine;
                                splits[1] = splits[1].Replace('\\', ',');
                                splits[1] = splits[1].Replace(" ", "");
                                splits[1] = splits[1].EndsWith(",") ? splits[1].Substring(0, splits[1].Length - 1) : splits[1];
                            }

                            RegistryHelper.GetValueData(valueInfo, splits[1]);
                            RegistryInteropWrapperWindows.Win32AddSubKeyValue(valueInfo);
                        }
                        else
                        {
                            RegistryValueInfo regValueInfo = new RegistryValueInfo();
                            regValueInfo.pParentKey = pRootKey;

                            if (splits[0].StartsWith("\""))
                                regValueInfo.pValueName = splits[0].Substring(1, splits[0].Length - 2);
                            else
                                regValueInfo.pValueName = splits[0];

                            RegistryHelper.GetValueKind(regValueInfo, splits[1]);

                            if ((regValueInfo.pType == (ulong)RegistryApi.REG_BINARY) ||
                             (regValueInfo.pType == (ulong)RegistryApi.REG_MULTI_SZ) ||
                             (regValueInfo.pType == (ulong)RegistryApi.REG_EXPAND_SZ) ||
                             (regValueInfo.pType == (ulong)RegistryApi.REG_RESOURCE_LIST) ||
                             (regValueInfo.pType == (ulong)RegistryApi.REG_RESOURCE_REQUIREMENTS_LIST) ||
                             (regValueInfo.pType == (ulong)RegistryApi.REG_FULL_RESOURCE_DESCRIPTOR))
                            {
                                while (!reader.EndOfStream && currentLine.Contains(@"\"))
                                {
                                    splits[1] += currentLine;
                                    currentLine = reader.ReadLine();
                                }
                                splits[1] += currentLine;
                                splits[1] = splits[1].Replace('\\', ',');
                                splits[1] = splits[1].Replace(" ", "");
                                splits[1] = splits[1].EndsWith(",") ? splits[1].Substring(0, splits[1].Length - 1) : splits[1];
                            }

                            RegistryHelper.GetValueData(regValueInfo, splits[1]);
                            RegistryInteropWrapper.ApiRegSetValueEx(plugin.handle.Handle,
                                                   regValueInfo.pParentKey,
                                                   regValueInfo.pValueName,
                                                   (uint)regValueInfo.pType,
                                                    regValueInfo.bDataBuf as byte[]);
                        }
                    }
                }
            }
        }

        private void Do_RecursiveExportKey(StreamWriter writer, object phKey)
        {
            writer.WriteLine();

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
            {
                RegistryKey key = phKey as RegistryKey;
                try
                {
                    writer.WriteLine("[" + key.ToString() + "]");

                    Do_ExportKeyValues(writer, key, 0);

                    if (key.SubKeyCount != 0)
                    {
                        foreach (string subKeyName in key.GetSubKeyNames())
                        {
                            RegistryKey SubKey = key.OpenSubKey(subKeyName);
                            Do_RecursiveExportKey(writer, SubKey);
                        }
                    }
                }
                catch (Exception e)
                {
                    Logger.LogException("Do_RecursiveExportKey()| Exception occured for the Key :" + key.ToString(), e);
                }
            }
            else
            {
                RegistryEnumKeyInfo keyInfo = phKey as RegistryEnumKeyInfo;
                uint dwSubKeyCount, dwValueCount = 0;

                try
                {
                    writer.WriteLine("[" + keyInfo.OrigKey + "]");

                    if (keyInfo.sKeyname.Trim().ToUpper().Equals(Properties.Resources.HKEY_LIKEWISE))
                    {
                        if (keyInfo.pKey == IntPtr.Zero)
                            RegistryInteropWrapper.ApiRegOpenKeyExW(plugin.handle.Handle,
                                                                    IntPtr.Zero,
                                                                    keyInfo.OrigKey,
                                                                    out keyInfo.pKey);
                    }
                    else
                    {
                        if (keyInfo.pKey == IntPtr.Zero)
                            RegistryInteropWrapper.ApiRegOpenKeyExW(plugin.handle.Handle,
                                                                         keyInfo.pRootKey,
                                                                         keyInfo.sKeyname,
                                                                         out keyInfo.pKey);
                    }

                    RegistryInteropWrapper.ApiRegQueryInfoEx(
                                   plugin.handle.Handle,
                                   keyInfo.pKey,
                                   out dwSubKeyCount,
                                   out dwValueCount);

                    Do_ExportKeyValues(writer, keyInfo, dwValueCount);

                    if (dwSubKeyCount != 0)
                    {
                        List<RegistryEnumKeyInfo> keys = new List<RegistryEnumKeyInfo>();

                        RegistryInteropWrapper.ApiRegEnumKeyEx(
                                                plugin.handle.Handle,
                                                keyInfo.pKey,
                                                dwSubKeyCount,
                                                out keys);

                        if (keys != null && keys.Count != 0)
                        {
                            foreach (RegistryEnumKeyInfo regSubKey in keys)
                            {
                                Do_RecursiveExportKey(writer, regSubKey);
                                RegistryInteropWrapper.ApiRegCloseKey(plugin.handle.Handle, regSubKey.pKey);
                            }
                        }
                    }
                }
                catch (Exception e)
                {
                    Logger.LogException("Do_RecursiveExportKey()| Exception occured for the Key :" + keyInfo.sKeyname, e);
                }
            }
        }

        private void Do_ExportKeyValues(StreamWriter writer, object ObjInfo, uint valuecount)
        {
            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
            {
                SubKeyValueInfo valueInfo = null;
                RegistryKey key = ObjInfo as RegistryKey;

                if (key.ValueCount != 0)
                {
                    foreach (string sValueName in key.GetValueNames())
                    {
                        try
                        {
                            valueInfo = new SubKeyValueInfo();
                            valueInfo.sValue = sValueName;
                            valueInfo.sParentKey = key;
                            RegistryInteropWrapperWindows.Win32RegValueKind(key, sValueName, valueInfo);
                            string sValue = String.IsNullOrEmpty(valueInfo.sValue) ?
                                                        "@" :
                                                        string.Concat("\"", valueInfo.sValue, "\"");
                            writer.WriteLine(String.Concat(sValue, "=", RegistryHelper.GetFormatSpecificType((ulong)valueInfo.RegDataType), RegistryHelper.GetFormatSpecificData(valueInfo)));
                        }
                        catch (Exception e)
                        {
                            Logger.LogException("Do_ExportKeyValues()| Exception occured for the KeyValue :" + sValueName, e);
                        }
                    }
                }
            }
            else
            {
                if (valuecount != 0)
                {
                    List<RegistryValueInfo> values = new List<RegistryValueInfo>();
                    RegistryEnumKeyInfo keyInfo = ObjInfo as RegistryEnumKeyInfo;

                    RegistryInteropWrapper.ApiRegEnumValues(
                                        plugin.handle.Handle,
                                        keyInfo.pKey,
                                        valuecount,
                                        out values);

                    if (values != null && values.Count != 0)
                    {
                        foreach (RegistryValueInfo value in values)
                        {
                            if (!String.IsNullOrEmpty(value.pValueName))
                            {
                                try
                                {
                                    RegistryInteropWrapper.ApiRegGetValue(plugin.handle.Handle, value, out value.bDataBuf);
                                    string sRegValueName = value.pValueName.Equals("@") ?
                                                        value.pValueName :
                                                        string.Concat("\"", value.pValueName, "\"");
                                    writer.WriteLine(String.Concat(sRegValueName, "=", RegistryHelper.GetFormatSpecificType(value.pType), RegistryHelper.GetFormatSpecificData(value)));
                                }
                                catch (Exception e)
                                {
                                    Logger.LogException("Do_ExportKeyValues()| Exception occured for the KeyValue :" + value.pValueName, e);
                                }
                            }
                        }
                    }
                }
            }
        }

        private bool Do_AddKeyValue(SubKeyValueInfo valueInfo)
        {
            bool bSuccess = false;

            if (valueInfo != null)
            {
                bSuccess = RegistryInteropWrapperWindows.Win32AddSubKeyValue(valueInfo);
            }

            return bSuccess;
        }

        private void Do_MoveSubKeys(RegistryKey oldKey, RegistryKey newKey)
        {
            // Copy the Key Names to newKey
            if (oldKey.ValueCount != 0)
            {
                foreach (string valueName in oldKey.GetValueNames())
                {
                    object value = oldKey.GetValue(valueName);
                    RegistryValueKind valueKind = oldKey.GetValueKind(valueName);
                    newKey.SetValue(valueName, value, valueKind);
                }
            }

            // Copy All the subkeys of oldKey to newKey
            if (oldKey.SubKeyCount != 0)
            {
                foreach (string subKeyName in oldKey.GetSubKeyNames())
                {
                    RegistryKey oldSubKey = oldKey.OpenSubKey(subKeyName);
                    RegistryKey newSubKey = newKey.CreateSubKey(subKeyName);
                    Do_MoveSubKeys(oldSubKey, newSubKey);
                }
            }
        }

        private void Do_MoveSubKeys(RegistryEnumKeyInfo oldKey, IntPtr newKey)
        {
            List<RegistryValueInfo> values = new List<RegistryValueInfo>();
            List<RegistryEnumKeyInfo> keys = new List<RegistryEnumKeyInfo>();

            // Copy the Value names to newKey
            if (oldKey.sValueCount != 0)
            {
                RegistryInteropWrapper.ApiRegEnumValues(
                                        plugin.handle.Handle,
                                        oldKey.pKey,
                                        oldKey.sValueCount,
                                        out values);

                if (values != null && values.Count != 0)
                {
                    foreach (RegistryValueInfo value in values)
                    {
						value.pParentKey = oldKey.pKey;

                        RegistryInteropWrapper.GetRegGetValueW(plugin.handle.Handle, value);

                        RegistryInteropWrapper.ApiRegSetValueEx(
                                        plugin.handle.Handle,
                                        newKey,
                                        value.pValueName,
                                        (uint)value.pType,
                                        value.bDataBuf as byte[]);
                    }
                }
            }

            // Copy All the subkeys of oldKey to newKey
            if (oldKey.sSubKeyCount != 0)
            {
                RegistryInteropWrapper.ApiRegEnumKeyEx(
                                        plugin.handle.Handle,
                                        oldKey.pKey,
                                        oldKey.sSubKeyCount,
                                        out keys);

                if (keys != null && keys.Count != 0)
                {
                    IntPtr phkResult;
                    foreach (RegistryEnumKeyInfo key in keys)
                    {
                        phkResult = IntPtr.Zero;
                        RegistryInteropWrapper.ApiRegCreateKeyEx(
                                        plugin.handle.Handle,
                                        newKey,
                                        Marshal.StringToHGlobalUni(key.sKeyname.LastIndexOf(@"\") < 0 ? key.sKeyname : key.sKeyname.Substring(key.sKeyname.LastIndexOf(@"\")+1)),
                                        out phkResult);

                        if (key.pKey == IntPtr.Zero)
                        {
                            RegistryInteropWrapper.ApiRegOpenKeyExW(
                                                  plugin.handle.Handle,
                                                  plugin.pRootHandle,
                                                  key.sKeyname,
                                                  out key.pKey);
                        }

                        RegistryInteropWrapper.ApiRegQueryInfoEx(
                                   plugin.handle.Handle,
                                   key.pKey,
                                   out key.sSubKeyCount,
                                   out key.sValueCount);

                        Do_MoveSubKeys(key, phkResult);

                        if (phkResult != IntPtr.Zero)
                            RegistryInteropWrapper.ApiRegCloseKey(
                                        plugin.handle.Handle, phkResult);

                        if (key.pKey != IntPtr.Zero)
                            RegistryInteropWrapper.ApiRegCloseKey(plugin.handle.Handle, key.pKey);
                    }
                }
            }
        }

        private void Do_CreateKey(SubKeyInfo keyInfo, LACTreeNode node)
        {
            if (keyInfo != null)
            {
                RegistryAddSubKeyDlg addDlg = new RegistryAddSubKeyDlg(string.Empty, true, keyInfo.sSubKey);
                if (addDlg.ShowDialog(this) == DialogResult.OK)
                {
                    RegistryInteropWrapperWindows.Win32CreateSubKey(keyInfo.sSubKey, addDlg.KeyName);

                    treeNode.IsModified = true;
                    treeNode.sc.ShowControl(treeNode);
                }
            }
        }

        private void Do_DeleteTree(RegistryEnumKeyInfo keyInfo)
        {
            if (keyInfo != null)
            {
                if (keyInfo.sSubKeyCount != 0)
                {
                    List<RegistryEnumKeyInfo> keys = new List<RegistryEnumKeyInfo>();

                    RegistryInteropWrapper.ApiRegEnumKeyEx(plugin.handle.Handle,
                                                keyInfo.pKey,
                                                keyInfo.sSubKeyCount,
                                                out keys);

                    if (keys != null && keys.Count != 0)
                    {
                        foreach (RegistryEnumKeyInfo key in keys)
                        {
                            if (key.pKey == IntPtr.Zero)
                            {
                                RegistryInteropWrapper.ApiRegOpenKeyExW(
                                                    plugin.handle.Handle,
                                                    key.pRootKey,
                                                    key.sKeyname,
                                                    out key.pKey);
                            }

                            RegistryInteropWrapper.ApiRegQueryInfoEx(
                                                plugin.handle.Handle,
                                                key.pKey,
                                                out key.sSubKeyCount,
                                                out key.sValueCount);

                            if (keyInfo.sSubKeyCount != 0)
                                Do_DeleteTree(key);

                            RegistryInteropWrapper.ApiRegDeleteKey(
                                        plugin.handle.Handle,
                                        key.pRootKey,
                                        key.pKey,
                                        key.sKeyname);
                        }
                    }
                }
            }
        }

        private void Do_CloseRegKeyHandles(LACTreeNode node)
        {
            foreach (LACTreeNode lnode in node.Nodes)
            {
                if (lnode.Nodes.Count == 0)
                {
                    RegistryEnumKeyInfo keyInfo = lnode.Tag as RegistryEnumKeyInfo;
                    if (keyInfo.pKey != IntPtr.Zero)
                        RegistryInteropWrapper.ApiRegCloseKey(
                                    plugin.handle.Handle,
                                    keyInfo.pKey);
					keyInfo.pKey=IntPtr.Zero;
                }
                else
                {
                    Do_CloseRegKeyHandles(lnode);

                    RegistryEnumKeyInfo keyInfo = lnode.Tag as RegistryEnumKeyInfo;
                    if (keyInfo.pKey != IntPtr.Zero)
                        RegistryInteropWrapper.ApiRegCloseKey(
                                    plugin.handle.Handle,
                                    keyInfo.pKey);
					keyInfo.pKey=IntPtr.Zero;
                }

            }
        }

        private void Do_CloseRegTree(RegistryEnumKeyInfo keyInfo)
        {
            if (keyInfo != null)
            {
                if (keyInfo.sSubKeyCount != 0)
                {
                    List<RegistryEnumKeyInfo> keys = new List<RegistryEnumKeyInfo>();

                    RegistryInteropWrapper.ApiRegEnumKeyEx(plugin.handle.Handle,
                                                keyInfo.pKey,
                                                keyInfo.sSubKeyCount,
                                                out keys);

                    if (keys != null && keys.Count != 0)
                    {
                        foreach (RegistryEnumKeyInfo key in keys)
                        {
                            if (key.pKey == IntPtr.Zero)
                            {
                                RegistryInteropWrapper.ApiRegOpenKeyExW(
                                                    plugin.handle.Handle,
                                                    key.pRootKey,
                                                    key.sKeyname,
                                                    out key.pKey);
                            }

                            RegistryInteropWrapper.ApiRegQueryInfoEx(
                                                plugin.handle.Handle,
                                                key.pKey,
                                                out key.sSubKeyCount,
                                                out key.sValueCount);

                            if (keyInfo.sSubKeyCount != 0)
                                Do_DeleteTree(key);

                            RegistryInteropWrapper.ApiRegDeleteKey(
                                        plugin.handle.Handle,
                                        key.pRootKey,
                                        key.pKey,
                                        key.sKeyname);
                        }
                    }
                }
            }
        }

        private void Do_CreateKey(RegistryEnumKeyInfo keyInfo, LACTreeNode node)
        {
            if (keyInfo != null)
            {
                RegistryAddSubKeyDlg addDlg = new RegistryAddSubKeyDlg(string.Empty, true, keyInfo);
                if (addDlg.ShowDialog(this) == DialogResult.OK)
                {
                    IntPtr phkResult = IntPtr.Zero;
                    int ret = RegistryInteropWrapper.ApiRegCreateKeyEx(plugin.handle.Handle, keyInfo.pKey, Marshal.StringToHGlobalUni(addDlg.KeyName), out phkResult);
                    if (ret == 0)
                    {
                        if (phkResult != IntPtr.Zero)
                            RegistryInteropWrapper.ApiRegCloseKey(plugin.handle.Handle, phkResult);

                        if (!keyInfo.sKeyname.Trim().Equals(Properties.Resources.HKEY_THIS_MACHINE))
							RegistryInteropWrapper.ApiRegCloseKey(plugin.handle.Handle, keyInfo.pKey);

                        node.IsModified = true;
                        node.sc.ShowControl(node);
                    }
                    else
                    {
                        container.ShowError(this, "Registry cannot create the key with the name specified.\n It is because of either the name exists or parent key info is wrong");
                    }
                }
            }
        }

        #endregion

        #region Event handlers

        private ListViewItem GetSelectedItem()
        {
            if (lvRegistryPage.SelectedItems.Count != 1)
            {
                return null;
            }

            return lvRegistryPage.SelectedItems[0];
        }

        private void lvRegistryPage_MouseUp(object sender, MouseEventArgs e)
        {
            ListView lvSender = sender as ListView;
            if (lvSender != null && e.Button == MouseButtons.Right)
            {
                ListViewHitTestInfo hti = null;
                try
                {
                    hti = lvSender.HitTest(e.X, e.Y);
                }
                catch (ObjectDisposedException)
                {
                    return;
                }
                if (hti != null && hti.Item != null)
                {
                    ListViewItem lvitem = hti.Item;
                    if (!lvitem.Selected)
                    {
                        lvitem.Selected = true;
                    }
                    if (e.Button == MouseButtons.Right)
                    {
                        ContextMenu cm = GetTreeContextMenu();
                        if (cm != null)
                        {
                            cm.Show(lvRegistryPage, new Point(e.X, e.Y));
                        }
                    }
                }
                else
                {
                    ContextMenu cm = GetListViewContextMenu();
                    if (cm != null)
                    {
                        cm.Show(lvSender, new Point(e.X, e.Y));
                    }
                }
            }
        }

        private void lvRegistryPage_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            if (lvRegistryPage.SelectedItems.Count == 0)
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
                    if (lvItem.Tag != null && lvItem.Tag is SubKeyValueInfo)
                    {
                        SubKeyValueInfo valueInfo = lvItem.Tag as SubKeyValueInfo;
                        if (valueInfo != null)
                        {
                            DoEditorWork(valueInfo, false, (ulong)valueInfo.RegDataType);
                        }
                    }
                    if (lvItem.Tag != null && lvItem.Tag is RegistryValueInfo)
                    {
                        RegistryValueInfo regValueInfo = lvItem.Tag as RegistryValueInfo;

                        if (regValueInfo != null)
                        {
                            if (!treeNode.Text.Trim().Equals(Properties.Resources.HKEY_THIS_MACHINE, StringComparison.InvariantCultureIgnoreCase))
                            {
                                RegistryInteropWrapper.ApiRegOpenKeyExW(plugin.handle.Handle,
                                                   plugin.pRootHandle,
                                                   regValueInfo.sKeyname,
                                                   out regValueInfo.pParentKey);
                            }
                            else {
                                regValueInfo.pParentKey = plugin.pRootHandle;
                            }
                            DoEditorWork(regValueInfo, false, (ulong)regValueInfo.pType);
                        }
                    }
                }
            }
        }

        private void lvRegistryPage_ColumnClick(object sender, ColumnClickEventArgs e)
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
            //this.lvRegistryPage.Sort();
            this.lvRegistryPage.Sorting = SortOrder.None;
        }

        #endregion

    }

    public class RegistryViewerClassesPage : RegistryEditorPage
    {
        public RegistryViewerClassesPage()
            : base(RegistryViewerPlugin.NodeType.HKEY_CLASSES_ROOT)
        {
        }
    }
    public class RegistryViewerUserPage : RegistryEditorPage
    {
        public RegistryViewerUserPage()
            : base(RegistryViewerPlugin.NodeType.HKEY_CURRENT_USER)
        {
        }
    }

    public class RegistryViewerMachinePage : RegistryEditorPage
    {
        public RegistryViewerMachinePage()
            : base(RegistryViewerPlugin.NodeType.HKEY_LOCAL_MACHINE)
        {
        }
    }

    public class RegistryViewerUsersPage : RegistryEditorPage
    {
        public RegistryViewerUsersPage()
            : base(RegistryViewerPlugin.NodeType.HKEY_USERS)
        {
        }
    }

    public class RegistryViewerConfigPage : RegistryEditorPage
    {
        public RegistryViewerConfigPage()
            : base(RegistryViewerPlugin.NodeType.HKEY_CURRENT_CONFIG)
        {
        }
    }

    public class RegistryViewerKeyPage : RegistryEditorPage
    {
        public RegistryViewerKeyPage()
            : base(RegistryViewerPlugin.NodeType.HKEY_SUBKEY)
        {
        }
    }

    public class RegistryViewerLikewisePage : RegistryEditorPage
    {
        public RegistryViewerLikewisePage()
            : base(RegistryViewerPlugin.NodeType.HKEY_LIKEWISE)
        {
        }
    }

    public class RegistryViewerLikewiseSubKeyPage : RegistryEditorPage
    {
        public RegistryViewerLikewiseSubKeyPage()
            : base(RegistryViewerPlugin.NodeType.HKEY_LIKEWISE_SUBKEY)
        {
        }
    }

    public interface IDirectoryPropertiesPage
    {
        void SetData(Likewise.LMC.Utilities.CredentialEntry ce, string sharename, object Object);
    }
}
