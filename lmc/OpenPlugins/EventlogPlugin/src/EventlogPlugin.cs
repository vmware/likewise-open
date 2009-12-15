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

using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.UtilityUIElements;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Netlogon;
using Likewise.LMC.NETAPI;
using System;
using System.Drawing;
using System.Xml;
using System.Collections.Generic;
using Likewise.LMC.Plugins.EventlogPlugin.Properties;
using Likewise.LMC.Eventlog;

namespace Likewise.LMC.Plugins.EventlogPlugin
{
public class EventlogPlugin: IPlugIn
{
    private IPlugInContainer _container;
    private Hostinfo         _hn = null;
    private LACTreeNode _pluginNode = null;
    private EventFilter ef = null;
    public EventViewerControl eventlogPage = null;
    private string _currentHost = "";
    public EventlogHandle eventLogHandle = null;
    public string[] logs = null;

    private List<IPlugIn> _extPlugins = null;

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
        Logger.Log("EventlogPlugin.GetName", Logger.eventLogLogLevel);

        return Properties.Resources.sTitleEventsPage;
    }

    public string GetDescription()
    {
        return Properties.Resources.PluginDescription;
    }

    public string GetPluginDllName()
    {
        return "Likewise.LMC.Plugins.EventlogPlugin.dll";
    }

    public IContextType GetContextType()
    {
        return IContextType.Hostinfo;
    }

