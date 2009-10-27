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
    partial class UserAddpasswordPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(UserAddpasswordPage));
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.pwtextBox = new System.Windows.Forms.TextBox();
            this.confirmpwtextBox = new System.Windows.Forms.TextBox();
            this.cbMustChangePwd = new System.Windows.Forms.CheckBox();
            this.cbCannotChangePwd = new System.Windows.Forms.CheckBox();
            this.cbNeverExpiresPwd = new System.Windows.Forms.CheckBox();
            this.cbAccountDisable = new System.Windows.Forms.CheckBox();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.txtcreatein = new System.Windows.Forms.TextBox();
            this.groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.groupBox3.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.groupBox2);
            this.groupBox1.Location = new System.Drawing.Point(41, 88);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(414, 2);
            this.groupBox1.TabIndex = 28;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "groupBox1";
            // 
            // groupBox2
            // 
            this.groupBox2.Location = new System.Drawing.Point(0, 31);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(414, 2);
            this.groupBox2.TabIndex = 29;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "groupBox2";
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(45, 39);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(24, 24);
            this.pictureBox1.TabIndex = 27;
            this.pictureBox1.TabStop = false;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(42, 114);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(66, 13);
            this.label1.TabIndex = 29;
            this.label1.Text = "Password:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(42, 141);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(116, 13);
            this.label3.TabIndex = 30;
            this.label3.Text = "Confirm password:";
            // 
            // pwtextBox
            // 
            this.pwtextBox.Location = new System.Drawing.Point(182, 109);
            this.pwtextBox.Name = "pwtextBox";
            this.pwtextBox.PasswordChar = '*';
            this.pwtextBox.Size = new System.Drawing.Size(273, 21);
            this.pwtextBox.TabIndex = 31;
            this.pwtextBox.UseSystemPasswordChar = true;
            this.pwtextBox.TextChanged += new System.EventHandler(this.pwtextBox_TextChanged);
            this.pwtextBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.pwtextBox_KeyDown);
            // 
            // confirmpwtextBox
            // 
            this.confirmpwtextBox.Location = new System.Drawing.Point(182, 136);
            this.confirmpwtextBox.Name = "confirmpwtextBox";
            this.confirmpwtextBox.PasswordChar = '*';
            this.confirmpwtextBox.Size = new System.Drawing.Size(273, 21);
            this.confirmpwtextBox.TabIndex = 32;
            this.confirmpwtextBox.UseSystemPasswordChar = true;
            this.confirmpwtextBox.TextChanged += new System.EventHandler(this.confirmpwtextBox_TextChanged);
            this.confirmpwtextBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.pwtextBox_KeyDown);
            // 
            // cbMustChangePwd
            // 
            this.cbMustChangePwd.Checked = true;
            this.cbMustChangePwd.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbMustChangePwd.Location = new System.Drawing.Point(42, 173);
            this.cbMustChangePwd.Name = "cbMustChangePwd";
            this.cbMustChangePwd.Size = new System.Drawing.Size(393, 25);
            this.cbMustChangePwd.TabIndex = 33;
            this.cbMustChangePwd.Text = "User must change password at next logon";
            this.cbMustChangePwd.UseVisualStyleBackColor = true;
            this.cbMustChangePwd.CheckedChanged += new System.EventHandler(this.cbMustChangePwd_CheckedChanged);
            // 
            // cbCannotChangePwd
            // 
            this.cbCannotChangePwd.Location = new System.Drawing.Point(42, 202);
            this.cbCannotChangePwd.Name = "cbCannotChangePwd";
            this.cbCannotChangePwd.Size = new System.Drawing.Size(393, 23);
            this.cbCannotChangePwd.TabIndex = 34;
            this.cbCannotChangePwd.Text = "User cannot change password";
            this.cbCannotChangePwd.UseVisualStyleBackColor = true;
            this.cbCannotChangePwd.CheckedChanged += new System.EventHandler(this.cbCannotChangePwd_CheckedChanged);
            // 
            // cbNeverExpiresPwd
            // 
            this.cbNeverExpiresPwd.Location = new System.Drawing.Point(42, 227);
            this.cbNeverExpiresPwd.Name = "cbNeverExpiresPwd";
            this.cbNeverExpiresPwd.Size = new System.Drawing.Size(393, 24);
            this.cbNeverExpiresPwd.TabIndex = 35;
            this.cbNeverExpiresPwd.Text = "Password never expires";
            this.cbNeverExpiresPwd.UseVisualStyleBackColor = true;
            this.cbNeverExpiresPwd.CheckedChanged += new System.EventHandler(this.cbNeverExpiresPwd_CheckedChanged);
            // 
            // cbAccountDisable
            // 
            this.cbAccountDisable.Location = new System.Drawing.Point(42, 251);
            this.cbAccountDisable.Name = "cbAccountDisable";
            this.cbAccountDisable.Size = new System.Drawing.Size(393, 20);
            this.cbAccountDisable.TabIndex = 36;
            this.cbAccountDisable.Text = "Account is disabled";
            this.cbAccountDisable.UseVisualStyleBackColor = true;
            this.cbAccountDisable.CheckedChanged += new System.EventHandler(this.cbAccountDisable_CheckedChanged);
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.groupBox4);
            this.groupBox3.Location = new System.Drawing.Point(41, 277);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(414, 2);
            this.groupBox3.TabIndex = 37;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "groupBox3";
            // 
            // groupBox4
            // 
            this.groupBox4.Location = new System.Drawing.Point(0, 31);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(414, 2);
            this.groupBox4.TabIndex = 29;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "groupBox4";
            // 
            // txtcreatein
            // 
            this.txtcreatein.BackColor = System.Drawing.SystemColors.Control;
            this.txtcreatein.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.txtcreatein.Location = new System.Drawing.Point(77, 45);
            this.txtcreatein.Name = "txtcreatein";
            this.txtcreatein.ReadOnly = true;
            this.txtcreatein.Size = new System.Drawing.Size(381, 14);
            this.txtcreatein.TabIndex = 38;
            // 
            // UserAddpasswordPage
            // 
            this.Controls.Add(this.txtcreatein);
            this.Controls.Add(this.groupBox3);
            this.Controls.Add(this.cbAccountDisable);
            this.Controls.Add(this.cbNeverExpiresPwd);
            this.Controls.Add(this.cbCannotChangePwd);
            this.Controls.Add(this.cbMustChangePwd);
            this.Controls.Add(this.confirmpwtextBox);
            this.Controls.Add(this.pwtextBox);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.pictureBox1);
            this.Name = "UserAddpasswordPage";
            this.groupBox1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.groupBox3.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.TextBox pwtextBox;
        private System.Windows.Forms.TextBox confirmpwtextBox;
        private System.Windows.Forms.CheckBox cbMustChangePwd;
        private System.Windows.Forms.CheckBox cbCannotChangePwd;
        private System.Windows.Forms.CheckBox cbNeverExpiresPwd;
        private System.Windows.Forms.CheckBox cbAccountDisable;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.GroupBox groupBox4;
        private System.Windows.Forms.TextBox txtcreatein;
    }
}
