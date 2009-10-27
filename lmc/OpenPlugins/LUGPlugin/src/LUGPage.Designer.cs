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

namespace Likewise.LMC.Plugins.LUG
{
    partial class LUGPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(LUGPage));
            this.panel4 = new System.Windows.Forms.Panel();
            this.panel5 = new System.Windows.Forms.Panel();
            this.panel3 = new System.Windows.Forms.Panel();
            this.panel2 = new System.Windows.Forms.Panel();
            this.lblFiltered = new System.Windows.Forms.Label();
            //this.label1 = new System.Windows.Forms.Label();
            this.cbLog = new System.Windows.Forms.ComboBox();
            this.panel1 = new System.Windows.Forms.Panel();
            this.Image = new System.Windows.Forms.ColumnHeader();
            this.Disabled = new System.Windows.Forms.ColumnHeader();
            this.LUGName = new System.Windows.Forms.ColumnHeader();
            this.FullName = new System.Windows.Forms.ColumnHeader();
            this.Description = new System.Windows.Forms.ColumnHeader();
            this.lvLUGBETA = new Likewise.LMC.ServerControl.LWListView();
            this.statusImageList = new System.Windows.Forms.ImageList(this.components);
            ((System.ComponentModel.ISupportInitialize)(this.picture)).BeginInit();
            this.panel3.SuspendLayout();
            this.panel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // lblCaption
            // 
            this.lblCaption.Size = new System.Drawing.Size(177, 23);
            this.lblCaption.Text = "Local {0} on {1}";
            // 
            // panel4
            // 
            this.panel4.Dock = System.Windows.Forms.DockStyle.Right;
            this.panel4.Location = new System.Drawing.Point(566, 59);
            this.panel4.Margin = new System.Windows.Forms.Padding(2);
            this.panel4.Name = "panel4";
            this.panel4.Size = new System.Drawing.Size(8, 243);
            this.panel4.TabIndex = 8;
            // 
            // panel5
            // 
            this.panel5.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel5.Location = new System.Drawing.Point(139, 302);
            this.panel5.Margin = new System.Windows.Forms.Padding(2);
            this.panel5.Name = "panel5";
            this.panel5.Size = new System.Drawing.Size(435, 8);
            this.panel5.TabIndex = 9;
            // 
            // panel3
            // 
            this.panel3.Controls.Add(this.panel2);
            this.panel3.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel3.Location = new System.Drawing.Point(139, 59);
            this.panel3.Margin = new System.Windows.Forms.Padding(2);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(427, 37);
            this.panel3.TabIndex = 10;
            // 
            // panel2
            // 
            this.panel2.Controls.Add(this.lblFiltered);
            //this.panel2.Controls.Add(this.label1);
            this.panel2.Controls.Add(this.cbLog);
            this.panel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel2.ForeColor = System.Drawing.SystemColors.Highlight;
            this.panel2.Location = new System.Drawing.Point(0, 0);
            this.panel2.Margin = new System.Windows.Forms.Padding(2);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(427, 34);
            this.panel2.TabIndex = 6;
            // 
            // lblFiltered
            // 
            this.lblFiltered.AutoSize = true;
            this.lblFiltered.Font = new System.Drawing.Font("Verdana", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblFiltered.ForeColor = System.Drawing.SystemColors.ControlText;
            this.lblFiltered.Location = new System.Drawing.Point(279, 10);
            this.lblFiltered.Name = "lblFiltered";
            this.lblFiltered.Size = new System.Drawing.Size(66, 16);
            this.lblFiltered.TabIndex = 2;
            this.lblFiltered.Text = "(filtered)";
            this.lblFiltered.Visible = false;
            // 
            // label1
            // 
            //this.label1.AutoSize = true;
            //this.label1.Font = new System.Drawing.Font("Verdana", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            //this.label1.ForeColor = System.Drawing.SystemColors.WindowText;
            //this.label1.Location = new System.Drawing.Point(5, 10);
            //this.label1.Name = "label1";
            //this.label1.Size = new System.Drawing.Size(110, 16);
            //this.label1.TabIndex = 1;
            //this.label1.Text = "Page Number:";
            // 
            // cbLog
            // 
            this.cbLog.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbLog.FormattingEnabled = true;
            this.cbLog.Location = new System.Drawing.Point(133, 8);
            this.cbLog.Name = "cbLog";
            this.cbLog.Size = new System.Drawing.Size(125, 21);
            this.cbLog.TabIndex = 0;
            this.cbLog.SelectedIndexChanged += new System.EventHandler(this.cbLog_SelectedIndexChanged);
            // 
            // panel1
            // 
            this.panel1.Dock = System.Windows.Forms.DockStyle.Left;
            this.panel1.Location = new System.Drawing.Point(131, 59);
            this.panel1.Margin = new System.Windows.Forms.Padding(2);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(8, 251);
            this.panel1.TabIndex = 11;
            // 
            // Image
            // 
            this.Image.Text = "";
            this.Image.Width = 18;
            // 
            // Disabled
            // 
            this.Disabled.Text = "";
            this.Disabled.Width = 0;
            // 
            // LUGName
            // 
            this.LUGName.Text = "Name";
            this.LUGName.Width = 82;
            // 
            // FullName
            // 
            this.FullName.Text = "Full Name";
            this.FullName.Width = 144;
            // 
            // Description
            // 
            this.Description.Text = "Description";
            this.Description.Width = 184;
            // 
            // lvLUGBETA
            // 
            this.lvLUGBETA.Alignment = System.Windows.Forms.ListViewAlignment.Left;
            this.lvLUGBETA.AllowDrop = true;
            this.lvLUGBETA.AutoArrange = false;
            this.lvLUGBETA.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.lvLUGBETA.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lvLUGBETA.FullRowSelect = true;
            this.lvLUGBETA.GridLines = true;
            this.lvLUGBETA.HideSelection = false;
            this.lvLUGBETA.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lvLUGBETA.Location = new System.Drawing.Point(139, 96);
            this.lvLUGBETA.MultiSelect = false;
            this.lvLUGBETA.Name = "lvLUGBETA";
            this.lvLUGBETA.Scrollable = Configurations.useListScrolling;
            this.lvLUGBETA.Size = new System.Drawing.Size(427, 206);
            this.lvLUGBETA.SmallImageList = this.statusImageList;
            this.lvLUGBETA.TabIndex = 12;
            this.lvLUGBETA.UseCompatibleStateImageBehavior = false;
            this.lvLUGBETA.View = System.Windows.Forms.View.Details;            
            this.lvLUGBETA.DoubleClick += new System.EventHandler(this.lvLUGBETA_DoubleClick);
            this.lvLUGBETA.MouseUp += new System.Windows.Forms.MouseEventHandler(this.lvLUGBETA_MouseUp);
            this.lvLUGBETA.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.lvLUGBETA_ColumnClick);
            this.lvLUGBETA.KeyDown += new System.Windows.Forms.KeyEventHandler(this.lvLUGBETA_KeyDown);
            // 
            // statusImageList
            // 
            this.statusImageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("statusImageList.ImageStream")));
            this.statusImageList.TransparentColor = System.Drawing.Color.Transparent;
            this.statusImageList.Images.SetKeyName(0, "LocalUser_16.ico");
            this.statusImageList.Images.SetKeyName(1, "DisabledUser_16.ico");
            this.statusImageList.Images.SetKeyName(2, "LocalGroup_16.ico");
            // 
            // LUGPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.Controls.Add(this.lvLUGBETA);
            this.Controls.Add(this.panel3);
            this.Controls.Add(this.panel4);
            this.Controls.Add(this.panel5);
            this.Controls.Add(this.panel1);
            this.HelpKeyword = "likewise.chm::/Centeris_Likewise_Console/Security/Users/About_Users.htm";
            this.Name = "LUGPage";
            this.Size = new System.Drawing.Size(574, 310);
            this.Controls.SetChildIndex(this.pnlActions, 0);
            this.Controls.SetChildIndex(this.panel1, 0);
            this.Controls.SetChildIndex(this.panel5, 0);
            this.Controls.SetChildIndex(this.panel4, 0);
            this.Controls.SetChildIndex(this.panel3, 0);
            this.Controls.SetChildIndex(this.lvLUGBETA, 0);
            ((System.ComponentModel.ISupportInitialize)(this.picture)).EndInit();
            this.panel3.ResumeLayout(false);
            this.panel2.ResumeLayout(false);
            this.panel2.PerformLayout();
            this.ResumeLayout(false);
        }

        #endregion

        private System.Windows.Forms.Panel panel4;
        private System.Windows.Forms.Panel panel5;
        private System.Windows.Forms.Panel panel3;
        private System.Windows.Forms.Panel panel1;

        private System.Windows.Forms.ColumnHeader LUGName;
        private System.Windows.Forms.ColumnHeader Disabled;
        private System.Windows.Forms.ColumnHeader FullName;
        private System.Windows.Forms.ColumnHeader Description;
        private System.Windows.Forms.ColumnHeader Image;

        private Likewise.LMC.ServerControl.LWListView lvLUGBETA;
        private System.Windows.Forms.ImageList statusImageList;

        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.Label lblFiltered;
        //private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox cbLog;
    }
}
