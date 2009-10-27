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
    partial class DomainGeneralEditPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DomainGeneralEditPage));
            this.txtDescription = new System.Windows.Forms.TextBox();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.Namelabel = new System.Windows.Forms.Label();
            this.lbDescription = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.labelDomainLevel = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.lblForestLevel = new System.Windows.Forms.Label();
            this.textBoxDomainName = new System.Windows.Forms.TextBox();
            this.pnlData.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.textBoxDomainName);
            this.pnlData.Controls.Add(this.lblForestLevel);
            this.pnlData.Controls.Add(this.label4);
            this.pnlData.Controls.Add(this.labelDomainLevel);
            this.pnlData.Controls.Add(this.label2);
            this.pnlData.Controls.Add(this.label1);
            this.pnlData.Controls.Add(this.Namelabel);
            this.pnlData.Controls.Add(this.groupBox1);
            this.pnlData.Controls.Add(this.pictureBox1);
            this.pnlData.Controls.Add(this.txtDescription);
            this.pnlData.Controls.Add(this.lbDescription);
            this.pnlData.Size = new System.Drawing.Size(386, 472);
            // 
            // txtDescription
            // 
            this.txtDescription.Location = new System.Drawing.Point(16, 150);
            this.txtDescription.Name = "txtDescription";
            this.txtDescription.Size = new System.Drawing.Size(354, 20);
            this.txtDescription.TabIndex = 3;
            this.txtDescription.TextChanged += new System.EventHandler(this.txtDescription_TextChanged);
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(21, 16);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(48, 48);
            this.pictureBox1.TabIndex = 21;
            this.pictureBox1.TabStop = false;
            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(13, 79);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(361, 2);
            this.groupBox1.TabIndex = 22;
            this.groupBox1.TabStop = false;
            // 
            // Namelabel
            // 
            this.Namelabel.AutoSize = true;
            this.Namelabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 10.25F);
            this.Namelabel.Location = new System.Drawing.Point(85, 36);
            this.Namelabel.Name = "Namelabel";
            this.Namelabel.Size = new System.Drawing.Size(128, 17);
            this.Namelabel.TabIndex = 24;
            this.Namelabel.Text = "                              ";
            // 
            // lbDescription
            // 
            this.lbDescription.AutoSize = true;
            this.lbDescription.Location = new System.Drawing.Point(13, 132);
            this.lbDescription.Name = "lbDescription";
            this.lbDescription.Size = new System.Drawing.Size(63, 13);
            this.lbDescription.TabIndex = 2;
            this.lbDescription.Text = "&Description:";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(13, 95);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(173, 13);
            this.label1.TabIndex = 25;
            this.label1.Text = "Domain &name (pre-Windows 2000):";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(13, 194);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(120, 13);
            this.label2.TabIndex = 27;
            this.label2.Text = "Do&main functional level:";
            // 
            // labelDomainLevel
            // 
            this.labelDomainLevel.AutoSize = true;
            this.labelDomainLevel.Location = new System.Drawing.Point(13, 213);
            this.labelDomainLevel.Name = "labelDomainLevel";
            this.labelDomainLevel.Size = new System.Drawing.Size(0, 13);
            this.labelDomainLevel.TabIndex = 28;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(11, 255);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(113, 13);
            this.label4.TabIndex = 29;
            this.label4.Text = "&Forest functional level:";
            // 
            // lblForestLevel
            // 
            this.lblForestLevel.AutoSize = true;
            this.lblForestLevel.Location = new System.Drawing.Point(13, 274);
            this.lblForestLevel.Name = "lblForestLevel";
            this.lblForestLevel.Size = new System.Drawing.Size(0, 13);
            this.lblForestLevel.TabIndex = 30;
            // 
            // textBoxDomainName
            // 
            this.textBoxDomainName.Location = new System.Drawing.Point(190, 92);
            this.textBoxDomainName.Name = "textBoxDomainName";
            this.textBoxDomainName.ReadOnly = true;
            this.textBoxDomainName.Size = new System.Drawing.Size(184, 20);
            this.textBoxDomainName.TabIndex = 31;
            // 
            // DomainGeneralEditPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.Name = "DomainGeneralEditPage";
            this.Size = new System.Drawing.Size(386, 472);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TextBox txtDescription;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label Namelabel;
        private System.Windows.Forms.Label lbDescription;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label lblForestLevel;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label labelDomainLevel;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox textBoxDomainName;

    }
}