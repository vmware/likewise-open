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
    partial class ResetPassword
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
            this.lblNewPassword = new System.Windows.Forms.Label();
            this.contextMenuStrip1 = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.whatsThisToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.lblConfirmPassword = new System.Windows.Forms.Label();
            this.txtNewpassword = new System.Windows.Forms.TextBox();
            this.txtConfirmpassword = new System.Windows.Forms.TextBox();
            this.checkBox = new System.Windows.Forms.CheckBox();
            this.lbllogoffdisplay = new System.Windows.Forms.Label();
            this.btnOk = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.contextMenuStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // lblNewPassword
            // 
            this.lblNewPassword.Location = new System.Drawing.Point(13, 13);
            this.lblNewPassword.Name = "lblNewPassword";
            this.lblNewPassword.Size = new System.Drawing.Size(127, 20);
            this.lblNewPassword.TabIndex = 0;
            this.lblNewPassword.Text = "&New password:";
            this.lblNewPassword.MouseClick += new System.Windows.Forms.MouseEventHandler(this.lblNewPassword_MouseClick);
            // 
            // contextMenuStrip1
            // 
            this.contextMenuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.whatsThisToolStripMenuItem});
            this.contextMenuStrip1.Name = "contextMenuStrip1";
            this.contextMenuStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.contextMenuStrip1.ShowImageMargin = false;
            this.contextMenuStrip1.Size = new System.Drawing.Size(122, 26);
            // 
            // whatsThisToolStripMenuItem
            // 
            this.whatsThisToolStripMenuItem.Name = "whatsThisToolStripMenuItem";
            this.whatsThisToolStripMenuItem.Size = new System.Drawing.Size(121, 22);
            this.whatsThisToolStripMenuItem.Text = "What\'s this ?";
            this.whatsThisToolStripMenuItem.TextDirection = System.Windows.Forms.ToolStripTextDirection.Horizontal;
            this.whatsThisToolStripMenuItem.Click += new System.EventHandler(this.whatsThisToolStripMenuItem_Click);
            // 
            // lblConfirmPassword
            // 
            this.lblConfirmPassword.Location = new System.Drawing.Point(13, 49);
            this.lblConfirmPassword.Name = "lblConfirmPassword";
            this.lblConfirmPassword.Size = new System.Drawing.Size(127, 17);
            this.lblConfirmPassword.TabIndex = 1;
            this.lblConfirmPassword.Text = "&Confirm password:";
            this.lblConfirmPassword.MouseClick += new System.Windows.Forms.MouseEventHandler(this.lblConfirmPassword_MouseClick);
            // 
            // txtNewpassword
            // 
            this.txtNewpassword.Location = new System.Drawing.Point(146, 13);
            this.txtNewpassword.MaxLength = 255;
            this.txtNewpassword.Name = "txtNewpassword";
            this.txtNewpassword.PasswordChar = '*';
            this.txtNewpassword.Size = new System.Drawing.Size(238, 20);
            this.txtNewpassword.TabIndex = 0;
            this.txtNewpassword.TextChanged += new System.EventHandler(this.txtNewpassword_TextChanged);
            // 
            // txtConfirmpassword
            // 
            this.txtConfirmpassword.Location = new System.Drawing.Point(146, 46);
            this.txtConfirmpassword.MaxLength = 255;
            this.txtConfirmpassword.Name = "txtConfirmpassword";
            this.txtConfirmpassword.PasswordChar = '*';
            this.txtConfirmpassword.Size = new System.Drawing.Size(238, 20);
            this.txtConfirmpassword.TabIndex = 2;
            this.txtConfirmpassword.TextChanged += new System.EventHandler(this.txtConfirmpassword_TextChanged);
            // 
            // checkBox
            // 
            this.checkBox.Location = new System.Drawing.Point(13, 72);
            this.checkBox.Name = "checkBox";
            this.checkBox.Size = new System.Drawing.Size(371, 18);
            this.checkBox.TabIndex = 3;
            this.checkBox.Text = "&User must change password at next logon";
            this.checkBox.UseVisualStyleBackColor = true;
            this.checkBox.CheckedChanged += new System.EventHandler(this.checkBox_CheckedChanged);
            this.checkBox.MouseUp += new System.Windows.Forms.MouseEventHandler(this.checkBox_MouseUp);
            // 
            // lbllogoffdisplay
            // 
            this.lbllogoffdisplay.Location = new System.Drawing.Point(13, 93);
            this.lbllogoffdisplay.Name = "lbllogoffdisplay";
            this.lbllogoffdisplay.Size = new System.Drawing.Size(372, 30);
            this.lbllogoffdisplay.TabIndex = 4;
            this.lbllogoffdisplay.Text = "The user must logoff and then logon again for the changes to take effect.";
            // 
            // btnOk
            // 
            this.btnOk.Location = new System.Drawing.Point(229, 130);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(75, 23);
            this.btnOk.TabIndex = 5;
            this.btnOk.Text = "OK";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            this.btnOk.MouseUp += new System.Windows.Forms.MouseEventHandler(this.btnOk_MouseUp);
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(310, 130);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 6;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            this.btnCancel.MouseUp += new System.Windows.Forms.MouseEventHandler(this.btnCancel_MouseUp);
            // 
            // toolTip1
            // 
            this.toolTip1.AutoPopDelay = 100;
            this.toolTip1.InitialDelay = 100;
            this.toolTip1.ReshowDelay = 100;
            this.toolTip1.Tag = "";
            // 
            // ResetPassword
            // 
            this.AcceptButton = this.btnOk;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(397, 165);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.lbllogoffdisplay);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.checkBox);
            this.Controls.Add(this.txtConfirmpassword);
            this.Controls.Add(this.txtNewpassword);
            this.Controls.Add(this.lblConfirmPassword);
            this.Controls.Add(this.lblNewPassword);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.HelpButton = true;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ResetPassword";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Reset Password";
            this.Load += new System.EventHandler(this.ResetPassword_Load);
            this.MouseClick += new System.Windows.Forms.MouseEventHandler(this.ResetPassword_MouseClick);
            this.contextMenuStrip1.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblNewPassword;
        private System.Windows.Forms.Label lblConfirmPassword;
        private System.Windows.Forms.TextBox txtNewpassword;
        private System.Windows.Forms.TextBox txtConfirmpassword;
        private System.Windows.Forms.CheckBox checkBox;
        private System.Windows.Forms.Label lbllogoffdisplay;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.ContextMenuStrip contextMenuStrip1;
        private System.Windows.Forms.ToolStripMenuItem whatsThisToolStripMenuItem;
    }
}