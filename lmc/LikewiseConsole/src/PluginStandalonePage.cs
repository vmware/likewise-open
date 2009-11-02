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
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;

namespace Likewise.LMC
{
    public partial class PluginStandalonePage : Likewise.LMC.ServerControl.MPPage
    {
        public Manage _controlManage = null;
        private LACTreeNode _rootNode = null;
        private List<LACTreeNode> _addedPlugins = new List<LACTreeNode>();
        public List<LACTreeNode> dummy = new List<LACTreeNode>();
        private List<LACTreeNode> _removedPlugins = new List<LACTreeNode>();
        private AddRemovePluginDlg _parentDlg = null;
        private LWTreeView _lmcTreeview = null;
        public int pluginCombobox_selectedIndex = 0;    
        public ListView PluginslistView = null;
        private int nodeindex = 0;

        #region accessors
        public ListView ChosenPlugins
        {
            get
            {
                return this.ChosenPluginlistView;
            }
        }
  
        public List<LACTreeNode> addedPlugins
        {
            get
            {
                return _addedPlugins;
            }

            set
            {
                _addedPlugins = value;
            }
        }

        public ComboBox PluginCombo
        {
            get
            {
                return this.PluginComboBox;
            }
        }

        #endregion

        public PluginStandalonePage(AddRemovePluginDlg parentDlg, Manage controlManage, LACTreeNode rootNode, LWTreeView lmcTreeview)
        {
            this.pageID = "PluginStandalonePage";
            InitializeComponent();
            SetPageTitle("Standalone");
            _parentDlg = parentDlg;
            _controlManage = controlManage;
            _rootNode = rootNode;
            _lmcTreeview = lmcTreeview;

            TreeNode node = lmcTreeview.Nodes[0];
            this.ChosenPluginlistView.Items.Clear();

            foreach (TreeNode n in node.Nodes)
            {
                ListViewItem lviArr;
                string str = n.Text;
                if (str.Contains("("))
                    str = n.Text.Substring(0, n.Text.IndexOf('('));
                string[] values = { str };
                lviArr = new ListViewItem(values);
                lviArr.Tag = n;
                lviArr.ImageIndex = (int)PluginStandalonePage.GetNodeType(n.Name);
                this.ChosenPluginlistView.Items.Add(lviArr);
            }

            visitOneNode(node as LACTreeNode, 0);

            if (this.PluginComboBox.Items.Count != 0)
            {
                this.PluginComboBox.SelectedIndex = 0;
            }
        }

        #region events handlers

        private void Addbtn_Click(object sender, EventArgs e)
        {
            if (!this._parentDlg.addPluginDlg_hasbeenOpened)
            {
                AddPluginsDlg f = new AddPluginsDlg(_parentDlg, _controlManage, _rootNode, this, this.ChosenPluginlistView);
                f.ShowDialog(this);
                _parentDlg.addedPlugins = dummy;
            }
        }

        private void Removebtn_Click(object sender, EventArgs e)
        {
            if (ChosenPluginlistView.SelectedItems.Count == 0)
                return;

            try
            {
                ListViewItem removeItem = ChosenPluginlistView.SelectedItems[0];

                // add to removedPlugins
                _removedPlugins.Add(removeItem.Tag as LACTreeNode);

                int idx = ChosenPluginlistView.SelectedItems[0].Index;
                if (idx >= 0)
                {
                    List<ListViewItem> lvItemList = new List<ListViewItem>();
                    int index = 0;

                    for (index = 0; index < ChosenPluginlistView.Items.Count; index++)
                    {
                        if (index != idx)
                            lvItemList.Add(ChosenPluginlistView.Items[index]);
                    }

                    ListViewItem[] lvItemArr = new ListViewItem[lvItemList.Count];
                    lvItemList.CopyTo(lvItemArr);

                    ChosenPluginlistView.Items.Clear();
                    ChosenPluginlistView.Items.AddRange(lvItemArr);
                    Removebtn.Enabled = false;
                    Aboutbtn.Enabled = false;
                }                

                foreach (AddedPluginInfo obj in PluginComboBox.Items)
                {
                    if (obj.nodeindex == (int)((LACTreeNode)removeItem.Tag).Tag)
                    {
                        PluginComboBox.Items.Remove(obj);
                        AddedPluginInfo parentnodeInfo = PluginComboBox.Items[pluginCombobox_selectedIndex] as AddedPluginInfo;
                        LACTreeNode n = parentnodeInfo.node as LACTreeNode;
                        if (n.Nodes.Count > obj.nodeindex)
                        {
                            n.Nodes.RemoveAt(obj.nodeindex);
                        }
                        break;
                    }
                }
            }
            catch(Exception ex)
            {
                Logger.LogException("PluginStandalonePage.Removebtn_Click", ex);
            }

            lblDescription.Text = string.Empty;

            _parentDlg.addedPlugins = _addedPlugins;
            _parentDlg.removedplugins = _removedPlugins;
        }


