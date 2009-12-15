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
    partial class UserAddWelcomePage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(UserAddWelcomePage));
            this.label1 = new System.Windows.Forms.Label();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.Fnamelabel = new System.Windows.Forms.Label();
            this.LNamelabel = new System.Windows.Forms.Label();
            this.FnametextBox = new System.Windows.Forms.TextBox();
            this.LnametextBox = new System.Windows.Forms.TextBox();
            this.Initiallabel = new System.Windows.Forms.Label();
            this.InitialtextBox = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.fullnametextBox = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.logonNametextBox = new System.Windows.Forms.TextBox();
            this.domainNamecomboBox = new System.Windows.Forms.ComboBox();
            this.Userlogonglabel = new System.Windows.Forms.Label();
            this.prelogontextBox = new System.Windows.Forms.TextBox();
            this.userlogonPretextBox = new System.Windows.Forms.TextBox();
            this.txtcreatein = new System.Windows.Forms.TextBox();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            //
            // label1
            //
            this.label1.Location = new System.Drawing.Point(92, 32);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(0, 13);
            this.label1.TabIndex = 0;
            //
            // pictureBox1
            //
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(39, 28);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(24, 24);
            this.pictureBox1.TabIndex = 22;
            this.pictureBox1.TabStop = false;
            //
            // groupBox1
            //
            this.groupBox1.Location = new System.Drawing.Point(41, 63);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(414, 2);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "groupBox1";
            //
            // groupBox2
            //
            this.groupBox2.Location = new System.Drawing.Point(40, 267);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(416, 2);
            this.groupBox2.TabIndex = 16;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "groupBox2";
            //
            // Fnamelabel
            //
            this.Fnamelabel.AutoSize = true;
            this.Fnamelabel.Location = new System.Drawing.Point(41, 77);
            this.Fnamelabel.Name = "Fnamelabel";
            this.Fnamelabel.Size = new System.Drawing.Size(72, 13);
            this.Fnamelabel.TabIndex = 2;
            this.Fnamelabel.Text = "First name:";
            //
            // LNamelabel
            //
            this.LNamelabel.AutoSize = true;
            this.LNamelabel.Location = new System.Drawing.Point(41, 106);
            this.LNamelabel.Name = "LNamelabel";
            this.LNamelabel.Size = new System.Drawing.Size(71, 13);
            this.LNamelabel.TabIndex = 6;
            this.LNamelabel.Text = "Last name:";
            //
            // FnametextBox
            //
            this.FnametextBox.Location = new System.Drawing.Point(120, 77);
            this.FnametextBox.MaxLength = 64;
            this.FnametextBox.Name = "FnametextBox";
            this.FnametextBox.Size = new System.Drawing.Size(148, 21);
            this.FnametextBox.TabIndex = 3;
            this.FnametextBox.TextChanged += new System.EventHandler(this.FnametextBox_TextChanged);
            //
            // LnametextBox
            //
            this.LnametextBox.Location = new System.Drawing.Point(120, 106);
            this.LnametextBox.MaxLength = 64;
            this.LnametextBox.Name = "LnametextBox";
            this.LnametextBox.Size = new System.Drawing.Size(315, 21);
            this.LnametextBox.TabIndex = 7;
            this.LnametextBox.TextChanged += new System.EventHandler(this.LnametextBox_TextChanged);
            //
            // Initiallabel
            //
            this.Initiallabel.AutoSize = true;
            this.Initiallabel.Location = new System.Drawing.Point(279, 77);
            this.Initiallabel.Name = "Initiallabel";
            this.Initiallabel.Size = new System.Drawing.Size(50, 13);
            this.Initiallabel.TabIndex = 4;
            this.Initiallabel.Text = "Initials:";
            //
            // InitialtextBox
            //
            this.InitialtextBox.Location = new System.Drawing.Point(335, 77);
            this.InitialtextBox.MaxLength = 6;
            this.InitialtextBox.Name = "InitialtextBox";
            this.InitialtextBox.Size = new System.Drawing.Size(100, 21);
            this.InitialtextBox.TabIndex = 5;
            this.InitialtextBox.TextChanged += new System.EventHandler(this.InitialtextBox_TextChanged);
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(41, 136);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(67, 13);
            this.label2.TabIndex = 8;
            this.label2.Text = "Full name:";
            //
            // fullnametextBox
            //
            this.fullnametextBox.Location = new System.Drawing.Point(120, 136);
            this.fullnametextBox.MaxLength = 64;
            this.fullnametextBox.Name = "fullnametextBox";
            this.fullnametextBox.Size = new System.Drawing.Size(315, 21);
            this.fullnametextBox.TabIndex = 9;
            this.fullnametextBox.TextChanged += new System.EventHandler(this.fullnametextBox_TextChanged);
            //
            // label3
            //
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(41, 164);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(109, 13);
            this.label3.TabIndex = 10;
            this.label3.Text = "User logon name:";
            //
            // logonNametextBox
            //
            this.logonNametextBox.Location = new System.Drawing.Point(39, 186);
            this.logonNametextBox.MaxLength = 256;
            this.logonNametextBox.Name = "logonNametextBox";
            this.logonNametextBox.Size = new System.Drawing.Size(195, 21);
            this.logonNametextBox.TabIndex = 11;
            this.logonNametextBox.TextChanged += new System.EventHandler(this.logonNametextBox_TextChanged);
            //
            // domainNamecomboBox
            //
            this.domainNamecomboBox.DropDownHeight = 120;
            this.domainNamecomboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.domainNamecomboBox.FormattingEnabled = true;
            this.domainNamecomboBox.IntegralHeight = false;
            this.domainNamecomboBox.Location = new System.Drawing.Point(248, 186);
            this.domainNamecomboBox.Name = "domainNamecomboBox";
            this.domainNamecomboBox.Size = new System.Drawing.Size(187, 21);
            this.domainNamecomboBox.TabIndex = 12;

            //
            // Userlogonglabel
            //
            this.Userlogonglabel.AutoSize = true;
            this.Userlogonglabel.Location = new System.Drawing.Point(41, 216);
            this.Userlogonglabel.Name = "Userlogonglabel";
            this.Userlogonglabel.Size = new System.Drawing.Size(227, 13);
            this.Userlogonglabel.TabIndex = 13;
            this.Userlogonglabel.Text = "User logon name (pre-windows 2000):";
            //
            // prelogontextBox
            //
            this.prelogontextBox.Location = new System.Drawing.Point(39, 235);
            this.prelogontextBox.Name = "prelogontextBox";
            this.prelogontextBox.ReadOnly = true;
            this.prelogontextBox.Size = new System.Drawing.Size(189, 21);
            this.prelogontextBox.TabIndex = 14;
            //
            // userlogonPretextBox
            //
            this.userlogonPretextBox.Location = new System.Drawing.Point(248, 235);
            this.userlogonPretextBox.MaxLength = 20;
            this.userlogonPretextBox.Name = "userlogonPretextBox";
            this.userlogonPretextBox.Size = new System.Drawing.Size(187, 21);
            this.userlogonPretextBox.TabIndex = 15;
            this.userlogonPretextBox.TextChanged += new System.EventHandler(this.userlogonPretextBox_TextChanged);
            //
            // txtcreatein
            //
            this.txtcreatein.BackColor = System.Drawing.SystemColors.Control;
            this.txtcreatein.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.txtcreatein.Location = new System.Drawing.Point(71, 35);
            this.txtcreatein.Name = "txtcreatein";
            this.txtcreatein.ReadOnly = true;
            this.txtcreatein.Size = new System.Drawing.Size(386, 14);
            this.txtcreatein.TabIndex = 40;
            //
            // UserAddWelcomePage
            //
            this.Controls.Add(this.txtcreatein);
            this.Controls.Add(this.userlogonPretextBox);
            this.Controls.Add(this.prelogontextBox);
            this.Controls.Add(this.Userlogonglabel);
            this.Controls.Add(this.domainNamecomboBox);
            this.Controls.Add(this.logonNametextBox);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.fullnametextBox);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.InitialtextBox);
            this.Controls.Add(this.Initiallabel);
            this.Controls.Add(this.LnametextBox);
            this.Controls.Add(this.FnametextBox);
            this.Controls.Add(this.LNamelabel);
            this.Controls.Add(this.Fnamelabel);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.pictureBox1);
            this.Controls.Add(this.label1);
            this.Name = "UserAddWelcomePage";
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Label Fnamelabel;
        private System.Windows.Forms.Label LNamelabel;
        private System.Windows.Forms.TextBox FnametextBox;
        private System.Windows.Forms.TextBox LnametextBox;
        private System.Windows.Forms.Label Initiallabel;
        private System.Windows.Forms.TextBox InitialtextBox;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox fullnametextBox;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox logonNametextBox;
        private System.Windows.Forms.ComboBox domainNamecomboBox;
        private System.Windows.Forms.Label Userlogonglabel;
        private System.Windows.Forms.TextBox prelogontextBox;
        private System.Windows.Forms.TextBox userlogonPretextBox;
        private System.Windows.Forms.TextBox txtcreatein;
    }
}
