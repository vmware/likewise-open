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

using System.Drawing;
using System.Windows.Forms;
using System;
using System.Xml;
using System.Collections.Generic;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;
using Likewise.LMC.Registry;

using Microsoft.Win32;

namespace Likewise.LMC.Plugins.RegistryViewerPlugin
{
    public class RegistryViewerPlugin : IPlugIn
    {
        #region Enum variables

        public enum NodeType
        {
            HKEY_CLASSES_ROOT,
            HKEY_CURRENT_USER,
            HKEY_LOCAL_MACHINE,
            HKEY_USERS,
            HKEY_CURRENT_CONFIG,
            HKEY_SUBKEY,
            HKEY_LIKEWISE,
            HKEY_LIKEWISE_SUBKEY,
            NONE
        }        

        #endregion

        #region Class data

        private string _currentHost = "";
        private IPlugInContainer _container;
        private LACTreeNode _pluginNode;
        public RegServerHandle handle = null;
        public bool IsConnectionSuccess = false;
        private IntPtr phToken = IntPtr.Zero;
		public IntPtr pRootHandle;

        private List<IPlugIn> _extPlugins = null;

        private string[] RegistryRootKeys = new string[]
                                        {
                                        Properties.Resources.HKEY_LIKEWISE,
                                        Properties.Resources.HKEY_LIKEWISE_IMPORT
                                        };

        public string RegRootKeySelected = Properties.Resources.HKEY_LIKEWISE;

        #endregion

        #region IPlugIn Members

        public string GetName()
        {
            Logger.Log("RegistryViewerPlugin.GetName", Logger.RegistryViewerLoglevel);

            return Properties.Resources.RegistryViewer;
        }

        public string GetPluginDllName()
        {
            return "Likewise.LMC.Plugins.RegistryViewer.dll";
        }

        public IContextType GetContextType()
        {
            return IContextType.Hostinfo;
        }

