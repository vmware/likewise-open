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
public partial class ResetPassword : Form
{
    #region Class Data
    IPlugInContainer IPlugInContainer;
    public PasswordInfo passwordinfo;
    private bool bNeverExpiresPwd = false;
    private string sUser = "";
    private string tooltipMessage = string.Empty;
    //  private string Nstr = "Normal";
    #endregion
    
    #region Constructors
    public ResetPassword()
    {
        InitializeComponent();
        passwordinfo = new PasswordInfo();
    }
    
    
    public ResetPassword(IPlugInContainer container, StandardPage parentPage, bool bPwdNeverExpires, string user)
    : this()
    {
        this.IPlugInContainer = container;
        this.bNeverExpiresPwd = bPwdNeverExpires;
        this.sUser = user;
    }
    #endregion
    
    #region Events
    private void btnOk_Click(object sender, EventArgs e)
    {
        if (!this.txtNewpassword.Text.Equals(this.txtConfirmpassword.Text))
        {
            string sMsg = "The New and Confirm passwords must match. Please re-type them.";//"The passwords do not match";
            MessageBox.Show(sMsg, CommonResources.GetString("Caption_Console"),
            MessageBoxButtons.OK, MessageBoxIcon.Error);
            this.txtNewpassword.Text = "";
            this.txtConfirmpassword.Text = "";
            
            return;
        }
        else if (this.txtNewpassword.Text != "" && this.txtNewpassword.Text.Length < passwordinfo.PasswordMaxLength)
        {
            ShowUserMessage();
            this.txtNewpassword.Text = "";
            this.txtConfirmpassword.Text = "";
            return;
        }
        else if (this.txtNewpassword.Text == "" && this.txtConfirmpassword.Text == "")
        {
            ShowUserMessage();
        }
        else
        {
            this.Close();
            this.DialogResult = DialogResult.OK;
        }
    }
    
    private void btnCancel_Click(object sender, EventArgs e)
    {
        this.Close();
    }
    #endregion
    
    private void txtConfirmpassword_TextChanged(object sender, EventArgs e)
    {
        this.passwordinfo.retypedpassword = this.txtConfirmpassword.Text;
    }
    
    private void txtNewpassword_TextChanged(object sender, EventArgs e)
    {
        this.passwordinfo.password = this.txtNewpassword.Text;
    }
    
    private void checkBox_CheckedChanged(object sender, EventArgs e)
    {
        if (this.checkBox.Checked)
        {
            this.passwordinfo.MustChangePwNextLogon = true;
        }
        else
        {
            this.passwordinfo.MustChangePwNextLogon = false;
        }
    }
    
    private void ResetPassword_Load(object sender, EventArgs e)
    {
        if (bNeverExpiresPwd)
        {
            this.checkBox.Enabled = false;
        }
        else
        {
            this.checkBox.Enabled = true;
        }
    }
    
    private void ShowUserMessage()
    {
        string sMsg = string.Format("LAC cannot complete the password change for user {0} because : \n" +
        "The password does not meet the password policy reruirements.\nCheck the minimum password length," +
        "password complexity and \npassword history requirements", sUser);
        MessageBox.Show(this, sMsg, CommonResources.GetString("Caption_Console"),
        MessageBoxButtons.OK, MessageBoxIcon.Error);
        return;
    }
    
    private void whatsThisToolStripMenuItem_Click(object sender, EventArgs e)
    {
        toolTip1.Show(tooltipMessage, this, new Point(30, 80));
    }
    private void lblNewPassword_MouseClick(object sender, MouseEventArgs e)
    {
        if (e.Button == MouseButtons.Right)
        {
            tooltipMessage = "Provides a space for you to type a case-sensitive \n " +
            "password up to 127 characters.If you are on a network \n" +
            "with computers running Windows 95 and Windows 98, avoid \n" +
            "passwords longer than 14 characters.Type the password again \n" +
            "in Confirm password to confirm the password ,It is a \n" +
            "security best practice to use a strong password. For more \n" +
            "information.";
            contextMenuStrip1.Show(lblNewPassword, e.Location);
        }
        else
        {
            tooltipMessage = string.Empty;
            toolTip1.Hide(this);
        }
    }
    private void ResetPassword_MouseClick(object sender, MouseEventArgs e)
    {
        if (e.Button == MouseButtons.Right)
        {
            tooltipMessage = "No Help topic is associated with this item.";
            contextMenuStrip1.Show(this, e.Location);
        }
        else
        {
            tooltipMessage = string.Empty;
            toolTip1.Hide(this);
        }
    }
    private void lblConfirmPassword_MouseClick(object sender, MouseEventArgs e)
    {
        if (e.Button == MouseButtons.Right)
        {
            tooltipMessage = "Provides a space for you to type a case-sensitive \n "+
            "password up to 127 characters.If you are on a network \n"+
            "with computers running Windows 95 and Windows 98, avoid \n"+
            "passwords longer than 14 characters.Type the password again \n"+
            "in Confirm password to confirm the password ,It is a \n"+
            "security best practice to use a strong password. For more \n"+
            "information.";
            contextMenuStrip1.Show(lblConfirmPassword, e.Location);
        }
        else
        {
            tooltipMessage = string.Empty;
            toolTip1.Hide(this);
        }
    }
    private void btnOk_MouseUp(object sender, MouseEventArgs e)
    {
        if (e.Button  == MouseButtons.Right)
        {
            tooltipMessage = "Closes the dialog box and saves any changes you have made.";
            contextMenuStrip1.Show(btnOk, e.Location);
        }
        else
        {
            tooltipMessage = string.Empty;
            toolTip1.Hide(this);
        }
    }
    private void btnCancel_MouseUp(object sender, MouseEventArgs e)
    {
        if (e.Button == MouseButtons.Right)
        {
            tooltipMessage = "Closes the dialog box without saving any changes you have made.";
            contextMenuStrip1.Show(btnCancel, e.Location);
        }
        else
        {
            tooltipMessage = string.Empty;
            toolTip1.Hide(this);
        }
    }
    private void checkBox_MouseUp(object sender, MouseEventArgs e)
    {
        if (e.Button == MouseButtons.Right)
        {
            tooltipMessage = "Click this to force the user to change the password at \n" + "the next logon.";
            contextMenuStrip1.Show(checkBox, e.Location);
        }
        else
        {
            tooltipMessage = string.Empty;
            toolTip1.Hide(this);
        }
    }
}

public class PasswordInfo
{
    #region Class Data
    public string password;
    public string retypedpassword;
    public int PasswordMaxLength = 7;
    
    public bool MustChangePwNextLogon = false;
    
    #endregion
}
}
