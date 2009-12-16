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
using System.IO;
using Likewise.LMC.ServerControl;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.FileShareManager
{
    public partial class NewShareFolderSetUpPage : WizardPage
    {
        #region Class Data

        public static string PAGENAME = "NewShareFolderSetUpPage";
        private IPlugIn plugin = null;
        private IPlugInContainer container = null;
        private char[] invalidChars = new char[] { '*', '+', '=', '[', ']', '"', ';', ':', '/', '?', '>', '<', ',', '|', '\\' };
        private NewShareWizardDlg parentDlg = null;

        #endregion

        #region Constructors

        public NewShareFolderSetUpPage(WizardDialog wizardDialog, IPlugIn plugin, IPlugInContainer container)
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
            string errorMsg = string.Empty;

            bool bIsValidData = ValidateData(out errorMsg);

            if (!String.IsNullOrEmpty(errorMsg))
            {
                bIsValidData = true;
                container.ShowError(errorMsg);
                return WizardDialog.NoPageChange;
            }

            if(!bIsValidData)
                return WizardDialog.NoPageChange;

            parentDlg.shareInfo.folderName = textBoxFoldername.Text.Trim();
            parentDlg.shareInfo.hostName = textBoxComputername.Text.Trim();
            parentDlg.shareInfo.shareName = textBoxSharename.Text.Trim();
            parentDlg.shareInfo.shareDesc = textBoxShareDesc.Text.Trim();

            return base.OnWizardNext();
        }

        public override bool OnSetActive()
        {
            textBoxFoldername.Text = parentDlg.shareInfo.folderName;
            textBoxComputername.Text = parentDlg.shareInfo.hostName;
            textBoxSharename.Text = parentDlg.shareInfo.shareName;
            textBoxShareDesc.Text = parentDlg.shareInfo.shareDesc;

            Wizard.enableButton(WizardDialog.WizardButton.Cancel);
            Wizard.enableButton(WizardDialog.WizardButton.Back);
            Wizard.hideButton(WizardDialog.WizardButton.Start);
            Wizard.hideButton(WizardDialog.WizardButton.Finish);
            Wizard.showButton(WizardDialog.WizardButton.Next);
            Wizard.enableButton(WizardDialog.WizardButton.Next);

            return true;
        }

        #endregion

        #region Helper methods

        private bool ValidateData(out string errorMsg)
        {
            errorMsg = string.Empty;

            if (String.IsNullOrEmpty(textBoxFoldername.Text.Trim()))
            {
                errorMsg = "The feild \"Folder to share\" is not allowed to be empty.";
                return false;
            }

            if (!Path.IsPathRooted(textBoxFoldername.Text.Trim()))
            {
                errorMsg = "The path entered for the folder is not valid. Enter a valid path.";
                return false;
            }

            if (!Directory.Exists(textBoxFoldername.Text.Trim()))
            {
                string sMsg = string.Format("The system cannot find the specified path \"{0}\". Do you want to create it?", textBoxFoldername.Text.Trim());
                DialogResult dlg = MessageBox.Show(this, sMsg, CommonResources.GetString("Caption_Console"), MessageBoxButtons.YesNo,
                                 MessageBoxIcon.Information, MessageBoxDefaultButton.Button1);
                if (dlg == DialogResult.OK)
                {
                    try
                    {
                        Directory.CreateDirectory(textBoxFoldername.Text.Trim());
                    }
                    catch
                    {
                        errorMsg = string.Format("The following error occurred while creating the folder \"{0}\".", textBoxFoldername.Text.Trim());
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }

            if (String.IsNullOrEmpty(textBoxSharename.Text.Trim()))
            {
                errorMsg = "The feild \"Share name\" is not allowed to be empty.";
                return false;
            }

            foreach (char ch in textBoxSharename.Text.Trim())
            {
                foreach (char c in invalidChars)
                {
                    if (ch == c)
                    {
                        errorMsg = string.Format("The share name \"{0}\" is invalid", textBoxSharename.Text.Trim());
                        break;
                    }
                }
            }

            return true;
        }

        #endregion

        #region Events

        private void buttonBrowse_Click(object sender, EventArgs e)
        {
            if (folderBrowserDialog.ShowDialog(this) == DialogResult.OK)
            {
                textBoxFoldername.Text = folderBrowserDialog.SelectedPath;
            }
        }

        #endregion
    }
}
