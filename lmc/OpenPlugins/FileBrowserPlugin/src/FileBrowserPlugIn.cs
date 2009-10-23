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
using Likewise.LMC.Plugins.FileBrowser.Properties;
using Likewise.LMC.ServerControl;
using Likewise.LMC.LMConsoleUtils;
using Likewise.LMC.NETAPI;
using Likewise.LMC.AuthUtils;
using Likewise.LMC.Plugins.FileBrowser;

namespace Likewise.LMC.Plugins.FileBrowser
{
    public class FileBrowserIPlugIn : IPlugIn
    {
        #region Enum variables

        public enum PluginNodeType
        {
            SHARES,
            SESSIONS,
            OPENFILES,
            PRINTERS,
            UNDEFINED
        }
        #endregion

        #region Class data

        private string _currentHost = "";
        private IPlugInContainer _container;
        private Hostinfo _hn;
        private LACTreeNode _pluginNode;
        public FileHandle fileHandle = null;

        private List<IPlugIn> _extPlugins = null;

        #endregion

        #region IPlugIn Members

        public Hostinfo HostInfo
        {
            get
            {
                return _hn;
            }
        }

        public string GetName()
        {
            Logger.Log("FileBrowserPlugIn.GetName", Logger.FileBrowserLogLevel);

            return Resources.sTitleTopLevelTab;
        }

        public string GetDescription()
        {
            return Resources.PluginDescription;
        }

        public string GetPluginDllName()
        {
            return "Likewise.LMC.Plugins.FileBrowser.dll";
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
                XmlElement gpoInfoElement = null;
                GPObjectInfo gpoInfo = pluginNode.Tag as GPObjectInfo;

                Manage.InitSerializePluginInfo(pluginNode, this, ref Id, out viewElement, ViewsNode, SelectedNode);

                Manage.CreateAppendHostInfoElement(_hn, ref viewElement, out HostInfoElement);
                Manage.CreateAppendGPOInfoElement(gpoInfo, ref HostInfoElement, out gpoInfoElement);

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
                Logger.LogException("FileBrowserPlugin.SerializePluginInfo()", ex);
            }
        }

