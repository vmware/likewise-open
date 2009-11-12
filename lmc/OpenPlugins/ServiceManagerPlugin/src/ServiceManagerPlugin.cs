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
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Windows.Forms;
using System.Diagnostics;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;
using Likewise.LMC.Services;

namespace Likewise.LMC.Plugins.ServiceManagerPlugin
{
    public class ServiceManagerPlugin : IPlugIn
    {
        #region Class Data

        private IPlugInContainer _container;
        private LACTreeNode _pluginNode;
        private List<IPlugIn> _extPlugins = null;
        public ServiceManagerHandle handle;
        public bool IsConnectionSuccess = false;

        #endregion

        #region IPlugIn Members

        public string GetName()
        {
            Logger.Log("ServiceManagerPlugin.GetName", Logger.ServiceManagerLoglevel);

            return Properties.Resources.ServiceManager;
        }

        public string GetPluginDllName()
        {
            return "Likewise.LMC.Plugins.ServiceManagerPlugin.dll";
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
                Logger.LogException("ServiceManagerPlugin.SerializePluginInfo()", ex);
            }
        }

        public void DeserializePluginInfo(XmlNode node, ref LACTreeNode pluginNode, string nodepath)
        {
            try
            {
                Hostinfo _hn = GetContext() as Hostinfo;
                Manage.DeserializeHostInfo(node, ref pluginNode, nodepath, ref _hn, false);
                pluginNode.Text = this.GetName();
                pluginNode.Name = this.GetName();
            }
            catch (Exception ex)
            {
                Logger.LogException("ServiceManagerPlugin.DeserializePluginInfo()", ex);
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
            Logger.Log("ServiceManagerPlugin.Initialize", Logger.ServiceManagerLoglevel);

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

            if (deadTree && _pluginNode != null)
            {
                _pluginNode.SetContext(null);
            }
        }

        public string GetDescription()
        {
            return Properties.Resources.ServiceManagerDesc;
        }

        public IContext GetContext()
        {
            return null;
        }

        public LACTreeNode GetPlugInNode()
        {
            return GetServiceManagerNode();
        }

        public void EnumChildren(LACTreeNode parentNode)
        {
            Logger.Log("ServiceManagerPlugin.EnumChildren", Logger.ServiceManagerLoglevel);

            return;
        }

        public void SetCursor(System.Windows.Forms.Cursor cursor)
        {
            Logger.Log("ServiceManagerPlugin.SetCursor", Logger.ServiceManagerLoglevel);

            if (_container != null)
            {
                _container.SetCursor(cursor);
            }
        }

        public ContextMenu GetTreeContextMenu(LACTreeNode nodeClicked)
        {
            Logger.Log("ServiceManagerPlugin.GetTreeContextMenu", Logger.ServiceManagerLoglevel);

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

            ServiceManagerEditorPage editorPage = nodeClicked.PluginPage as ServiceManagerEditorPage;
            contextMenu = new ContextMenu();

            m_item = new MenuItem("&Export List...", new EventHandler(editorPage.On_MenuClick));
            m_item.Enabled = false;
            m_item.Tag = _pluginNode;
            contextMenu.MenuItems.Add(m_item);

            m_item = new MenuItem("-");
            contextMenu.MenuItems.Add(m_item);

            m_item = new MenuItem("&Refresh", new EventHandler(editorPage.On_MenuClick));
            m_item.Tag = _pluginNode;
            contextMenu.MenuItems.Add(m_item);

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

        #region Private Methods

        private LACTreeNode GetServiceManagerNode()
        {
            Logger.Log("ServiceManagerPlugin.GetServiceManagerNode", Logger.ServiceManagerLoglevel);

            if (_pluginNode == null)
            {
                Icon ic = Properties.Resources.agent_lgr;
                _pluginNode = Manage.CreateIconNode(Properties.Resources.ServiceManager, ic, typeof(ServiceManagerEditorPage), this);
                _pluginNode.ImageIndex = (int)Manage.ManageImageType.Generic;
                _pluginNode.SelectedImageIndex = (int)Manage.ManageImageType.Generic;

                _pluginNode.IsPluginNode = true;
            }

            return _pluginNode;
        }

        private void ConnectToHost()
        {
            Logger.Log("ServiceManagerPlugin.ConnectToHost", Logger.ServiceManagerLoglevel);

            string hostName = System.Environment.MachineName;

            if (!String.IsNullOrEmpty(hostName))
            {
                if (_pluginNode != null && !String.IsNullOrEmpty(hostName))
                {
                    if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows &&
                        ServiceManagerWindowsWrapper.phSCManager == IntPtr.Zero)
                    {
                        IsConnectionSuccess = Do_LogonSCManager();
                        if (IsConnectionSuccess)
                        {
                            if (_pluginNode.PluginPage == null)
                            {
                                Type type = _pluginNode.NodeType;

                                object o = Activator.CreateInstance(type);
                                if (o is IPlugInPage)
                                {
                                    ((IPlugInPage)o).SetPlugInInfo(_container, _pluginNode.Plugin, _pluginNode, (LWTreeView)_pluginNode.TreeView, _pluginNode.sc);
                                }
                            }
                            else
                                ((ServiceManagerEditorPage)_pluginNode.PluginPage).Refresh();
                        }
                        if (!IsConnectionSuccess)
                        {
                            _container.ShowError("Unable to access the Services for the speficied user authentication");
                            return;
                        }
                    }
                    else
                    {
                        IsConnectionSuccess = true;
                    }
                    _pluginNode.Text = string.Concat(Properties.Resources.ServiceManager, " on ", hostName);
                }
            }
        }

        public bool Do_LogonSCManager()
        {
            int iRet = ServiceManagerWindowsWrapper.ApiOpnSCManager();
            if (iRet == 0)
                return true;

            return false;
        }

        #endregion
    }

    public interface IDirectoryPropertiesPage
    {
        void SetData();
    }
}
