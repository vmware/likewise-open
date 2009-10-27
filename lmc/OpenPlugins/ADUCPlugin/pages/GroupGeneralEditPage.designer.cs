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
    partial class GroupGeneralEditPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(GroupGeneralEditPage));
            this.rbtnUniversal = new System.Windows.Forms.RadioButton();
            this.rbtnDistribution = new System.Windows.Forms.RadioButton();
            this.rbtnSecurity = new System.Windows.Forms.RadioButton();
            this.rbtnGlobal = new System.Windows.Forms.RadioButton();
            this.groupBoxGroupType = new System.Windows.Forms.GroupBox();
            this.lblNotes = new System.Windows.Forms.Label();
            this.rbtnDomainLocal = new System.Windows.Forms.RadioButton();
            this.groupboxGroupScope = new System.Windows.Forms.GroupBox();
            this.txtEmail = new System.Windows.Forms.TextBox();
            this.lblEmail = new System.Windows.Forms.Label();
            this.txtDescription = new System.Windows.Forms.TextBox();
            this.lblDescription = new System.Windows.Forms.Label();
            this.txtPrewinGroup = new System.Windows.Forms.TextBox();
            this.lblGroupprewin = new System.Windows.Forms.Label();
            this.lblGroupName = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.txtNotes = new System.Windows.Forms.RichTextBox();
            this.pnlData.SuspendLayout();
            this.groupBoxGroupType.SuspendLayout();
            this.groupboxGroupScope.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.txtNotes);
            this.pnlData.Controls.Add(this.groupBoxGroupType);
            this.pnlData.Controls.Add(this.lblNotes);
            this.pnlData.Controls.Add(this.groupboxGroupScope);
            this.pnlData.Controls.Add(this.txtEmail);
            this.pnlData.Controls.Add(this.lblEmail);
            this.pnlData.Controls.Add(this.txtDescription);
            this.pnlData.Controls.Add(this.lblDescription);
            this.pnlData.Controls.Add(this.txtPrewinGroup);
            this.pnlData.Controls.Add(this.lblGroupprewin);
            this.pnlData.Controls.Add(this.lblGroupName);
            this.pnlData.Controls.Add(this.groupBox1);
            this.pnlData.Controls.Add(this.pictureBox1);
            this.pnlData.Size = new System.Drawing.Size(364, 354);
            // 
            // rbtnUniversal
            // 
            this.rbtnUniversal.Location = new System.Drawing.Point(9, 66);
            this.rbtnUniversal.Name = "rbtnUniversal";
            this.rbtnUniversal.Size = new System.Drawing.Size(81, 17);
            this.rbtnUniversal.TabIndex = 2;
            this.rbtnUniversal.Text = "&Universal";
            this.rbtnUniversal.UseVisualStyleBackColor = true;
            this.rbtnUniversal.CheckedChanged += new System.EventHandler(this.rbtn_CheckedChanged);
            // 
            // rbtnDistribution
            // 
            this.rbtnDistribution.Location = new System.Drawing.Point(14, 43);
            this.rbtnDistribution.Name = "rbtnDistribution";
            this.rbtnDistribution.Size = new System.Drawing.Size(100, 17);
            this.rbtnDistribution.TabIndex = 1;
            this.rbtnDistribution.Text = "&Distribution";
            this.rbtnDistribution.UseVisualStyleBackColor = true;
            this.rbtnDistribution.CheckedChanged += new System.EventHandler(this.rbtn_CheckedChanged);
            // 
            // rbtnSecurity
            // 
            this.rbtnSecurity.Checked = true;
            this.rbtnSecurity.Location = new System.Drawing.Point(14, 19);
            this.rbtnSecurity.Name = "rbtnSecurity";
            this.rbtnSecurity.Size = new System.Drawing.Size(100, 18);
            this.rbtnSecurity.TabIndex = 0;
            this.rbtnSecurity.TabStop = true;
            this.rbtnSecurity.Text = "&Security";
            this.rbtnSecurity.UseVisualStyleBackColor = true;
            this.rbtnSecurity.CheckedChanged += new System.EventHandler(this.rbtn_CheckedChanged);
            // 
            // rbtnGlobal
            // 
            this.rbtnGlobal.Checked = true;
            this.rbtnGlobal.Location = new System.Drawing.Point(9, 43);
            this.rbtnGlobal.Name = "rbtnGlobal";
            this.rbtnGlobal.Size = new System.Drawing.Size(81, 17);
            this.rbtnGlobal.TabIndex = 1;
            this.rbtnGlobal.TabStop = true;
            this.rbtnGlobal.Text = "&Global";
            this.rbtnGlobal.UseVisualStyleBackColor = true;
            this.rbtnGlobal.CheckedChanged += new System.EventHandler(this.rbtn_CheckedChanged);
            // 
            // groupBoxGroupType
            // 
            this.groupBoxGroupType.Controls.Add(this.rbtnDistribution);
            this.groupBoxGroupType.Controls.Add(this.rbtnSecurity);
            this.groupBoxGroupType.Location = new System.Drawing.Point(181, 159);
            this.groupBoxGroupType.Name = "groupBoxGroupType";
            this.groupBoxGroupType.Size = new System.Drawing.Size(165, 99);
            this.groupBoxGroupType.TabIndex = 48;
            this.groupBoxGroupType.TabStop = false;
            this.groupBoxGroupType.Text = "Group type";
            // 
            // lblNotes
            // 
            this.lblNotes.AutoSize = true;
            this.lblNotes.Location = new System.Drawing.Point(12, 267);
            this.lblNotes.Name = "lblNotes";
            this.lblNotes.Size = new System.Drawing.Size(38, 13);
            this.lblNotes.TabIndex = 49;
            this.lblNotes.Text = "&Notes:";
            // 
            // rbtnDomainLocal
            // 
            this.rbtnDomainLocal.Location = new System.Drawing.Point(9, 19);
            this.rbtnDomainLocal.Name = "rbtnDomainLocal";
            this.rbtnDomainLocal.Size = new System.Drawing.Size(114, 18);
            this.rbtnDomainLocal.TabIndex = 0;
            this.rbtnDomainLocal.Text = "D&omain local";
            this.rbtnDomainLocal.UseVisualStyleBackColor = true;
            this.rbtnDomainLocal.CheckedChanged += new System.EventHandler(this.rbtn_CheckedChanged);
            // 
            // groupboxGroupScope
            // 
            this.groupboxGroupScope.Controls.Add(this.rbtnUniversal);
            this.groupboxGroupScope.Controls.Add(this.rbtnGlobal);
            this.groupboxGroupScope.Controls.Add(this.rbtnDomainLocal);
            this.groupboxGroupScope.Location = new System.Drawing.Point(15, 159);
            this.groupboxGroupScope.Name = "groupboxGroupScope";
            this.groupboxGroupScope.Size = new System.Drawing.Size(143, 99);
            this.groupboxGroupScope.TabIndex = 47;
            this.groupboxGroupScope.TabStop = false;
            this.groupboxGroupScope.Text = "Group scope";
            // 
            // txtEmail
            // 
            this.txtEmail.Location = new System.Drawing.Point(97, 132);
            this.txtEmail.MaxLength = 256;
            this.txtEmail.Name = "txtEmail";
            this.txtEmail.Size = new System.Drawing.Size(249, 20);
            this.txtEmail.TabIndex = 46;
            this.txtEmail.TextChanged += new System.EventHandler(this.txtEmail_TextChanged);
            // 
            // lblEmail
            // 
            this.lblEmail.AutoSize = true;
            this.lblEmail.Location = new System.Drawing.Point(16, 132);
            this.lblEmail.Name = "lblEmail";
            this.lblEmail.Size = new System.Drawing.Size(38, 13);
            this.lblEmail.TabIndex = 45;
            this.lblEmail.Text = "E-&mail:";
            // 
            // txtDescription
            // 
            this.txtDescription.Location = new System.Drawing.Point(97, 102);
            this.txtDescription.MaxLength = 1024;
            this.txtDescription.Name = "txtDescription";
            this.txtDescription.Size = new System.Drawing.Size(249, 20);
            this.txtDescription.TabIndex = 44;
            this.txtDescription.TextChanged += new System.EventHandler(this.txtDescription_TextChanged);
            // 
            // lblDescription
            // 
            this.lblDescription.AutoSize = true;
            this.lblDescription.Location = new System.Drawing.Point(16, 104);
            this.lblDescription.Name = "lblDescription";
            this.lblDescription.Size = new System.Drawing.Size(63, 13);
            this.lblDescription.TabIndex = 43;
            this.lblDescription.Text = "D&escription:";
            // 
            // txtPrewinGroup
            // 
            this.txtPrewinGroup.Location = new System.Drawing.Point(189, 72);
            this.txtPrewinGroup.MaxLength = 256;
            this.txtPrewinGroup.Name = "txtPrewinGroup";
            this.txtPrewinGroup.Size = new System.Drawing.Size(157, 20);
            this.txtPrewinGroup.TabIndex = 42;
            this.txtPrewinGroup.TextChanged += new System.EventHandler(this.txtPrewinGroup_TextChanged);
            // 
            // lblGroupprewin
            // 
            this.lblGroupprewin.Location = new System.Drawing.Point(15, 75);
            this.lblGroupprewin.Name = "lblGroupprewin";
            this.lblGroupprewin.Size = new System.Drawing.Size(163, 24);
            this.lblGroupprewin.TabIndex = 41;
            this.lblGroupprewin.Text = "Group name(pre-&Windows 2000):";
            // 
            // lblGroupName
            // 
            this.lblGroupName.AutoSize = true;
            this.lblGroupName.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
            this.lblGroupName.Location = new System.Drawing.Point(53, 33);
            this.lblGroupName.Name = "lblGroupName";
            this.lblGroupName.Size = new System.Drawing.Size(52, 13);
            this.lblGroupName.TabIndex = 40;
            this.lblGroupName.Text = "               ";
            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(18, 59);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(330, 2);
            this.groupBox1.TabIndex = 39;
            this.groupBox1.TabStop = false;
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(20, 26);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(24, 24);
            this.pictureBox1.TabIndex = 38;
            this.pictureBox1.TabStop = false;
            // 
            // txtNotes
            // 
            this.txtNotes.AcceptsTab = true;
            this.txtNotes.Location = new System.Drawing.Point(15, 283);
            this.txtNotes.MaxLength = 1024;
            this.txtNotes.Name = "txtNotes";
            this.txtNotes.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.ForcedVertical;
            this.txtNotes.Size = new System.Drawing.Size(333, 60);
            this.txtNotes.TabIndex = 51;
            this.txtNotes.Text = "";
            this.txtNotes.TextChanged += new System.EventHandler(this.txtNotes_TextChanged);
            // 
            // GroupGeneralEditPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Name = "GroupGeneralEditPage";
            this.Size = new System.Drawing.Size(364, 354);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            this.groupBoxGroupType.ResumeLayout(false);
            this.groupboxGroupScope.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBoxGroupType;
        private System.Windows.Forms.RadioButton rbtnDistribution;
        private System.Windows.Forms.RadioButton rbtnSecurity;
        private System.Windows.Forms.Label lblNotes;
        private System.Windows.Forms.GroupBox groupboxGroupScope;
        private System.Windows.Forms.RadioButton rbtnUniversal;
        private System.Windows.Forms.RadioButton rbtnGlobal;
        private System.Windows.Forms.RadioButton rbtnDomainLocal;
        private System.Windows.Forms.TextBox txtEmail;
        private System.Windows.Forms.Label lblEmail;
        private System.Windows.Forms.TextBox txtDescription;
        private System.Windows.Forms.Label lblDescription;
        private System.Windows.Forms.TextBox txtPrewinGroup;
        private System.Windows.Forms.Label lblGroupprewin;
        private System.Windows.Forms.Label lblGroupName;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.RichTextBox txtNotes;

    }
}
