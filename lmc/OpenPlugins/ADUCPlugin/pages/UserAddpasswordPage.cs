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
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
public partial class UserAddpasswordPage : WizardPage
{
    
    #region Class Data
    private ADUserAddDlg _userAddDlg;
    #endregion
    
    
    #region Constructors
    public UserAddpasswordPage()
    {
        InitializeComponent();
    }
    
    public UserAddpasswordPage(ADUserAddDlg userAddDlg)
    : this()
    {
        this._userAddDlg = userAddDlg;
        this.txtcreatein.Text = "Create in: " + userAddDlg.userInfo.OUPath;
    }
    #endregion
    
    #region Override Methods
    public override string OnWizardNext()
    {
        if (!this.pwtextBox.Text.Equals(this.confirmpwtextBox.Text))
        {
            string sMsg = "The passwords do not match";
            ShowMessage(sMsg);
            this.pwtextBox.Text = "";
            this.confirmpwtextBox.Text = "";
            return WizardDialog.NoPageChange;
        }
        this._userAddDlg.userInfo.bMustChangePwd = this.cbMustChangePwd.Checked;
        this._userAddDlg.userInfo.bCannotChangePwd = this.cbCannotChangePwd.Checked;
        this._userAddDlg.userInfo.bNeverExpiresPwd = this.cbNeverExpiresPwd.Checked;
        this._userAddDlg.userInfo.bAcountDisable = this.cbAccountDisable.Checked;
        return base.OnWizardNext();
    }
    
    public override string OnWizardBack()
    {
        if (!this.pwtextBox.Text.Trim().Equals(this.confirmpwtextBox.Text.Trim()))
        {
            string sMsg = "The passwords do not match";
            ShowMessage(sMsg);
            this.pwtextBox.Text = "";
            this.confirmpwtextBox.Text = "";
            return WizardDialog.NoPageChange; ;
        }
        this._userAddDlg.userInfo.bMustChangePwd = this.cbMustChangePwd.Checked;
        this._userAddDlg.userInfo.bCannotChangePwd = this.cbCannotChangePwd.Checked;
        this._userAddDlg.userInfo.bNeverExpiresPwd = this.cbNeverExpiresPwd.Checked;
        this._userAddDlg.userInfo.bAcountDisable = this.cbAccountDisable.Checked;
        return base.OnWizardBack();
    }
    
    public override bool OnSetActive()
    {
        cbMustChangePwd.CheckedChanged -= new EventHandler(cbMustChangePwd_CheckedChanged);
        cbCannotChangePwd.CheckedChanged -= new EventHandler(cbCannotChangePwd_CheckedChanged);
        cbNeverExpiresPwd.CheckedChanged -= new EventHandler(cbNeverExpiresPwd_CheckedChanged);
        cbAccountDisable.CheckedChanged -= new EventHandler(cbAccountDisable_CheckedChanged);

        cbMustChangePwd.Checked = _userAddDlg.userInfo.bMustChangePwd;
        cbCannotChangePwd.Checked = _userAddDlg.userInfo.bCannotChangePwd;
        cbNeverExpiresPwd.Checked = _userAddDlg.userInfo.bNeverExpiresPwd;
        cbAccountDisable.Checked = _userAddDlg.userInfo.bAcountDisable;

        cbMustChangePwd.CheckedChanged += new EventHandler(cbMustChangePwd_CheckedChanged);
        cbCannotChangePwd.CheckedChanged += new EventHandler(cbCannotChangePwd_CheckedChanged);
        cbNeverExpiresPwd.CheckedChanged += new EventHandler(cbNeverExpiresPwd_CheckedChanged);
        cbAccountDisable.CheckedChanged += new EventHandler(cbAccountDisable_CheckedChanged);
        
        Wizard.enableButton(WizardDialog.WizardButton.Cancel);
        Wizard.enableButton(WizardDialog.WizardButton.Back);
        Wizard.enableButton(WizardDialog.WizardButton.Next);
        Wizard.showButton(WizardDialog.WizardButton.Next);
        Wizard.hideButton(WizardDialog.WizardButton.Finish);
        Wizard.hideButton(WizardDialog.WizardButton.Start);
        
        return true;
    }
    #endregion
    
