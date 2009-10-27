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

using Likewise.LMC.LMConsoleUtils;

namespace Likewise.LMC
{
    partial class ObjectsListPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ObjectsListPage));
            this.tabControl = new System.Windows.Forms.TabControl();
            this.tabPageDomainOUs = new System.Windows.Forms.TabPage();
            this.btnBack = new System.Windows.Forms.Button();
            this.btnCreateLinkGPO = new System.Windows.Forms.Button();
            this.cbDomainOUs = new System.Windows.Forms.ComboBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.lvDomainOUs = new Likewise.LMC.ServerControl.LWListView();
            this.columnHeader1 = new System.Windows.Forms.ColumnHeader();
            this.columnHeader2 = new System.Windows.Forms.ColumnHeader();
            this.ObjectSmallImageList = new System.Windows.Forms.ImageList(this.components);
            this.tabPageAll = new System.Windows.Forms.TabPage();
            this.btnCreateGPO = new System.Windows.Forms.Button();
            this.cbDomainAll = new System.Windows.Forms.ComboBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.lvGPOs = new Likewise.LMC.ServerControl.LWListView();
            this.colName = new System.Windows.Forms.ColumnHeader();
            this.btnOk = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.tabControl.SuspendLayout();
            this.tabPageDomainOUs.SuspendLayout();
            this.tabPageAll.SuspendLayout();
            this.SuspendLayout();
            // 
            // tabControl
            // 
            this.tabControl.Controls.Add(this.tabPageDomainOUs);
            this.tabControl.Controls.Add(this.tabPageAll);
            this.tabControl.Location = new System.Drawing.Point(1, 2);
            this.tabControl.Name = "tabControl";
            this.tabControl.SelectedIndex = 0;
            this.tabControl.Size = new System.Drawing.Size(469, 280);
            this.tabControl.TabIndex = 0;
            this.tabControl.SelectedIndexChanged += new System.EventHandler(this.tabControl_SelectedIndexChanged);
            // 
            // tabPageDomainOUs
            // 
            this.tabPageDomainOUs.Controls.Add(this.btnBack);
            this.tabPageDomainOUs.Controls.Add(this.btnCreateLinkGPO);
            this.tabPageDomainOUs.Controls.Add(this.cbDomainOUs);
            this.tabPageDomainOUs.Controls.Add(this.label3);
            this.tabPageDomainOUs.Controls.Add(this.label4);
            this.tabPageDomainOUs.Controls.Add(this.lvDomainOUs);
            this.tabPageDomainOUs.Location = new System.Drawing.Point(4, 22);
            this.tabPageDomainOUs.Name = "tabPageDomainOUs";
            this.tabPageDomainOUs.Padding = new System.Windows.Forms.Padding(3);
            this.tabPageDomainOUs.Size = new System.Drawing.Size(461, 254);
            this.tabPageDomainOUs.TabIndex = 0;
            this.tabPageDomainOUs.Text = "Domain/OUs";
            this.tabPageDomainOUs.UseVisualStyleBackColor = true;
            // 
            // btnBack
            // 
            this.btnBack.Image = global::Likewise.LMC.Properties.Resources.back;
            this.btnBack.Location = new System.Drawing.Point(355, 8);
            this.btnBack.Name = "btnBack";
            this.btnBack.Size = new System.Drawing.Size(26, 22);
            this.btnBack.TabIndex = 10;
            this.btnBack.UseVisualStyleBackColor = true;
            this.btnBack.Click += new System.EventHandler(this.btnBack_Click);
            // 
            // btnCreateLinkGPO
            // 
            this.btnCreateLinkGPO.AccessibleRole = System.Windows.Forms.AccessibleRole.ToolTip;
            this.btnCreateLinkGPO.Location = new System.Drawing.Point(385, 8);
            this.btnCreateLinkGPO.Name = "btnCreateLinkGPO";
            this.btnCreateLinkGPO.Size = new System.Drawing.Size(72, 22);
            this.btnCreateLinkGPO.TabIndex = 9;
            this.btnCreateLinkGPO.Text = "Create GPO";
            this.btnCreateLinkGPO.UseVisualStyleBackColor = true;
            this.btnCreateLinkGPO.Click += new System.EventHandler(this.btnCreateLinkGPO_Click);
            // 
            // cbDomainOUs
            // 
            this.cbDomainOUs.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbDomainOUs.FormattingEnabled = true;
            this.cbDomainOUs.Location = new System.Drawing.Point(57, 8);
            this.cbDomainOUs.Name = "cbDomainOUs";
            this.cbDomainOUs.Size = new System.Drawing.Size(294, 21);
            this.cbDomainOUs.TabIndex = 8;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(6, 36);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(232, 13);
            this.label3.TabIndex = 7;
            this.label3.Text = "Domains, OUs and linked Group Policy Objects:";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(6, 11);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(45, 13);
            this.label4.TabIndex = 6;
            this.label4.Text = "Look &in:";
            // 
            // lvDomainOUs
            // 
            this.lvDomainOUs.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1,
            this.columnHeader2});
            this.lvDomainOUs.FullRowSelect = true;
            this.lvDomainOUs.Location = new System.Drawing.Point(6, 54);
            this.lvDomainOUs.MultiSelect = false;
            this.lvDomainOUs.Name = "lvDomainOUs";
            this.lvDomainOUs.Size = new System.Drawing.Size(448, 193);
            this.lvDomainOUs.SmallImageList = this.ObjectSmallImageList;
            this.lvDomainOUs.TabIndex = 5;
            this.lvDomainOUs.UseCompatibleStateImageBehavior = false;
            this.lvDomainOUs.View = System.Windows.Forms.View.Details;
            this.lvDomainOUs.AfterLabelEdit += new System.Windows.Forms.LabelEditEventHandler(this.lvDomainOUs_AfterLabelEdit);
            this.lvDomainOUs.SelectedIndexChanged += new System.EventHandler(this.lvDomainOUs_SelectedIndexChanged);
            this.lvDomainOUs.DoubleClick += new System.EventHandler(this.lvDomainOUs_DoubleClick);
            this.lvDomainOUs.MouseUp += new System.Windows.Forms.MouseEventHandler(this.lvDomainOUs_MouseUp);
            // 
            // columnHeader1
            // 
            this.columnHeader1.Text = "Name";
            this.columnHeader1.Width = 200;
            // 
            // columnHeader2
            // 
            this.columnHeader2.Text = "Domain";
            this.columnHeader2.Width = 200;
            // 
            // ObjectSmallImageList
            // 
            this.ObjectSmallImageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("ObjectSmallImageList.ImageStream")));
            this.ObjectSmallImageList.TransparentColor = System.Drawing.Color.Transparent;
            this.ObjectSmallImageList.Images.SetKeyName(0, "computer.ico");
            this.ObjectSmallImageList.Images.SetKeyName(1, "Group_16.ico");
            this.ObjectSmallImageList.Images.SetKeyName(2, "User.ico");
            this.ObjectSmallImageList.Images.SetKeyName(3, "Folder.ico");
            this.ObjectSmallImageList.Images.SetKeyName(4, "OrganizationalUnit.ico");
            this.ObjectSmallImageList.Images.SetKeyName(5, "GPMC.ico");
            this.ObjectSmallImageList.Images.SetKeyName(6, "ADUC.ico");
            this.ObjectSmallImageList.Images.SetKeyName(7, "DisabledUser.ico");
            this.ObjectSmallImageList.Images.SetKeyName(8, "DisabledComputer.ico");
            this.ObjectSmallImageList.Images.SetKeyName(9, "FolderOpen.ico");
            // 
            // tabPageAll
            // 
            this.tabPageAll.Controls.Add(this.btnCreateGPO);
            this.tabPageAll.Controls.Add(this.cbDomainAll);
            this.tabPageAll.Controls.Add(this.label2);
            this.tabPageAll.Controls.Add(this.label1);
            this.tabPageAll.Controls.Add(this.lvGPOs);
            this.tabPageAll.Location = new System.Drawing.Point(4, 22);
            this.tabPageAll.Name = "tabPageAll";
            this.tabPageAll.Padding = new System.Windows.Forms.Padding(3);
            this.tabPageAll.Size = new System.Drawing.Size(461, 254);
            this.tabPageAll.TabIndex = 1;
            this.tabPageAll.Text = "All";
            this.tabPageAll.UseVisualStyleBackColor = true;
            // 
            // btnCreateGPO
            // 
            this.btnCreateGPO.AccessibleRole = System.Windows.Forms.AccessibleRole.ToolTip;
            this.btnCreateGPO.Location = new System.Drawing.Point(361, 9);
            this.btnCreateGPO.Name = "btnCreateGPO";
            this.btnCreateGPO.Size = new System.Drawing.Size(83, 22);
            this.btnCreateGPO.TabIndex = 5;
            this.btnCreateGPO.Text = "Create GPO";
            this.btnCreateGPO.UseVisualStyleBackColor = true;
            this.btnCreateGPO.Click += new System.EventHandler(this.btnCreateGPO_Click);
            // 
            // cbDomainAll
            // 
            this.cbDomainAll.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbDomainAll.FormattingEnabled = true;
            this.cbDomainAll.Location = new System.Drawing.Point(58, 9);
            this.cbDomainAll.Name = "cbDomainAll";
            this.cbDomainAll.Size = new System.Drawing.Size(295, 21);
            this.cbDomainAll.TabIndex = 4;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(7, 37);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(222, 13);
            this.label2.TabIndex = 3;
            this.label2.Text = "All Group Policy Objects stored in this domain:";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(7, 12);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(45, 13);
            this.label1.TabIndex = 2;
            this.label1.Text = "Look &in:";
            // 
            // lvGPOs
            // 
            this.lvGPOs.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colName});
            this.lvGPOs.FullRowSelect = true;
            this.lvGPOs.Location = new System.Drawing.Point(7, 55);
            this.lvGPOs.MultiSelect = false;
            this.lvGPOs.Name = "lvGPOs";
            this.lvGPOs.Size = new System.Drawing.Size(448, 193);
            this.lvGPOs.SmallImageList = this.ObjectSmallImageList;
            this.lvGPOs.TabIndex = 1;
            this.lvGPOs.UseCompatibleStateImageBehavior = false;
            this.lvGPOs.View = System.Windows.Forms.View.Details;
            this.lvGPOs.AfterLabelEdit += new System.Windows.Forms.LabelEditEventHandler(this.lvGPOs_AfterLabelEdit);
            this.lvGPOs.SelectedIndexChanged += new System.EventHandler(this.lvGPOs_SelectedIndexChanged);
            this.lvGPOs.DoubleClick += new System.EventHandler(this.lvGPOs_DoubleClick);
            this.lvGPOs.MouseUp += new System.Windows.Forms.MouseEventHandler(this.lvGPOs_MouseUp);
            // 
            // colName
            // 
            this.colName.Text = "Name";
            this.colName.Width = 425;
            // 
            // btnOk
            // 
            this.btnOk.Enabled = false;
            this.btnOk.Location = new System.Drawing.Point(308, 291);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(75, 23);
            this.btnOk.TabIndex = 1;
            this.btnOk.Text = "&OK";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.Location = new System.Drawing.Point(395, 291);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 3;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // ObjectsListPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(472, 322);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.tabControl);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ObjectsListPage";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Browse for a Group Policy Object";
            this.tabControl.ResumeLayout(false);
            this.tabPageDomainOUs.ResumeLayout(false);
            this.tabPageDomainOUs.PerformLayout();
            this.tabPageAll.ResumeLayout(false);
            this.tabPageAll.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabControl tabControl;
        private System.Windows.Forms.TabPage tabPageDomainOUs;
        private System.Windows.Forms.TabPage tabPageAll;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Label label1;
        private Likewise.LMC.ServerControl.LWListView lvGPOs;
        private System.Windows.Forms.ComboBox cbDomainAll;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ColumnHeader colName;
        private System.Windows.Forms.ComboBox cbDomainOUs;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private Likewise.LMC.ServerControl.LWListView lvDomainOUs;
        private System.Windows.Forms.ColumnHeader columnHeader1;
        private System.Windows.Forms.ColumnHeader columnHeader2;
        private System.Windows.Forms.Button btnCreateLinkGPO;
        private System.Windows.Forms.Button btnCreateGPO;
        private System.Windows.Forms.Button btnBack;
        private System.Windows.Forms.ImageList ObjectSmallImageList;
    }
}