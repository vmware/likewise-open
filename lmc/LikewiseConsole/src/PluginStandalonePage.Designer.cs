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

namespace Likewise.LMC
{
    partial class PluginStandalonePage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(PluginStandalonePage));
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.PluginComboBox = new System.Windows.Forms.ComboBox();
            this.ChosenPluginlistView = new Likewise.LMC.ServerControl.LWListView();
            this.chPlugin = new System.Windows.Forms.ColumnHeader();
            this.imageList = new System.Windows.Forms.ImageList(this.components);
            this.DescipGroupbox = new System.Windows.Forms.GroupBox();
            this.lblDescription = new System.Windows.Forms.Label();
            this.Addbtn = new System.Windows.Forms.Button();
            this.Removebtn = new System.Windows.Forms.Button();
            this.Aboutbtn = new System.Windows.Forms.Button();
            this.pnlData.SuspendLayout();
            this.DescipGroupbox.SuspendLayout();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.Aboutbtn);
            this.pnlData.Controls.Add(this.Removebtn);
            this.pnlData.Controls.Add(this.Addbtn);
            this.pnlData.Controls.Add(this.DescipGroupbox);
            this.pnlData.Controls.Add(this.ChosenPluginlistView);
            this.pnlData.Controls.Add(this.PluginComboBox);
            this.pnlData.Controls.Add(this.label2);
            this.pnlData.Controls.Add(this.label1);
            this.pnlData.Size = new System.Drawing.Size(364, 472);
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(9, 16);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(340, 32);
            this.label1.TabIndex = 0;
            this.label1.Text = "Use this page to add or remove a stand-alone plug-in from the console.";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(9, 76);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(95, 13);
            this.label2.TabIndex = 1;
            this.label2.Text = "Plug-ins added to: ";
            // 
            // PluginComboBox
            // 
            this.PluginComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.PluginComboBox.FormattingEnabled = true;
            this.PluginComboBox.Location = new System.Drawing.Point(124, 73);
            this.PluginComboBox.Name = "PluginComboBox";
            this.PluginComboBox.Size = new System.Drawing.Size(228, 21);
            this.PluginComboBox.TabIndex = 2;
            this.PluginComboBox.SelectedIndexChanged += new System.EventHandler(this.pluginComboBox_selectedIndexChanged);
            // 
            // ChosenPluginlistView
            // 
            this.ChosenPluginlistView.Alignment = System.Windows.Forms.ListViewAlignment.SnapToGrid;
            this.ChosenPluginlistView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.chPlugin});
            this.ChosenPluginlistView.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.None;
            this.ChosenPluginlistView.HideSelection = false;
            this.ChosenPluginlistView.Location = new System.Drawing.Point(12, 100);
            this.ChosenPluginlistView.MultiSelect = false;
            this.ChosenPluginlistView.Name = "ChosenPluginlistView";
            this.ChosenPluginlistView.Size = new System.Drawing.Size(340, 210);
            this.ChosenPluginlistView.SmallImageList = this.imageList;
            this.ChosenPluginlistView.TabIndex = 4;
            this.ChosenPluginlistView.UseCompatibleStateImageBehavior = false;
            this.ChosenPluginlistView.View = System.Windows.Forms.View.Details;
            this.ChosenPluginlistView.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.ChosenPluginlistView_MouseDoubleClick);
            this.ChosenPluginlistView.SelectedIndexChanged += new System.EventHandler(this.ChosenPluginlistView_SelectedIndexChanged);
            // 
            // chPlugin
            // 
            this.chPlugin.Text = "Plugin";
            this.chPlugin.Width = 330;
            // 
            // imageList
            // 
            this.imageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList.ImageStream")));
            this.imageList.TransparentColor = System.Drawing.Color.Transparent;
            this.imageList.Images.SetKeyName(0, "");
            this.imageList.Images.SetKeyName(1, "");
            this.imageList.Images.SetKeyName(2, "");
            this.imageList.Images.SetKeyName(3, "");
            this.imageList.Images.SetKeyName(4, "OrganizationalUnit.ico");
            this.imageList.Images.SetKeyName(5, "GPMC.ico");
            this.imageList.Images.SetKeyName(6, "");
            this.imageList.Images.SetKeyName(7, "");
            this.imageList.Images.SetKeyName(8, "");
            this.imageList.Images.SetKeyName(9, "");
            this.imageList.Images.SetKeyName(10, "");
            this.imageList.Images.SetKeyName(11, "Cell_32.ico");
            this.imageList.Images.SetKeyName(12, "Admin.ico");
            // 
            // DescipGroupbox
            // 
            this.DescipGroupbox.Controls.Add(this.lblDescription);
            this.DescipGroupbox.Location = new System.Drawing.Point(12, 316);
            this.DescipGroupbox.Name = "DescipGroupbox";
            this.DescipGroupbox.Size = new System.Drawing.Size(340, 110);
            this.DescipGroupbox.TabIndex = 5;
            this.DescipGroupbox.TabStop = false;
            this.DescipGroupbox.Text = "Description";
            // 
            // lblDescription
            // 
            this.lblDescription.Location = new System.Drawing.Point(7, 20);
            this.lblDescription.Name = "lblDescription";
            this.lblDescription.Size = new System.Drawing.Size(327, 77);
            this.lblDescription.TabIndex = 0;
            // 
            // Addbtn
            // 
            this.Addbtn.Location = new System.Drawing.Point(12, 437);
            this.Addbtn.Name = "Addbtn";
            this.Addbtn.Size = new System.Drawing.Size(75, 23);
            this.Addbtn.TabIndex = 6;
            this.Addbtn.Text = "A&dd";
            this.Addbtn.UseVisualStyleBackColor = true;
            this.Addbtn.Click += new System.EventHandler(this.Addbtn_Click);
            // 
            // Removebtn
            // 
            this.Removebtn.Enabled = false;
            this.Removebtn.Location = new System.Drawing.Point(93, 437);
            this.Removebtn.Name = "Removebtn";
            this.Removebtn.Size = new System.Drawing.Size(75, 23);
            this.Removebtn.TabIndex = 7;
            this.Removebtn.Text = "&Remove";
            this.Removebtn.UseVisualStyleBackColor = true;
            this.Removebtn.Click += new System.EventHandler(this.Removebtn_Click);
            // 
            // Aboutbtn
            // 
            this.Aboutbtn.Enabled = false;
            this.Aboutbtn.Location = new System.Drawing.Point(174, 437);
            this.Aboutbtn.Name = "Aboutbtn";
            this.Aboutbtn.Size = new System.Drawing.Size(75, 23);
            this.Aboutbtn.TabIndex = 8;
            this.Aboutbtn.Text = "A&bout...";
            this.Aboutbtn.UseVisualStyleBackColor = true;
            this.Aboutbtn.Click += new System.EventHandler(this.Aboutbtn_Click);
            // 
            // PluginStandalonePage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.Name = "PluginStandalonePage";
            this.Size = new System.Drawing.Size(364, 472);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            this.DescipGroupbox.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox PluginComboBox;
        private Likewise.LMC.ServerControl.LWListView ChosenPluginlistView;
        private System.Windows.Forms.Button Aboutbtn;
        private System.Windows.Forms.Button Removebtn;
        private System.Windows.Forms.Button Addbtn;
        private System.Windows.Forms.GroupBox DescipGroupbox;
        private System.Windows.Forms.Label lblDescription;
        private System.Windows.Forms.ImageList imageList;
        private System.Windows.Forms.ColumnHeader chPlugin;
    }
}
