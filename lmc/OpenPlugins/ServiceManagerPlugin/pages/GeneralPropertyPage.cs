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

namespace Likewise.LMC.Plugins.ServiceManagerPlugin
{
    public partial class GeneralPropertyPage : MPPage, IDirectoryPropertiesPage
    {
        #region Class Data

        private string serviceName;
        private ServiceManagerPlugin _plugin = null;

        #endregion               

        #region Construtcors

        public GeneralPropertyPage(IPlugInContainer container, ServiceManagerPlugin plugin, string serviceName)
        {
            InitializeComponent();

            this.pageID = "GeneralPropertyPage";            
            SetPageTitle("General");
            this.serviceName = serviceName;
            this._plugin = plugin;
        }     

        #endregion

        #region overriden functions

        public void SetData()
        {
            this.ParentContainer.btnApply.Enabled = true;
            this.ParentContainer.DataChanged = true;

            foreach (ManagementObject mo in ServiceManagerWindowsWrapper.GetServiceCollection(_plugin.HostInfo.hostName, _plugin.HostInfo.creds.UserName, _plugin.HostInfo.creds.Password, serviceName))
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

        #endregion

        #region Event Handlers

        private void btnStart_Click(object sender, EventArgs e)
        {
            //Start Parameters are not implemented
            ServiceController sc = new ServiceController(serviceName);
            if (sc != null)
            {
                sc.Start();
                sc.WaitForStatus(ServiceControllerStatus.Running);
            }
            SetData();
        }

        private void btnStop_Click(object sender, EventArgs e)
        {
            ServiceController sc = new ServiceController(serviceName);
            if (sc != null)
            {
                sc.Stop();
                sc.WaitForStatus(ServiceControllerStatus.Stopped);
            }
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
