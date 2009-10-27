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

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class ComputerAddHostServerPage : WizardPage
{
    #region Class Data
    private ADComputerAddDlg _computerAddDlg;
    public static string PAGENAME = "ComputerAddHostServerPage";
    #endregion
    
    #region Constructors
    public ComputerAddHostServerPage()
    {
        InitializeComponent();
    }
    
    public ComputerAddHostServerPage(ADComputerAddDlg computerAddDlg)
    : this()
    {
        this._computerAddDlg = computerAddDlg;
        this._computerAddDlg.Text = "Host server";
        this.txtCreatein.Text = "Create in: " + computerAddDlg.computerInfo.DomainName;
    }
    #endregion
    
    #region Override Methods
    public override string OnWizardNext()
    {
        if (rbRemoteSvrSelected.Checked)
        {
            if (this.textBox.Text.Trim() != "")
            {
                this._computerAddDlg.computerInfo.RemoteServers = textBox.Text.Trim();
            
}
        
}

            
            return base.OnWizardNext();
        }
        
        public override bool OnSetActive()
        {
            Wizard.enableButton(WizardDialog.WizardButton.Back);
            Wizard.enableButton(WizardDialog.WizardButton.Cancel);
            Wizard.enableButton(WizardDialog.WizardButton.Cancel);
            Wizard.showButton(WizardDialog.WizardButton.Next);
            Wizard.enableButton(WizardDialog.WizardButton.Next);
            Wizard.disableButton(WizardDialog.WizardButton.Start);
            Wizard.hideButton(WizardDialog.WizardButton.Start);
            Wizard.disableButton(WizardDialog.WizardButton.Finish);
            Wizard.hideButton(WizardDialog.WizardButton.Finish);
            
            if (!this._computerAddDlg.computerInfo.RemoteServers.Trim().Equals("Default"))
            {
                this.rbRemoteSvrSelected.Checked = true;
                this.textBox.Text = this._computerAddDlg.computerInfo.RemoteServers;
            }
            else
            {
                this.rbRemoteDefault.Checked = true;
            }
            return true;
        }
        #endregion
        
        #region Event Handlers
        private void rbRemoteDefault_CheckedChanged(object sender, EventArgs e)
        {
            this._computerAddDlg.computerInfo.RemoteServers = "Default";
        }
        
        private void rbRemoteSvrSelected_CheckedChanged(object sender, EventArgs e)
        {
            this.textBox.Enabled = rbRemoteSvrSelected.Checked;
            this.btnSearch.Enabled = rbRemoteSvrSelected.Checked;
        }
        #endregion
        
    }
}