    #region Methods
    private void ShowMessage(string sMsg)
    {
        MessageBox.Show(
        sMsg,
        CommonResources.GetString("Caption_Console"),
        MessageBoxButtons.OK,
        MessageBoxIcon.None);
        return;
    }
    
    
    /// <summary>
    /// Sets the checkbox states
    /// </summary>
    private void  setCheckboxStates()
    {
        if (this.cbMustChangePwd.Checked )
        {
            if (this.cbNeverExpiresPwd.Checked && this.cbNeverExpiresPwd.Checked)
            {
                string sMsg = "You specified that the password should never expire.\nThe User will not be required to change the password at next logon";
                ShowMessage(sMsg);
                this.cbMustChangePwd.Checked = false;
            }
            else if (this.cbCannotChangePwd.Checked && !this.cbNeverExpiresPwd.Checked)
            {
                string sMsg = "You cannot check both User must change password at next longon.\nUser cannot change password for the same User.";
                ShowMessage(sMsg);
                this.cbCannotChangePwd.Checked = false;
            }
            else if (this.cbNeverExpiresPwd.Checked && !this.cbCannotChangePwd.Checked)
            {
                string sMsg = "You specified that the password should never expire.\nThe User will not be required to change the password at next logon";
                ShowMessage(sMsg);
                this.cbMustChangePwd.Checked = false;
            }
        }
        else if (this.cbCannotChangePwd.Checked)
        {
            if (this.cbMustChangePwd.Checked)
            {
                string sMsg = "You cannot check both User must change password at next longon.\nUser cannot change password for the same User.";
                ShowMessage(sMsg);
                this.cbCannotChangePwd.Checked = false;
            }
        }
        else if (this.cbNeverExpiresPwd.Checked)
        {
            if (this.cbMustChangePwd.Checked)
            {
                string sMsg = "You specified that the password should never expire \n The User will not be required to change the password at next logon";
                ShowMessage(sMsg);
                this.cbMustChangePwd.Checked = false;
            }
        }
    }
    #endregion
    
    #region Events
    private void pwtextBox_TextChanged(object sender, EventArgs e)
    {
        this._userAddDlg.userInfo.passWord = this.pwtextBox.Text;
    }
    
    private void confirmpwtextBox_TextChanged(object sender, EventArgs e)
    {
        this._userAddDlg.userInfo.retypedpassWord = this.confirmpwtextBox.Text;
    }
    
    private void cbMustChangePwd_CheckedChanged(object sender, EventArgs e)
    {
        if (cbMustChangePwd.Checked)
        {
            setCheckboxStates();
        }
    }
    
    private void cbCannotChangePwd_CheckedChanged(object sender, EventArgs e)
    {
        if (cbCannotChangePwd.Checked)
        {
            setCheckboxStates();
        }
    }
    
    private void cbNeverExpiresPwd_CheckedChanged(object sender, EventArgs e)
    {
        if (cbNeverExpiresPwd.Checked)
        {
            setCheckboxStates();
        }
    }
    
    private void cbAccountDisable_CheckedChanged(object sender, EventArgs e)
    {
        if (cbAccountDisable.Checked)
        {
            this._userAddDlg.userInfo.bAcountDisable = true;
            setCheckboxStates();
        }
    }
    
    private void pwtextBox_KeyDown(object sender, KeyEventArgs e)
    {
        if (e.Control == true && e.KeyCode == Keys.C)
        {
            //ToolTip tTip = new ToolTip();
            //string str = " Not Allowed \n You Can't copy text from a password field";
            //tTip.Show(str, this.pwtextBox, 10000);
            Application.DoEvents();
            e.SuppressKeyPress = true;
        }
    }    
    
    #endregion
    
}
}