        private void pluginComboBox_selectedIndexChanged(object sender, EventArgs e)
        {
            if (PluginComboBox.SelectedIndex < 0 ||
                pluginCombobox_selectedIndex == PluginComboBox.SelectedIndex)
                return;

            pluginCombobox_selectedIndex = PluginComboBox.SelectedIndex;

            try
            {
                AddedPluginInfo obj = PluginComboBox.Items[pluginCombobox_selectedIndex] as AddedPluginInfo;
                LACTreeNode node = obj.node as LACTreeNode;
                showNode(GetIndexNode(_rootNode, (int)node.Tag));
            }
            catch (Exception ex)
            {
                Logger.LogException("PluginStandalonePage.pluginComboBox_selectedIndexChanged", ex);
            }
        }

        private void ChosenPluginlistView_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (ChosenPluginlistView.SelectedItems.Count == 0)
            {
                Removebtn.Enabled = false;
                Aboutbtn.Enabled = false;
                return;
            }
            Removebtn.Enabled = true;
            Aboutbtn.Enabled = true;

            LACTreeNode node = ChosenPluginlistView.SelectedItems[0].Tag as LACTreeNode;
            if (node!=null)
                lblDescription.Text = node.Plugin.GetDescription();

        }

        private void Aboutbtn_Click(object sender, EventArgs e)
        {
            if (ChosenPluginlistView.SelectedItems.Count == 0)
                return;
            AboutPluginForm abtForm = new AboutPluginForm();

            LACTreeNode node = ChosenPluginlistView.SelectedItems[0].Tag as LACTreeNode;

            if (node != null)
                abtForm.SetData(node);

            abtForm.ShowDialog(this);
            if (ChosenPluginlistView.SelectedItems.Count != 0)
                ChosenPluginlistView.SelectedItems[0].Selected = true;
        }

        #endregion

        #region helper_functions
        private void visitOneNode(LACTreeNode node, int level)
        {
            if (!node.IsPluginNode)
                return;

            string str = node.Text;

            if (node.Text.Contains("("))
                str = node.Text.Substring(0, node.Text.IndexOf('('));

            Logger.Log(
                "Node: " + node.Text + " Level: " + level,
                Logger.manageLogLevel);

            AddedPluginInfo pluginInfo = null;

            //Tried to avoid referring the tree node from the Console Root. 
            LACTreeNode pluginnode = node.DeepCopy() as LACTreeNode;               

            if (level > 0)
            {
                char[] spacing = new char[level * 2];
                for (int i = 0; i < level * 2; i++)
                    spacing[i] = ' ';

                pluginInfo = new AddedPluginInfo(string.Concat(new string(spacing), str), level, nodeindex, pluginnode);
                this.PluginComboBox.Items.Add(pluginInfo);
            }
            else
            {
                pluginInfo = new AddedPluginInfo(str, level, nodeindex, pluginnode);
                this.PluginComboBox.Items.Add(pluginInfo);
            }
            nodeindex++;
            node.Tag = pluginnode.Tag = this.PluginComboBox.Items.Count - 1;       

            foreach (LACTreeNode n in node.Nodes)
            {
                if (n.IsPluginNode)
                {
                    pluginnode.Nodes.Add(n.DeepCopy());
                    visitOneNode(n, level + 1);
                }
            }
        }

