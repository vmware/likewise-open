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

namespace Likewise.LMC.ServerControl
{
    partial class Manage
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

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Manage));
            this.pnlBody = new System.Windows.Forms.Panel();
            this.manageLargeImageList = new System.Windows.Forms.ImageList(this.components);
            this.manageImageList = new System.Windows.Forms.ImageList(this.components);
            this.SuspendLayout();
            // 
            // pnlBody
            // 
            this.pnlBody.BackColor = System.Drawing.SystemColors.Window;
            this.pnlBody.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnlBody.Location = new System.Drawing.Point(0, 0);
            this.pnlBody.Margin = new System.Windows.Forms.Padding(2);
            this.pnlBody.Name = "pnlBody";
            this.pnlBody.Size = new System.Drawing.Size(496, 280);
            this.pnlBody.TabIndex = 1;
            // 
            // manageLargeImageList
            // 
            this.manageLargeImageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("manageLargeImageList.ImageStream")));
            this.manageLargeImageList.TransparentColor = System.Drawing.Color.Transparent;
            this.manageLargeImageList.Images.SetKeyName(0, "computer_48.bmp");
            this.manageLargeImageList.Images.SetKeyName(1, "Group_48.bmp");
            this.manageLargeImageList.Images.SetKeyName(2, "user_48.bmp");
            this.manageLargeImageList.Images.SetKeyName(3, "Folder.ico");
            this.manageLargeImageList.Images.SetKeyName(4, "aduc_48.bmp");
            this.manageLargeImageList.Images.SetKeyName(5, "DisabledUser.ico");
            this.manageLargeImageList.Images.SetKeyName(6, "DisabledComp.ico");
            this.manageLargeImageList.Images.SetKeyName(7, "FolderOpen.ico");
            this.manageLargeImageList.Images.SetKeyName(8, "EventViewer_48.ico");
            this.manageLargeImageList.Images.SetKeyName(9, "Cell_48.ico");
            // 
            // manageImageList
            // 
            this.manageImageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("manageImageList.ImageStream")));
            this.manageImageList.TransparentColor = System.Drawing.Color.Transparent;
            this.manageImageList.Images.SetKeyName(0, "computer_16.bmp");
            this.manageImageList.Images.SetKeyName(1, "group_16.bmp");
            this.manageImageList.Images.SetKeyName(2, "user_16.bmp");
            this.manageImageList.Images.SetKeyName(3, "Folder.ico");
            this.manageImageList.Images.SetKeyName(4, "OrganizationalUnit.ico");
            this.manageImageList.Images.SetKeyName(5, "GPMC.ico");
            this.manageImageList.Images.SetKeyName(6, "aduc_16.bmp");
            this.manageImageList.Images.SetKeyName(7, "DisabledUser.ico");
            this.manageImageList.Images.SetKeyName(8, "DisabledComp.ico");
            this.manageImageList.Images.SetKeyName(9, "FolderOpen.ico");
            this.manageImageList.Images.SetKeyName(10, "EventViewer_48.ico");
            this.manageImageList.Images.SetKeyName(11, "Cell_48.ico");
            this.manageImageList.Images.SetKeyName(12, "Admin.ico");
            this.manageImageList.Images.SetKeyName(13, "BlockedOrganizationalUnit.ico");
            // 
            // Manage
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Controls.Add(this.pnlBody);
            this.DoubleBuffered = true;
            this.Margin = new System.Windows.Forms.Padding(2);
            this.Name = "Manage";
            this.Size = new System.Drawing.Size(496, 280);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel pnlBody;
        public System.Windows.Forms.ImageList manageLargeImageList;
        public System.Windows.Forms.ImageList manageImageList;
    
    }
}
