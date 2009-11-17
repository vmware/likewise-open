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

namespace Likewise.LMC.Plugins.FileShareManager
{
    partial class NewSharePermissionsPage
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
            this.panel1 = new System.Windows.Forms.Panel();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.radioButtonUsersReadyOnly = new System.Windows.Forms.RadioButton();
            this.radioButtonAdminAccess = new System.Windows.Forms.RadioButton();
            this.radioButtonCustomPer = new System.Windows.Forms.RadioButton();
            this.radioButtonUsernoAccess = new System.Windows.Forms.RadioButton();
            this.btnCustom = new System.Windows.Forms.Button();
            this.label4 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.SystemColors.Window;
            this.panel1.Controls.Add(this.pictureBox1);
            this.panel1.Controls.Add(this.label2);
            this.panel1.Controls.Add(this.label1);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(490, 72);
            this.panel1.TabIndex = 0;
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = global::Likewise.LMC.Plugins.FileShareManager.Properties.Resources.shared_folders;
            this.pictureBox1.Location = new System.Drawing.Point(430, 12);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(42, 50);
            this.pictureBox1.TabIndex = 2;
            this.pictureBox1.TabStop = false;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(52, 29);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(346, 26);
            this.label2.TabIndex = 1;
            this.label2.Text = "Permissions let you control who can see the folder and the \r\nlevel of access they" +
                " have.";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(31, 12);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(182, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Shared Folder Permissions";
            // 
            // label3
            // 
            this.label3.Location = new System.Drawing.Point(9, 86);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(420, 13);
            this.label3.TabIndex = 2;
            this.label3.Text = "Set the kind of permissions you want for the shared folder.";
            // 
            // radioButtonUsersReadyOnly
            // 
            this.radioButtonUsersReadyOnly.Checked = true;
            this.radioButtonUsersReadyOnly.Location = new System.Drawing.Point(33, 111);
            this.radioButtonUsersReadyOnly.Name = "radioButtonUsersReadyOnly";
            this.radioButtonUsersReadyOnly.Size = new System.Drawing.Size(408, 17);
            this.radioButtonUsersReadyOnly.TabIndex = 3;
            this.radioButtonUsersReadyOnly.TabStop = true;
            this.radioButtonUsersReadyOnly.Text = "&Allow users have ready-only access.";
            this.radioButtonUsersReadyOnly.UseVisualStyleBackColor = true;
            // 
            // radioButtonAdminAccess
            // 
            this.radioButtonAdminAccess.Location = new System.Drawing.Point(33, 136);
            this.radioButtonAdminAccess.Name = "radioButtonAdminAccess";
            this.radioButtonAdminAccess.Size = new System.Drawing.Size(437, 17);
            this.radioButtonAdminAccess.TabIndex = 4;
            this.radioButtonAdminAccess.TabStop = true;
            this.radioButtonAdminAccess.Text = "Administrator have full access; other users have &read-only access.";
            this.radioButtonAdminAccess.UseVisualStyleBackColor = true;
            // 
            // radioButtonCustomPer
            // 
            this.radioButtonCustomPer.Location = new System.Drawing.Point(33, 183);
            this.radioButtonCustomPer.Name = "radioButtonCustomPer";
            this.radioButtonCustomPer.Size = new System.Drawing.Size(279, 17);
            this.radioButtonCustomPer.TabIndex = 6;
            this.radioButtonCustomPer.TabStop = true;
            this.radioButtonCustomPer.Text = "&Customize permissions";
            this.radioButtonCustomPer.UseVisualStyleBackColor = true;
            // 
            // radioButtonUsernoAccess
            // 
            this.radioButtonUsernoAccess.Location = new System.Drawing.Point(33, 158);
            this.radioButtonUsernoAccess.Name = "radioButtonUsernoAccess";
            this.radioButtonUsernoAccess.Size = new System.Drawing.Size(396, 17);
            this.radioButtonUsernoAccess.TabIndex = 7;
            this.radioButtonUsernoAccess.TabStop = true;
            this.radioButtonUsernoAccess.Text = "Administrator have full access; other users &no access.";
            this.radioButtonUsernoAccess.UseVisualStyleBackColor = true;
            // 
            // btnCustom
            // 
            this.btnCustom.Enabled = false;
            this.btnCustom.Location = new System.Drawing.Point(73, 208);
            this.btnCustom.Name = "btnCustom";
            this.btnCustom.Size = new System.Drawing.Size(75, 23);
            this.btnCustom.TabIndex = 8;
            this.btnCustom.Text = "C&ustom..";
            this.btnCustom.UseVisualStyleBackColor = true;
            // 
            // label4
            // 
            this.label4.AutoEllipsis = true;
            this.label4.Location = new System.Drawing.Point(3, 240);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(481, 13);
            this.label4.TabIndex = 9;
            this.label4.Text = "When you set custom permissions, you are only setting them for the folder itself." +
                "";
            // 
            // label5
            // 
            this.label5.AutoEllipsis = true;
            this.label5.Location = new System.Drawing.Point(8, 271);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(471, 30);
            this.label5.TabIndex = 10;
            this.label5.Text = "It is suggested that you set specific permissions on the folder items themselves\r" +
                "\nif desired.";
            // 
            // NewSharePermissionsPage
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Controls.Add(this.label5);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.btnCustom);
            this.Controls.Add(this.radioButtonUsernoAccess);
            this.Controls.Add(this.radioButtonCustomPer);
            this.Controls.Add(this.radioButtonAdminAccess);
            this.Controls.Add(this.radioButtonUsersReadyOnly);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.panel1);
            this.MaximumSize = new System.Drawing.Size(490, 313);
            this.MinimumSize = new System.Drawing.Size(490, 313);
            this.Name = "NewSharePermissionsPage";
            this.Size = new System.Drawing.Size(490, 313);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.RadioButton radioButtonUsersReadyOnly;
        private System.Windows.Forms.RadioButton radioButtonAdminAccess;
        private System.Windows.Forms.RadioButton radioButtonCustomPer;
        private System.Windows.Forms.RadioButton radioButtonUsernoAccess;
        private System.Windows.Forms.Button btnCustom;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label5;
    }
}