        public void DeserializePluginInfo(XmlNode node, ref LACTreeNode pluginNode, string nodepath)
        {
            try
            {
                Manage.DeserializeHostInfo(node, ref pluginNode, nodepath, ref _hn, false);
                pluginNode.Text = this.GetName();
                pluginNode.Name = this.GetName();
            }
            catch (Exception ex)
            {
                Logger.LogException("FileBrowserPlugin.DeserializePluginInfo()", ex);
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
            Logger.Log("FileBrowserPlugIn.Initialize", Logger.FileBrowserLogLevel);

            _container = container;
        }

        public void SetContext(IContext ctx)
        {
            Hostinfo hn = ctx as Hostinfo;

            Logger.Log(String.Format("FileBrowserPlugIn.SetHost(hn: {0}\n)",
            hn == null ? "<null>" : hn.ToString()), Logger.FileBrowserLogLevel);

            bool deadTree = false;

            if (_pluginNode != null &&
                _pluginNode.Nodes != null &&
                _hn != null &&
                hn != null &&
                hn.hostName !=
                _hn.hostName)
            {
                foreach (TreeNode node in _pluginNode.Nodes)
                {
                    _pluginNode.Nodes.Remove(node);
                }
                deadTree = true;
            }

            _hn = hn;

            if (HostInfo == null)
            {
                _hn = new Hostinfo();
            }

            ConnectToHost();

            if (_pluginNode != null && _pluginNode.Nodes.Count == 0 && _hn.IsConnectionSuccess)
            {
                BuildNodesToPlugin();
            }

            if (deadTree && _pluginNode != null)
            {
                _pluginNode.SetContext(_hn);
            }
        }

        public IContext GetContext()
        {
            return _hn;
        }

        public LACTreeNode GetPlugInNode()
        {
            return GetFileBrowserNode();
        }

        public void EnumChildren(LACTreeNode parentNode)
        {
            Logger.Log("FileBrowserPlugIn.EnumChildren", Logger.FileBrowserLogLevel);

            if (parentNode == _pluginNode)
            {
                BuildNodesToPlugin();
            }

            //
            // Here is a place to break out the enumeration for each node and path
            //

            return;
        }

        public void SetCursor(System.Windows.Forms.Cursor cursor)
        {
            Logger.Log("FileBrowserPlugIn.SetCursor", Logger.FileBrowserLogLevel);

            if (_container != null)
            {
                _container.SetCursor(cursor);
            }
        }

        public ContextMenu GetTreeContextMenu(LACTreeNode nodeClicked)
        {
            Logger.Log("FileBrowserPlugIn.GetTreeContextMenu", Logger.FileBrowserLogLevel);

            if (nodeClicked == null)
            {
                return null;
            }
            else
            {
                ContextMenu fileBrowserContextMenu = null;

                StandardPage fileBrowserPage = (StandardPage)nodeClicked.PluginPage;

                if (fileBrowserPage == null)
                {
                    Type type = nodeClicked.NodeType;
                    object o = Activator.CreateInstance(type);
                    if (o is IPlugInPage)
                    {
                        ((IPlugInPage)o).SetPlugInInfo(_container, nodeClicked.Plugin, nodeClicked, (LWTreeView)nodeClicked.TreeView, nodeClicked.sc);
                    }
                }
                if (_pluginNode == nodeClicked)
                {
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Network))
                {
                    fileBrowserContextMenu = new ContextMenu();

                    MenuItem m_item = new MenuItem("Connect to network share...", new EventHandler(cm_OnConnect));
                    fileBrowserContextMenu.MenuItems.Add(0, m_item);
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Computer))
                {
                    FilesDetailPage detailsPage = fileBrowserPage as FilesDetailPage;
                    if (detailsPage != null)
                    {
                        fileBrowserContextMenu = detailsPage.GetTreeContextMenu();
                    }
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Home))
                {
                    FilesDetailPage detailsPage = fileBrowserPage as FilesDetailPage;
                    if (detailsPage != null)
                    {
                        fileBrowserContextMenu = detailsPage.GetTreeContextMenu();
                    }
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Desktop))
                {
                    FilesDetailPage detailsPage = fileBrowserPage as FilesDetailPage;
                    if (detailsPage != null)
                    {
                        fileBrowserContextMenu = detailsPage.GetTreeContextMenu();
                    }
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Documents))
                {
                    FilesDetailPage detailsPage = fileBrowserPage as FilesDetailPage;
                    if (detailsPage != null)
                    {
                        fileBrowserContextMenu = detailsPage.GetTreeContextMenu();
                    }
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Music))
                {
                    FilesDetailPage detailsPage = fileBrowserPage as FilesDetailPage;
                    if (detailsPage != null)
                    {
                        fileBrowserContextMenu = detailsPage.GetTreeContextMenu();
                    }
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Pictures))
                {
                    FilesDetailPage detailsPage = fileBrowserPage as FilesDetailPage;
                    if (detailsPage != null)
                    {
                        fileBrowserContextMenu = detailsPage.GetTreeContextMenu();
                    }
                }
                else if (nodeClicked.Name.Trim().Equals(Resources.Videos))
                {
                    FilesDetailPage detailsPage = fileBrowserPage as FilesDetailPage;
                    if (detailsPage != null)
                    {
                        fileBrowserContextMenu = detailsPage.GetTreeContextMenu();
                    }
                }
                return fileBrowserContextMenu;
            }
        }

        public void SetSingleSignOn(bool useSingleSignOn)
        {
            // do nothing
        }

        #endregion

        #region Private helper functions

        private LACTreeNode GetFileBrowserNode()
        {
            Logger.Log("FileBrowserPlugIn.GetFileBrowserNode", Logger.FileBrowserLogLevel);

            if (_pluginNode == null)
            {
                Icon ic = Resources.SharedFolder2;
                _pluginNode = Manage.CreateIconNode("File Browser", ic, typeof(FilesBrowserPluginPage), this);
                _pluginNode.ImageIndex = (int)Manage.ManageImageType.Generic;
                _pluginNode.SelectedImageIndex = (int)Manage.ManageImageType.Generic;    
               
                _pluginNode.IsPluginNode = true;
            }

            return _pluginNode;
        }

        private void BuildNodesToPlugin()
        {
            if (_pluginNode != null)
            {
                Icon ic = Resources.SharedFolder2;

                LACTreeNode networkNode = Manage.CreateIconNode(Resources.Network, ic, typeof(FilesDetailPage), this);
                _pluginNode.Nodes.Add(networkNode);

                LACTreeNode computerNode = Manage.CreateIconNode(Resources.Computer, ic, typeof(FilesDetailPage), this);
                _pluginNode.Nodes.Add(computerNode);

                LACTreeNode homeNode = Manage.CreateIconNode(Resources.Home, ic, typeof(FilesDetailPage), this);
                computerNode.Nodes.Add(homeNode);

                LACTreeNode deskNode = Manage.CreateIconNode(Resources.Desktop, ic, typeof(FilesDetailPage), this);
                computerNode.Nodes.Add(deskNode);

                LACTreeNode docNode = Manage.CreateIconNode(Resources.Documents, ic, typeof(FilesDetailPage), this);
                computerNode.Nodes.Add(docNode);

                LACTreeNode musicNode = Manage.CreateIconNode(Resources.Music, ic, typeof(FilesDetailPage), this);
                computerNode.Nodes.Add(musicNode);

                LACTreeNode pictNode = Manage.CreateIconNode(Resources.Pictures, ic, typeof(FilesDetailPage), this);
                computerNode.Nodes.Add(pictNode);

                LACTreeNode videoNode = Manage.CreateIconNode(Resources.Videos, ic, typeof(FilesDetailPage), this);
                computerNode.Nodes.Add(videoNode);
            }
        }

        private void ConnectToHost()
        {
            Logger.Log("FileBrowserPlugIn.ConnectToHost", Logger.FileBrowserLogLevel);

            if (_hn.creds.Invalidated)
            {
                _container.ShowError("File Browser PlugIn cannot connect to domain due to invalid credentials");
                _hn.IsConnectionSuccess = false;
                return;
            }
            if (!String.IsNullOrEmpty(_hn.hostName))
            {
                if (_currentHost != _hn.hostName)
                {
                    if (fileHandle != null)
                    {
                        fileHandle.Dispose();
                        fileHandle = null;
                    }

                    if (_pluginNode != null && !String.IsNullOrEmpty(_hn.hostName))
                    {
                        OpenHandle(_hn.hostName);
                    }

                    _currentHost = _hn.hostName;
                }
                _hn.IsConnectionSuccess = true;
            }
            else
                _hn.IsConnectionSuccess = false;
        }

        private void cm_OnConnect(object sender, EventArgs e)
        {
            //check if we are joined to a domain -- if not, use simple bind
            uint requestedFields = (uint)Hostinfo.FieldBitmaskBits.FQ_HOSTNAME;
            //string domainFQDN = null;

            if (_hn == null)
            {
                _hn = new Hostinfo();
            }

            //TODO: kerberize eventlog, so that creds are meaningful.
            //for now, there's no reason to attempt single sign-on
            requestedFields |= (uint)Hostinfo.FieldBitmaskBits.FORCE_USER_PROMPT;

            if (_hn != null)
            {
                if (!_container.GetTargetMachineInfo(this, _hn, requestedFields))
                {
                    Logger.Log(
                    "Could not find information about target machine",
                    Logger.FileBrowserLogLevel);                   
                }
                else
                {
                    if (_pluginNode != null && !String.IsNullOrEmpty(_hn.hostName))
                    {
                        _pluginNode.sc.ShowControl(_pluginNode);
                    }
                    else
                    {
                        Logger.ShowUserError("Unable to find the hostname that enterted");
                        _hn.IsConnectionSuccess = false;
                    }
                }
            }
        }

        #endregion

        #region eventlog API wrappers

        public void OpenHandle(string hostname)
        {
            try
            {
                if (fileHandle == null)
                {
                    fileHandle = HandleAdapter.OpenHandle(hostname);
                }
            }
            catch (Exception e)
            {
                Logger.LogException("EventViewerPlugin.OpenEventLog", e);
                fileHandle = null;
            }
        }

        public bool IsHandleBinded()
        {
            if (fileHandle == null)
            {
                return false;
            }
            else
            {
                return true;
            }
        }       

        public void CloseEventLog()
        {
            if (fileHandle == null)
            {
                return;
            }
            fileHandle.Dispose();
            fileHandle = null;
        }

        #endregion
    }
}
