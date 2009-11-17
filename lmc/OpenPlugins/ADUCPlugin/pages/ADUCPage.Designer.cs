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
    partial class ADUCPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ADUCPage));
            this.lvChildNodes = new Likewise.LMC.ServerControl.LWListView();
            this.NodeName = new System.Windows.Forms.ColumnHeader();
            this.NodeClass = new System.Windows.Forms.ColumnHeader();
            this.DN = new System.Windows.Forms.ColumnHeader();
            this.aducLargeImageList = new System.Windows.Forms.ImageList(this.components);
            this.aducImageList = new System.Windows.Forms.ImageList(this.components);
            this.backgroundWorker = new System.ComponentModel.BackgroundWorker();
            this.timer = new System.Windows.Forms.Timer(this.components);
            this.lblNoitemstodisplay = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.picture)).BeginInit();
            this.pnlHeader.SuspendLayout();
            this.SuspendLayout();
            // 
            // lvChildNodes
            // 
            this.lvChildNodes.BackColor = System.Drawing.SystemColors.Window;
            this.lvChildNodes.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.lvChildNodes.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.NodeName,
            this.NodeClass,
            this.DN});
            this.lvChildNodes.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lvChildNodes.FullRowSelect = true;
            this.lvChildNodes.HideSelection = false;
            this.lvChildNodes.LargeImageList = this.aducLargeImageList;
            this.lvChildNodes.Location = new System.Drawing.Point(131, 59);
            this.lvChildNodes.Name = "lvChildNodes";
            this.lvChildNodes.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.lvChildNodes.Size = new System.Drawing.Size(416, 251);
            this.lvChildNodes.SmallImageList = this.aducImageList;
            this.lvChildNodes.TabIndex = 4;
            this.lvChildNodes.TileSize = new System.Drawing.Size(48, 48);
            this.lvChildNodes.UseCompatibleStateImageBehavior = false;
            this.lvChildNodes.View = System.Windows.Forms.View.Details;
            this.lvChildNodes.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.lvChildNodes_MouseDoubleClick);
            this.lvChildNodes.MouseUp += new System.Windows.Forms.MouseEventHandler(this.lvChildNodes_MouseUp);
            this.lvChildNodes.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.lvChildNodes_ColumnClick);
            // 
            // NodeName
            // 
            this.NodeName.Text = "Name";
            this.NodeName.Width = 142;
            // 
            // NodeClass
            // 
            this.NodeClass.Text = "Class";
            this.NodeClass.Width = 71;
            // 
            // DN
            // 
            this.DN.Text = "Distinguished name";
            this.DN.Width = 177;
            // 
            // aducLargeImageList
            // 
            this.aducLargeImageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("aducLargeImageList.ImageStream")));
            this.aducLargeImageList.TransparentColor = System.Drawing.Color.Transparent;
            this.aducLargeImageList.Images.SetKeyName(0, "computer_48.bmp");
            this.aducLargeImageList.Images.SetKeyName(1, "Group_48.bmp");
            this.aducLargeImageList.Images.SetKeyName(2, "user_48.bmp");
            this.aducLargeImageList.Images.SetKeyName(3, "Folder.ico");
            this.aducLargeImageList.Images.SetKeyName(4, "aduc_48.bmp");
            this.aducLargeImageList.Images.SetKeyName(5, "DisabledUser.ico");
            this.aducLargeImageList.Images.SetKeyName(6, "DisabledComp.ico");
            // 
            // aducImageList
            // 
            this.aducImageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("aducImageList.ImageStream")));
            this.aducImageList.TransparentColor = System.Drawing.Color.Transparent;
            this.aducImageList.Images.SetKeyName(0, "computer_16.bmp");
            this.aducImageList.Images.SetKeyName(1, "group_16.bmp");
            this.aducImageList.Images.SetKeyName(2, "user_16.bmp");
            this.aducImageList.Images.SetKeyName(3, "Folder.ico");
            this.aducImageList.Images.SetKeyName(4, "OrganizationalUnit.ico");
            this.aducImageList.Images.SetKeyName(5, "GPMC.ico");
            this.aducImageList.Images.SetKeyName(6, "aduc_16.bmp");
            this.aducImageList.Images.SetKeyName(7, "DisabledUser.ico");
            this.aducImageList.Images.SetKeyName(8, "DisabledComp.ico");
            this.aducImageList.Images.SetKeyName(9, "folderopen.ico");
            this.aducImageList.Images.SetKeyName(10, "EventViewer_48.ico");
            this.aducImageList.Images.SetKeyName(11, "Cell.ico");
            // 
            // lblNoitemstodisplay
            // 
            this.lblNoitemstodisplay.AutoSize = true;
            this.lblNoitemstodisplay.Location = new System.Drawing.Point(229, 91);
            this.lblNoitemstodisplay.Name = "lblNoitemstodisplay";
            this.lblNoitemstodisplay.Size = new System.Drawing.Size(193, 13);
            this.lblNoitemstodisplay.TabIndex = 5;
            this.lblNoitemstodisplay.Text = "There are no items to show in this view.";
            this.lblNoitemstodisplay.Visible = false;
            // 
            // ADUCPage
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Controls.Add(this.lblNoitemstodisplay);
            this.Controls.Add(this.lvChildNodes);
            this.Name = "ADUCPage";
            this.Controls.SetChildIndex(this.pnlHeader, 0);
            this.Controls.SetChildIndex(this.pnlActions, 0);
            this.Controls.SetChildIndex(this.lvChildNodes, 0);
            this.Controls.SetChildIndex(this.lblNoitemstodisplay, 0);
            ((System.ComponentModel.ISupportInitialize)(this.picture)).EndInit();
            this.pnlHeader.ResumeLayout(false);
            this.pnlHeader.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ColumnHeader NodeName;
        private System.Windows.Forms.ColumnHeader NodeClass;
        private System.Windows.Forms.ColumnHeader DN;
        private System.Windows.Forms.ImageList aducImageList;
        private System.Windows.Forms.ImageList aducLargeImageList;
        private Likewise.LMC.ServerControl.LWListView lvChildNodes;
        private System.ComponentModel.BackgroundWorker backgroundWorker;
        private System.Windows.Forms.Timer timer;
        private System.Windows.Forms.Label lblNoitemstodisplay;
    }
}
