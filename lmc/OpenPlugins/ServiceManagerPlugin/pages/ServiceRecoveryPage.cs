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
using System.Runtime.InteropServices;

using Likewise.LMC.ServerControl;
using Likewise.LMC.Services;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.Plugins.ServiceManagerPlugin
{
    public partial class ServiceRecoveryPage : MPPage, IDirectoryPropertiesPage
    {
        #region Class Data

        private string serviceName;
        private ServiceManagerPlugin _plugin = null;
        private string[] sFailureMsgs = new string[]{
                                "Take No Action",
                                "Restart the Service",
                                "Run a program",
                                "Restart the Computer"
                                };
        private ServiceManagerApi.SERVICE_FAILURE_ACTIONS failureActions;

        private bool IsNoAction = false;
        private bool IsResetService = false;
        private bool IsRunProg = false;
        private bool IsResetComp = false;

        #endregion

        #region Constructors

        public ServiceRecoveryPage()
        {
            InitializeComponent();
        }

        public ServiceRecoveryPage(IPlugInContainer container, ServiceManagerPlugin plugin, string serviceName)
            : base()
        {
            InitializeComponent();

            this.pageID = "ServiceRecoveryPage";
            this.serviceName = serviceName;
            this._plugin = plugin;
        }

        #endregion

        #region Overriden functions

        public void SetData()
        {
            cbFirstfailure.Items.AddRange(sFailureMsgs);
            cbSecondFailure.Items.AddRange(sFailureMsgs);
            cbSubsquentFailure.Items.AddRange(sFailureMsgs);

            cbFirstfailure.SelectedIndexChanged -= new EventHandler(this.cbFirstfailure_SelectedIndexChanged);
            cbSecondFailure.SelectedIndexChanged -= new EventHandler(this.cbFirstfailure_SelectedIndexChanged);
            cbSubsquentFailure.SelectedIndexChanged -= new EventHandler(this.cbFirstfailure_SelectedIndexChanged);

            cbFirstfailure.SelectedItem = cbSecondFailure.SelectedItem = cbSubsquentFailure.SelectedItem = sFailureMsgs[0];

            failureActions = ServiceManagerWindowsWrapper.ApiQueryServiceConfig2(serviceName);
            ServiceManagerApi.SC_ACTION[] actions = new ServiceManagerApi.SC_ACTION[failureActions.cActions];
            int offset = 0;
            for (int i = 0; i < failureActions.cActions; i++)
            {
                ServiceManagerApi.SC_ACTION action = new ServiceManagerApi.SC_ACTION();
                action.Type = Marshal.ReadInt32(failureActions.lpsaActions, offset);
                offset += sizeof(Int32);
                action.Delay = (Int32)Marshal.ReadInt32(failureActions.lpsaActions, offset);
                offset += sizeof(Int32);
                actions[i] = action;

                switch (action.Type)
                {
                    case (int)ServiceManagerApi.SC_ACTION_TYPE.SC_ACTION_NONE:
                        txtDays.Text = action.Delay.ToString();
                        if (i == 0)
                            cbFirstfailure.SelectedItem = 0;
                        else if (i == 1)
                            cbSecondFailure.SelectedItem = 0;
                        else
                            cbSubsquentFailure.SelectedItem = 0;
                        break;

                    case (int)ServiceManagerApi.SC_ACTION_TYPE.SC_ACTION_REBOOT:
                        txtMinutes.Text = action.Delay.ToString();
                        if (i == 0)
                            cbFirstfailure.SelectedItem = 3;
                        else if (i == 1)
                            cbSecondFailure.SelectedItem = 3;
                        else
                            cbSubsquentFailure.SelectedItem = 3;
                        break;

                    case (int)ServiceManagerApi.SC_ACTION_TYPE.SC_ACTION_RESTART:
                        btnBrowse.Enabled = true;
                        if (i == 0)
                            cbFirstfailure.SelectedItem = 1;
                        else if (i == 1)
                            cbSecondFailure.SelectedItem = 1;
                        else
                            cbSubsquentFailure.SelectedItem = 1;
                        break;

                    case (int)ServiceManagerApi.SC_ACTION_TYPE.SC_ACTION_RUN_COMMAND:
                        txtProgram.Text = failureActions.lpCommand;
                        if (i == 0)
                            cbFirstfailure.SelectedItem = 2;
                        else if (i == 1)
                            cbSecondFailure.SelectedItem = 2;
                        else
                            cbSubsquentFailure.SelectedItem = 2;
                        break;

                    default:
                        break;
                }
            }
            if (failureActions.cActions != 0)
                SetControlStates(sFailureMsgs[0]);

            cbFirstfailure.SelectedIndexChanged += new EventHandler(this.cbFirstfailure_SelectedIndexChanged);
            cbSecondFailure.SelectedIndexChanged += new EventHandler(this.cbFirstfailure_SelectedIndexChanged);
            cbSubsquentFailure.SelectedIndexChanged += new EventHandler(this.cbFirstfailure_SelectedIndexChanged);
        }

        #endregion

        #region Helper functions

        private void SetControlStates(string sFailureText)
        {
            switch (sFailureText)
            {
                case "Take No Action":
                    if ((cbFirstfailure.SelectedItem.ToString().Trim().Equals(sFailureText.Trim())) &&
                      (cbSecondFailure.SelectedItem.ToString().Trim().Equals(sFailureText.Trim())) &&
                      (cbSubsquentFailure.SelectedItem.ToString().Trim().Equals(sFailureText.Trim())))
                        IsNoAction = true;
                    break;

                case "Restart the Service":
                    if ((cbFirstfailure.SelectedItem.ToString().Trim().Equals(sFailureText.Trim())) ||
                       (cbSecondFailure.SelectedItem.ToString().Trim().Equals(sFailureText.Trim())) ||
                       (cbSubsquentFailure.SelectedItem.ToString().Trim().Equals(sFailureText.Trim())))
                    {
                        IsResetService = true;
                        IsNoAction = false;
                    }
                    break;

                case "Run a program":
                    if ((cbFirstfailure.SelectedItem.ToString().Trim().Equals(sFailureText.Trim())) ||
                       (cbSecondFailure.SelectedItem.ToString().Trim().Equals(sFailureText.Trim())) ||
                       (cbSubsquentFailure.SelectedItem.ToString().Trim().Equals(sFailureText.Trim())))
                    {
                        IsRunProg = true;
                        IsNoAction = false;
                    }
                    break;

                case "Restart the Computer":
                    if ((cbFirstfailure.SelectedItem.ToString().Trim().Equals(sFailureText.Trim())) ||
                       (cbSecondFailure.SelectedItem.ToString().Trim().Equals(sFailureText.Trim())) ||
                       (cbSubsquentFailure.SelectedItem.ToString().Trim().Equals(sFailureText.Trim())))
                    {
                        IsResetComp = true;
                        IsNoAction = false;
                    }
                    break;
            }

            if (IsNoAction)
            {
                pnlResetDays.Enabled = pnlResetMinutes.Enabled = groupBoxRunProgram.Enabled = btnComputerOptions.Enabled = false;
                return;
            }
            else
            {
                pnlResetMinutes.Enabled = IsResetService;
                groupBoxRunProgram.Enabled = IsRunProg;
                btnComputerOptions.Enabled = IsResetComp;
            }
        }

        #endregion

        #region Event Handlers

        private void cbFirstfailure_SelectedIndexChanged(object sender, EventArgs e)
        {
            ComboBox cb = sender as ComboBox;
            SetControlStates(cb.SelectedItem.ToString());
        }

        private void btnBrowse_Click(object sender, EventArgs e)
        {
            OpenFileDialog openFileDlg = new OpenFileDialog();
            openFileDlg.CheckFileExists = true;
            openFileDlg.Filter = "Executable Files(*.exe,*.com,*.cmd,*.bat)|*.exe";
            openFileDlg.InitialDirectory = System.Environment.SystemDirectory;
            openFileDlg.Multiselect = false;

            if (openFileDlg.ShowDialog(this) == DialogResult.OK)
                txtProgram.Text = openFileDlg.FileName;
        }

        private void btnComputerOptions_Click(object sender, EventArgs e)
        {
            ServicesComputerOptionsPage CompOptions = new ServicesComputerOptionsPage(serviceName);
            CompOptions.sRestartMunites = failureActions.dwResetPeriod;
            CompOptions.sRebootMsg = failureActions.lpRebootMsg;
            if (CompOptions.ShowDialog(this) == DialogResult.OK)
            {
                failureActions.lpRebootMsg = CompOptions.sRebootMsg;
                failureActions.dwResetPeriod = CompOptions.sRestartMunites;
            }
        }

        #endregion

        #region Overriden functions

        public bool OnApply()
        {
            Logger.Log("In ServiceRecoveryPage: OnApply()");

            ServiceManagerApi.SERVICE_FAILURE_ACTIONS failureActionUpdate = new ServiceManagerApi.SERVICE_FAILURE_ACTIONS();
            failureActions.cActions = 2;
            IntPtr lpsaActions = IntPtr.Zero;

            ServiceManagerApi.SC_ACTION action = new ServiceManagerApi.SC_ACTION();
            if (txtDays.Enabled)
            {
                action.Type = (int)ServiceManagerApi.SC_ACTION_TYPE.SC_ACTION_NONE;
                action.Delay = (int)TimeSpan.FromMinutes(int.Parse(txtDays.Text.Trim())).TotalMilliseconds;

                lpsaActions = Marshal.AllocHGlobal(Marshal.SizeOf(action) * 2);
                if (lpsaActions == IntPtr.Zero)
                {
                    Logger.Log(String.Format("Unable to allocate memory for service action, error was: 0x{0:X}", Marshal.GetLastWin32Error()));
                }
                Marshal.StructureToPtr(action, lpsaActions, false);
                if (txtMinutes.Enabled)
                {
                    action = new ServiceManagerApi.SC_ACTION();
                    action.Type = (int)ServiceManagerApi.SC_ACTION_TYPE.SC_ACTION_RESTART;
                    action.Delay = (int)TimeSpan.FromMinutes(int.Parse(txtMinutes.Text.Trim())).TotalMilliseconds;

                    IntPtr nextAction = (IntPtr)(lpsaActions.ToInt64() + Marshal.SizeOf(action));
                    Marshal.StructureToPtr(action, nextAction, false);
                }
                failureActions.lpsaActions = lpsaActions;
            }

            if (txtProgram.Enabled)
                failureActionUpdate.lpCommand = txtProgram.Text.Trim() + " " + txtCommandlines.Text.Trim();

            if (btnBrowse.Enabled)
            {
                failureActionUpdate.lpRebootMsg = failureActions.lpRebootMsg;
                failureActionUpdate.dwResetPeriod = failureActions.dwResetPeriod;
            }

            if (ServiceManagerWindowsWrapper.ApiChangeServiceConfig2(
                                            serviceName, failureActionUpdate))
            {
                Logger.Log("ServiceRecoveryPage:OnApply():ApiChangeServiceConfig2()");
                //return false;
            }

            if (lpsaActions != IntPtr.Zero)
                Marshal.FreeHGlobal(lpsaActions);

            return true;
        }

        #endregion
    }
}
