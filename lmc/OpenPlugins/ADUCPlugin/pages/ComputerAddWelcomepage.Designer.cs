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
    partial class ComputerAddWelcomepage
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

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ComputerAddWelcomepage));
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.txtWindowsCompName = new System.Windows.Forms.TextBox();
            this.lblCompName = new System.Windows.Forms.Label();
            this.txtCompName = new System.Windows.Forms.TextBox();
            this.lblComputerName = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.txtUserGroup = new System.Windows.Forms.TextBox();
            this.btnChange = new System.Windows.Forms.Button();
            this.cbWindowsComputer = new System.Windows.Forms.CheckBox();
            this.cbBackupComputer = new System.Windows.Forms.CheckBox();
            this.txtcreatein = new System.Windows.Forms.TextBox();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(43, 69);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(390, 2);
            this.groupBox1.TabIndex = 26;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "groupBox1";
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(50, 16);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(48, 48);
            this.pictureBox1.TabIndex = 25;
            this.pictureBox1.TabStop = false;
            // 
            // txtWindowsCompName
            // 
            this.txtWindowsCompName.Location = new System.Drawing.Point(50, 145);
            this.txtWindowsCompName.MaxLength = 15;
            this.txtWindowsCompName.Name = "txtWindowsCompName";
            this.txtWindowsCompName.Size = new System.Drawing.Size(222, 21);
            this.txtWindowsCompName.TabIndex = 34;
            this.txtWindowsCompName.TextChanged += new System.EventHandler(this.txtWindowsCompName_TextChanged);
            this.txtWindowsCompName.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.txtWindowsCompName_KeyPress);
            // 
            // lblCompName
            // 
            this.lblCompName.AutoSize = true;
            this.lblCompName.Location = new System.Drawing.Point(47, 129);
            this.lblCompName.Name = "lblCompName";
            this.lblCompName.Size = new System.Drawing.Size(225, 13);
            this.lblCompName.TabIndex = 33;
            this.lblCompName.Text = "Computer name (&pre-Windows 2000):";
            // 
            // txtCompName
            // 
            this.txtCompName.Location = new System.Drawing.Point(50, 99);
            this.txtCompName.MaxLength = 64;
            this.txtCompName.Name = "txtCompName";
            this.txtCompName.Size = new System.Drawing.Size(383, 21);
            this.txtCompName.TabIndex = 32;
            this.txtCompName.TextChanged += new System.EventHandler(this.txtCompName_TextChanged);
            // 
            // lblComputerName
            // 
            this.lblComputerName.AutoSize = true;
            this.lblComputerName.Location = new System.Drawing.Point(47, 83);
            this.lblComputerName.Name = "lblComputerName";
            this.lblComputerName.Size = new System.Drawing.Size(109, 13);
            this.lblComputerName.TabIndex = 31;
            this.lblComputerName.Text = "Computer n&ame: ";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(48, 174);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(367, 13);
            this.label2.TabIndex = 35;
            this.label2.Text = "The following user or group can join this computer to a domain";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(48, 195);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(95, 13);
            this.label3.TabIndex = 36;
            this.label3.Text = "&User or &group :";
            // 
            // txtUserGroup
            // 
            this.txtUserGroup.Location = new System.Drawing.Point(50, 212);
            this.txtUserGroup.Name = "txtUserGroup";
            this.txtUserGroup.ReadOnly = true;
            this.txtUserGroup.Size = new System.Drawing.Size(254, 21);
            this.txtUserGroup.TabIndex = 37;
			this.txtUserGroup.Text = "Default: Domain Admins";
            // 
            // btnChange
            // 
			this.btnChange.Enabled = false;
            this.btnChange.Location = new System.Drawing.Point(309, 212);
            this.btnChange.Name = "btnChange";
            this.btnChange.Size = new System.Drawing.Size(60, 22);
            this.btnChange.TabIndex = 38;
            this.btnChange.Text = "&Change:";
            this.btnChange.UseVisualStyleBackColor = true;
            this.btnChange.Click += new System.EventHandler(this.btnChange_Click);
            // 
            // cbWindowsComputer
            // 
            this.cbWindowsComputer.Location = new System.Drawing.Point(50, 240);
            this.cbWindowsComputer.Name = "cbWindowsComputer";
            this.cbWindowsComputer.Size = new System.Drawing.Size(410, 25);
            this.cbWindowsComputer.TabIndex = 39;
            this.cbWindowsComputer.Text = "Assign this computer account as a pre Windows 2000 computer";
            this.cbWindowsComputer.UseVisualStyleBackColor = true;
            // 
            // cbBackupComputer
            // 
            this.cbBackupComputer.Location = new System.Drawing.Point(50, 265);
            this.cbBackupComputer.Name = "cbBackupComputer";
            this.cbBackupComputer.Size = new System.Drawing.Size(410, 24);
            this.cbBackupComputer.TabIndex = 40;
            this.cbBackupComputer.Text = "Assign this computer account as a backup domain controller";
            this.cbBackupComputer.UseVisualStyleBackColor = true;
            // 
            // txtcreatein
            // 
            this.txtcreatein.BackColor = System.Drawing.SystemColors.Control;
            this.txtcreatein.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.txtcreatein.Location = new System.Drawing.Point(107, 35);
            this.txtcreatein.Name = "txtcreatein";
            this.txtcreatein.ReadOnly = true;
            this.txtcreatein.Size = new System.Drawing.Size(344, 14);
            this.txtcreatein.TabIndex = 41;
            // 
            // ComputerAddWelcomepage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.txtcreatein);
            this.Controls.Add(this.cbBackupComputer);
            this.Controls.Add(this.cbWindowsComputer);
            this.Controls.Add(this.btnChange);
            this.Controls.Add(this.txtUserGroup);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.txtWindowsCompName);
            this.Controls.Add(this.lblCompName);
            this.Controls.Add(this.txtCompName);
            this.Controls.Add(this.lblComputerName);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.pictureBox1);
            this.Name = "ComputerAddWelcomepage";
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.TextBox txtWindowsCompName;
        private System.Windows.Forms.Label lblCompName;
        private System.Windows.Forms.TextBox txtCompName;
        private System.Windows.Forms.Label lblComputerName;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox txtUserGroup;
        private System.Windows.Forms.Button btnChange;
        private System.Windows.Forms.CheckBox cbWindowsComputer;
        private System.Windows.Forms.CheckBox cbBackupComputer;
        private System.Windows.Forms.TextBox txtcreatein;
    }
}
