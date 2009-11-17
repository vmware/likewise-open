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

using System.Windows.Forms;

namespace Likewise.LMC.ServerControl
{
    partial class StandardPage
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
            this.pnlHeader = new System.Windows.Forms.Panel();
            this.lnkHelp = new System.Windows.Forms.LinkLabel();
            this.pbHelp = new System.Windows.Forms.PictureBox();
            this.lblSubCaption = new System.Windows.Forms.Label();
            this.border1 = new Likewise.LMC.ServerControl.Border();
            this.lblCaption = new System.Windows.Forms.Label();
            this.picture = new System.Windows.Forms.PictureBox();
            this.pnlActions = new System.Windows.Forms.Panel();
            this.pnlHeader.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pbHelp)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.picture)).BeginInit();
            this.SuspendLayout();
            // 
            // pnlHeader
            // 
            this.pnlHeader.Controls.Add(this.lnkHelp);
            this.pnlHeader.Controls.Add(this.pbHelp);
            this.pnlHeader.Controls.Add(this.lblSubCaption);
            this.pnlHeader.Controls.Add(this.border1);
            this.pnlHeader.Controls.Add(this.lblCaption);
            this.pnlHeader.Controls.Add(this.picture);
            this.pnlHeader.Dock = System.Windows.Forms.DockStyle.Top;
            this.pnlHeader.Location = new System.Drawing.Point(0, 0);
            this.pnlHeader.Margin = new System.Windows.Forms.Padding(2);
            this.pnlHeader.Name = "pnlHeader";
            this.pnlHeader.Size = new System.Drawing.Size(547, 59);
            this.pnlHeader.TabIndex = 2;
            // 
            // lnkHelp
            // 
            this.lnkHelp.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.lnkHelp.AutoSize = true;
            this.lnkHelp.Location = new System.Drawing.Point(513, 43);
            this.lnkHelp.Name = "lnkHelp";
            this.lnkHelp.Size = new System.Drawing.Size(29, 13);
            this.lnkHelp.TabIndex = 7;
            this.lnkHelp.TabStop = true;
            this.lnkHelp.Text = "Help";
            this.lnkHelp.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.lnkHelp_LinkClicked);
            // 
            // pbHelp
            // 
            this.pbHelp.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.pbHelp.Location = new System.Drawing.Point(491, 40);
            this.pbHelp.Name = "pbHelp";
            this.pbHelp.Size = new System.Drawing.Size(16, 16);
            this.pbHelp.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.pbHelp.TabIndex = 6;
            this.pbHelp.TabStop = false;
            this.pbHelp.Click += new System.EventHandler(this.pbHelp_Click);
            // 
            // lblSubCaption
            // 
            this.lblSubCaption.AutoSize = true;
            this.lblSubCaption.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic))));
            this.lblSubCaption.ForeColor = System.Drawing.Color.Black;
            this.lblSubCaption.Location = new System.Drawing.Point(59, 36);
            this.lblSubCaption.Name = "lblSubCaption";
            this.lblSubCaption.Size = new System.Drawing.Size(0, 16);
            this.lblSubCaption.TabIndex = 5;
            this.lblSubCaption.Visible = false;
            // 
            // border1
            // 
            this.border1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(10)))), ((int)(((byte)(42)))), ((int)(((byte)(67)))));
            this.border1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.border1.Location = new System.Drawing.Point(0, 58);
            this.border1.Margin = new System.Windows.Forms.Padding(2);
            this.border1.Name = "border1";
            this.border1.Size = new System.Drawing.Size(547, 1);
            this.border1.TabIndex = 3;
            // 
            // lblCaption
            // 
            this.lblCaption.AutoSize = true;
            this.lblCaption.Font = new System.Drawing.Font("Verdana", 13.8F);
            this.lblCaption.Location = new System.Drawing.Point(60, 11);
            this.lblCaption.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.lblCaption.Name = "lblCaption";
            this.lblCaption.Size = new System.Drawing.Size(0, 23);
            this.lblCaption.TabIndex = 2;
            // 
            // picture
            // 
            this.picture.Location = new System.Drawing.Point(8, 3);
            this.picture.Margin = new System.Windows.Forms.Padding(2);
            this.picture.Name = "picture";
            this.picture.Size = new System.Drawing.Size(48, 48);
            this.picture.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
            this.picture.TabIndex = 0;
            this.picture.TabStop = false;
            // 
            // pnlActions
            // 
            this.pnlActions.Dock = System.Windows.Forms.DockStyle.Left;
            this.pnlActions.Location = new System.Drawing.Point(0, 59);
            this.pnlActions.Margin = new System.Windows.Forms.Padding(2);
            this.pnlActions.Name = "pnlActions";
            this.pnlActions.Size = new System.Drawing.Size(131, 251);
            this.pnlActions.TabIndex = 3;
            // 
            // StandardPage
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.BackColor = System.Drawing.SystemColors.Window;
            this.Controls.Add(this.pnlActions);
            this.Controls.Add(this.pnlHeader);
            this.DoubleBuffered = true;
            this.Margin = new System.Windows.Forms.Padding(2);
            this.Name = "StandardPage";
            this.Size = new System.Drawing.Size(547, 310);
            this.pnlHeader.ResumeLayout(false);
            this.pnlHeader.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pbHelp)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.picture)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        protected System.Windows.Forms.PictureBox picture;
        protected System.Windows.Forms.Label lblCaption;
        private Border border1;
        protected System.Windows.Forms.Panel pnlActions;
        protected Label lblSubCaption;
        private PictureBox pbHelp;
        private LinkLabel lnkHelp;
        protected Panel pnlHeader;
        
    }
}
