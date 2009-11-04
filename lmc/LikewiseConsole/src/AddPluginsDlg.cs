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
using System.IO;
using System.Reflection;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;
#if QUARTZ
#if LMC_ENTERPRISE
using Likewise.LMC.Plugins.GPOEPlugin;
#endif
#endif
using System.Diagnostics;

namespace Likewise.LMC
{
    public partial class AddPluginsDlg : Form, IComparer<ListViewItem>
    {

        #region data

#if !QUARTZ
        private const string sRootPluginName = @"Likewise.LMC.Plugins.RootPlugin.dll";
#else
        private const string sRootPluginName = @"Likewise.LMC.Plugins.RootPlugin.unix.dll";
#endif
        private Manage _controlManage = null;
        private LACTreeNode _rootNode = null;
        private PluginStandalonePage _standalonePage = null;
        private List<LACTreeNode> _addedPlugins = new List<LACTreeNode>();
        private AddRemovePluginDlg _grandParent;
        public  ListView PluginslistView = null;

        #endregion

        #region accessors
        public List<LACTreeNode> addedPlugins
        {
            get
            {
                return _addedPlugins;
            }
        }
        #endregion

        public AddPluginsDlg(AddRemovePluginDlg grandparentDlg, Manage controlManage, LACTreeNode rootNode, PluginStandalonePage standalonePage,ListView listView)
        {
            InitializeComponent();

            this.Text = "Add Stand-Alone Plug-In";

            _controlManage = controlManage;
            _rootNode = rootNode;
            _standalonePage = standalonePage;
            _grandParent = grandparentDlg;
            this.PluginslistView = listView;

            //fill in PluginListview with existing plug-ins
            Logger.Log("AddPluginsDlg constructor", Logger.manageLogLevel);

            string[] dllPathList = Directory.GetFiles(Application.StartupPath, "*.dll");
            List<ListViewItem> liItems = new List<ListViewItem>();
            ImageList il = new ImageList();
            foreach (string dllPath in dllPathList)
            {
                if (Path.GetFileName(dllPath) == sRootPluginName)
                    continue;

                try
                {
                    Assembly pluginDll = Assembly.LoadFile(dllPath);                   

                    LACTreeNodeList nodesInAssembly = _controlManage.LoadPlugInsFromAssembly(pluginDll);

                    foreach (LACTreeNode node in nodesInAssembly)
                    {
                        ListViewItem lvi = new ListViewItem(node.Text);
                        lvi.Tag = node;

                        il.Images.Add(node.image);
                        lvi.ImageIndex = il.Images.Count - 1;
                        lvi.StateImageIndex = il.Images.Count - 1; 

                        // PluginlistView.Items.Add(lviArr);                       
                        liItems.Add(lvi);
                    }
                }
                catch (BadImageFormatException)
                {
                    //
                    // Ignore DLLs that are not managed code.
                    //
                }
                catch (Exception e)
                {
                    Debugger.Log(9, "Exception", e.ToString());                 
                }
            }

            // set the image list
            PluginlistView.SmallImageList = il;            

            // sort the items
            liItems.Sort(this);

            // add to the list view
            ListViewItem[] lvItems = new ListViewItem[liItems.Count];
            liItems.CopyTo(lvItems);
            PluginlistView.Items.AddRange(lvItems);

            if (PluginlistView.Items.Count > 0)
                PluginlistView.Items[0].Selected = true;
        }

        private void pluginListViewClicked(object sender, EventArgs e)
        {
            Logger.Log("Plugin ListView clicked", Logger.manageLogLevel);
        }

        private void pluginListview_selectedindexChanged(object sender, EventArgs e)
        {
            if (PluginlistView.SelectedItems.Count == 0)
            {
                return;
            }

            LACTreeNode node = PluginlistView.SelectedItems[0].Tag as LACTreeNode;
            if (node != null)
                lblDescription.Text = node.Plugin.GetDescription();
        }      

