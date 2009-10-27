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
    partial class OUGeneralEditPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(OUGeneralEditPage));
            this.txtDescription = new System.Windows.Forms.TextBox();
            this.lbStreet = new System.Windows.Forms.Label();
            this.lbCity = new System.Windows.Forms.Label();
            this.lbstate = new System.Windows.Forms.Label();
            this.txtCity = new System.Windows.Forms.TextBox();
            this.txtstate = new System.Windows.Forms.TextBox();
            this.lbzip = new System.Windows.Forms.Label();
            this.lbCountry = new System.Windows.Forms.Label();
            this.txtZip = new System.Windows.Forms.TextBox();
            this.UserGeneralimageList = new System.Windows.Forms.ImageList(this.components);
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.userNamelabel = new System.Windows.Forms.Label();
            this.lbDescription = new System.Windows.Forms.Label();
            this.rtbStreet = new System.Windows.Forms.RichTextBox();
            this.cbcountry = new System.Windows.Forms.ComboBox();
            this.pnlData.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.cbcountry);
            this.pnlData.Controls.Add(this.rtbStreet);
            this.pnlData.Controls.Add(this.userNamelabel);
            this.pnlData.Controls.Add(this.groupBox1);
            this.pnlData.Controls.Add(this.pictureBox1);
            this.pnlData.Controls.Add(this.txtZip);
            this.pnlData.Controls.Add(this.lbCountry);
            this.pnlData.Controls.Add(this.lbzip);
            this.pnlData.Controls.Add(this.txtstate);
            this.pnlData.Controls.Add(this.txtCity);
            this.pnlData.Controls.Add(this.lbstate);
            this.pnlData.Controls.Add(this.lbCity);
            this.pnlData.Controls.Add(this.lbStreet);
            this.pnlData.Controls.Add(this.txtDescription);
            this.pnlData.Controls.Add(this.lbDescription);
            this.pnlData.Size = new System.Drawing.Size(386, 411);
            // 
            // txtDescription
            // 
            this.txtDescription.Location = new System.Drawing.Point(120, 79);
            this.txtDescription.MaxLength = 1024;
            this.txtDescription.Name = "txtDescription";
            this.txtDescription.Size = new System.Drawing.Size(254, 20);
            this.txtDescription.TabIndex = 3;
            this.txtDescription.TextChanged += new System.EventHandler(this.txtDescription_TextChanged);
            // 
            // lbStreet
            // 
            this.lbStreet.AutoSize = true;
            this.lbStreet.Location = new System.Drawing.Point(13, 115);
            this.lbStreet.Name = "lbStreet";
            this.lbStreet.Size = new System.Drawing.Size(38, 13);
            this.lbStreet.TabIndex = 9;
            this.lbStreet.Text = "&Street:";
            // 
            // lbCity
            // 
            this.lbCity.AutoSize = true;
            this.lbCity.Location = new System.Drawing.Point(13, 191);
            this.lbCity.Name = "lbCity";
            this.lbCity.Size = new System.Drawing.Size(27, 13);
            this.lbCity.TabIndex = 10;
            this.lbCity.Text = "&City:";
            // 
            // lbstate
            // 
            this.lbstate.AutoSize = true;
            this.lbstate.Location = new System.Drawing.Point(13, 225);
            this.lbstate.Name = "lbstate";
            this.lbstate.Size = new System.Drawing.Size(81, 13);
            this.lbstate.TabIndex = 11;
            this.lbstate.Text = "State/pro&vince:";
            // 
            // txtCity
            // 
            this.txtCity.Location = new System.Drawing.Point(120, 187);
            this.txtCity.MaxLength = 128;
            this.txtCity.Name = "txtCity";
            this.txtCity.Size = new System.Drawing.Size(254, 20);
            this.txtCity.TabIndex = 13;
            this.txtCity.TextChanged += new System.EventHandler(this.txtCity_TextChanged);
            // 
            // txtstate
            // 
            this.txtstate.Location = new System.Drawing.Point(120, 221);
            this.txtstate.MaxLength = 128;
            this.txtstate.Name = "txtstate";
            this.txtstate.Size = new System.Drawing.Size(254, 20);
            this.txtstate.TabIndex = 14;
            this.txtstate.TextChanged += new System.EventHandler(this.txtstate_TextChanged);
            // 
            // lbzip
            // 
            this.lbzip.AutoSize = true;
            this.lbzip.Location = new System.Drawing.Point(13, 262);
            this.lbzip.Name = "lbzip";
            this.lbzip.Size = new System.Drawing.Size(87, 13);
            this.lbzip.TabIndex = 15;
            this.lbzip.Text = "&Zip/Postal Code:";
            // 
            // lbCountry
            // 
            this.lbCountry.AutoSize = true;
            this.lbCountry.Location = new System.Drawing.Point(13, 299);
            this.lbCountry.Name = "lbCountry";
            this.lbCountry.Size = new System.Drawing.Size(80, 13);
            this.lbCountry.TabIndex = 16;
            this.lbCountry.Text = "C&ountry/region:";
            // 
            // txtZip
            // 
            this.txtZip.Location = new System.Drawing.Point(120, 258);
            this.txtZip.MaxLength = 40;
            this.txtZip.Name = "txtZip";
            this.txtZip.Size = new System.Drawing.Size(254, 20);
            this.txtZip.TabIndex = 18;
            this.txtZip.TextChanged += new System.EventHandler(this.txtZip_TextChanged);
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
            this.pictureBox1.Location = new System.Drawing.Point(13, 18);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(33, 32);
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
            // userNamelabel
            // 
            this.userNamelabel.AutoSize = true;
            this.userNamelabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 10.25F);
            this.userNamelabel.Location = new System.Drawing.Point(67, 26);
            this.userNamelabel.Name = "userNamelabel";
            this.userNamelabel.Size = new System.Drawing.Size(128, 17);
            this.userNamelabel.TabIndex = 24;
            this.userNamelabel.Text = "                              ";
            // 
            // lbDescription
            // 
            this.lbDescription.AutoSize = true;
            this.lbDescription.Location = new System.Drawing.Point(13, 83);
            this.lbDescription.Name = "lbDescription";
            this.lbDescription.Size = new System.Drawing.Size(63, 13);
            this.lbDescription.TabIndex = 2;
            this.lbDescription.Text = "&Description:";
            // 
            // rtbStreet
            // 
            this.rtbStreet.AcceptsTab = true;
            this.rtbStreet.Location = new System.Drawing.Point(121, 109);
            this.rtbStreet.MaxLength = 1024;
            this.rtbStreet.Name = "rtbStreet";
            this.rtbStreet.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.ForcedVertical;
            this.rtbStreet.Size = new System.Drawing.Size(252, 67);
            this.rtbStreet.TabIndex = 25;
            this.rtbStreet.Text = "";
            this.rtbStreet.TextChanged += new System.EventHandler(this.rtbStreet_TextChanged);
            // 
            // cbcountry
            // 
            this.cbcountry.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbcountry.FormattingEnabled = true;
            this.cbcountry.Location = new System.Drawing.Point(120, 295);
            this.cbcountry.Name = "cbcountry";
            this.cbcountry.Size = new System.Drawing.Size(253, 21);
            this.cbcountry.Sorted = true;
            this.cbcountry.TabIndex = 26;
            this.cbcountry.SelectedIndexChanged += new System.EventHandler(this.cbcountry_SelectedIndexChanged);
            // 
            // OUGeneralEditPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.Name = "OUGeneralEditPage";
            this.Size = new System.Drawing.Size(386, 411);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TextBox txtDescription;
        private System.Windows.Forms.Label lbstate;
        private System.Windows.Forms.Label lbCity;
        private System.Windows.Forms.Label lbStreet;
        private System.Windows.Forms.TextBox txtZip;
        private System.Windows.Forms.Label lbCountry;
        private System.Windows.Forms.Label lbzip;
        private System.Windows.Forms.TextBox txtstate;
        private System.Windows.Forms.TextBox txtCity;
        private System.Windows.Forms.ImageList UserGeneralimageList;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label userNamelabel;
        private System.Windows.Forms.Label lbDescription;
        private System.Windows.Forms.RichTextBox rtbStreet;
        private System.Windows.Forms.ComboBox cbcountry;

    }
}