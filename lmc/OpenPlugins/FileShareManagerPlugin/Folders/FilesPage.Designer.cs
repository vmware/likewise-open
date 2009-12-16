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

namespace Likewise.LMC.Plugins.FileShareManager
{
    partial class FilesPage
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
            this.panel1 = new System.Windows.Forms.Panel();
            this.panel3 = new System.Windows.Forms.Panel();
            this.panel4 = new System.Windows.Forms.Panel();
            this.panel5 = new System.Windows.Forms.Panel();
            this.lvFilePage = new Likewise.LMC.ServerControl.LWListView();
            this.File = new System.Windows.Forms.ColumnHeader();
            this.Opener = new System.Windows.Forms.ColumnHeader();
            this.NumLocks = new System.Windows.Forms.ColumnHeader();
            this.OpenMode = new System.Windows.Forms.ColumnHeader();
            this.FileId = new System.Windows.Forms.ColumnHeader();
            ((System.ComponentModel.ISupportInitialize)(this.picture)).BeginInit();
            this.pnlHeader.SuspendLayout();
            this.SuspendLayout();
            //
            // lblCaption
            //
            this.lblCaption.Size = new System.Drawing.Size(185, 23);
            this.lblCaption.Text = "Open Files on {0}";
            //
            // panel1
            //
            this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel1.Location = new System.Drawing.Point(131, 59);
            this.panel1.Margin = new System.Windows.Forms.Padding(2);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(416, 8);
            this.panel1.TabIndex = 5;
            //
            // panel3
            //
            this.panel3.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel3.Location = new System.Drawing.Point(131, 302);
            this.panel3.Margin = new System.Windows.Forms.Padding(2);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(416, 8);
            this.panel3.TabIndex = 5;
            //
            // panel4
            //
            this.panel4.Dock = System.Windows.Forms.DockStyle.Left;
            this.panel4.Location = new System.Drawing.Point(131, 67);
            this.panel4.Margin = new System.Windows.Forms.Padding(2);
            this.panel4.Name = "panel4";
            this.panel4.Size = new System.Drawing.Size(8, 235);
            this.panel4.TabIndex = 5;
            //
            // panel5
            //
            this.panel5.Dock = System.Windows.Forms.DockStyle.Right;
            this.panel5.Location = new System.Drawing.Point(539, 67);
            this.panel5.Margin = new System.Windows.Forms.Padding(2);
            this.panel5.Name = "panel5";
            this.panel5.Size = new System.Drawing.Size(8, 235);
            this.panel5.TabIndex = 5;
            //
            // lvFilePage
            //
            this.lvFilePage.BackColor = System.Drawing.SystemColors.Window;
            this.lvFilePage.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.lvFilePage.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.File,
            this.Opener,
            this.NumLocks,
            this.OpenMode,
            this.FileId});
            this.lvFilePage.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lvFilePage.FullRowSelect = true;
            this.lvFilePage.HideSelection = false;
            this.lvFilePage.Location = new System.Drawing.Point(139, 67);
            this.lvFilePage.MultiSelect = false;
            this.lvFilePage.Name = "lvFilePage";
            this.lvFilePage.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.lvFilePage.Size = new System.Drawing.Size(400, 235);
            this.lvFilePage.TabIndex = 6;
            this.lvFilePage.TileSize = new System.Drawing.Size(48, 48);
            this.lvFilePage.UseCompatibleStateImageBehavior = false;
            this.lvFilePage.View = System.Windows.Forms.View.Details;
            this.lvFilePage.SelectedIndexChanged += new System.EventHandler(this.lvFilePage_SelectedIndexChanged);
            this.lvFilePage.MouseUp += new System.Windows.Forms.MouseEventHandler(this.lvFilePage_MouseUp);
            this.lvFilePage.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.lvFilePage_ColumnClick);
            //
            // File
            //
            this.File.Text = "Open File";
            this.File.Width = 160;
            //
            // Opener
            //
            this.Opener.Text = "Accessed By";
            this.Opener.Width = 79;
            //
            // NumLocks
            //
            this.NumLocks.Text = "# of Locks";
            this.NumLocks.Width = 80;
            //
            // OpenMode
            //
            this.OpenMode.Text = "Open Mode";
            this.OpenMode.Width = 80;
            //
            // FileId
            //
            this.FileId.Text = "FileId";
            this.FileId.Width = 0;
            //
            // FilesPage
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Controls.Add(this.lvFilePage);
            this.Controls.Add(this.panel5);
            this.Controls.Add(this.panel4);
            this.Controls.Add(this.panel3);
            this.Controls.Add(this.panel1);
            this.HelpKeyword = "likewise.chm::/Centeris_Likewise_Console/File_and_Print/Open_Files_Subtab.htm";
            this.Name = "FilesPage";
            this.Controls.SetChildIndex(this.pnlHeader, 0);
            this.Controls.SetChildIndex(this.pnlActions, 0);
            this.Controls.SetChildIndex(this.panel1, 0);
            this.Controls.SetChildIndex(this.panel3, 0);
            this.Controls.SetChildIndex(this.panel4, 0);
            this.Controls.SetChildIndex(this.panel5, 0);
            this.Controls.SetChildIndex(this.lvFilePage, 0);
            ((System.ComponentModel.ISupportInitialize)(this.picture)).EndInit();
            this.pnlHeader.ResumeLayout(false);
            this.pnlHeader.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Panel panel3;
        private System.Windows.Forms.Panel panel4;
        private System.Windows.Forms.Panel panel5;
        private Likewise.LMC.ServerControl.LWListView lvFilePage;
        private System.Windows.Forms.ColumnHeader File;
        private System.Windows.Forms.ColumnHeader Opener;
        private System.Windows.Forms.ColumnHeader NumLocks;
        private System.Windows.Forms.ColumnHeader OpenMode;
        private System.Windows.Forms.ColumnHeader FileId;
    }
}
