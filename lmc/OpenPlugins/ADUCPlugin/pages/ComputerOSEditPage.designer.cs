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

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    partial class ComputerOSEditPage
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
            this.txtServicePack = new System.Windows.Forms.TextBox();
            this.label7 = new System.Windows.Forms.Label();
            this.txtVersion = new System.Windows.Forms.TextBox();
            this.label8 = new System.Windows.Forms.Label();
            this.txtName = new System.Windows.Forms.TextBox();
            this.label9 = new System.Windows.Forms.Label();
            this.pnlData.SuspendLayout();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.txtServicePack);
            this.pnlData.Controls.Add(this.label7);
            this.pnlData.Controls.Add(this.txtVersion);
            this.pnlData.Controls.Add(this.label8);
            this.pnlData.Controls.Add(this.txtName);
            this.pnlData.Controls.Add(this.label9);
            this.pnlData.Size = new System.Drawing.Size(363, 378);
            // 
            // txtServicePack
            // 
            this.txtServicePack.Location = new System.Drawing.Point(99, 89);
            this.txtServicePack.Name = "txtServicePack";
            this.txtServicePack.ReadOnly = true;
            this.txtServicePack.Size = new System.Drawing.Size(194, 20);
            this.txtServicePack.TabIndex = 58;
            // 
            // label7
            // 
            this.label7.Location = new System.Drawing.Point(12, 92);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(89, 17);
            this.label7.TabIndex = 57;
            this.label7.Text = "&Service Pack:";
            // 
            // txtVersion
            // 
            this.txtVersion.Location = new System.Drawing.Point(99, 53);
            this.txtVersion.Name = "txtVersion";
            this.txtVersion.ReadOnly = true;
            this.txtVersion.Size = new System.Drawing.Size(194, 20);
            this.txtVersion.TabIndex = 56;
            // 
            // label8
            // 
            this.label8.Location = new System.Drawing.Point(12, 56);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(62, 18);
            this.label8.TabIndex = 55;
            this.label8.Text = "V&ersion:";
            // 
            // txtName
            // 
            this.txtName.Location = new System.Drawing.Point(99, 18);
            this.txtName.MaxLength = 64;
            this.txtName.Name = "txtName";
            this.txtName.ReadOnly = true;
            this.txtName.Size = new System.Drawing.Size(231, 20);
            this.txtName.TabIndex = 54;
            // 
            // label9
            // 
            this.label9.Location = new System.Drawing.Point(14, 21);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(69, 23);
            this.label9.TabIndex = 53;
            this.label9.Text = "Na&me:";
            // 
            // ComputerOSEditPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Name = "ComputerOSEditPage";
            this.Size = new System.Drawing.Size(363, 378);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TextBox txtServicePack;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.TextBox txtVersion;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.TextBox txtName;
        private System.Windows.Forms.Label label9;

    }
}
