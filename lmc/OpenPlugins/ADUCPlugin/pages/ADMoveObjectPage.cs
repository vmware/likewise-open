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
using Likewise.LMC.LDAP;
using Likewise.LMC.Plugins.ADUCPlugin.Properties;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ADMoveObjectPage : Form
{
    #region Class Data
    IPlugInContainer IPlugInContainer;
    
    private ADUCPlugin _addplugin;
    private LWTreeView _origLWTreeView;
    private LACTreeNode _aducRootnode;
    
    public MoveInfo moveInfo;
    #endregion
    
    #region Constructors
    public ADMoveObjectPage()
    {
        InitializeComponent();
        moveInfo = new MoveInfo();
    }
    
    /// <summary>
    /// Override constructor gets the Plugin node and all its childs
    /// Adds the all nodes to the treeview
    /// </summary>
    /// <param name="container"></param>
    /// <param name="parentPage"></param>
    /// <param name="plugin"></param>
    /// <param name="origTreeview"></param>
    public ADMoveObjectPage(IPlugInContainer container, StandardPage parentPage, ADUCPlugin plugin, LWTreeView origTreeview)
    : this()
    {
        this.IPlugInContainer = container;
        _addplugin = plugin;
        
        _origLWTreeView = origTreeview;
        
        _aducRootnode = _addplugin.GetPlugInNode();
        
        if (_aducRootnode.Nodes.Count > 0)
        {
                /*TreeNode[] newTreeArray = new TreeNode[_aducRootnode.Nodes.Count];
                // Iterate through the root nodes in the Nodes property.
                _aducRootnode.Nodes.CopyTo(newTreeArray, 0);
                foreach (TreeNode node in newTreeArray)
                {
                    LACTreeNode lacnode = node as LACTreeNode;
                    //   treeView1.Nodes.Add(lacnode.DeepCopy() as TreeNode);
                }*/
            ADUCDirectoryNode rootNode =
            ADUCDirectoryNode.GetDirectoryRoot(_addplugin.GetpluginContext(),          
            _addplugin.GetRootDN(),
            Resources.ADUC, typeof (ADUCPage), _addplugin);
            
            if (rootNode != null)
            {
                treeView1.Nodes.Add(rootNode);
                rootNode.ListContainerChildren();
                treeView1.ExpandAll();
            }
        }
        else
        {
            Logger.Log("The LWTreeView control does not have any nodes.");
        }
    }
    #endregion
    
    
    #region Helper Methods
    
    /// <summary>
    /// Method to reconstruct the treeview after the selected object is moved successfully.
    /// </summary>
    /// <param name="aducRootnode"></param>
    private void recoverOrigTreeview(TreeNode aducRootnode)
    {
        if (_origLWTreeView.Nodes.Count > 0)
        {
            
            // Iterate through the root nodes in the Nodes property.
            for (int i = 0; i < _origLWTreeView.Nodes.Count; i++)
            {
                if (_origLWTreeView.Nodes[i].Name == "Console")
                {
                    if (_origLWTreeView != null)
                    {
                        if (_origLWTreeView != null)
                        {
                            _origLWTreeView.Nodes[0].Nodes.Insert(i, aducRootnode);
                        
}
                    
}

                        
                        {
                            return;
                        }
                    }
                }
            }
        }
        #endregion
        
        
        #region Event Handlers
        
        /// <summary>
        /// When a Node is selected, ShowNode already gets called because of LWTreeView1_NodeMouseClick
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void treeView1_AfterSelect(object sender, TreeViewEventArgs e)
        {
            // When a Node is selected, ShowNode already gets called because of LWTreeView1_NodeMouseClick
            LACTreeNode node = e.Node as LACTreeNode;
            
            if (node is ADUCDirectoryNode)
            {
                ADUCDirectoryNode dirnode = node as ADUCDirectoryNode;
                
                dirnode.ListContainerChildren();
                dirnode.Collapse();
                dirnode.Expand();
                moveInfo.newParentDn = dirnode.DistinguishedName;
            }
        }
        
        /// <summary>
        /// Gets the seleted  object class information
        /// And changes the distinguished name from old to new for the object that is being moved
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void treeView1_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            
            if (sender != null)
            {
                if (e.Button != System.Windows.Forms.MouseButtons.Right)
                {
                    LACTreeNode node = e.Node as LACTreeNode;
                    
                    
                    ADUCDirectoryNode dirnode = node as ADUCDirectoryNode;
                    
                    if (dirnode != null)
                    {
                        // dirnode.ListChildren();
                        dirnode.ListContainerChildren();
                        dirnode.Collapse();
                        dirnode.Expand();
                        moveInfo.newParentDn = dirnode.DistinguishedName;
                        moveInfo.newParentDirnode = dirnode;
                        
                    }
                }
            }
        }
        
        private void btnOk_Click(object sender, EventArgs e)
        {
            Logger.Log(
            "newparent DN is " + moveInfo.newParentDn,
            Logger.ldapLogLevel);
            if (moveInfo.newParentDn.Equals("",StringComparison.InvariantCultureIgnoreCase))
            {
                MessageBox.Show(
                this,
                "Please select a location to move this object, or click \"Cancel\" to cancel the move operation.",
                CommonResources.GetString("Caption_Console"),
                MessageBoxButtons.OK);
            }
            else
            {
                this.DialogResult = DialogResult.OK;
                this.Close();
            }
        }
        
        private void btnCancel_Click(object sender, EventArgs e)
        {
            moveInfo.newParentDn = null;
            this.Close();
        }
        
        #endregion
        
    }
    
    public class MoveInfo
    {
        #region Class Data
        public string newParentDn = "";
        public string rdn = "";
        public ADUCDirectoryNode newParentDirnode;
        //may add more attributes for group
        #endregion
    }
}
