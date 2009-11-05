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
using System.IO;
using System.Diagnostics;
using System.Reflection;
using System.Windows.Forms;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC
{
    public partial class AddRemovePluginDlg : MPContainer
    {
        #region Class Data
        private Manage _controlManage = null;
        private LACTreeNode _rootNode = null;
        private LWTreeView _lmcTreeview = null;
        private List<LACTreeNode> _addedplugins = new List<LACTreeNode>();
        private List<LACTreeNode> _removedplugings = new List<LACTreeNode>();
        private PluginStandalonePage _pluginStandalonePage;      
        public bool addPluginDlg_closeClicked = false;
        public bool addPluginDlg_hasbeenOpened = false;


        #endregion

        #region accessors
        public List<LACTreeNode> addedPlugins
        {
            get
            {
                return _addedplugins;
            }

            set
            {
                _addedplugins = value;
            }
        }

        public List<LACTreeNode> removedplugins
        {
            get
            {
                return _removedplugings;
            }
            set
            {
                _removedplugings = value;
            }
        }
        #endregion

        public AddRemovePluginDlg(Manage controlManage, LACTreeNode rootNode, LWTreeView lmcTreeview)
        {
            InitializeComponent();
            Text = "Add/Remove Plug-in...";
            _controlManage = controlManage;
            _rootNode = rootNode;
            _lmcTreeview = lmcTreeview;         
         
            ((EditDialog)this).ShowApplyButton = false;

            InitializePages();
        }


        private void InitializePages()
        {
            MPPage page = null;

            page = new PluginStandalonePage(this, _controlManage, _rootNode, _lmcTreeview);                
            this.AddPage(page,
                         new MPMenuItem(page.PageID, "Standalone", "Standalone"),
                         MPMenu.POSITION_BEGINING
                );

            
            this._pluginStandalonePage = page as PluginStandalonePage;

            /*
            page = new PluginExtensionsPage();
            this.AddPage(page,
                         new MPMenuItem(page.PageID, "Extensions", "Extensions"),
                         MPMenu.POSITION_END
            );
             */
             
        }

        private void cancelBtn_clicked(object sender, EventArgs e)
        {
            this.Close();
        }

        private void OkBtn_clicked(object sender, EventArgs e)
        {             
            //check whether the AddpluginDlg has been opened and close before commit the result
            if (addPluginDlg_hasbeenOpened && !addPluginDlg_closeClicked)
            {
                MessageBox.Show(
                    "Please close the 'Add Plug-in dialog' before you click Ok", 
                    CommonResources.GetString("Caption_Console"),
                    MessageBoxButtons.OK);

                return;
            }

            if (!addPluginDlg_hasbeenOpened || (addPluginDlg_hasbeenOpened && addPluginDlg_closeClicked))
            //go through lmcTreeview to locate the correct place that we want to create this plug-in at 
            //based on pluginCombobox_selectedIndex             
            {
                insertNode(_rootNode);
                //foreach (string node_text in _removedplugings)
                //    deleteNode(_rootNode, node_text);
                deleteNode(_rootNode);

                return;
            }
        }         


        #region helper_functions
        private void insertNode(LACTreeNode rootnode)
        {
            int index = (int)rootnode.Tag;

            if (index == _pluginStandalonePage.pluginCombobox_selectedIndex)
            {
                //find the correct node to insert the added plug-ins
                if (_addedplugins.Count > 0)
                    foreach (LACTreeNode lacnode in _addedplugins)
                    {
                        Type t = lacnode.Plugin.GetType();
                        Assembly pluginDll = Assembly.GetAssembly(t);

                        LACTreeNodeList nodesInAssembly = _controlManage.LoadPlugInsFromAssembly(pluginDll);
                        foreach (LACTreeNode node in nodesInAssembly)
                        {
                            if (node.t.Namespace.Equals("Likewise.LMC.Plugins.GPOEPlugin", StringComparison.InvariantCultureIgnoreCase))
                            {
                                node.Text = lacnode.Text;
                                node.Tag = lacnode.Tag;                                
                            }
                            node.SetContext(lacnode.Plugin.GetContext());
                            node.Plugin.SetContext(lacnode.Plugin.GetContext());
                            //node.Plugin.SetContext(rootnode.Plugin.GetContext());
                            rootnode.Nodes.Add(node);
                            rootnode.Plugin.AddExtPlugin(node.Plugin);
                            if (_removedplugings.Contains(lacnode))
                            {
                                _removedplugings.Remove(lacnode);                                
                                _removedplugings.Add(node);
                            }
                        }                        
                    }
                return;
            }

            foreach (LACTreeNode n in rootnode.Nodes)
            {
                if(n.IsPluginNode)
                    insertNode(n);
            }
        }

        private void deleteNode(LACTreeNode node)
        {
            int index = (int)node.Tag;

            if (index == _pluginStandalonePage.pluginCombobox_selectedIndex)
            {
                //find the correct node to Remove from the added plug-ins
                if(_removedplugings.Count > 0)
                    foreach (LACTreeNode lacnode in _removedplugings)
                    {
                        node.Nodes.Remove(lacnode);
                        if (LMCMainForm.navigationHistory.Contains(lacnode))
                        {
                            LMCMainForm.navigationHistory.Remove(lacnode);                            
                        }
                    }
                return;
            }

            foreach (LACTreeNode n in node.Nodes)
            {
                if(n.IsPluginNode)
                    deleteNode(n);
            }
        }

        #endregion
    }
}

