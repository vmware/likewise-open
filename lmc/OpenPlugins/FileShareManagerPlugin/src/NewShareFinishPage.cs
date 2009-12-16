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
    public partial class NewShareFinishPage : WizardPage
    {

        #region Class Data

        public static string PAGENAME = "NewShareFinishPage";
        private IPlugIn plugin = null;
        private IPlugInContainer container = null;
        private NewShareWizardDlg parentDlg = null;

        #endregion

        #region Constructors

        public NewShareFinishPage(WizardDialog wizardDialog, IPlugIn plugin, IPlugInContainer container)
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
            return "";
        }


        public override bool OnWizardFinish()
        {
            parentDlg.shareInfo.commitChnages = true;

            parentDlg.sharedFolderList.Add(parentDlg.shareInfo);

            if (checkBox.Checked)
            {
                WizardDialog.FirstPage = "NewShareWelcomePage";
                return false;
            }
            else
                WizardDialog.FirstPage = "";

            return base.OnWizardFinish();
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
            textBoxFoldername.Text = parentDlg.shareInfo.folderName;

            Wizard.disableButton(WizardDialog.WizardButton.Cancel);
            Wizard.disableButton(WizardDialog.WizardButton.Back);
            Wizard.showButton(WizardDialog.WizardButton.Finish);
            Wizard.enableButton(WizardDialog.WizardButton.Finish);
            Wizard.hideButton(WizardDialog.WizardButton.Next);
            Wizard.disableButton(WizardDialog.WizardButton.Next);
            Wizard.disableButton(WizardDialog.WizardButton.Start);
            Wizard.hideButton(WizardDialog.WizardButton.Start);

            return true;
        }

        #endregion

        #region Events

        #endregion
    }
}
