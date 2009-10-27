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
    partial class MultiItemsGeneralEditPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MultiItemsGeneralEditPage));
            this.FNamelabel = new System.Windows.Forms.Label();
            this.LNameLabel = new System.Windows.Forms.Label();
            this.Initiallabel = new System.Windows.Forms.Label();
            this.Displaynamelabel = new System.Windows.Forms.Label();
            this.descriplabel = new System.Windows.Forms.Label();
            this.Officelabel = new System.Windows.Forms.Label();
            this.telelabel = new System.Windows.Forms.Label();
            this.UserGeneralimageList = new System.Windows.Forms.ImageList(this.components);
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.lblUserName = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.lblComputers = new System.Windows.Forms.Label();
            this.lblGroups = new System.Windows.Forms.Label();
            this.lblUsers = new System.Windows.Forms.Label();
            this.lblOUs = new System.Windows.Forms.Label();
            this.lblContacts = new System.Windows.Forms.Label();
            this.lblOthers = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.lblSummary = new System.Windows.Forms.Label();
            this.checkBox = new System.Windows.Forms.CheckBox();
            this.txtDescription = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.pnlData.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.label2);
            this.pnlData.Controls.Add(this.txtDescription);
            this.pnlData.Controls.Add(this.checkBox);
            this.pnlData.Controls.Add(this.lblSummary);
            this.pnlData.Controls.Add(this.label3);
            this.pnlData.Controls.Add(this.lblOthers);
            this.pnlData.Controls.Add(this.lblContacts);
            this.pnlData.Controls.Add(this.lblComputers);
            this.pnlData.Controls.Add(this.lblGroups);
            this.pnlData.Controls.Add(this.lblUsers);
            this.pnlData.Controls.Add(this.lblOUs);
            this.pnlData.Controls.Add(this.label1);
            this.pnlData.Controls.Add(this.lblUserName);
            this.pnlData.Controls.Add(this.groupBox2);
            this.pnlData.Controls.Add(this.groupBox1);
            this.pnlData.Controls.Add(this.pictureBox1);
            this.pnlData.Controls.Add(this.telelabel);
            this.pnlData.Controls.Add(this.Officelabel);
            this.pnlData.Controls.Add(this.descriplabel);
            this.pnlData.Controls.Add(this.Displaynamelabel);
            this.pnlData.Controls.Add(this.Initiallabel);
            this.pnlData.Controls.Add(this.LNameLabel);
            this.pnlData.Controls.Add(this.FNamelabel);
            this.pnlData.Size = new System.Drawing.Size(348, 385);
            // 
            // FNamelabel
            // 
            this.FNamelabel.Location = new System.Drawing.Point(19, 84);
            this.FNamelabel.Name = "FNamelabel";
            this.FNamelabel.Size = new System.Drawing.Size(94, 19);
            this.FNamelabel.TabIndex = 0;
            this.FNamelabel.Text = "Object type";
            // 
            // LNameLabel
            // 
            this.LNameLabel.Location = new System.Drawing.Point(19, 118);
            this.LNameLabel.Name = "LNameLabel";
            this.LNameLabel.Size = new System.Drawing.Size(107, 16);
            this.LNameLabel.TabIndex = 2;
            this.LNameLabel.Text = "Organization units:";
            // 
            // Initiallabel
            // 
            this.Initiallabel.Location = new System.Drawing.Point(154, 84);
            this.Initiallabel.Name = "Initiallabel";
            this.Initiallabel.Size = new System.Drawing.Size(137, 17);
            this.Initiallabel.TabIndex = 4;
            this.Initiallabel.Text = "Number Selected:";
            // 
            // Displaynamelabel
            // 
            this.Displaynamelabel.Location = new System.Drawing.Point(19, 136);
            this.Displaynamelabel.Name = "Displaynamelabel";
            this.Displaynamelabel.Size = new System.Drawing.Size(87, 15);
            this.Displaynamelabel.TabIndex = 9;
            this.Displaynamelabel.Text = "Users:";
            // 
            // descriplabel
            // 
            this.descriplabel.Location = new System.Drawing.Point(18, 153);
            this.descriplabel.Name = "descriplabel";
            this.descriplabel.Size = new System.Drawing.Size(88, 16);
            this.descriplabel.TabIndex = 10;
            this.descriplabel.Text = "Groups:";
            // 
            // Officelabel
            // 
            this.Officelabel.Location = new System.Drawing.Point(18, 171);
            this.Officelabel.Name = "Officelabel";
            this.Officelabel.Size = new System.Drawing.Size(88, 16);
            this.Officelabel.TabIndex = 11;
            this.Officelabel.Text = "Computers:";
            // 
            // telelabel
            // 
            this.telelabel.Location = new System.Drawing.Point(19, 232);
            this.telelabel.Name = "telelabel";
            this.telelabel.Size = new System.Drawing.Size(87, 16);
            this.telelabel.TabIndex = 15;
            this.telelabel.Text = "Summary";
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
            this.pictureBox1.Size = new System.Drawing.Size(40, 35);
            this.pictureBox1.TabIndex = 21;
            this.pictureBox1.TabStop = false;
            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(3, 64);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(325, 2);
            this.groupBox1.TabIndex = 22;
            this.groupBox1.TabStop = false;
            // 
            // groupBox2
            // 
            this.groupBox2.Location = new System.Drawing.Point(4, 227);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(325, 2);
            this.groupBox2.TabIndex = 23;
            this.groupBox2.TabStop = false;
            // 
            // lblUserName
            // 
            this.lblUserName.Location = new System.Drawing.Point(67, 34);
            this.lblUserName.Name = "lblUserName";
            this.lblUserName.Size = new System.Drawing.Size(262, 19);
            this.lblUserName.TabIndex = 24;
            this.lblUserName.Text = "The  following objects types are selected.";
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(19, 190);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(87, 15);
            this.label1.TabIndex = 27;
            this.label1.Text = "Contacts:";
            // 
            // lblComputers
            // 
            this.lblComputers.AutoSize = true;
            this.lblComputers.Location = new System.Drawing.Point(153, 171);
            this.lblComputers.Name = "lblComputers";
            this.lblComputers.Size = new System.Drawing.Size(0, 13);
            this.lblComputers.TabIndex = 31;
            // 
            // lblGroups
            // 
            this.lblGroups.AutoSize = true;
            this.lblGroups.Location = new System.Drawing.Point(153, 153);
            this.lblGroups.Name = "lblGroups";
            this.lblGroups.Size = new System.Drawing.Size(0, 13);
            this.lblGroups.TabIndex = 30;
            // 
            // lblUsers
            // 
            this.lblUsers.AutoSize = true;
            this.lblUsers.Location = new System.Drawing.Point(154, 135);
            this.lblUsers.Name = "lblUsers";
            this.lblUsers.Size = new System.Drawing.Size(0, 13);
            this.lblUsers.TabIndex = 29;
            // 
            // lblOUs
            // 
            this.lblOUs.AutoSize = true;
            this.lblOUs.Location = new System.Drawing.Point(154, 118);
            this.lblOUs.Name = "lblOUs";
            this.lblOUs.Size = new System.Drawing.Size(0, 13);
            this.lblOUs.TabIndex = 28;
            // 
            // lblContacts
            // 
            this.lblContacts.AutoSize = true;
            this.lblContacts.Location = new System.Drawing.Point(153, 189);
            this.lblContacts.Name = "lblContacts";
            this.lblContacts.Size = new System.Drawing.Size(0, 13);
            this.lblContacts.TabIndex = 32;
            // 
            // lblOthers
            // 
            this.lblOthers.AutoSize = true;
            this.lblOthers.Location = new System.Drawing.Point(153, 208);
            this.lblOthers.Name = "lblOthers";
            this.lblOthers.Size = new System.Drawing.Size(0, 13);
            this.lblOthers.TabIndex = 33;
            // 
            // label3
            // 
            this.label3.Location = new System.Drawing.Point(18, 208);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(78, 16);
            this.label3.TabIndex = 34;
            this.label3.Text = "Others:";
            // 
            // lblSummary
            // 
            this.lblSummary.Location = new System.Drawing.Point(153, 232);
            this.lblSummary.Name = "lblSummary";
            this.lblSummary.Size = new System.Drawing.Size(50, 13);
            this.lblSummary.TabIndex = 35;
            this.lblSummary.Text = "Summary";
            // 
            // checkBox
            // 
            this.checkBox.Location = new System.Drawing.Point(16, 268);
            this.checkBox.Name = "checkBox";
            this.checkBox.Size = new System.Drawing.Size(312, 23);
            this.checkBox.TabIndex = 36;
            this.checkBox.Text = "Change the description text for all the selected objects.";
            this.checkBox.UseVisualStyleBackColor = true;
            this.checkBox.CheckedChanged += new System.EventHandler(this.checkBox_CheckedChanged);
            // 
            // txtDescription
            // 
            this.txtDescription.Enabled = false;
            this.txtDescription.Location = new System.Drawing.Point(32, 318);
            this.txtDescription.MaxLength = 1024;
            this.txtDescription.Name = "txtDescription";
            this.txtDescription.Size = new System.Drawing.Size(259, 20);
            this.txtDescription.TabIndex = 37;
            // 
            // label2
            // 
            this.label2.Enabled = false;
            this.label2.Location = new System.Drawing.Point(30, 300);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(76, 14);
            this.label2.TabIndex = 38;
            this.label2.Text = "&Description:";
            // 
            // MultiItemsGeneralEditPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.Name = "MultiItemsGeneralEditPage";
            this.Size = new System.Drawing.Size(348, 385);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label FNamelabel;
        private System.Windows.Forms.Label Initiallabel;
        private System.Windows.Forms.Label LNameLabel;
        private System.Windows.Forms.Label Officelabel;
        private System.Windows.Forms.Label descriplabel;
        private System.Windows.Forms.Label Displaynamelabel;
        private System.Windows.Forms.Label telelabel;
        private System.Windows.Forms.ImageList UserGeneralimageList;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Label lblUserName;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label lblComputers;
        private System.Windows.Forms.Label lblGroups;
        private System.Windows.Forms.Label lblUsers;
        private System.Windows.Forms.Label lblOUs;
        private System.Windows.Forms.Label lblContacts;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label lblOthers;
        private System.Windows.Forms.Label lblSummary;
        private System.Windows.Forms.CheckBox checkBox;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox txtDescription;

    }
}