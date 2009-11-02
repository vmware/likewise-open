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
            this.Image = new System.Windows.Forms.ColumnHeader();
            this.Disabled = new System.Windows.Forms.ColumnHeader();
            this.LUGName = new System.Windows.Forms.ColumnHeader();
            this.FullName = new System.Windows.Forms.ColumnHeader();
            this.Description = new System.Windows.Forms.ColumnHeader();
            this.lvLUGBETA = new Likewise.LMC.ServerControl.LWListView();
            this.statusImageList = new System.Windows.Forms.ImageList(this.components);
            ((System.ComponentModel.ISupportInitialize)(this.picture)).BeginInit();
            this.pnlHeader.SuspendLayout();
            this.SuspendLayout();
            // 
            // lblCaption
            // 
            this.lblCaption.Size = new System.Drawing.Size(177, 23);
            this.lblCaption.Text = "Local {0} on {1}";
            // 
            // pnlHeader
            // 
            this.pnlHeader.Size = new System.Drawing.Size(574, 59);
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
            this.lvLUGBETA.AutoArrange = false;
            this.lvLUGBETA.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.lvLUGBETA.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lvLUGBETA.FullRowSelect = true;
            this.lvLUGBETA.HideSelection = false;
            this.lvLUGBETA.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lvLUGBETA.Location = new System.Drawing.Point(131, 59);
            this.lvLUGBETA.MultiSelect = false;
            this.lvLUGBETA.Name = "lvLUGBETA";
            this.lvLUGBETA.Size = new System.Drawing.Size(443, 251);
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
            this.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
            this.Controls.Add(this.lvLUGBETA);
            this.HelpKeyword = "likewise.chm::/Centeris_Likewise_Console/Security/Users/About_Users.htm";
            this.Name = "LUGPage";
            this.Size = new System.Drawing.Size(574, 310);
            this.Controls.SetChildIndex(this.pnlHeader, 0);
            this.Controls.SetChildIndex(this.pnlActions, 0);
            this.Controls.SetChildIndex(this.lvLUGBETA, 0);
            ((System.ComponentModel.ISupportInitialize)(this.picture)).EndInit();
            this.pnlHeader.ResumeLayout(false);
            this.pnlHeader.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion


        private System.Windows.Forms.ColumnHeader LUGName;
        private System.Windows.Forms.ColumnHeader Disabled;
        private System.Windows.Forms.ColumnHeader FullName;
        private System.Windows.Forms.ColumnHeader Description;
        private System.Windows.Forms.ColumnHeader Image;

        private Likewise.LMC.ServerControl.LWListView lvLUGBETA;
        private System.Windows.Forms.ImageList statusImageList;
        //private System.Windows.Forms.Label label1;
    }
}