        public void SerializePluginInfo(LACTreeNode pluginNode, ref int Id, out XmlElement viewElement, XmlElement ViewsNode, TreeNode SelectedNode)
        {
            viewElement = null;

            try
            {
                if (pluginNode == null || !pluginNode._IsPlugIn)
                    return;

                XmlElement HostInfoElement = null;

                Manage.InitSerializePluginInfo(pluginNode, this, ref Id, out viewElement, ViewsNode, SelectedNode);

                Manage.CreateAppendHostInfoElement(null, ref viewElement, out HostInfoElement);

                if (pluginNode != null && pluginNode.Nodes.Count != 0)
                {
                    foreach (LACTreeNode lacnode in pluginNode.Nodes)
                    {
                        XmlElement innerelement = null;
                        pluginNode.Plugin.SerializePluginInfo(lacnode, ref Id, out innerelement, viewElement, SelectedNode);
                        if (innerelement != null)
                        {
                            viewElement.AppendChild(innerelement);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.LogException("RegistryViewerPlugin.SerializePluginInfo()", ex);
            }
        }

        public void DeserializePluginInfo(XmlNode node, ref LACTreeNode pluginNode, string nodepath)
        {
            try
            {
                Hostinfo hn = GetContext() as Hostinfo;
                Manage.DeserializeHostInfo(node, ref pluginNode, nodepath, ref hn, false);
                pluginNode.Text = this.GetName();
                pluginNode.Name = this.GetName();
            }
            catch (Exception ex)
            {
                Logger.LogException("RegistryViewerPlugin.DeserializePluginInfo()", ex);
            }
        }

        public void AddExtPlugin(IPlugIn extPlugin)
        {
            if (_extPlugins == null)
            {
                _extPlugins = new List<IPlugIn>();
            }

            _extPlugins.Add(extPlugin);
        }

        public void Initialize(IPlugInContainer container)
        {
            Logger.Log("RegistryViewerPlugin.Initialize", Logger.RegistryViewerLoglevel);

            _container = container;
        }

        public void SetContext(IContext ctx)
        {
            bool deadTree = false;

            if (_pluginNode != null &&
                _pluginNode.Nodes != null)
            {
                foreach (TreeNode node in _pluginNode.Nodes)
                {
                    _pluginNode.Nodes.Remove(node);
                }
                deadTree = true;
            }

            ConnectToHost();

            if (_pluginNode != null && _pluginNode.Nodes.Count == 0 && IsConnectionSuccess)
            {
                BuildNodesToPlugin();
            }

            if (deadTree && _pluginNode != null)
            {
                _pluginNode.SetContext(null);
            }
        }

        public string GetDescription()
        {
            return Properties.Resources.RegistryViewerDesc;
        }

        public IContext GetContext()
        {
            return null;
        }

        public LACTreeNode GetPlugInNode()
        {
            return GetRegistryViewerNode();
        }

        public void EnumChildren(LACTreeNode parentNode)
        {
            Logger.Log("RegistryViewerPlugin.EnumChildren", Logger.RegistryViewerLoglevel);

            return;
        }

        public void SetCursor(System.Windows.Forms.Cursor cursor)
        {
            Logger.Log("RegistryViewerPlugin.SetCursor", Logger.RegistryViewerLoglevel);

            if (_container != null)
            {
                _container.SetCursor(cursor);
            }
        }

        public ContextMenu GetTreeContextMenu(LACTreeNode nodeClicked)
        {
            Logger.Log("RegistryViewerPlugin.GetTreeContextMenu", Logger.RegistryViewerLoglevel);

            if (nodeClicked == null)
            {
                return null;
            }

            ContextMenu contextMenu = null;
            MenuItem m_item;

            if (nodeClicked.PluginPage == null)
            {
                Type type = nodeClicked.NodeType;

                object o = Activator.CreateInstance(type);
                if (o is IPlugInPage)
                {
                    ((IPlugInPage)o).SetPlugInInfo(_container, nodeClicked.Plugin, nodeClicked, (LWTreeView)nodeClicked.TreeView, nodeClicked.sc);
                }
            }

            RegistryEditorPage editorPage = nodeClicked.PluginPage as RegistryEditorPage;
            contextMenu = new ContextMenu();

            if (_pluginNode == nodeClicked)
            {
                m_item = new MenuItem("&Import...", new EventHandler(editorPage.On_MenuClick));                
                m_item.Tag = _pluginNode;
                contextMenu.MenuItems.Add(m_item);
            }
            else
            {
                m_item = new MenuItem("&New", new EventHandler(editorPage.On_MenuClick));
                m_item.Tag = nodeClicked;
                contextMenu.MenuItems.Add(m_item);

                MenuItem m_Inneritem = new MenuItem("Key", new EventHandler(editorPage.On_MenuClick));
                m_Inneritem.Tag = nodeClicked;
                m_item.MenuItems.Add(m_Inneritem);

                m_Inneritem = new MenuItem("-");
                m_item.MenuItems.Add(m_Inneritem);

                m_Inneritem = new MenuItem("String Value", new EventHandler(editorPage.On_MenuClick));
                m_Inneritem.Tag = nodeClicked;
                m_item.MenuItems.Add(m_Inneritem);

                m_Inneritem = new MenuItem("Binary Value", new EventHandler(editorPage.On_MenuClick));
                m_Inneritem.Tag = nodeClicked;
                m_item.MenuItems.Add(m_Inneritem);

                m_Inneritem = new MenuItem("DWORD Value", new EventHandler(editorPage.On_MenuClick));
                m_Inneritem.Tag = nodeClicked;
                m_item.MenuItems.Add(m_Inneritem);

                m_Inneritem = new MenuItem("Multi-String Value", new EventHandler(editorPage.On_MenuClick));
                m_Inneritem.Tag = nodeClicked;
                m_item.MenuItems.Add(m_Inneritem);

                m_Inneritem = new MenuItem("Expandable String Value", new EventHandler(editorPage.On_MenuClick));
                m_Inneritem.Enabled = Configurations.currentPlatform == LikewiseTargetPlatform.Windows;
                m_Inneritem.Tag = nodeClicked;
                m_item.MenuItems.Add(m_Inneritem);

                switch (nodeClicked.Text)
                {
                    case "HKEY_CLASSES_ROOT":
                    case "HKEY_CURRENT_CONFIG":
                    case "HKEY_CURRENT_USER":
                    case "HKEY_LOCAL_MACHINE":
                    case "HKEY_USERS":
                        if (Configurations.currentPlatform != LikewiseTargetPlatform.Windows)
                            goto default;
                        break;

                    case "HKEY_LIKEWISE":
                    case "HKEY_LIKEWISE_IMPORT":
                        if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                            goto default;
                        break;

                    default:
                        m_item = new MenuItem("-");
                        contextMenu.MenuItems.Add(m_item);

                        m_item = new MenuItem("&Delete", new EventHandler(editorPage.On_MenuClick));
                        m_item.Tag = nodeClicked;
                        contextMenu.MenuItems.Add(m_item);

                        m_item = new MenuItem("&Rename", new EventHandler(editorPage.On_MenuClick));
                        m_item.Tag = nodeClicked;
                        contextMenu.MenuItems.Add(m_item);
                        break;
                }

                m_item = new MenuItem("-");
                contextMenu.MenuItems.Add(m_item);               

                m_item = new MenuItem("&Export...", new EventHandler(editorPage.On_MenuClick));              
                m_item.Tag = nodeClicked;
                contextMenu.MenuItems.Add(m_item);

                m_item = new MenuItem("-");
                contextMenu.MenuItems.Add(m_item);

                m_item = new MenuItem("&Refresh", new EventHandler(editorPage.On_MenuClick));
                m_item.Tag = nodeClicked;
                contextMenu.MenuItems.Add(m_item);
            }

            m_item = new MenuItem("-");
            contextMenu.MenuItems.Add(m_item);

            m_item = new MenuItem("&Help", new EventHandler(editorPage.On_MenuClick));
            contextMenu.MenuItems.Add(contextMenu.MenuItems.Count, m_item);

            return contextMenu;
        }


        public void SetSingleSignOn(bool useSingleSignOn)
        {
            // do nothing
        }

        public bool PluginSelected()
        {
            return true;
        }

        #endregion

        #region Private helper functions

        private LACTreeNode GetRegistryViewerNode()
        {
            Logger.Log("RegistryViewerPlugin.GetRegistryViewerNode", Logger.RegistryViewerLoglevel);

            if (_pluginNode == null)
            {
                Icon ic = Properties.Resources.agent_lgr;
                _pluginNode = Manage.CreateIconNode("Registry Editor", ic, typeof(RegistryEditorPage), this);
                _pluginNode.ImageIndex = (int)Manage.ManageImageType.Generic;
                _pluginNode.SelectedImageIndex = (int)Manage.ManageImageType.Generic;

                _pluginNode.IsPluginNode = true;
            }

            return _pluginNode;
        }

        public bool Do_LogonUserSet()
        {
            phToken = IntPtr.Zero;

            return RegistryInteropWrapperWindows.RegLogonUser(
                                    out phToken,
                                    System.Environment.UserName,
                                    System.Environment.UserDomainName,
                                    "");
        }

        public void Do_LogonUserHandleClose()
        {
            if (phToken != null && phToken != IntPtr.Zero)
            {
                RegistryInteropWrapperWindows.HandleClose(phToken);
            }
        }

        private void BuildNodesToPlugin()
        {
            if (_pluginNode != null)
            {
                Icon ic = Properties.Resources.Reports;

                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                {
                    LACTreeNode classesNode = Manage.CreateIconNode(
                                              Properties.Resources.HKEY_CLASSES_ROOT,
                                              ic,
                                              typeof(RegistryViewerClassesPage),
                                              this);
                    classesNode.Tag = HKEY.HKEY_CLASSES_ROOT;
                    classesNode.sc = _pluginNode.sc;
                    _pluginNode.Nodes.Add(classesNode);

                    LACTreeNode userNode = Manage.CreateIconNode(
                                              Properties.Resources.HKEY_CURRENT_USER,
                                              ic,
                                              typeof(RegistryViewerUserPage),
                                              this);
                    userNode.Tag = HKEY.HEKY_CURRENT_USER;
                    userNode.sc = _pluginNode.sc;
                    _pluginNode.Nodes.Add(userNode);

                    LACTreeNode machineNode = Manage.CreateIconNode(
                                              Properties.Resources.HKEY_LOCAL_MACHINE,
                                              ic,
                                              typeof(RegistryViewerMachinePage),
                                              this);
                    machineNode.Tag = HKEY.HKEY_LOCAL_MACHINE;
                    machineNode.sc = _pluginNode.sc;
                    _pluginNode.Nodes.Add(machineNode);

                    LACTreeNode usersNode = Manage.CreateIconNode(Properties.Resources.HKEY_USERS,
                                              ic,
                                              typeof(RegistryViewerUsersPage),
                                              this);
                    usersNode.Tag = HKEY.HKEY_USERS;
                    usersNode.sc = _pluginNode.sc;
                    _pluginNode.Nodes.Add(usersNode);

                    LACTreeNode configNode = Manage.CreateIconNode(Properties.Resources.HKEY_CURRENT_CONFIG,
                                              ic,
                                              typeof(RegistryViewerConfigPage),
                                              this);
                    configNode.Tag = HKEY.HKEY_CURRENT_CONFIG;
                    configNode.sc = _pluginNode.sc;
                    _pluginNode.Nodes.Add(configNode);
                }
                else
                {
                    int RootKeyCount = 0;
                    string[] sRegistryRootKeys = null;

                    RegistryInteropWrapper.ApiRegEnumRootKeys(handle.Handle, out sRegistryRootKeys, out RootKeyCount);
                    if (sRegistryRootKeys == null)
                        sRegistryRootKeys = RegistryRootKeys;

                    foreach (string sRootKey in sRegistryRootKeys)
                    {
                        LACTreeNode likewiseNode = Manage.CreateIconNode(sRootKey.ToUpper(),
                                                  ic,
                                                  typeof(RegistryViewerLikewisePage),
                                                  this);
                        likewiseNode.Tag = sRootKey;
                        likewiseNode.sc = _pluginNode.sc;
                        _pluginNode.Nodes.Add(likewiseNode);
                    }
                }
                _pluginNode.Text = string.Format(string.Concat(Properties.Resources.RegistryViewer, " on {0}"), System.Environment.MachineName);
            }
        }

        private void ConnectToHost()
        {
            IsConnectionSuccess = false;
            _currentHost = System.Environment.MachineName;

            Logger.Log("RegistryViewerPlugin.ConnectToHost", Logger.RegistryViewerLoglevel);

            if (!String.IsNullOrEmpty(_currentHost))
            {
                if (handle != null)
                {
                    handle.Dispose();
                    handle = null;
                }
                if (_pluginNode != null && !String.IsNullOrEmpty(_currentHost))
                {
                    if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                    {
                        IsConnectionSuccess = Do_LogonUserSet();
                        if (!IsConnectionSuccess)
                        {
                            _container.ShowError("Unable to access the Registry for the speficied user authentication");
                            return;
                        }
                        else {
                            Do_LogonUserHandleClose();
                        }
                    }
                    else
                    {
                        IsConnectionSuccess = OpenHandle();
                        if (!IsConnectionSuccess)
                        {
                            Logger.ShowUserError("Unable to get registry handle");
                            return;
                        }
                    }
                    if (handle != null)
                        _pluginNode.Nodes.Clear();
                }
            }
        }

        public bool OpenHandle()
        {
            try
            {
                if (handle == null)
                {
                    IntPtr handle_t = RegistryInteropWrapper.OpenHandle();
                    handle = (handle_t == IntPtr.Zero) ? null : new RegServerHandle(handle_t);
                }
                return (handle != null);
            }
            catch (Exception e)
            {
                Logger.LogException("RegistryViewerPlugin.RegOpenServer", e);
                handle = null;
                return false;
            }
        }

        #endregion     
    }     
}
