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
using Likewise.LMC.ServerControl;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.FileBrowser
{
    public partial class NewSharePermissionsPage : WizardPage
    {
        #region Class Data

        public static string PAGENAME = "NewSharePermissionsPage";
        private IPlugIn plugin = null;
        private IPlugInContainer container = null;        
        private NewShareWizardDlg parentDlg = null;

        #endregion

        #region Constructors

        public NewSharePermissionsPage(WizardDialog wizardDialog, IPlugIn plugin, IPlugInContainer container)
        {
            InitializeComponent();

            this.plugin = plugin;
            this.container = container;
            this.parentDlg = wizardDialog as NewShareWizardDlg;
        }

        #endregion

        #region Override Methods

        public override string OnWizardBack()
        {
            return base.OnWizardBack();
        }

        public override string OnWizardNext()
        { 
            return base.OnWizardNext();
        }

        public override bool OnSetActive()
        {
            ReSetControlData();

            Wizard.enableButton(WizardDialog.WizardButton.Cancel);
            Wizard.enableButton(WizardDialog.WizardButton.Back);
            Wizard.hideButton(WizardDialog.WizardButton.Start);
            Wizard.hideButton(WizardDialog.WizardButton.Finish);
            Wizard.showButton(WizardDialog.WizardButton.Next);
            Wizard.enableButton(WizardDialog.WizardButton.Next);

            return true;
        }

        #endregion

        #region Helper Functions

        private void ReSetControlData()
        {
            radioButtonAdminAccess.Checked =
            radioButtonUsernoAccess.Checked =
            radioButtonCustomPer.Checked = false;

            radioButtonUsersReadyOnly.Checked = true;
        }

        #endregion
    }
}
