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
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.FileShareManager
{
    public partial class NewShareWelcomePage : WizardPage
    {

        #region Class Data

        public static string PAGENAME = "NewShareWelcomePage";
        private FileShareManagerIPlugIn plugin = null;
        private IPlugInContainer container = null;
        private NewShareWizardDlg parentDlg = null;

        #endregion

        #region Constructors

        public NewShareWelcomePage(WizardDialog wizardDialog, IPlugIn plugin, IPlugInContainer container)
        {
            InitializeComponent();

            this.plugin = plugin as FileShareManagerIPlugIn;
            this.container = container;
            this.parentDlg = wizardDialog as NewShareWizardDlg;
        }

        #endregion

        #region Override Methods


        public override string OnWizardBack()
        {
            return "";
        }

        public override string OnWizardNext()
        {
            return base.OnWizardNext();
        }

        public override bool OnWizardCancel()
        {
            return base.OnWizardCancel();
        }

        /// <summary>
        /// When activating the control on a wizard dialog loads the specific data.
        /// </summary>
        /// <returns></returns>
        public override bool OnSetActive()
        {
            if (parentDlg.shareInfo.commitChnages)
            {
                parentDlg.shareInfo = new ShareInfo();
                parentDlg.shareInfo.hostName = plugin.HostInfo.hostName;
            }

            Wizard.enableButton(WizardDialog.WizardButton.Cancel);
            Wizard.disableButton(WizardDialog.WizardButton.Back);
            Wizard.hideButton(WizardDialog.WizardButton.Finish);
            Wizard.disableButton(WizardDialog.WizardButton.Finish);
            Wizard.hideButton(WizardDialog.WizardButton.Start);
            Wizard.showButton(WizardDialog.WizardButton.Next);
            Wizard.enableButton(WizardDialog.WizardButton.Next);

            return true;
        }

        #endregion

        #region Events

        private void FirewallLinklabel_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            ProcessStartInfo psi = new ProcessStartInfo();
            psi.UseShellExecute = true;
            psi.FileName = "http://www.microsoft.com/windowsxp/using/networking/security/winfirewall.mspx";
            psi.Verb = "open";
            psi.WindowStyle = ProcessWindowStyle.Normal;
            Process.Start(psi);
            return;
        }

        #endregion
    }
}
