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

namespace Likewise.LMC.Plugins.LUG
{
    partial class NewUserDlg
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(NewUserDlg));
            this.lbUserName = new System.Windows.Forms.Label();
            this.tbUserName = new System.Windows.Forms.TextBox();
            this.tbFullName = new System.Windows.Forms.TextBox();
            this.tbDescription = new System.Windows.Forms.TextBox();
            this.lbFullName = new System.Windows.Forms.Label();
            this.lbDescription = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.tbPassword = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.tbConfirmPassword = new System.Windows.Forms.TextBox();
            this.cbMustChange = new System.Windows.Forms.CheckBox();
            this.cbCannotChange = new System.Windows.Forms.CheckBox();
            this.cbNeverExpires = new System.Windows.Forms.CheckBox();
            this.cbIsDisabled = new System.Windows.Forms.CheckBox();
            this.errorProvider = new System.Windows.Forms.ErrorProvider(this.components);
            ((System.ComponentModel.ISupportInitialize)(this.errorProvider)).BeginInit();
            this.SuspendLayout();
            //
            // btnApply
            //
            this.btnApply.Location = new System.Drawing.Point(309, 6);
            //
            // btnOK
            //
            this.btnOK.Location = new System.Drawing.Point(149, 6);
            //
            // btnCancel
            //
            this.btnCancel.Location = new System.Drawing.Point(230, 6);
            // 
            // lbUserName
            // 
            this.lbUserName.AutoSize = true;
            this.lbUserName.Location = new System.Drawing.Point(12, 17);
            this.lbUserName.Name = "lbUserName";
            this.lbUserName.Size = new System.Drawing.Size(63, 13);
            this.lbUserName.TabIndex = 1;
            this.lbUserName.Text = "&User Name:";
            // 
            // tbUserName
            // 
            this.tbUserName.Location = new System.Drawing.Point(91, 13);
            this.tbUserName.MaxLength = 255;
            this.tbUserName.Name = "tbUserName";
            this.tbUserName.Size = new System.Drawing.Size(270, 20);
            this.tbUserName.TabIndex = 3;
            // 
            // tbFullName
            // 
            this.tbFullName.Location = new System.Drawing.Point(91, 39);
            this.tbFullName.Name = "tbFullName";
            this.tbFullName.Size = new System.Drawing.Size(270, 20);
            this.tbFullName.TabIndex = 4;
            // 
            // tbDescription
            // 
            this.tbDescription.Location = new System.Drawing.Point(91, 65);
            this.tbDescription.Name = "tbDescription";
            this.tbDescription.Size = new System.Drawing.Size(270, 20);
            this.tbDescription.TabIndex = 5;
            // 
            // lbFullName
            // 
            this.lbFullName.AutoSize = true;
            this.lbFullName.Location = new System.Drawing.Point(12, 43);
            this.lbFullName.Name = "lbFullName";
            this.lbFullName.Size = new System.Drawing.Size(57, 13);
            this.lbFullName.TabIndex = 3;
            this.lbFullName.Text = "&Full Name:";
            // 
            // lbDescription
            // 
            this.lbDescription.AutoSize = true;
            this.lbDescription.Location = new System.Drawing.Point(12, 65);
            this.lbDescription.Name = "lbDescription";
            this.lbDescription.Size = new System.Drawing.Size(63, 13);
            this.lbDescription.TabIndex = 5;
            this.lbDescription.Text = "&Description:";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 119);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(56, 13);
            this.label1.TabIndex = 7;
            this.label1.Text = "&Password:";
            // 
            // tbPassword
            // 
            this.tbPassword.Location = new System.Drawing.Point(120, 115);
            this.tbPassword.Name = "tbPassword";
            this.tbPassword.PasswordChar = '*';
            this.tbPassword.Size = new System.Drawing.Size(240, 20);
            this.tbPassword.TabIndex = 6;
            this.tbPassword.WordWrap = false;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(12, 145);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(93, 13);
            this.label3.TabIndex = 9;
            this.label3.Text = "&Confirm password:";
            // 
            // tbConfirmPassword
            // 
            this.tbConfirmPassword.Location = new System.Drawing.Point(120, 141);
            this.tbConfirmPassword.Name = "tbConfirmPassword";
            this.tbConfirmPassword.PasswordChar = '*';
            this.tbConfirmPassword.Size = new System.Drawing.Size(240, 20);
            this.tbConfirmPassword.TabIndex = 7;
            this.tbConfirmPassword.WordWrap = false;
            // 
            // cbMustChange
            // 
            this.cbMustChange.Location = new System.Drawing.Point(15, 191);
            this.cbMustChange.Name = "cbMustChange";
            this.cbMustChange.Size = new System.Drawing.Size(300, 17);
            this.cbMustChange.TabIndex = 8;
            this.cbMustChange.Text = "User &must change password at next logon";
            this.cbMustChange.UseVisualStyleBackColor = true;
            // 
            // cbCannotChange
            // 
            this.cbCannotChange.Location = new System.Drawing.Point(15, 214);
            this.cbCannotChange.Name = "cbCannotChange";
            this.cbCannotChange.Size = new System.Drawing.Size(300, 17);
            this.cbCannotChange.TabIndex = 9;
            this.cbCannotChange.Text = "U&ser cannot change password";
            this.cbCannotChange.UseVisualStyleBackColor = true;
            // 
            // cbNeverExpires
            // 
            this.cbNeverExpires.Location = new System.Drawing.Point(15, 237);
            this.cbNeverExpires.Name = "cbNeverExpires";
            this.cbNeverExpires.Size = new System.Drawing.Size(300, 17);
            this.cbNeverExpires.TabIndex = 10;
            this.cbNeverExpires.Text = "Pass&word never expires";
            this.cbNeverExpires.UseVisualStyleBackColor = true;
            // 
            // cbIsDisabled
            // 
            this.cbIsDisabled.Location = new System.Drawing.Point(15, 260);
            this.cbIsDisabled.Name = "cbIsDisabled";
            this.cbIsDisabled.Size = new System.Drawing.Size(300, 17);
            this.cbIsDisabled.TabIndex = 11;
            this.cbIsDisabled.Text = "Account is disa&bled";
            this.cbIsDisabled.UseVisualStyleBackColor = true;
            // 
            // errorProvider
            // 
            this.errorProvider.BlinkStyle = System.Windows.Forms.ErrorBlinkStyle.NeverBlink;
            this.errorProvider.ContainerControl = this;
            // 
            // NewUserDlg
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.ClientSize = new System.Drawing.Size(387, 338);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.tbConfirmPassword);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.tbPassword);
            this.Controls.Add(this.lbFullName);
            this.Controls.Add(this.tbFullName);
            this.Controls.Add(this.tbUserName);
            this.Controls.Add(this.lbUserName);
            this.Controls.Add(this.cbCannotChange);
            this.Controls.Add(this.cbIsDisabled);
            this.Controls.Add(this.cbMustChange);
            this.Controls.Add(this.cbNeverExpires);
            this.Controls.Add(this.lbDescription);
            this.Controls.Add(this.tbDescription);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "NewUserDlg";
            this.StartPosition = System.Windows.Forms.FormStartPosition.WindowsDefaultLocation;
            this.Text = "New User";
            this.Controls.SetChildIndex(this.tbDescription, 0);
            this.Controls.SetChildIndex(this.lbDescription, 0);
            this.Controls.SetChildIndex(this.cbNeverExpires, 0);
            this.Controls.SetChildIndex(this.cbMustChange, 0);
            this.Controls.SetChildIndex(this.cbIsDisabled, 0);
            this.Controls.SetChildIndex(this.cbCannotChange, 0);
            this.Controls.SetChildIndex(this.lbUserName, 0);
            this.Controls.SetChildIndex(this.tbUserName, 0);
            this.Controls.SetChildIndex(this.tbFullName, 0);
            this.Controls.SetChildIndex(this.lbFullName, 0);
            this.Controls.SetChildIndex(this.tbPassword, 0);
            this.Controls.SetChildIndex(this.label1, 0);
            this.Controls.SetChildIndex(this.tbConfirmPassword, 0);
            this.Controls.SetChildIndex(this.label3, 0);
            ((System.ComponentModel.ISupportInitialize)(this.errorProvider)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lbUserName;
        private System.Windows.Forms.TextBox tbUserName;
        private System.Windows.Forms.TextBox tbFullName;
        private System.Windows.Forms.TextBox tbDescription;
        private System.Windows.Forms.Label lbFullName;
        private System.Windows.Forms.Label lbDescription;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox tbPassword;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox tbConfirmPassword;
        private System.Windows.Forms.CheckBox cbMustChange;
        private System.Windows.Forms.CheckBox cbCannotChange;
        private System.Windows.Forms.CheckBox cbNeverExpires;
        private System.Windows.Forms.CheckBox cbIsDisabled;
        private System.Windows.Forms.ErrorProvider errorProvider;
    }
}
