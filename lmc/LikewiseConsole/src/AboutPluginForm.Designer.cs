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

namespace Likewise.LMC
{
    partial class AboutPluginForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AboutPluginForm));
            this.pictureBox = new System.Windows.Forms.PictureBox();
            this.lblCorporation = new System.Windows.Forms.Label();
            this.imageList = new System.Windows.Forms.ImageList(this.components);
            this.btnOk = new System.Windows.Forms.Button();
            this.DescipGroupbox = new System.Windows.Forms.GroupBox();
            this.lblAboutPlugin = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox)).BeginInit();
            this.DescipGroupbox.SuspendLayout();
            this.SuspendLayout();
            //
            // pictureBox
            //
            this.pictureBox.Location = new System.Drawing.Point(15, 27);
            this.pictureBox.Name = "pictureBox";
            this.pictureBox.Size = new System.Drawing.Size(44, 39);
            this.pictureBox.TabIndex = 0;
            this.pictureBox.TabStop = false;
            //
            // lblCorporation
            //
            this.lblCorporation.AutoSize = true;
            this.lblCorporation.Location = new System.Drawing.Point(69, 30);
            this.lblCorporation.Name = "lblCorporation";
            this.lblCorporation.Size = new System.Drawing.Size(93, 26);
            this.lblCorporation.TabIndex = 1;
            this.lblCorporation.Text = "{0}\r\nLikewise Software";
            //
            // imageList
            //
            this.imageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList.ImageStream")));
            this.imageList.TransparentColor = System.Drawing.Color.Transparent;
            this.imageList.Images.SetKeyName(0, "FolderOpen_32.ico");
            this.imageList.Images.SetKeyName(1, "LocalGroup_32.ico");
            this.imageList.Images.SetKeyName(2, "EvenViewer_32.ico");
            this.imageList.Images.SetKeyName(3, "Cell_32.ico");
            this.imageList.Images.SetKeyName(4, "SharedFolder_32.ico");
            this.imageList.Images.SetKeyName(5, "ADUC_32.ico");
            this.imageList.Images.SetKeyName(6, "ADUC.ico");
            this.imageList.Images.SetKeyName(7, "Admin.ico");
            //
            // btnOk
            //
            this.btnOk.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOk.Location = new System.Drawing.Point(246, 196);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(75, 23);
            this.btnOk.TabIndex = 3;
            this.btnOk.Text = "&OK";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            //
            // DescipGroupbox
            //
            this.DescipGroupbox.Controls.Add(this.lblAboutPlugin);
            this.DescipGroupbox.Location = new System.Drawing.Point(12, 80);
            this.DescipGroupbox.Name = "DescipGroupbox";
            this.DescipGroupbox.Size = new System.Drawing.Size(309, 107);
            this.DescipGroupbox.TabIndex = 6;
            this.DescipGroupbox.TabStop = false;
            this.DescipGroupbox.Text = "Description";
            //
            // lblAboutPlugin
            //
            this.lblAboutPlugin.Location = new System.Drawing.Point(6, 17);
            this.lblAboutPlugin.Name = "lblAboutPlugin";
            this.lblAboutPlugin.Size = new System.Drawing.Size(297, 78);
            this.lblAboutPlugin.TabIndex = 0;
            //
            // AboutPluginForm
            //
            this.AcceptButton = this.btnOk;
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.ClientSize = new System.Drawing.Size(333, 229);
            this.Controls.Add(this.DescipGroupbox);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.lblCorporation);
            this.Controls.Add(this.pictureBox);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "AboutPluginForm";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "About {0}";
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox)).EndInit();
            this.DescipGroupbox.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblCorporation;
        private System.Windows.Forms.ImageList imageList;
        public System.Windows.Forms.PictureBox pictureBox;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.GroupBox DescipGroupbox;
        private System.Windows.Forms.Label lblAboutPlugin;
    }
}