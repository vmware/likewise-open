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

namespace Likewise.LMC.Plugins.FileShareManager
{
    partial class SharesPage
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
            this.lvSharePage = new Likewise.LMC.ServerControl.LWListView();
            this.Share = new System.Windows.Forms.ColumnHeader();
            this.Path = new System.Windows.Forms.ColumnHeader();
            this.Comment = new System.Windows.Forms.ColumnHeader();
            this.CurrentUses = new System.Windows.Forms.ColumnHeader();
            ((System.ComponentModel.ISupportInitialize)(this.picture)).BeginInit();
            this.pnlActions.SuspendLayout();
            this.SuspendLayout();
            // 
            // lblCaption
            // 
            this.lblCaption.Size = new System.Drawing.Size(149, 23);
            this.lblCaption.Text = "Shares on {0}";
            this.pnlActions.Location = new System.Drawing.Point(8, 67);
            this.pnlActions.Size = new System.Drawing.Size(131, 235);
            // 
            // panel1
            // 
            this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel1.Location = new System.Drawing.Point(0, 59);
            this.panel1.Margin = new System.Windows.Forms.Padding(2);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(547, 8);
            this.panel1.TabIndex = 5;
            // 
            // panel3
            // 
            this.panel3.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel3.Location = new System.Drawing.Point(0, 302);
            this.panel3.Margin = new System.Windows.Forms.Padding(2);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(547, 8);
            this.panel3.TabIndex = 5;
            // 
            // panel4
            // 
            this.panel4.Dock = System.Windows.Forms.DockStyle.Left;
            this.panel4.Location = new System.Drawing.Point(0, 67);
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
            // lvSharePage
            // 
            this.lvSharePage.BackColor = System.Drawing.SystemColors.Window;
            this.lvSharePage.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.lvSharePage.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.Share,
            this.Path,
            this.Comment,
            this.CurrentUses});
            this.lvSharePage.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lvSharePage.FullRowSelect = true;
            this.lvSharePage.HideSelection = false;
            this.lvSharePage.Location = new System.Drawing.Point(139, 67);
            this.lvSharePage.MultiSelect = false;
            this.lvSharePage.Name = "lvSharePage";
            this.lvSharePage.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.lvSharePage.Size = new System.Drawing.Size(400, 235);
            this.lvSharePage.TabIndex = 7;
            this.lvSharePage.TileSize = new System.Drawing.Size(48, 48);
            this.lvSharePage.UseCompatibleStateImageBehavior = false;
            this.lvSharePage.View = System.Windows.Forms.View.Details;
            this.lvSharePage.SelectedIndexChanged += new System.EventHandler(this.lvSharePage_SelectedIndexChanged);
            this.lvSharePage.MouseUp += new System.Windows.Forms.MouseEventHandler(this.lvSharePage_MouseUp);
            this.lvSharePage.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.lvSharePage_ColumnClick);
            // 
            // Share
            // 
            this.Share.Text = "Share";
            this.Share.Width = 80;
            // 
            // Path
            // 
            this.Path.Text = "Path";
            this.Path.Width = 120;
            // 
            // Comment
            // 
            this.Comment.Text = "Description";
            this.Comment.Width = 119;
            // 
            // CurrentUses
            // 
            this.CurrentUses.Text = "Current Users";
            this.CurrentUses.Width = 80;
            // 
            // SharesPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.Controls.Add(this.lvSharePage);
            this.Controls.Add(this.panel5);
            this.Controls.Add(this.panel4);
            this.Controls.Add(this.panel3);
            this.Controls.Add(this.panel1);
            this.HelpKeyword = "likewise.chm::/Centeris_Likewise_Console/File_and_Print/Shares/Concepts.htm";
            this.Name = "SharesPage";
            this.Controls.SetChildIndex(this.panel1, 0);
            this.Controls.SetChildIndex(this.panel3, 0);
            this.Controls.SetChildIndex(this.panel4, 0);
            this.Controls.SetChildIndex(this.panel5, 0);
            this.Controls.SetChildIndex(this.pnlActions, 0);
            this.Controls.SetChildIndex(this.lvSharePage, 0);
            ((System.ComponentModel.ISupportInitialize)(this.picture)).EndInit();
            this.pnlActions.ResumeLayout(false);
            this.pnlActions.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Panel panel3;
        private System.Windows.Forms.Panel panel4;
        private System.Windows.Forms.Panel panel5;          
        private Likewise.LMC.ServerControl.LWListView lvSharePage;
        private System.Windows.Forms.ColumnHeader Share;
        private System.Windows.Forms.ColumnHeader Path;
        private System.Windows.Forms.ColumnHeader Comment;
        private System.Windows.Forms.ColumnHeader CurrentUses;
    }
}
