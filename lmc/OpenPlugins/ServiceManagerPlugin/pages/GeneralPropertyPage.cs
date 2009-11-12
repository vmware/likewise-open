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
using System.ServiceProcess;
using System.Management;

using Likewise.LMC.ServerControl;
using Likewise.LMC.Services;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.Plugins.ServiceManagerPlugin
{
    public partial class GeneralPropertyPage : MPPage, IDirectoryPropertiesPage
    {
        #region Class Data

        private string serviceName;
        private ServiceManagerPlugin _plugin = null;
        private ServicePropertiesDlg _parentDlg = null;

        #endregion               

        #region Construtcors

        public GeneralPropertyPage(ServicePropertiesDlg parentDlg, IPlugInContainer container, ServiceManagerPlugin plugin, string serviceName)
        {
            InitializeComponent();

            this.pageID = "GeneralPropertyPage";
            SetPageTitle("General");
            this.serviceName = serviceName;
            this._plugin = plugin;
            this._parentDlg = parentDlg;
        }

        #endregion

        #region overriden functions

        public void SetData()
        {
            this.ParentContainer.btnApply.Enabled = true;
            this.ParentContainer.DataChanged = true;

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows) {
                GetWindowsServiceInfo();
            }
            else{
                GetUnixServiceInfo();
            }
        }

        #endregion

        #region Helper functions

        private void GetWindowsServiceInfo()
        {
            foreach (ManagementObject mo in ServiceManagerWindowsWrapper.GetServiceCollection(serviceName))
            {
                bool canPause = (bool)mo["AcceptPause"];
                string status = mo["State"].ToString();

                txtDescription.Text = mo["Description"].ToString();
                txtPathToExecute.Text = mo["PathName"].ToString();
                lblServiceStatusValue.Text = mo["State"].ToString();
                lblServiceNameValue.Text = mo["Name"].ToString();
                txtDisplayName.Text = mo["DisplayName"].ToString();

                if (string.Equals(status, "Started"))
                {
                    //Service is started
                    btnStop.Enabled = true;
                    if (canPause)
                        btnPause.Enabled = true;
                    btnStart.Enabled = btnResume.Enabled = !btnStop.Enabled;
                }
                else if (string.Equals(status, "Stopped"))
                {
                    btnStart.Enabled = true;
                    btnPause.Enabled = btnResume.Enabled = btnStop.Enabled = !btnStart.Enabled;
                }
                else if (string.Equals(status, "Running"))
                {
                    btnStop.Enabled = true;
                    if (canPause)
                        btnPause.Enabled = true;
                    btnStart.Enabled = btnResume.Enabled = !btnStop.Enabled;
                }
                else if (string.Equals(status, "Paused"))
                {
                    btnStop.Enabled = btnResume.Enabled = true;
                    btnStart.Enabled = btnPause.Enabled = !btnStop.Enabled;
                }

                int compareResult = string.Compare(mo["StartMode"].ToString(), "Disabled");

                if (compareResult < 0)
                    cmbStartupType.SelectedIndex = 0;
                else if (compareResult > 0)
                    cmbStartupType.SelectedIndex = 1;
                else
                {
                    cmbStartupType.SelectedIndex = 2;
                    //Disable all buttons when service is disabled
                    btnStart.Enabled = btnStop.Enabled = btnPause.Enabled = btnResume.Enabled = false;
                }
            }
        }

        private void GetUnixServiceInfo()
        {
            txtStartParameters.Text = string.Empty;
            IntPtr pHandle = ServiceManagerInteropWrapper.ApiLwSmAcquireServiceHandle(serviceName);
            if (pHandle != IntPtr.Zero)
            {
                string sArgs = string.Empty;
                ServiceManagerApi.LW_SERVICE_STATUS serviceStatus = ServiceManagerInteropWrapper.ApiLwSmQueryServiceStatus(pHandle);
                ServiceManagerApi.LW_SERVICE_INFO serviceInfo = ServiceManagerInteropWrapper.ApiLwSmQueryServiceInfo(pHandle);

                lblServiceNameValue.Text = serviceName;
                txtDisplayName.Text = serviceInfo.pwszName;
                txtDescription.Text = serviceInfo.pwszDescription;
                txtPathToExecute.Text = serviceInfo.pwszPath;

                foreach (string args in serviceInfo.ppwszArgs)
                    txtStartParameters.Text += " " + args;

                txtStartParameters.Text = txtStartParameters.Text.Trim();
                btnPause.Visible = btnResume.Visible = false;

                switch (serviceStatus.state)
                {
                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_DEAD:
                        lblServiceStatusValue.Text = "Dead";
                        break;

                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_PAUSED:
                        lblServiceStatusValue.Text = "Paused";
                        break;

                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_RUNNING:
                        lblServiceStatusValue.Text = "Running";
                        break;

                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_STARTING:
                        lblServiceStatusValue.Text = "Starting";
                        break;

                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_STOPPED:
                        lblServiceStatusValue.Text = "Stopped";
                        break;

                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_STOPPING:
                        lblServiceStatusValue.Text = "Stopping";
                        break;
                }

                switch (serviceStatus.state)
                {
                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_RUNNING:
                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_STARTING:
                        btnStart.Enabled = false;
                        btnStop.Enabled = true;
                        break;


                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_STOPPED:
                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_STOPPING:
                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_PAUSED:
                    case ServiceManagerApi.LW_SERVICE_STATE.LW_SERVICE_STATE_DEAD:
                        btnStart.Enabled = true;
                        btnStop.Enabled = false;
                        break;
                }
                ServiceManagerInteropWrapper.ApiLwSmReleaseServiceHandle(pHandle);
            }
        }

        #endregion

        #region Event Handlers

        private void btnStart_Click(object sender, EventArgs e)
        {
            //Start Parameters are not implemented
            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows) {
                ServiceController sc = new ServiceController(serviceName);
                if (sc != null)
                {
                    sc.Start();
                    sc.WaitForStatus(ServiceControllerStatus.Running);
                }
            }
            else {
                IntPtr pHandle = ServiceManagerInteropWrapper.ApiLwSmAcquireServiceHandle(serviceName);
                if (pHandle != IntPtr.Zero)
                {
                    ServiceManagerInteropWrapper.ApiLwSmStartService(pHandle);
                    ServiceManagerInteropWrapper.ApiLwSmReleaseServiceHandle(pHandle);
                }
            }

            _parentDlg.commit = true;
            SetData();
        }

        private void btnStop_Click(object sender, EventArgs e)
        {
            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows) {
                ServiceController sc = new ServiceController(serviceName);
                if (sc != null)
                {
                    sc.Stop();
                    sc.WaitForStatus(ServiceControllerStatus.Stopped);
                }
            }
            else {
                IntPtr pHandle = ServiceManagerInteropWrapper.ApiLwSmAcquireServiceHandle(serviceName);
                if (pHandle != IntPtr.Zero) {
                    ServiceManagerInteropWrapper.ApiLwSmStopService(pHandle);
                    ServiceManagerInteropWrapper.ApiLwSmReleaseServiceHandle(pHandle);
                }
            }

			 _parentDlg.commit = true;
            SetData();
        }

        private void btnPause_Click(object sender, EventArgs e)
        {
            ServiceController sc = new ServiceController(serviceName);
            if (sc != null)
            {
                sc.Pause();
                sc.WaitForStatus(ServiceControllerStatus.Paused);
            }
			 _parentDlg.commit = true;
            SetData();
        }

        private void btnResume_Click(object sender, EventArgs e)
        {
            ServiceController sc = new ServiceController(serviceName);
            if (sc != null)
            {
                sc.Continue();
                sc.WaitForStatus(ServiceControllerStatus.Running);
            }
			 _parentDlg.commit = true;
            SetData();
        }

        private void txtStartParameters_TextChanged(object sender, EventArgs e)
        {
            this.ParentContainer.btnApply.Enabled = true;
            this.ParentContainer.DataChanged = true;
        }

        private void txtDisplayName_TextChanged(object sender, EventArgs e)
        {
            this.ParentContainer.btnApply.Enabled = true;
            this.ParentContainer.DataChanged = true;
        }

        #endregion
    }
}
