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
    partial class UserGeneralEditPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(UserGeneralEditPage));
            this.FNamelabel = new System.Windows.Forms.Label();
            this.FnametextBox = new System.Windows.Forms.TextBox();
            this.LNameLabel = new System.Windows.Forms.Label();
            this.LnametextBox = new System.Windows.Forms.TextBox();
            this.Initiallabel = new System.Windows.Forms.Label();
            this.InitialTextBox = new System.Windows.Forms.TextBox();
            this.Displaynamelabel = new System.Windows.Forms.Label();
            this.descriplabel = new System.Windows.Forms.Label();
            this.Officelabel = new System.Windows.Forms.Label();
            this.DisplayNametextBox = new System.Windows.Forms.TextBox();
            this.DescriptextBox = new System.Windows.Forms.TextBox();
            this.OfficetextBox = new System.Windows.Forms.TextBox();
            this.telelabel = new System.Windows.Forms.Label();
            this.Emaillabel = new System.Windows.Forms.Label();
            this.Webpagelabel = new System.Windows.Forms.Label();
            this.TelephonetextBox = new System.Windows.Forms.TextBox();
            this.emailtextBox = new System.Windows.Forms.TextBox();
            this.webpagetextBox = new System.Windows.Forms.TextBox();
            this.UserGeneralimageList = new System.Windows.Forms.ImageList(this.components);
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.lblUserName = new System.Windows.Forms.Label();
            this.btnTelOther = new System.Windows.Forms.Button();
            this.btnWebOther = new System.Windows.Forms.Button();
            this.pnlData.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.btnWebOther);
            this.pnlData.Controls.Add(this.btnTelOther);
            this.pnlData.Controls.Add(this.lblUserName);
            this.pnlData.Controls.Add(this.groupBox2);
            this.pnlData.Controls.Add(this.groupBox1);
            this.pnlData.Controls.Add(this.pictureBox1);
            this.pnlData.Controls.Add(this.webpagetextBox);
            this.pnlData.Controls.Add(this.emailtextBox);
            this.pnlData.Controls.Add(this.TelephonetextBox);
            this.pnlData.Controls.Add(this.Webpagelabel);
            this.pnlData.Controls.Add(this.Emaillabel);
            this.pnlData.Controls.Add(this.telelabel);
            this.pnlData.Controls.Add(this.OfficetextBox);
            this.pnlData.Controls.Add(this.DescriptextBox);
            this.pnlData.Controls.Add(this.DisplayNametextBox);
            this.pnlData.Controls.Add(this.Officelabel);
            this.pnlData.Controls.Add(this.descriplabel);
            this.pnlData.Controls.Add(this.Displaynamelabel);
            this.pnlData.Controls.Add(this.InitialTextBox);
            this.pnlData.Controls.Add(this.Initiallabel);
            this.pnlData.Controls.Add(this.LnametextBox);
            this.pnlData.Controls.Add(this.LNameLabel);
            this.pnlData.Controls.Add(this.FnametextBox);
            this.pnlData.Controls.Add(this.FNamelabel);
            this.pnlData.Size = new System.Drawing.Size(386, 411);
            // 
            // FNamelabel
            // 
            this.FNamelabel.AutoSize = true;
            this.FNamelabel.Location = new System.Drawing.Point(13, 89);
            this.FNamelabel.Name = "FNamelabel";
            this.FNamelabel.Size = new System.Drawing.Size(60, 13);
            this.FNamelabel.TabIndex = 0;
            this.FNamelabel.Text = "&First Name:";
            // 
            // FnametextBox
            // 
            this.FnametextBox.Location = new System.Drawing.Point(125, 84);
            this.FnametextBox.MaxLength = 64;
            this.FnametextBox.Name = "FnametextBox";
            this.FnametextBox.Size = new System.Drawing.Size(102, 20);
            this.FnametextBox.TabIndex = 1;
            this.FnametextBox.TextChanged += new System.EventHandler(this.FnametextBox_TextChanged);
            // 
            // LNameLabel
            // 
            this.LNameLabel.AutoSize = true;
            this.LNameLabel.Location = new System.Drawing.Point(13, 123);
            this.LNameLabel.Name = "LNameLabel";
            this.LNameLabel.Size = new System.Drawing.Size(61, 13);
            this.LNameLabel.TabIndex = 2;
            this.LNameLabel.Text = "&Last Name:";
            // 
            // LnametextBox
            // 
            this.LnametextBox.Location = new System.Drawing.Point(125, 120);
            this.LnametextBox.MaxLength = 64;
            this.LnametextBox.Name = "LnametextBox";
            this.LnametextBox.Size = new System.Drawing.Size(254, 20);
            this.LnametextBox.TabIndex = 3;
            this.LnametextBox.TextChanged += new System.EventHandler(this.LnametextBox_TextChanged);
            // 
            // Initiallabel
            // 
            this.Initiallabel.AutoSize = true;
            this.Initiallabel.Location = new System.Drawing.Point(231, 89);
            this.Initiallabel.Name = "Initiallabel";
            this.Initiallabel.Size = new System.Drawing.Size(39, 13);
            this.Initiallabel.TabIndex = 4;
            this.Initiallabel.Text = "&Initials:";
            // 
            // InitialTextBox
            // 
            this.InitialTextBox.Location = new System.Drawing.Point(277, 84);
            this.InitialTextBox.MaxLength = 6;
            this.InitialTextBox.Name = "InitialTextBox";
            this.InitialTextBox.Size = new System.Drawing.Size(102, 20);
            this.InitialTextBox.TabIndex = 5;
            this.InitialTextBox.TextChanged += new System.EventHandler(this.InitialTextBox_TextChanged);
            // 
            // Displaynamelabel
            // 
            this.Displaynamelabel.AutoSize = true;
            this.Displaynamelabel.Location = new System.Drawing.Point(13, 157);
            this.Displaynamelabel.Name = "Displaynamelabel";
            this.Displaynamelabel.Size = new System.Drawing.Size(75, 13);
            this.Displaynamelabel.TabIndex = 9;
            this.Displaynamelabel.Text = "Di&splay Name:";
            // 
            // descriplabel
            // 
            this.descriplabel.AutoSize = true;
            this.descriplabel.Location = new System.Drawing.Point(13, 191);
            this.descriplabel.Name = "descriplabel";
            this.descriplabel.Size = new System.Drawing.Size(63, 13);
            this.descriplabel.TabIndex = 10;
            this.descriplabel.Text = "&Description:";
            // 
            // Officelabel
            // 
            this.Officelabel.AutoSize = true;
            this.Officelabel.Location = new System.Drawing.Point(13, 225);
            this.Officelabel.Name = "Officelabel";
            this.Officelabel.Size = new System.Drawing.Size(38, 13);
            this.Officelabel.TabIndex = 11;
            this.Officelabel.Text = "Offi&ce:";
            // 
            // DisplayNametextBox
            // 
            this.DisplayNametextBox.Location = new System.Drawing.Point(125, 154);
            this.DisplayNametextBox.MaxLength = 256;
            this.DisplayNametextBox.Name = "DisplayNametextBox";
            this.DisplayNametextBox.Size = new System.Drawing.Size(254, 20);
            this.DisplayNametextBox.TabIndex = 12;
            this.DisplayNametextBox.TextChanged += new System.EventHandler(this.DisplayNametextBox_TextChanged);
            // 
            // DescriptextBox
            // 
            this.DescriptextBox.Location = new System.Drawing.Point(125, 188);
            this.DescriptextBox.MaxLength = 1024;
            this.DescriptextBox.Name = "DescriptextBox";
            this.DescriptextBox.Size = new System.Drawing.Size(254, 20);
            this.DescriptextBox.TabIndex = 13;
            this.DescriptextBox.TextChanged += new System.EventHandler(this.DescriptextBox_TextChanged);
            // 
            // OfficetextBox
            // 
            this.OfficetextBox.Location = new System.Drawing.Point(125, 222);
            this.OfficetextBox.MaxLength = 128;
            this.OfficetextBox.Name = "OfficetextBox";
            this.OfficetextBox.Size = new System.Drawing.Size(254, 20);
            this.OfficetextBox.TabIndex = 14;
            this.OfficetextBox.TextChanged += new System.EventHandler(this.OfficetextBox_TextChanged);
            // 
            // telelabel
            // 
            this.telelabel.AutoSize = true;
            this.telelabel.Location = new System.Drawing.Point(13, 294);
            this.telelabel.Name = "telelabel";
            this.telelabel.Size = new System.Drawing.Size(101, 13);
            this.telelabel.TabIndex = 15;
            this.telelabel.Text = "&Telephone Number:";
            // 
            // Emaillabel
            // 
            this.Emaillabel.AutoSize = true;
            this.Emaillabel.Location = new System.Drawing.Point(13, 329);
            this.Emaillabel.Name = "Emaillabel";
            this.Emaillabel.Size = new System.Drawing.Size(35, 13);
            this.Emaillabel.TabIndex = 16;
            this.Emaillabel.Text = "E&mail:";
            // 
            // Webpagelabel
            // 
            this.Webpagelabel.AutoSize = true;
            this.Webpagelabel.Location = new System.Drawing.Point(13, 364);
            this.Webpagelabel.Name = "Webpagelabel";
            this.Webpagelabel.Size = new System.Drawing.Size(61, 13);
            this.Webpagelabel.TabIndex = 17;
            this.Webpagelabel.Text = "&Web Page:";
            // 
            // TelephonetextBox
            // 
            this.TelephonetextBox.Location = new System.Drawing.Point(125, 290);
            this.TelephonetextBox.MaxLength = 64;
            this.TelephonetextBox.Name = "TelephonetextBox";
            this.TelephonetextBox.Size = new System.Drawing.Size(173, 20);
            this.TelephonetextBox.TabIndex = 18;
            this.TelephonetextBox.TextChanged += new System.EventHandler(this.TelephonetextBox_TextChanged);
            // 
            // emailtextBox
            // 
            this.emailtextBox.Location = new System.Drawing.Point(125, 326);
            this.emailtextBox.MaxLength = 256;
            this.emailtextBox.Name = "emailtextBox";
            this.emailtextBox.Size = new System.Drawing.Size(254, 20);
            this.emailtextBox.TabIndex = 19;
            this.emailtextBox.TextChanged += new System.EventHandler(this.emailtextBox_TextChanged);
            // 
            // webpagetextBox
            // 
            this.webpagetextBox.Location = new System.Drawing.Point(125, 357);
            this.webpagetextBox.MaxLength = 1024;
            this.webpagetextBox.Name = "webpagetextBox";
            this.webpagetextBox.Size = new System.Drawing.Size(173, 20);
            this.webpagetextBox.TabIndex = 20;
            this.webpagetextBox.TextChanged += new System.EventHandler(this.webpagetextBox_TextChanged);
            // 
            // UserGeneralimageList
            // 
            this.UserGeneralimageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("UserGeneralimageList.ImageStream")));
            this.UserGeneralimageList.TransparentColor = System.Drawing.Color.Transparent;
            this.UserGeneralimageList.Images.SetKeyName(0, "User_24.ico");
            this.UserGeneralimageList.Images.SetKeyName(1, "User.ico");
            this.UserGeneralimageList.Images.SetKeyName(2, "User_16.ico");
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(16, 23);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(24, 24);
            this.pictureBox1.TabIndex = 21;
            this.pictureBox1.TabStop = false;
            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(13, 65);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(361, 2);
            this.groupBox1.TabIndex = 22;
            this.groupBox1.TabStop = false;
            // 
            // groupBox2
            // 
            this.groupBox2.Location = new System.Drawing.Point(13, 263);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(361, 2);
            this.groupBox2.TabIndex = 23;
            this.groupBox2.TabStop = false;
            // 
            // lblUserName
            // 
            this.lblUserName.AutoSize = true;
            this.lblUserName.Location = new System.Drawing.Point(50, 29);
            this.lblUserName.Name = "lblUserName";
            this.lblUserName.Size = new System.Drawing.Size(97, 13);
            this.lblUserName.TabIndex = 24;
            this.lblUserName.Text = "                              ";
            // 
            // btnTelOther
            // 
            this.btnTelOther.Location = new System.Drawing.Point(304, 290);
            this.btnTelOther.Name = "btnTelOther";
            this.btnTelOther.Size = new System.Drawing.Size(75, 21);
            this.btnTelOther.TabIndex = 25;
            this.btnTelOther.Text = "Othe&r...";
            this.btnTelOther.UseVisualStyleBackColor = true;
            this.btnTelOther.Click += new System.EventHandler(this.btnTelOther_Click);
            // 
            // btnWebOther
            // 
            this.btnWebOther.Location = new System.Drawing.Point(304, 356);
            this.btnWebOther.Name = "btnWebOther";
            this.btnWebOther.Size = new System.Drawing.Size(75, 21);
            this.btnWebOther.TabIndex = 26;
            this.btnWebOther.Text = "Othe&r...";
            this.btnWebOther.UseVisualStyleBackColor = true;
            this.btnWebOther.Click += new System.EventHandler(this.btnWebOther_Click);
            // 
            // UserGeneralEditPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.Name = "UserGeneralEditPage";
            this.Size = new System.Drawing.Size(386, 411);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label FNamelabel;
        private System.Windows.Forms.TextBox InitialTextBox;
        private System.Windows.Forms.Label Initiallabel;
        private System.Windows.Forms.TextBox LnametextBox;
        private System.Windows.Forms.Label LNameLabel;
        private System.Windows.Forms.TextBox FnametextBox;
        private System.Windows.Forms.Label Officelabel;
        private System.Windows.Forms.Label descriplabel;
        private System.Windows.Forms.Label Displaynamelabel;
        private System.Windows.Forms.TextBox webpagetextBox;
        private System.Windows.Forms.TextBox emailtextBox;
        private System.Windows.Forms.TextBox TelephonetextBox;
        private System.Windows.Forms.Label Webpagelabel;
        private System.Windows.Forms.Label Emaillabel;
        private System.Windows.Forms.Label telelabel;
        private System.Windows.Forms.TextBox OfficetextBox;
        private System.Windows.Forms.TextBox DescriptextBox;
        private System.Windows.Forms.TextBox DisplayNametextBox;
        private System.Windows.Forms.ImageList UserGeneralimageList;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Label lblUserName;
        private System.Windows.Forms.Button btnWebOther;
        private System.Windows.Forms.Button btnTelOther;

    }
}