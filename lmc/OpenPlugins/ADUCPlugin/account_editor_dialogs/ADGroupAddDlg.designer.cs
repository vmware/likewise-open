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
    partial class ADGroupAddDlg
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ADGroupAddDlg));
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.pictureBox = new System.Windows.Forms.PictureBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.lblGroupName = new System.Windows.Forms.Label();
            this.txtGroupname = new System.Windows.Forms.TextBox();
            this.lblGroupNamePrewin = new System.Windows.Forms.Label();
            this.txtprewin = new System.Windows.Forms.TextBox();
            this.groupboxGroupScope = new System.Windows.Forms.GroupBox();
            this.rbtnUniversal = new System.Windows.Forms.RadioButton();
            this.rbtnGlobal = new System.Windows.Forms.RadioButton();
            this.rbtnDomainLocal = new System.Windows.Forms.RadioButton();
            this.groupBoxGroupType = new System.Windows.Forms.GroupBox();
            this.rbtnDistribution = new System.Windows.Forms.RadioButton();
            this.rbtnSecurity = new System.Windows.Forms.RadioButton();
            this.btnOk = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.txtcreatein = new System.Windows.Forms.TextBox();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox)).BeginInit();
            this.groupboxGroupScope.SuspendLayout();
            this.groupBoxGroupType.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBox2
            // 
            this.groupBox2.Location = new System.Drawing.Point(8, 281);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(450, 2);
            this.groupBox2.TabIndex = 34;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "groupBox2";
            // 
            // pictureBox
            // 
            this.pictureBox.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox.Image")));
            this.pictureBox.Location = new System.Drawing.Point(12, 17);
            this.pictureBox.Name = "pictureBox";
            this.pictureBox.Size = new System.Drawing.Size(24, 24);
            this.pictureBox.TabIndex = 33;
            this.pictureBox.TabStop = false;
            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(11, 54);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(445, 2);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "groupBox2";
            // 
            // lblGroupName
            // 
            this.lblGroupName.AutoSize = true;
            this.lblGroupName.Location = new System.Drawing.Point(15, 69);
            this.lblGroupName.Name = "lblGroupName";
            this.lblGroupName.Size = new System.Drawing.Size(68, 13);
            this.lblGroupName.TabIndex = 2;
            this.lblGroupName.Text = "Group n&ame:";
            // 
            // txtGroupname
            // 
            this.txtGroupname.Location = new System.Drawing.Point(18, 89);
            this.txtGroupname.MaxLength = 64;
            this.txtGroupname.Name = "txtGroupname";
            this.txtGroupname.Size = new System.Drawing.Size(438, 20);
            this.txtGroupname.TabIndex = 3;
            this.txtGroupname.TextChanged += new System.EventHandler(this.txtGroupname_TextChanged);
            // 
            // lblGroupNamePrewin
            // 
            this.lblGroupNamePrewin.AutoSize = true;
            this.lblGroupNamePrewin.Location = new System.Drawing.Point(16, 119);
            this.lblGroupNamePrewin.Name = "lblGroupNamePrewin";
            this.lblGroupNamePrewin.Size = new System.Drawing.Size(163, 13);
            this.lblGroupNamePrewin.TabIndex = 4;
            this.lblGroupNamePrewin.Text = "Group name(pre-&Windows 2000):";
            // 
            // txtprewin
            // 
            this.txtprewin.Location = new System.Drawing.Point(18, 141);
            this.txtprewin.MaxLength = 256;
            this.txtprewin.Name = "txtprewin";
            this.txtprewin.Size = new System.Drawing.Size(438, 20);
            this.txtprewin.TabIndex = 5;
            this.txtprewin.TextChanged += new System.EventHandler(this.txtprewin_TextChanged);
            // 
            // groupboxGroupScope
            // 
            this.groupboxGroupScope.Controls.Add(this.rbtnUniversal);
            this.groupboxGroupScope.Controls.Add(this.rbtnGlobal);
            this.groupboxGroupScope.Controls.Add(this.rbtnDomainLocal);
            this.groupboxGroupScope.Location = new System.Drawing.Point(18, 167);
            this.groupboxGroupScope.Name = "groupboxGroupScope";
            this.groupboxGroupScope.Size = new System.Drawing.Size(210, 99);
            this.groupboxGroupScope.TabIndex = 6;
            this.groupboxGroupScope.TabStop = false;
            this.groupboxGroupScope.Text = "Group scope";
            // 
            // rbtnUniversal
            // 
            this.rbtnUniversal.Location = new System.Drawing.Point(7, 66);
            this.rbtnUniversal.Name = "rbtnUniversal";
            this.rbtnUniversal.Size = new System.Drawing.Size(143, 17);
            this.rbtnUniversal.TabIndex = 2;
            this.rbtnUniversal.Text = "&Universal";
            this.rbtnUniversal.UseVisualStyleBackColor = true;
            // 
            // rbtnGlobal
            // 
            this.rbtnGlobal.Checked = true;
            this.rbtnGlobal.Location = new System.Drawing.Point(7, 43);
            this.rbtnGlobal.Name = "rbtnGlobal";
            this.rbtnGlobal.Size = new System.Drawing.Size(143, 17);
            this.rbtnGlobal.TabIndex = 1;
            this.rbtnGlobal.TabStop = true;
            this.rbtnGlobal.Text = "&Global";
            this.rbtnGlobal.UseVisualStyleBackColor = true;
            // 
            // rbtnDomainLocal
            // 
            this.rbtnDomainLocal.Location = new System.Drawing.Point(7, 20);
            this.rbtnDomainLocal.Name = "rbtnDomainLocal";
            this.rbtnDomainLocal.Size = new System.Drawing.Size(143, 17);
            this.rbtnDomainLocal.TabIndex = 0;
            this.rbtnDomainLocal.Text = "D&omain local";
            this.rbtnDomainLocal.UseVisualStyleBackColor = true;
            this.rbtnDomainLocal.CheckedChanged += new System.EventHandler(this.rbtn_CheckedChanged);
            // 
            // groupBoxGroupType
            // 
            this.groupBoxGroupType.Controls.Add(this.rbtnDistribution);
            this.groupBoxGroupType.Controls.Add(this.rbtnSecurity);
            this.groupBoxGroupType.Location = new System.Drawing.Point(249, 168);
            this.groupBoxGroupType.Name = "groupBoxGroupType";
            this.groupBoxGroupType.Size = new System.Drawing.Size(209, 99);
            this.groupBoxGroupType.TabIndex = 7;
            this.groupBoxGroupType.TabStop = false;
            this.groupBoxGroupType.Text = "Group type";
            // 
            // rbtnDistribution
            // 
            this.rbtnDistribution.Location = new System.Drawing.Point(7, 43);
            this.rbtnDistribution.Name = "rbtnDistribution";
            this.rbtnDistribution.Size = new System.Drawing.Size(142, 17);
            this.rbtnDistribution.TabIndex = 1;
            this.rbtnDistribution.Text = "&Distribution";
            this.rbtnDistribution.UseVisualStyleBackColor = true;
            // 
            // rbtnSecurity
            // 
            this.rbtnSecurity.Checked = true;
            this.rbtnSecurity.Location = new System.Drawing.Point(7, 20);
            this.rbtnSecurity.Name = "rbtnSecurity";
            this.rbtnSecurity.Size = new System.Drawing.Size(142, 17);
            this.rbtnSecurity.TabIndex = 0;
            this.rbtnSecurity.TabStop = true;
            this.rbtnSecurity.Text = "&Security";
            this.rbtnSecurity.UseVisualStyleBackColor = true;
            // 
            // btnOk
            // 
            this.btnOk.Enabled = false;
            this.btnOk.Location = new System.Drawing.Point(302, 289);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(75, 23);
            this.btnOk.TabIndex = 8;
            this.btnOk.Text = "OK";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(383, 289);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 9;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // txtcreatein
            // 
            this.txtcreatein.BackColor = System.Drawing.SystemColors.Control;
            this.txtcreatein.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.txtcreatein.Location = new System.Drawing.Point(45, 24);
            this.txtcreatein.Name = "txtcreatein";
            this.txtcreatein.ReadOnly = true;
            this.txtcreatein.Size = new System.Drawing.Size(413, 13);
            this.txtcreatein.TabIndex = 35;
            // 
            // ADGroupAddDlg
            // 
            this.AcceptButton = this.btnOk;
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(477, 322);
            this.Controls.Add(this.txtcreatein);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.groupBoxGroupType);
            this.Controls.Add(this.groupboxGroupScope);
            this.Controls.Add(this.txtprewin);
            this.Controls.Add(this.lblGroupNamePrewin);
            this.Controls.Add(this.txtGroupname);
            this.Controls.Add(this.lblGroupName);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.pictureBox);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ADGroupAddDlg";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "New Object - Group";
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox)).EndInit();
            this.groupboxGroupScope.ResumeLayout(false);
            this.groupBoxGroupType.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.PictureBox pictureBox;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label lblGroupName;
        private System.Windows.Forms.TextBox txtGroupname;
        private System.Windows.Forms.Label lblGroupNamePrewin;
        private System.Windows.Forms.TextBox txtprewin;
        private System.Windows.Forms.GroupBox groupboxGroupScope;
        private System.Windows.Forms.RadioButton rbtnDomainLocal;
        private System.Windows.Forms.RadioButton rbtnUniversal;
        private System.Windows.Forms.RadioButton rbtnGlobal;
        private System.Windows.Forms.GroupBox groupBoxGroupType;
        private System.Windows.Forms.RadioButton rbtnDistribution;
        private System.Windows.Forms.RadioButton rbtnSecurity;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.TextBox txtcreatein;
    }
}