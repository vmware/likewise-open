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
using System.Diagnostics;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;
using Likewise.LMC.Services;

namespace Likewise.LMC.Plugins.ServiceManagerPlugin
{
    public partial class ServiceManagerEditorPage : StandardPage
    {
        #region Class Data

        private ListViewColumnSorter lvwColumnSorter;
        private ServiceManagerPlugin plugin = null;
        private ServiceInfo serviceInfo = null;

        #endregion

        #region Constructors

        public ServiceManagerEditorPage()
        {
            InitializeComponent();

            // Create an instance of a ListView column sorter and assign it
            // to the ListView control.
            lvwColumnSorter = new ListViewColumnSorter();
            //this.lvRegistryPage.ListViewItemSorter = lvwColumnSorter;
        }

        #endregion

        #region overriden functions

        public override void SetPlugInInfo(IPlugInContainer container, IPlugIn pi, LACTreeNode treeNode, LWTreeView lmctreeview, CServerControl sc)
        {
            plugin = pi as ServiceManagerPlugin;

            base.SetPlugInInfo(container, pi, treeNode, lmctreeview, sc);
            bEnableActionMenu = false;
            ShowHeaderPane(true);

            Refresh();
        }

        public override void Refresh()
        {
            base.Refresh();

            if (plugin == null)
                plugin = pi as ServiceManagerPlugin;

            this.lblCaption.Text = plugin.GetPlugInNode().Text;

            if (plugin.IsConnectionSuccess)
            {
                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                {
                    if (!plugin.Do_LogonSCManager())
                    {
                        Logger.Log("Service Control Manager.Refresh(): Failed to authenticate the specified user");
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

            if (treeNode.Nodes.Count == 0 || treeNode.IsModified)
            {
                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                {
                    Dictionary<string, string[]> services = ServiceManagerWindowsWrapper.EnumManagementServices();
                    if (services != null && services.Count != 0)
                    {
                        foreach (string name in services.Keys)
                        {
                            ListViewItem lvItem = new ListViewItem(services[name]);
                            lvItem.Tag = name;
                            lvService.Items.Add(lvItem);
                        }
                    }
                }
                else
                {
                    string[] sServiceEnum = ServiceManagerInteropWrapper.ApiLwSmEnumerateServices();
                    if (sServiceEnum != null)
                    {
                        foreach (string name in sServiceEnum)
                        {
                            string serviceState = string.Empty;
                            IntPtr pHandle = ServiceManagerInteropWrapper.ApiLwSmAcquireServiceHandle(name);
                            if (pHandle != IntPtr.Zero)
                            {
                                ServiceManagerApi.LW_SERVICE_INFO serviceInfo = ServiceManagerInteropWrapper.ApiLwSmQueryServiceInfo(pHandle);
                                ServiceManagerApi.LW_SERVICE_STATUS serviceStatus = ServiceManagerInteropWrapper.ApiLwSmQueryServiceStatus(pHandle);

                                switch (serviceStatus.state)
                                {
                                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_DEAD:
                                        serviceState = "Dead";
                                        break;

                                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_PAUSED:
                                        serviceState = "Paused";
                                        break;

                                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_RUNNING:
                                        serviceState = "Running";
                                        break;

                                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_STARTING:
                                        serviceState = "Starting";
                                        break;

                                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_STOPPED:
                                        serviceState = "Stopped";
                                        break;

                                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_STOPPING:
                                        serviceState = "Stopping";
                                        break;
                                }

                                ListViewItem lvItem = new ListViewItem(new string[] { name,
                                                serviceInfo.pwszDescription, 
                                                serviceState,
                                                (serviceInfo.bAutostart)?"Automatic": "Manual", 
                                                serviceInfo.pwszPath });
                                lvItem.Tag = name;
                                lvService.Items.Add(lvItem);

                                ServiceManagerInteropWrapper.ApiLwSmReleaseServiceHandle(pHandle);
                            }
                        }
                    }
                }
            }

            lvService.Columns[lvService.Columns.Count - 1].Width = 200;
        }

        #endregion

        #region Helper functions

        public void On_MenuClick(object sender, EventArgs e)
        {
            MenuItem mi = sender as MenuItem;

            if (mi.Text.Trim().Equals("&Refresh"))
            {
                treeNode.IsModified = true;
                treeNode.sc.ShowControl(treeNode);
                return;
            }
            else if (mi.Text.Trim().Equals("&Help"))
            {
                ProcessStartInfo psi = new ProcessStartInfo();
                psi.UseShellExecute = true;
                psi.FileName = CommonResources.GetString("LAC_Help");
                psi.Verb = "open";
                psi.WindowStyle = ProcessWindowStyle.Normal;
                Process.Start(psi);
                return;
            }
            else if (mi.Text.Trim().Equals("&Properties"))
            {
                ServicePropertiesDlg dlg = new ServicePropertiesDlg(base.container, this, plugin, serviceInfo.serviceName.Trim());
                dlg.Show();

                if (dlg.commit)
                {
                    treeNode.IsModified = true;
                    treeNode.sc.ShowControl(treeNode);
                    return;
                }
            }
            else
            {
                if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                {
                    switch (mi.Text.Trim())
                    {
                        case "&Restart":
                            ServiceManagerWindowsWrapper.WMIServiceRestart(serviceInfo.serviceName);
                            break;

                        default:
                            Do_WinServiceInvoke(mi.Text.Trim());
                            break;
                    }
                }
                else
                {
                    int iRet = 0;
                    IntPtr pHandle = ServiceManagerInteropWrapper.ApiLwSmAcquireServiceHandle(mi.Tag as string);
                    switch (mi.Text.Trim())
                    {
                        case "&Restart":
                            if (pHandle != IntPtr.Zero)
                            {
                                iRet = ServiceManagerInteropWrapper.ApiLwSmRefreshService(pHandle);
                            }
                            break;

                        case "&Start":
                            if (pHandle != IntPtr.Zero)
                            {
                                StartAllServiceDependencies(pHandle, ref iRet);
                            }
                            break;

                        case "&Stop":
                            if (pHandle != IntPtr.Zero)
                            {
                                iRet = ServiceManagerInteropWrapper.ApiLwSmStopService(pHandle);
                            }
                            break;

                        default:
                            break;
                    }
                    if (iRet == (int)41202)
                    {
                        container.ShowError("The service is unable to start.\nPlease check the all its dependencies are started");
                    }
                    else if (iRet != 0)
                        container.ShowError("Failed to start the specified service: error code:" + iRet);

                    ServiceManagerInteropWrapper.ApiLwSmReleaseServiceHandle(pHandle);

                    if (iRet == 0)
                    {
                        treeNode.IsModified = true;
                        treeNode.sc.ShowControl(treeNode);
                        return;
                    }
                }
            }
        }

        private ServiceInfo GetUnixServiceInfo(string sservicename)
        {
            ServiceInfo serviceInfo = new ServiceInfo();
            serviceInfo.serviceName = lvService.SelectedItems[0].SubItems[0].Text;

            IntPtr pHandle = ServiceManagerInteropWrapper.ApiLwSmAcquireServiceHandle(serviceInfo.serviceName);
            if (pHandle != IntPtr.Zero)
            {
                ServiceManagerApi.LW_SERVICE_STATUS serviceStatus = ServiceManagerInteropWrapper.ApiLwSmQueryServiceStatus(pHandle);
                switch (serviceStatus.state)
                {
                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_RUNNING:
                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_STARTING:
                        serviceInfo.IsRunning = true;
                        break;

                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_STOPPED:
                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_STOPPING:
                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_PAUSED:
                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_DEAD:
                        serviceInfo.IsRunning = false;
                        break;
                }
                ServiceManagerInteropWrapper.ApiLwSmReleaseServiceHandle(pHandle);
            }

            return serviceInfo;
        }

        private int StartAllServiceDependencies(IntPtr pHandle, ref int iReturn)
        {
            if (pHandle != null)
            {
                string[] serviceDependencies = null;

                iReturn = ServiceManagerInteropWrapper.ApiLwSmQueryServiceDependencyClosure(pHandle, out serviceDependencies);
                if (iReturn != 0)
                    return iReturn;

                if (serviceDependencies != null && serviceDependencies.Length != 0) {
                    foreach (string service in serviceDependencies)
                    {
                        IntPtr pDHandle = ServiceManagerInteropWrapper.ApiLwSmAcquireServiceHandle(service);
                        if (pDHandle != IntPtr.Zero)
                        {
                            string[] serviceInnerDependencies = null;
                            iReturn = ServiceManagerInteropWrapper.ApiLwSmQueryServiceDependencyClosure(pDHandle, out serviceInnerDependencies);
                            if (iReturn == 0)
                            {
                                if (serviceInnerDependencies == null)
                                {
                                    iReturn = ServiceManagerInteropWrapper.ApiLwSmStartService(pDHandle);
                                    if (iReturn != 0)
                                        return iReturn;
                                }
                                else
                                    StartAllServiceDependencies(pDHandle, ref iReturn);
                            }
                            else
                                return iReturn;
                        }
                    }
                }
                else {
                    iReturn = ServiceManagerInteropWrapper.ApiLwSmStartService(pHandle);
                }
            }
            return iReturn;
        }


        private string GetServiceAction(string sServiceType)
        {
            string ServiceAction = string.Empty;

            switch (sServiceType)
            {
                case "&Start":
                    ServiceAction = "StartService";
                    break;

                case "&Stop":
                    ServiceAction = "StopService";
                    break;

                case "&Pause":
                    ServiceAction = "PauseService";
                    break;

                case "&Resume":
                    ServiceAction = "ResumeService";
                    break;

                case "&Restart":
                    ServiceAction = "RestartService";
                    break;

                default:
                    break;
            }
            return ServiceAction;
        }


        private void Do_WinServiceInvoke(string sServiceType)
        {
            string ServiceAction = GetServiceAction(sServiceType);

            if (ServiceManagerWindowsWrapper.InvokeWMIServiceMethod(ServiceAction, serviceInfo.serviceName))
            {
                if (ServiceAction == "StartService" || (ServiceAction == "ResumeService"))
                    lvService.SelectedItems[0].SubItems[2].Text = "Started";
                else if (ServiceAction == "PauseService")
                    lvService.SelectedItems[0].SubItems[2].Text = "Paused";             
                else
                    lvService.SelectedItems[0].SubItems[2].Text = string.Empty;
            }
            else
            {
                container.ShowError("Failed to " + sServiceType + " the service " + lvService.SelectedItems[0].SubItems[0].Text + ".");
            }

            this.Update();
        }

        #endregion

        #region ContextMenu builder functions

        public ContextMenu GetTreeContextMenu()
        {
            ContextMenu cm = new ContextMenu();

            MenuItem m_item = new MenuItem("&Start", new EventHandler(On_MenuClick));
            m_item.Tag = lvService.SelectedItems[0].Tag;
            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                m_item.Enabled = serviceInfo == null ? false : (serviceInfo.IsDisabled) ? false : !serviceInfo.IsRunning;
            else
                m_item.Enabled = serviceInfo == null ? false : !serviceInfo.IsRunning;
            cm.MenuItems.Add(0, m_item);

            m_item = new MenuItem("&Stop", new EventHandler(On_MenuClick));
            m_item.Tag = lvService.SelectedItems[0].Tag;
            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                m_item.Enabled = serviceInfo == null ? false : (serviceInfo.IsDisabled) ? false :
                                                                                          (serviceInfo.IsAcceptStop) ? serviceInfo.IsRunning : false;
            else
                m_item.Enabled = serviceInfo == null ? false : serviceInfo.IsRunning;
            cm.MenuItems.Add(m_item);

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
            {
                m_item = new MenuItem("&Pause", new EventHandler(On_MenuClick));
                m_item.Tag = lvService.SelectedItems[0].Tag;
                m_item.Enabled = serviceInfo == null ? false : (serviceInfo.IsDisabled) ? false :
                                                                                          (serviceInfo.IsAcceptPause) ? (!serviceInfo.IsPaused && serviceInfo.IsRunning) : false;
                cm.MenuItems.Add(m_item);

                m_item = new MenuItem("&Resume", new EventHandler(On_MenuClick));
                m_item.Tag = lvService.SelectedItems[0].Tag;
                m_item.Enabled = serviceInfo == null ? false : (serviceInfo.IsDisabled) ? false :
                                                                                         (serviceInfo.IsAcceptPause) ? serviceInfo.IsPaused : false;
                cm.MenuItems.Add(m_item);
            }

            m_item = new MenuItem("&Restart", new EventHandler(On_MenuClick));
            m_item.Tag = lvService.SelectedItems[0].Tag;
            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                m_item.Enabled = serviceInfo == null ? false : (serviceInfo.IsDisabled) ? false :
                                                                                          serviceInfo.IsRunning ? true : false;
            else
                m_item.Enabled = serviceInfo == null ? false : serviceInfo.IsRunning;
            cm.MenuItems.Add(m_item);

            m_item = new MenuItem("-");
            cm.MenuItems.Add(m_item);

            m_item = new MenuItem("&Refresh", new EventHandler(On_MenuClick));
            m_item.Tag = lvService.SelectedItems[0].Tag;
            cm.MenuItems.Add(m_item);

            m_item = new MenuItem("-");
            cm.MenuItems.Add(m_item);

            m_item = new MenuItem("&Properties", new EventHandler(On_MenuClick));
            m_item.Tag = lvService.SelectedItems[0].Tag;
            cm.MenuItems.Add(m_item);

            m_item = new MenuItem("-");
            cm.MenuItems.Add(m_item);

            m_item = new MenuItem("&Help", new EventHandler(On_MenuClick));
            cm.MenuItems.Add(cm.MenuItems.Count, m_item);

            return cm;
        }

        #endregion

        #region Events

        private void lvService_MouseUp(object sender, MouseEventArgs e)
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
                    if (lvitem.Tag is string && Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                        serviceInfo = ServiceManagerWindowsWrapper.GetServiceStateInfo(lvitem.Tag as string);
                    else
                        serviceInfo = GetUnixServiceInfo(lvitem.SubItems[0].Text.Trim());

                    ContextMenu cm = GetTreeContextMenu();
                    if (cm != null)
                    {
                        cm.Show(lvService, new Point(e.X, e.Y));
                    }
                }
                else
                {
                    ContextMenu cm = plugin.GetTreeContextMenu(treeNode);
                    if (cm != null)
                    {
                        cm.Show(lvSender, new Point(e.X, e.Y));
                    }
                }
            }
        }

        #endregion
    }
}