        private void showNode(LACTreeNode node)
        {
            if (!node.IsPluginNode)
                return;

            //list the content under this node in "ChosenPluginlistView"
            this.ChosenPluginlistView.Items.Clear();

            int index = (int)node.Tag;
            if (index == this.pluginCombobox_selectedIndex)
            {                
                foreach (LACTreeNode n in node.Nodes)
                {
                    if (!n.IsPluginNode)
                        continue;
                    string str = n.Text;
                    if (n.Text.Contains("("))
                        str = n.Text.Substring(0, n.Text.IndexOf('('));
                    string[] values = { str };
                    ListViewItem lviArr = new ListViewItem(values);
                    lviArr.Tag = GetIndexNode(_rootNode, (int)n.Tag);
                    lviArr.ImageIndex = (int)PluginStandalonePage.GetNodeType(n.Name);
                    this.ChosenPluginlistView.Items.Add(lviArr);
                }
                return;
            }

            foreach (LACTreeNode n in node.Nodes)
            {
                if (n.IsPluginNode)
                    showNode(n);
            }
        }

        private LACTreeNode GetIndexNode(LACTreeNode node, int idx)
        {
            if (!node.IsPluginNode)
                return node;

            if (idx == (int)node.Tag)
                return node;

            foreach (LACTreeNode n in node.Nodes)
            {
                if (!n.IsPluginNode)
                    continue;

                if (idx == (int)n.Tag)
                    return n;
            }

            foreach (LACTreeNode n in node.Nodes)
            {
                if (n.IsPluginNode)
                {
                    LACTreeNode lnode = GetIndexNode(n, idx);
                    if (lnode != null)
                        return lnode;
                }
            }

            return node;
        }

        public static Manage.ManageImageType GetNodeType(string type)
        {
            if (!string.IsNullOrEmpty(type))
            {
                if (type.Equals("Event Viewer", StringComparison.InvariantCultureIgnoreCase))
                {
                    return Manage.ManageImageType.EventLog; 
                }
                else if (type.Equals("Active Directory Users & Computers", StringComparison.InvariantCultureIgnoreCase))
                {
                    return Manage.ManageImageType.Generic; 
                }
                else if (type.Equals("Local Users and Groups", StringComparison.InvariantCultureIgnoreCase))
                {
                    return Manage.ManageImageType.Group;
                }
                else if (type.Equals("Likewise Cell Manager", StringComparison.InvariantCultureIgnoreCase))
                {
                    return Manage.ManageImageType.CellManager;
                }
                else if (type.Equals("File and Print", StringComparison.InvariantCultureIgnoreCase))
                {
                    return Manage.ManageImageType.Generic;
                }
                else if (type.Equals("Likewise iConsole", StringComparison.InvariantCultureIgnoreCase))
                {
                    return Manage.ManageImageType.LikewiseIconsole;
                }
            }

            return Manage.ManageImageType.Generic;
        }

        #endregion            

        private void ChosenPluginlistView_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            ListViewItem currentItem = ChosenPluginlistView.GetItemAt(e.X, e.Y);
            if (currentItem != null)
            {
                int index = 0;
                int selItemStartLength = (PluginComboBox.SelectedItem.ToString().Length - PluginComboBox.SelectedItem.ToString().TrimStart(null).Length) + 2;
                int comboIndex = 0;
                foreach (object objItem in PluginComboBox.Items)
                {
                    int startLength = (objItem.ToString().Length - objItem.ToString().TrimStart(null).Length);
                    if (startLength == selItemStartLength)
                    {
                        if (index == currentItem.Index)
                        {
                            PluginComboBox.SelectedIndex = comboIndex;
                            break;
                        }
                        index += 1;
                    }
                    comboIndex += 1;
                }
            }
        }
      

    }

    public class AddedPluginInfo
    {
        #region ClassData

        public string nodename = string.Empty;
        public int level = 0;
        public int nodeindex = 0;
        public object node = null;

        #endregion

        public AddedPluginInfo(string nodeName, int level, int index, object node)
        {
            this.nodename = nodeName;
            this.level = level;
            this.nodeindex = index;
            this.node = node;
        }

        public override string ToString()
        {
            return nodename;
        }
    }
}