        private void AddBtn_Click(object sender, EventArgs e)
        {
            Logger.Log("AddPluginsDlg.AddBtn_Click", Logger.manageLogLevel);

            if (PluginlistView.SelectedItems.Count == 0)
                return;
            foreach (ListViewItem selectedItem in PluginlistView.SelectedItems)
            {
                Logger.Log(
                    "The selectedItem is " + selectedItem.SubItems[0].Text,
                    Logger.manageLogLevel);

                LACTreeNode node = selectedItem.Tag as LACTreeNode;

                // Allow the plug-in to take itself out of the list if it wants
                if (node.Plugin.PluginSelected() == false)
                {
                    Logger.Log(
                        "The selectedItem is " + selectedItem.SubItems[0].Text + " opted out of selection",
                        Logger.manageLogLevel);

                    return;
                }

                node = new LACTreeNode(node.Name, node.image, node.t, node.Plugin);
#if QUARTZ
                #if LMC_ENTERPRISE
                if (node.Plugin is GPOEPlugin)
                {                    
                    GPOEPlugin gPlugin = node.Plugin as GPOEPlugin;                    

                    uint requestedFields = (uint)Hostinfo.FieldBitmaskBits.FQDN;

                    //if (!String.IsNullOrEmpty(gPlugin._hn.domainName))
                    //{
                    //    requestedFields |= (uint)Hostinfo.FieldBitmaskBits.FORCE_USER_PROMPT;
                    //}

                    if (gPlugin._usingSimpleBind || gPlugin._hn.IsConnectionSuccess)
                    {
                        requestedFields |= (uint)Hostinfo.FieldBitmaskBits.CREDS_NT4DOMAIN;
                        requestedFields |= (uint)Hostinfo.FieldBitmaskBits.CREDS_USERNAME;
                        requestedFields |= (uint)Hostinfo.FieldBitmaskBits.CREDS_PASSWORD;
                    }

                    if (!gPlugin._container.GetTargetMachineInfo(gPlugin, gPlugin._hn, requestedFields))
                    {
                        Logger.Log(
                            "GPOE could not find information about target domain",
                            Logger.GPOELogLevel);
                        if (requestedFields == (uint)Hostinfo.FieldBitmaskBits.FQDN)
                            AddBtn_Click(sender, e);
                        return;
                    }
                    else
                    {   
                        if (!gPlugin._usingSimpleBind && !gPlugin._hn.IsConnectionSuccess)
                        {
                            gPlugin._usingSimpleBind = true;
                            return;
                        }
                    }
                    if (gPlugin._hn.IsConnectionSuccess)
                    {
                        ObjectSelectDlg selectDlg = new ObjectSelectDlg(
                            _standalonePage._controlManage,
                            node.PluginPage as StandardPage,
                            gPlugin._hn,
                            node.Plugin as IPlugIn);
                        selectDlg.ShowDialog(this);

                        if (selectDlg.distinguishedName == null)
                        {
                            continue;
                        }
                        else
                        {
                            node.Text = selectDlg.displayName + " [" + selectDlg.dirContext.DomainControllerName + "] Policy";
                            GPObjectInfo gpoInfo = new GPObjectInfo(selectDlg.dirContext, selectDlg.distinguishedName, selectDlg.displayName);
                            gpoInfo.HostInfo = gPlugin._hn;
                            node.Tag = gpoInfo;
                        }
                        node.ImageIndex = selectedItem.ImageIndex;
                        _standalonePage.dummy.Add(node);

                        ListViewItem lviArr;
                        string[] values = { node.Text };
                        lviArr = new ListViewItem(values);
                        lviArr.ImageIndex = node.ImageIndex;
                        lviArr.Tag = node;
                        ListViewItem item = _standalonePage.ChosenPlugins.Items.Add(lviArr);                      
                    }
                }
                else  //Not a GPOEPlugin
                #endif
#endif
                {
                    node.ImageIndex = selectedItem.ImageIndex;
                    _standalonePage.dummy.Add(node);

                    ListViewItem lviArr;
                    string[] values = { node.Text };
                    lviArr = new ListViewItem(values);
                    lviArr.ImageIndex = node.ImageIndex;
                    lviArr.Tag = node;
                    ListViewItem item = _standalonePage.ChosenPlugins.Items.Add(lviArr);

                    //Inserting the new item to the combo
                    /*int level = _standalonePage.PluginCombo.SelectedIndex;
                    level += 1;
                    char[] spacing = new char[level * 2];
                    for (int i = 0; i < level * 2; i++)
                        spacing[i] = ' ';
                    _standalonePage.PluginCombo.Items.Insert(item.Index+1, string.Concat(new string(spacing), node.Text));*/
                }
            }
        }
     
        private void CloseBtn_Click(object sender, EventArgs e)
        {
            if (_standalonePage.ChosenPlugins.Items.Count > 0 && _standalonePage.dummy.Count > 0)
            {
                //foreach (LACTreeNode node in _addedPlugins)
                //  _rootNode.Nodes.Add(node); 

                _standalonePage.addedPlugins = _standalonePage.dummy;
            }
            this._grandParent.addPluginDlg_closeClicked = true;
            this._grandParent.addPluginDlg_hasbeenOpened = false;
            this.Close();
        }

        private bool IsPlugInAlreadyAdded(LACTreeNode lacNode)
        {
            bool bIsFound=false;
            if (PluginslistView != null && PluginslistView.Items.Count != 0)
            {
                foreach (ListViewItem lvItem in PluginslistView.Items)
                {
                    if (lvItem.Text.Trim().Equals(lacNode.Text.Trim()))
                        bIsFound = true;
                }
            }
            return bIsFound;
        }

        #region IComparer<ListViewItem> Members

        public int Compare(ListViewItem x, ListViewItem y)
        {
            return string.Compare(x.Text, y.Text);
        }

        #endregion
    }
}