    void IPlugIn.Initialize(IPlugInContainer container)
    {
        Logger.Log("EventlogPlugin.Initialize", Logger.eventLogLogLevel);

        _container = container;
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
            Manage.CreateAppendHostInfoElement(_hn, ref viewElement, out HostInfoElement);

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
            Logger.LogException("EventlogPlugin.SerializePluginInfo()", ex);
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
            Logger.LogException("EventlogPlugin.DeserializePluginInfo()", ex);
        }
    }

    public void SetContext(IContext ctx)
    {
        Hostinfo hn = ctx as Hostinfo;
        Logger.Log(String.Format("EventlogPlugin.SetHost(hn: {0}\n)",
        hn == null ? "<null>" : hn.ToString()), Logger.eventLogLogLevel);

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

        if (_pluginNode != null && _pluginNode.Nodes.Count == 0 && _hn.IsConnectionSuccess)
        {
            BuildLogNodes();
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
        Logger.Log("EventlogPlugin.GetPluginNode", Logger.eventLogLogLevel);

        if (_pluginNode == null)
        {

            Logger.Log("EventlogPlugin.GetPluginNode: running Manage.CreateIconNode(EventViewerControl)",
            Logger.eventLogLogLevel);
            _pluginNode = Manage.CreateIconNode(Properties.Resources.sTitleEventsPage,
            Properties.Resources.EventViewer_48,
            typeof(PluginNodePage),
            this);

            _pluginNode.ImageIndex = (int)Manage.ManageImageType.EventLog;
            _pluginNode.SelectedImageIndex = (int)Manage.ManageImageType.EventLog;
            _pluginNode.IsPluginNode = true;
        }

        return _pluginNode;
    }

    public void EnumChildren(LACTreeNode parentNode)
    {
        Logger.Log("EventlogPlugin.EnumChildren", Logger.eventLogLogLevel);
        if (_pluginNode != null && parentNode == _pluginNode && Hostinfo.HasCreds(_hn))
        {
            EventViewerControl eventlogPage = (EventViewerControl)_pluginNode.PluginPage;
            eventlogPage.LoadData(EventViewerControl.EventViewerNodeType.PLUGIN);
        }
        return;
    }

    public void SetCursor(Cursor cursor)
    {
        Logger.Log("EventlogPlugin.SetCursor", Logger.eventLogLogLevel);

        if (_container != null)
        {
            _container.SetCursor(cursor);
        }
    }

    public ContextMenu GetTreeContextMenu(LACTreeNode nodeClicked)
    {
        Logger.Log("EventlogPlugin.GetTreeContextMenu", Logger.eventLogLogLevel);

        if (nodeClicked == null)
        {
            return null;
        }
        else
        {
            eventlogPage = (EventViewerControl)nodeClicked.PluginPage;
            if (eventlogPage == null)
            {
                Type type = nodeClicked.NodeType;

                object o = Activator.CreateInstance(type);
                if (o is IPlugInPage)
                {
                    ((IPlugInPage)o).SetPlugInInfo(_container, nodeClicked.Plugin, nodeClicked, (LWTreeView) nodeClicked.TreeView, nodeClicked.sc);
                    eventlogPage = (EventViewerControl)nodeClicked.PluginPage;
                }

            }
            ContextMenu eventlogContextMenu = null;
            if (eventlogPage != null)
            {
                eventlogContextMenu = eventlogPage.GetTreeContextMenu();
            }
            if (_pluginNode == nodeClicked)
            {
                eventlogContextMenu = new ContextMenu();

                MenuItem m_item = new MenuItem("Set Target Machine", new EventHandler(cm_OnConnect));
                eventlogContextMenu.MenuItems.Add(0, m_item);

                m_item = new MenuItem("-");
                eventlogContextMenu.MenuItems.Add(1, m_item);

                m_item = new MenuItem("&View");

                MenuItem subm_item = new MenuItem("&Add/Remove Columns...", new EventHandler(cm_OnMenuClick));
                m_item.MenuItems.Add(subm_item);

                subm_item = new MenuItem("-");
                m_item.MenuItems.Add(subm_item);

                subm_item = new MenuItem("C&ustomize View...", new EventHandler(cm_OnMenuClick));
                m_item.MenuItems.Add(subm_item);

                eventlogContextMenu.MenuItems.Add(2, m_item);

                m_item = new MenuItem("-");
                eventlogContextMenu.MenuItems.Add(3,m_item);

                m_item = new MenuItem("&Help", new EventHandler(cm_OnMenuClick));
                eventlogContextMenu.MenuItems.Add(eventlogContextMenu.MenuItems.Count, m_item);
            }
            return eventlogContextMenu;
        }
    }

    public void SetSingleSignOn(bool useSingleSignOn)
    {
        // do nothing
    }

    public bool PluginSelected()
    {
        return SelectComputer();
    }

    public void AddExtPlugin(IPlugIn extPlugin)
    {
        if (_extPlugins == null)
        {
            _extPlugins = new List<IPlugIn>();
        }

        _extPlugins.Add(extPlugin);
    }

    #endregion

    #region eventlog API wrappers

    public bool OpenEventLog(string hostname)
    {
        try
        {
            if (eventLogHandle == null)
            {
                eventLogHandle = EventlogAdapter.OpenEventlog(hostname);
            }

            return (eventLogHandle != null);
        }
        catch (Exception e)
        {
            Logger.LogException("EventViewerPlugin.OpenEventLog", e);
            eventLogHandle = null;
            return false;
        }
    }

    public bool EventLogIsOpen()
    {
        if (eventLogHandle == null)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    public EventLogRecord[] ReadEventLog(UInt32 dwLastRecordId,
    UInt32 nMaxRecords,
    string sqlQuery)
    {
        EventLogRecord[] result = null;

        if (eventLogHandle == null)
        {
            return null;
        }

        result = EventlogAdapter.ReadEventLog(eventLogHandle,
        dwLastRecordId,
        nMaxRecords,
        sqlQuery);

        return result;
    }

    public UInt32 CountEventLog(string sqlQuery)
    {
        UInt32 result = EventlogAdapter.CountLogs(eventLogHandle, sqlQuery);
        return result;
    }

    public void CloseEventLog()
    {
        if (eventLogHandle == null)
        {
            return;
        }

        eventLogHandle.Dispose();

        eventLogHandle = null;

    }



    #endregion

    #region HelperFunctions

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
                Logger.eventLogLogLevel);
                if (eventLogHandle != null)
                    _hn.IsConnectionSuccess = true;
            }
            else
            {
                if (_pluginNode != null && !String.IsNullOrEmpty(_hn.hostName))
                {
                    logs = null;

                    if (eventLogHandle == null)
                    {
                        _container.ShowError("Unable to open the event log; eventlog server may be disabled");
                        _pluginNode.sc.ShowControl(_pluginNode);
                    }
                }
            }
        }
    }

    private void BuildLogNodes()
    {
        if (_pluginNode != null)
        {
            if (eventLogHandle != null)
                _pluginNode.Text = "Event Viewer for " + _hn.hostName + "";
            else
                _pluginNode.Text = Properties.Resources.sTitleEventsPage;

            _pluginNode.Nodes.Clear();

            //UInt32 eventCount = EventlogAdapter.GetCategoryCount(eventLogHandle);
            UInt32 eventCount = 4;

            if (eventCount != 0)
            {
                //logs = EventlogAdapter.GetDistinctCategories(eventLogHandle, eventCount);
                logs = new string[]{
                                                    "Application",
                                                    "Security",
                                                    "System",
                                                    "WebBrowser"
                                               };
                if (logs != null)
                {
                    foreach (string log in logs)
                    {
                        //this bitmap never gets used for display, but is required by the LACTreeNode constructor.
                        Bitmap bmp = new Bitmap(Resources.EventViewer_48.ToBitmap(), 32, 32);

                        LACTreeNode logNode = new LACTreeNode(log.ToString(), bmp, typeof(LogNodePage), this);
                        logNode.Tag = ef;
                        logNode.ImageIndex = (int)Manage.ManageImageType.EventLog;
                        logNode.SelectedImageIndex = (int)Manage.ManageImageType.EventLog;
                        logNode.sc = _pluginNode.sc;
                        _pluginNode.Nodes.Add(logNode);
                    }
                }
            }
            _pluginNode.ExpandAll();
            _currentHost = _hn.hostName;

            EventViewerControl eventlogPage = _pluginNode.PluginPage as EventViewerControl;
            if (eventlogPage != null)
                eventlogPage.LoadData(EventViewerControl.EventViewerNodeType.PLUGIN);
        }
    }

    private void cm_OnMenuClick(object sender, EventArgs e)
    {
        MenuItem mi = sender as MenuItem;
        if (mi == null)
        {
            return;
        }
        if (eventlogPage == null)
        {
            return;
        }
        switch (mi.Text.Trim())
        {
            case "&Add/Remove Columns...":
            eventlogPage.cm_OnMenuClick(sender, e);
            break;
            case "C&ustomize View...":
            eventlogPage.cm_OnMenuClick(sender, e);
            break;
            case "&Help":
            eventlogPage.cm_OnMenuClick(sender, e);
            break;
        }
    }

    private void ConnectToDomain()
    {
        Logger.Log("EventlogPlugin.ConnectToDomain", Logger.eventLogLogLevel);

        if (_hn.creds.Invalidated)
        {
            _container.ShowError("EventlogPlugin cannot connect to domain due to invalid credentials");
            _hn.IsConnectionSuccess = false;
            return;
        }

        if (!String.IsNullOrEmpty(_hn.hostName))
        {
            if (_currentHost != _hn.hostName)
            {
                if (eventLogHandle != null)
                {
                    eventLogHandle.Dispose();
                    eventLogHandle = null;
                }

                if (_pluginNode != null && !String.IsNullOrEmpty(_hn.hostName))
                {
                    _hn.IsConnectionSuccess = OpenEventLog(_hn.hostName);
                    if (!_hn.IsConnectionSuccess)
                    {
                        Logger.ShowUserError("Unable to open the event log; eventlog server may be disabled");
                        return;
                    }

                    if (eventLogHandle != null)
                        _pluginNode.Nodes.Clear();
                }
                _currentHost = _hn.hostName;
            }
            _hn.IsConnectionSuccess = true;
        }
        else
        {
            _hn.IsConnectionSuccess = false;
        }
    }

    private bool SelectComputer()
    {
        Logger.Log(String.Format(
        "LUGPlugin.SelectComputer: hn: {0}",
        _hn));

        SelectComputerDialog selectDlg = new SelectComputerDialog(
            _hn.hostName,
            _hn.creds.UserName);

        if (selectDlg.ShowDialog() == DialogResult.OK)
        {
            int result = (int)ErrorCodes.WIN32Enum.ERROR_SUCCESS;

            _hn.hostName = selectDlg.GetHostname();

            if (!selectDlg.UseDefaultUserCreds())
            {
                _hn.creds.UserName = selectDlg.GetUsername();
                _hn.creds.Password = selectDlg.GetPassword();

                // Only add a connection if we are not using default creds.
                result = (int)LUGAPI.NetAddConnection(
                    _hn.hostName,
                    _hn.creds.UserName,
                    _hn.creds.Password);

                if (result != (int)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
                {
                    MessageBox.Show(
                       "Unable to connect to system:\n" + ErrorCodes.WIN32String(result),
                       "Likewise Administrative Console",
                       MessageBoxButtons.OK,
                       MessageBoxIcon.Exclamation);

                    return false;
                }
            }

            // This function is going to set _hn.IsConnectionSuccess... return
            // that value for this selection.
            ConnectToDomain();

            return _hn.IsConnectionSuccess;
        }

        return false;
    }

    #endregion

}

#region derived classes

public class PluginNodePage : EventViewerControl
{
    #region Constructor
    public PluginNodePage()
    : base(EventViewerNodeType.PLUGIN)
    {

    }
    #endregion
}

public class LogNodePage : EventViewerControl
{
    #region Constructor
    public LogNodePage()
    : base(EventViewerNodeType.LOG)
    {

    }
    #endregion
}
public interface IDirectoryPropertiesPage
{
    void SetData();
}

#endregion

}
