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

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    partial class ADRenameUserDlg
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
            this.userlogonPretextBox = new System.Windows.Forms.TextBox();
            this.prelogontextBox = new System.Windows.Forms.TextBox();
            this.Userlogonglabel = new System.Windows.Forms.Label();
            this.domainNamecomboBox = new System.Windows.Forms.ComboBox();
            this.logonNametextBox = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.displaynametextBox = new System.Windows.Forms.TextBox();
            this.InitialtextBox = new System.Windows.Forms.TextBox();
            this.Initiallabel = new System.Windows.Forms.Label();
            this.LnametextBox = new System.Windows.Forms.TextBox();
            this.FnametextBox = new System.Windows.Forms.TextBox();
            this.LNamelabel = new System.Windows.Forms.Label();
            this.Fnamelabel = new System.Windows.Forms.Label();
            this.btkcancel = new System.Windows.Forms.Button();
            this.btnOK = new System.Windows.Forms.Button();
            this.lDisplayName = new System.Windows.Forms.Label();
            this.lbFullName = new System.Windows.Forms.Label();
            this.FullNametextbox = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // userlogonPretextBox
            // 
            this.userlogonPretextBox.Location = new System.Drawing.Point(219, 207);
            this.userlogonPretextBox.MaxLength = 20;
            this.userlogonPretextBox.Name = "userlogonPretextBox";
            this.userlogonPretextBox.Size = new System.Drawing.Size(187, 20);
            this.userlogonPretextBox.TabIndex = 45;
            this.userlogonPretextBox.TextChanged += new System.EventHandler(this.userlogonPretextBox_TextChanged);
            // 
            // prelogontextBox
            // 
            this.prelogontextBox.Location = new System.Drawing.Point(14, 207);
            this.prelogontextBox.Name = "prelogontextBox";
            this.prelogontextBox.ReadOnly = true;
            this.prelogontextBox.Size = new System.Drawing.Size(191, 20);
            this.prelogontextBox.TabIndex = 44;
            // 
            // Userlogonglabel
            // 
            this.Userlogonglabel.AutoSize = true;
            this.Userlogonglabel.Location = new System.Drawing.Point(12, 189);
            this.Userlogonglabel.Name = "Userlogonglabel";
            this.Userlogonglabel.Size = new System.Drawing.Size(185, 13);
            this.Userlogonglabel.TabIndex = 43;
            this.Userlogonglabel.Text = "User logon name (pre-windows 2000):";
            // 
            // domainNamecomboBox
            // 
            this.domainNamecomboBox.DropDownHeight = 120;
            this.domainNamecomboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.domainNamecomboBox.FormattingEnabled = true;
            this.domainNamecomboBox.IntegralHeight = false;
            this.domainNamecomboBox.Location = new System.Drawing.Point(219, 158);
            this.domainNamecomboBox.Name = "domainNamecomboBox";
            this.domainNamecomboBox.Size = new System.Drawing.Size(187, 21);
            this.domainNamecomboBox.TabIndex = 42;
            // 
            // logonNametextBox
            // 
            this.logonNametextBox.Location = new System.Drawing.Point(15, 158);
            this.logonNametextBox.MaxLength = 256;
            this.logonNametextBox.Name = "logonNametextBox";
            this.logonNametextBox.Size = new System.Drawing.Size(190, 20);
            this.logonNametextBox.TabIndex = 41;
            this.logonNametextBox.TextChanged += new System.EventHandler(this.logonNametextBox_TextChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(12, 136);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(90, 13);
            this.label3.TabIndex = 40;
            this.label3.Text = "User logon name:";
            // 
            // displaynametextBox
            // 
            this.displaynametextBox.Location = new System.Drawing.Point(91, 104);
            this.displaynametextBox.MaxLength = 64;
            this.displaynametextBox.Name = "displaynametextBox";
            this.displaynametextBox.Size = new System.Drawing.Size(315, 20);
            this.displaynametextBox.TabIndex = 39;
            this.displaynametextBox.TextChanged += new System.EventHandler(this.displaynametextBox_TextChanged);
            // 
            // InitialtextBox
            // 
            this.InitialtextBox.Location = new System.Drawing.Point(306, 45);
            this.InitialtextBox.MaxLength = 6;
            this.InitialtextBox.Name = "InitialtextBox";
            this.InitialtextBox.Size = new System.Drawing.Size(100, 20);
            this.InitialtextBox.TabIndex = 35;
            this.InitialtextBox.TextChanged += new System.EventHandler(this.InitialtextBox_TextChanged);
            // 
            // Initiallabel
            // 
            this.Initiallabel.AutoSize = true;
            this.Initiallabel.Location = new System.Drawing.Point(250, 48);
            this.Initiallabel.Name = "Initiallabel";
            this.Initiallabel.Size = new System.Drawing.Size(39, 13);
            this.Initiallabel.TabIndex = 34;
            this.Initiallabel.Text = "Initials:";
            // 
            // LnametextBox
            // 
            this.LnametextBox.Location = new System.Drawing.Point(91, 74);
            this.LnametextBox.MaxLength = 64;
            this.LnametextBox.Name = "LnametextBox";
            this.LnametextBox.Size = new System.Drawing.Size(315, 20);
            this.LnametextBox.TabIndex = 37;
            this.LnametextBox.TextChanged += new System.EventHandler(this.LnametextBox_TextChanged);
            // 
            // FnametextBox
            // 
            this.FnametextBox.Location = new System.Drawing.Point(91, 45);
            this.FnametextBox.MaxLength = 64;
            this.FnametextBox.Name = "FnametextBox";
            this.FnametextBox.Size = new System.Drawing.Size(148, 20);
            this.FnametextBox.TabIndex = 33;
            this.FnametextBox.TextChanged += new System.EventHandler(this.FnametextBox_TextChanged);
            // 
            // LNamelabel
            // 
            this.LNamelabel.AutoSize = true;
            this.LNamelabel.Location = new System.Drawing.Point(12, 77);
            this.LNamelabel.Name = "LNamelabel";
            this.LNamelabel.Size = new System.Drawing.Size(59, 13);
            this.LNamelabel.TabIndex = 36;
            this.LNamelabel.Text = "Last name:";
            // 
            // Fnamelabel
            // 
            this.Fnamelabel.AutoSize = true;
            this.Fnamelabel.Location = new System.Drawing.Point(12, 48);
            this.Fnamelabel.Name = "Fnamelabel";
            this.Fnamelabel.Size = new System.Drawing.Size(58, 13);
            this.Fnamelabel.TabIndex = 32;
            this.Fnamelabel.Text = "First name:";
            // 
            // btkcancel
            // 
            this.btkcancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btkcancel.Location = new System.Drawing.Point(331, 238);
            this.btkcancel.Name = "btkcancel";
            this.btkcancel.Size = new System.Drawing.Size(75, 23);
            this.btkcancel.TabIndex = 31;
            this.btkcancel.Text = "Cancel";
            this.btkcancel.UseVisualStyleBackColor = true;
            this.btkcancel.Click += new System.EventHandler(this.btkcancel_Click);
            // 
            // btnOK
            // 
            this.btnOK.Enabled = false;
            this.btnOK.Location = new System.Drawing.Point(250, 238);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(75, 23);
            this.btnOK.TabIndex = 30;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // lDisplayName
            // 
            this.lDisplayName.AutoSize = true;
            this.lDisplayName.Location = new System.Drawing.Point(12, 107);
            this.lDisplayName.Name = "lDisplayName";
            this.lDisplayName.Size = new System.Drawing.Size(73, 13);
            this.lDisplayName.TabIndex = 38;
            this.lDisplayName.Text = "Display name:";
            // 
            // lbFullName
            // 
            this.lbFullName.AutoSize = true;
            this.lbFullName.Location = new System.Drawing.Point(13, 19);
            this.lbFullName.Name = "lbFullName";
            this.lbFullName.Size = new System.Drawing.Size(55, 13);
            this.lbFullName.TabIndex = 46;
            this.lbFullName.Text = "Full name:";
            // 
            // FullNametextbox
            // 
            this.FullNametextbox.Location = new System.Drawing.Point(91, 16);
            this.FullNametextbox.Name = "FullNametextbox";
            this.FullNametextbox.Size = new System.Drawing.Size(315, 20);
            this.FullNametextbox.TabIndex = 47;
            this.FullNametextbox.TextChanged += new System.EventHandler(this.fullnametextBox_TextChanged);
            // 
            // ADRenameUserDlg
            // 
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btkcancel;
            this.ClientSize = new System.Drawing.Size(416, 269);
            this.Controls.Add(this.FullNametextbox);
            this.Controls.Add(this.lbFullName);
            this.Controls.Add(this.userlogonPretextBox);
            this.Controls.Add(this.prelogontextBox);
            this.Controls.Add(this.Userlogonglabel);
            this.Controls.Add(this.domainNamecomboBox);
            this.Controls.Add(this.logonNametextBox);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.displaynametextBox);
            this.Controls.Add(this.lDisplayName);
            this.Controls.Add(this.InitialtextBox);
            this.Controls.Add(this.Initiallabel);
            this.Controls.Add(this.LnametextBox);
            this.Controls.Add(this.FnametextBox);
            this.Controls.Add(this.LNamelabel);
            this.Controls.Add(this.Fnamelabel);
            this.Controls.Add(this.btkcancel);
            this.Controls.Add(this.btnOK);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ADRenameUserDlg";
            this.ShowIcon = false;
            this.Text = "Rename User";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox userlogonPretextBox;
        private System.Windows.Forms.TextBox prelogontextBox;
        private System.Windows.Forms.Label Userlogonglabel;
        private System.Windows.Forms.ComboBox domainNamecomboBox;
        private System.Windows.Forms.TextBox logonNametextBox;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox displaynametextBox;
        private System.Windows.Forms.TextBox InitialtextBox;
        private System.Windows.Forms.Label Initiallabel;
        private System.Windows.Forms.TextBox LnametextBox;
        private System.Windows.Forms.TextBox FnametextBox;
        private System.Windows.Forms.Label LNamelabel;
        private System.Windows.Forms.Label Fnamelabel;
        private System.Windows.Forms.Button btkcancel;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Label lDisplayName;
        private System.Windows.Forms.Label lbFullName;
        private System.Windows.Forms.TextBox FullNametextbox;

    }
}