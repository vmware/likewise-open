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

using Likewise.LMC.Utilities;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    partial class ComputerMemOfPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ComputerMemOfPage));
            this.MemOflabel = new System.Windows.Forms.Label();
            this.MemoflistView = new Likewise.LMC.ServerControl.LWListView();
            this.Namecolumn = new System.Windows.Forms.ColumnHeader();
            this.ACFolerColumn = new System.Windows.Forms.ColumnHeader();
            this.LargeImageList = new System.Windows.Forms.ImageList(this.components);
            this.smallImageList = new System.Windows.Forms.ImageList(this.components);
            this.Addbutton = new System.Windows.Forms.Button();
            this.RemoveButton = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.PrimaryGrouplabel = new System.Windows.Forms.Label();
            this.DomainUserlabel = new System.Windows.Forms.Label();
            this.SetPrimaryGroupbutton = new System.Windows.Forms.Button();
            this.setPrimaryGrouplabel = new System.Windows.Forms.Label();
            this.pnlData.SuspendLayout();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.setPrimaryGrouplabel);
            this.pnlData.Controls.Add(this.SetPrimaryGroupbutton);
            this.pnlData.Controls.Add(this.DomainUserlabel);
            this.pnlData.Controls.Add(this.PrimaryGrouplabel);
            this.pnlData.Controls.Add(this.groupBox1);
            this.pnlData.Controls.Add(this.RemoveButton);
            this.pnlData.Controls.Add(this.Addbutton);
            this.pnlData.Controls.Add(this.MemoflistView);
            this.pnlData.Controls.Add(this.MemOflabel);
            this.pnlData.Size = new System.Drawing.Size(386, 411);
            // 
            // MemOflabel
            // 
            this.MemOflabel.AutoSize = true;
            this.MemOflabel.Location = new System.Drawing.Point(18, 18);
            this.MemOflabel.Name = "MemOflabel";
            this.MemOflabel.Size = new System.Drawing.Size(60, 13);
            this.MemOflabel.TabIndex = 0;
            this.MemOflabel.Text = "Member of:";
            // 
            // MemoflistView
            // 
            this.MemoflistView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.Namecolumn,
            this.ACFolerColumn});
            this.MemoflistView.FullRowSelect = true;
            this.MemoflistView.HideSelection = false;
            this.MemoflistView.LargeImageList = this.LargeImageList;
            this.MemoflistView.Location = new System.Drawing.Point(16, 45);
            this.MemoflistView.Name = "MemoflistView";
            this.MemoflistView.ShowGroups = false;
            this.MemoflistView.Size = new System.Drawing.Size(349, 168);
            this.MemoflistView.SmallImageList = this.smallImageList;
            this.MemoflistView.TabIndex = 1;
            this.MemoflistView.UseCompatibleStateImageBehavior = false;
            this.MemoflistView.View = System.Windows.Forms.View.Details;
            this.MemoflistView.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.MemoflistView_MouseDoubleClick);
            this.MemoflistView.SelectedIndexChanged += new System.EventHandler(this.MemoflistView_SelectedIndexChanged);
            this.MemoflistView.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.MemoflistView_ColumnClick);
            // 
            // Namecolumn
            // 
            this.Namecolumn.Text = "Name";
            this.Namecolumn.Width = 158;
            // 
            // ACFolerColumn
            // 
            this.ACFolerColumn.Text = "Active Directory Folder";
            this.ACFolerColumn.Width = 274;
            // 
            // LargeImageList
            // 
            this.LargeImageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("LargeImageList.ImageStream")));
            this.LargeImageList.TransparentColor = System.Drawing.Color.Transparent;
            this.LargeImageList.Images.SetKeyName(0, "computer_48.bmp");
            this.LargeImageList.Images.SetKeyName(1, "Group_48.bmp");
            this.LargeImageList.Images.SetKeyName(2, "user_48.bmp");
            this.LargeImageList.Images.SetKeyName(3, "Folder.ico");
            this.LargeImageList.Images.SetKeyName(4, "aduc_48.bmp");
            this.LargeImageList.Images.SetKeyName(5, "Admin.ico");
            this.LargeImageList.Images.SetKeyName(6, "ADUC.ico");
            this.LargeImageList.Images.SetKeyName(7, "DisabledUser.ico");
            this.LargeImageList.Images.SetKeyName(8, "DisabledComp.ico");
            // 
            // smallImageList
            // 
            this.smallImageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("smallImageList.ImageStream")));
            this.smallImageList.TransparentColor = System.Drawing.Color.Transparent;
            this.smallImageList.Images.SetKeyName(0, "computer_16.bmp");
            this.smallImageList.Images.SetKeyName(1, "group_16.bmp");
            this.smallImageList.Images.SetKeyName(2, "user_16.bmp");
            this.smallImageList.Images.SetKeyName(3, "Folder.ico");
            this.smallImageList.Images.SetKeyName(4, "aduc_16.bmp");
            this.smallImageList.Images.SetKeyName(5, "Admin.ico");
            this.smallImageList.Images.SetKeyName(6, "ADUC.ico");
            this.smallImageList.Images.SetKeyName(7, "DisabledUser.ico");
            this.smallImageList.Images.SetKeyName(8, "DisabledComp.ico");
            // 
            // Addbutton
            // 
            this.Addbutton.Location = new System.Drawing.Point(21, 231);
            this.Addbutton.Name = "Addbutton";
            this.Addbutton.Size = new System.Drawing.Size(75, 23);
            this.Addbutton.TabIndex = 2;
            this.Addbutton.Text = "Add...";
            this.Addbutton.UseVisualStyleBackColor = true;
            this.Addbutton.Click += new System.EventHandler(this.Addbutton_Click);
            // 
            // RemoveButton
            // 
            this.RemoveButton.Enabled = false;
            this.RemoveButton.Location = new System.Drawing.Point(110, 231);
            this.RemoveButton.Name = "RemoveButton";
            this.RemoveButton.Size = new System.Drawing.Size(75, 23);
            this.RemoveButton.TabIndex = 3;
            this.RemoveButton.Text = "Remove";
            this.RemoveButton.UseVisualStyleBackColor = true;
            this.RemoveButton.Click += new System.EventHandler(this.RemoveButton_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(24, 271);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(339, 2);
            this.groupBox1.TabIndex = 4;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "groupBox1";
            // 
            // PrimaryGrouplabel
            // 
            this.PrimaryGrouplabel.AutoSize = true;
            this.PrimaryGrouplabel.Location = new System.Drawing.Point(24, 291);
            this.PrimaryGrouplabel.Name = "PrimaryGrouplabel";
            this.PrimaryGrouplabel.Size = new System.Drawing.Size(74, 13);
            this.PrimaryGrouplabel.TabIndex = 5;
            this.PrimaryGrouplabel.Text = "Primary group:";
            // 
            // DomainUserlabel
            // 
            this.DomainUserlabel.Location = new System.Drawing.Point(123, 291);
            this.DomainUserlabel.Name = "DomainUserlabel";
            this.DomainUserlabel.Size = new System.Drawing.Size(240, 13);
            this.DomainUserlabel.TabIndex = 6;
            this.DomainUserlabel.Text = "Domain Computers";
            // 
            // SetPrimaryGroupbutton
            // 
            this.SetPrimaryGroupbutton.Enabled = false;
            this.SetPrimaryGroupbutton.Location = new System.Drawing.Point(21, 324);
            this.SetPrimaryGroupbutton.Name = "SetPrimaryGroupbutton";
            this.SetPrimaryGroupbutton.Size = new System.Drawing.Size(123, 23);
            this.SetPrimaryGroupbutton.TabIndex = 7;
            this.SetPrimaryGroupbutton.Text = "Set Primary Group";
            this.SetPrimaryGroupbutton.UseVisualStyleBackColor = true;
            this.SetPrimaryGroupbutton.Click += new System.EventHandler(this.SetPrimaryGroupbutton_Click);
            // 
            // setPrimaryGrouplabel
            // 
            this.setPrimaryGrouplabel.Location = new System.Drawing.Point(156, 324);
            this.setPrimaryGrouplabel.Name = "setPrimaryGrouplabel";
            this.setPrimaryGrouplabel.Size = new System.Drawing.Size(207, 48);
            this.setPrimaryGrouplabel.TabIndex = 8;
            this.setPrimaryGrouplabel.Text = "There is no need to change Primary group unless you have Macintosh clients or POS" +
                "IX-compliant applications.";
            // 
            // ComputerMemOfPage
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Name = "ComputerMemOfPage";
            this.Size = new System.Drawing.Size(386, 411);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label MemOflabel;
        private Likewise.LMC.ServerControl.LWListView MemoflistView;
        private System.Windows.Forms.ColumnHeader Namecolumn;
        private System.Windows.Forms.ColumnHeader ACFolerColumn;
        private System.Windows.Forms.Button RemoveButton;
        private System.Windows.Forms.Button Addbutton;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Button SetPrimaryGroupbutton;
        private System.Windows.Forms.Label DomainUserlabel;
        private System.Windows.Forms.Label PrimaryGrouplabel;
        private System.Windows.Forms.Label setPrimaryGrouplabel;
        private System.Windows.Forms.ImageList smallImageList;
        private System.Windows.Forms.ImageList LargeImageList;
    }
}