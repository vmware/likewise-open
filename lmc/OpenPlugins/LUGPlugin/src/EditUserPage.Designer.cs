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
    partial class EditUserPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(EditUserPage));
            this.cbIsDisabled = new System.Windows.Forms.CheckBox();
            this.cbNeverExpires = new System.Windows.Forms.CheckBox();
            this.cbCannotChange = new System.Windows.Forms.CheckBox();
            this.cbMustChange = new System.Windows.Forms.CheckBox();
            this.lbDescription = new System.Windows.Forms.Label();
            this.lbFullName = new System.Windows.Forms.Label();
            this.tbDescription = new System.Windows.Forms.TextBox();
            this.tbFullName = new System.Windows.Forms.TextBox();
            this.cbAccountLockedOut = new System.Windows.Forms.CheckBox();
            this.gbSeparator = new System.Windows.Forms.GroupBox();
            this.pbUser = new System.Windows.Forms.PictureBox();
            this.lbUser = new System.Windows.Forms.Label();
            this.errorProvider = new System.Windows.Forms.ErrorProvider(this.components);
            this.pnlData.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pbUser)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.errorProvider)).BeginInit();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.lbUser);
            this.pnlData.Controls.Add(this.pbUser);
            this.pnlData.Controls.Add(this.gbSeparator);
            this.pnlData.Controls.Add(this.cbAccountLockedOut);
            this.pnlData.Controls.Add(this.cbIsDisabled);
            this.pnlData.Controls.Add(this.cbNeverExpires);
            this.pnlData.Controls.Add(this.cbCannotChange);
            this.pnlData.Controls.Add(this.cbMustChange);
            this.pnlData.Controls.Add(this.lbDescription);
            this.pnlData.Controls.Add(this.lbFullName);
            this.pnlData.Controls.Add(this.tbDescription);
            this.pnlData.Controls.Add(this.tbFullName);
            this.pnlData.Dock = System.Windows.Forms.DockStyle.None;
            this.pnlData.Size = new System.Drawing.Size(386, 380);
            // 
            // cbIsDisabled
            // 
            this.cbIsDisabled.Location = new System.Drawing.Point(17, 205);
            this.cbIsDisabled.Name = "cbIsDisabled";
            this.cbIsDisabled.Size = new System.Drawing.Size(300, 17);
            this.cbIsDisabled.TabIndex = 22;
            this.cbIsDisabled.Text = "Account is disa&bled";
            this.cbIsDisabled.UseVisualStyleBackColor = true;
            //
            // cbNeverExpires
            //
            this.cbNeverExpires.Location = new System.Drawing.Point(17, 182);
            this.cbNeverExpires.Name = "cbNeverExpires";
            this.cbNeverExpires.Size = new System.Drawing.Size(300, 17);
            this.cbNeverExpires.TabIndex = 21;
            this.cbNeverExpires.Text = "Pass&word never expires";
            this.cbNeverExpires.UseVisualStyleBackColor = true;
            this.cbNeverExpires.CheckedChanged += new System.EventHandler(this.cbNeverExpirers_CheckedChanged);
            // 
            // cbCannotChange
            // 
            this.cbCannotChange.Location = new System.Drawing.Point(17, 159);
            this.cbCannotChange.Name = "cbCannotChange";
            this.cbCannotChange.Size = new System.Drawing.Size(300, 17);
            this.cbCannotChange.TabIndex = 20;
            this.cbCannotChange.Text = "U&ser cannot change password";
            this.cbCannotChange.UseVisualStyleBackColor = true;
            this.cbCannotChange.CheckedChanged += new System.EventHandler(this.cbCannotChange_CheckedChanged);
            // 
            // cbMustChange
            // 
            this.cbMustChange.Location = new System.Drawing.Point(17, 136);
            this.cbMustChange.Name = "cbMustChange";
            this.cbMustChange.Size = new System.Drawing.Size(300, 17);
            this.cbMustChange.TabIndex = 19;
            this.cbMustChange.Text = "User &must change password at next logon";
            this.cbMustChange.UseVisualStyleBackColor = true;
            this.cbMustChange.CheckedChanged += new System.EventHandler(this.cbMustChange_CheckedChanged);
            // 
            // lbDescription
            // 
            this.lbDescription.AutoSize = true;
            this.lbDescription.Location = new System.Drawing.Point(14, 101);
            this.lbDescription.Name = "lbDescription";
            this.lbDescription.Size = new System.Drawing.Size(63, 13);
            this.lbDescription.TabIndex = 17;
            this.lbDescription.Text = "&Description:";
            // 
            // lbFullName
            // 
            this.lbFullName.AutoSize = true;
            this.lbFullName.Location = new System.Drawing.Point(14, 68);
            this.lbFullName.Name = "lbFullName";
            this.lbFullName.Size = new System.Drawing.Size(57, 13);
            this.lbFullName.TabIndex = 15;
            this.lbFullName.Text = "&Full Name:";
            // 
            // tbDescription
            // 
            this.tbDescription.Location = new System.Drawing.Point(98, 97);
            this.tbDescription.Name = "tbDescription";
            this.tbDescription.Size = new System.Drawing.Size(270, 20);
            this.tbDescription.TabIndex = 18;
            //
            // tbFullName
            //
            this.tbFullName.Location = new System.Drawing.Point(98, 64);
            this.tbFullName.Name = "tbFullName";
            this.tbFullName.Size = new System.Drawing.Size(270, 20);
            this.tbFullName.TabIndex = 16;
            //
            // cbAccountLockedOut
            //
            this.cbAccountLockedOut.Enabled = false;
            this.cbAccountLockedOut.Location = new System.Drawing.Point(17, 228);
            this.cbAccountLockedOut.Name = "cbAccountLockedOut";
            this.cbAccountLockedOut.Size = new System.Drawing.Size(300, 17);
            this.cbAccountLockedOut.TabIndex = 23;
            this.cbAccountLockedOut.Text = "Account is &locked out";
            this.cbAccountLockedOut.UseVisualStyleBackColor = true;
            //
            // gbSeparator
            //
            this.gbSeparator.BackColor = System.Drawing.Color.Black;
            this.gbSeparator.Location = new System.Drawing.Point(17, 50);
            this.gbSeparator.Margin = new System.Windows.Forms.Padding(1);
            this.gbSeparator.Name = "gbSeparator";
            this.gbSeparator.Size = new System.Drawing.Size(351, 1);
            this.gbSeparator.TabIndex = 24;
            this.gbSeparator.TabStop = false;
            // 
            // pbUser
            // 
            this.pbUser.Image = ((System.Drawing.Image)(resources.GetObject("pbUser.Image")));
            this.pbUser.Location = new System.Drawing.Point(17, 8);
            this.pbUser.Name = "pbUser";
            this.pbUser.Size = new System.Drawing.Size(32, 32);
            this.pbUser.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
            this.pbUser.TabIndex = 26;
            this.pbUser.TabStop = false;
            // 
            // lbUser
            // 
            this.lbUser.AutoSize = true;
            this.lbUser.Location = new System.Drawing.Point(64, 18);
            this.lbUser.Name = "lbUser";
            this.lbUser.Size = new System.Drawing.Size(21, 13);
            this.lbUser.TabIndex = 27;
            this.lbUser.Text = "{0}";
            // 
            // errorProvider
            // 
            this.errorProvider.BlinkStyle = System.Windows.Forms.ErrorBlinkStyle.NeverBlink;
            this.errorProvider.ContainerControl = this;
            // 
            // EditUserPage
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Name = "EditUserPage";
            this.Size = new System.Drawing.Size(386, 380);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pbUser)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.errorProvider)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.CheckBox cbIsDisabled;
        private System.Windows.Forms.CheckBox cbNeverExpires;
        private System.Windows.Forms.CheckBox cbCannotChange;
        private System.Windows.Forms.CheckBox cbMustChange;
        private System.Windows.Forms.Label lbDescription;
        private System.Windows.Forms.Label lbFullName;
        private System.Windows.Forms.TextBox tbDescription;
        private System.Windows.Forms.TextBox tbFullName;
        private System.Windows.Forms.CheckBox cbAccountLockedOut;
        private System.Windows.Forms.GroupBox gbSeparator;
        private System.Windows.Forms.PictureBox pbUser;
        private System.Windows.Forms.Label lbUser;
        private System.Windows.Forms.ErrorProvider errorProvider;
    }
}
