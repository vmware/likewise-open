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
    partial class PluginExtensionsPage
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
            this.label1 = new System.Windows.Forms.Label();
            this.ExtendablePlugincomboBox = new System.Windows.Forms.ComboBox();
            this.label2 = new System.Windows.Forms.Label();
            this.AddAddExtscheckBox = new System.Windows.Forms.CheckBox();
            this.label3 = new System.Windows.Forms.Label();
            this.AvailableExtscheckedListBox = new System.Windows.Forms.CheckedListBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.descriptionLabel = new System.Windows.Forms.Label();
            this.AboutBtn = new System.Windows.Forms.Button();
            this.DownloadBtn = new System.Windows.Forms.Button();
            this.pnlData.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.DownloadBtn);
            this.pnlData.Controls.Add(this.AboutBtn);
            this.pnlData.Controls.Add(this.groupBox1);
            this.pnlData.Controls.Add(this.AvailableExtscheckedListBox);
            this.pnlData.Controls.Add(this.label3);
            this.pnlData.Controls.Add(this.AddAddExtscheckBox);
            this.pnlData.Controls.Add(this.label2);
            this.pnlData.Controls.Add(this.ExtendablePlugincomboBox);
            this.pnlData.Controls.Add(this.label1);
            this.pnlData.Size = new System.Drawing.Size(364, 472);
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(13, 12);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(335, 29);
            this.label1.TabIndex = 0;
            this.label1.Text = "Use this page to enable plug-in extensions. To add a particular extension, select" +
                " the check box next to it.";
            // 
            // ExtendablePlugincomboBox
            // 
            this.ExtendablePlugincomboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.ExtendablePlugincomboBox.FormattingEnabled = true;
            this.ExtendablePlugincomboBox.Location = new System.Drawing.Point(16, 69);
            this.ExtendablePlugincomboBox.Name = "ExtendablePlugincomboBox";
            this.ExtendablePlugincomboBox.Size = new System.Drawing.Size(332, 21);
            this.ExtendablePlugincomboBox.TabIndex = 1;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(16, 50);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(151, 13);
            this.label2.TabIndex = 2;
            this.label2.Text = "Plug-ins that can be extended:";
            // 
            // AddAddExtscheckBox
            // 
            this.AddAddExtscheckBox.AutoSize = true;
            this.AddAddExtscheckBox.Location = new System.Drawing.Point(16, 102);
            this.AddAddExtscheckBox.Name = "AddAddExtscheckBox";
            this.AddAddExtscheckBox.Size = new System.Drawing.Size(111, 17);
            this.AddAddExtscheckBox.TabIndex = 3;
            this.AddAddExtscheckBox.Text = "Add all extensions";
            this.AddAddExtscheckBox.UseVisualStyleBackColor = true;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(16, 131);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(106, 13);
            this.label3.TabIndex = 4;
            this.label3.Text = "Available extensions:";
            // 
            // AvailableExtscheckedListBox
            // 
            this.AvailableExtscheckedListBox.FormattingEnabled = true;
            this.AvailableExtscheckedListBox.Location = new System.Drawing.Point(16, 149);
            this.AvailableExtscheckedListBox.Name = "AvailableExtscheckedListBox";
            this.AvailableExtscheckedListBox.Size = new System.Drawing.Size(332, 169);
            this.AvailableExtscheckedListBox.TabIndex = 5;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.descriptionLabel);
            this.groupBox1.Location = new System.Drawing.Point(16, 324);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(332, 100);
            this.groupBox1.TabIndex = 6;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Description";
            // 
            // descriptionLabel
            // 
            this.descriptionLabel.Location = new System.Drawing.Point(6, 20);
            this.descriptionLabel.Name = "descriptionLabel";
            this.descriptionLabel.Size = new System.Drawing.Size(312, 64);
            this.descriptionLabel.TabIndex = 0;
            // 
            // AboutBtn
            // 
            this.AboutBtn.Location = new System.Drawing.Point(16, 431);
            this.AboutBtn.Name = "AboutBtn";
            this.AboutBtn.Size = new System.Drawing.Size(75, 23);
            this.AboutBtn.TabIndex = 7;
            this.AboutBtn.Text = "About...";
            this.AboutBtn.UseVisualStyleBackColor = true;
            // 
            // DownloadBtn
            // 
            this.DownloadBtn.Location = new System.Drawing.Point(97, 432);
            this.DownloadBtn.Name = "DownloadBtn";
            this.DownloadBtn.Size = new System.Drawing.Size(75, 23);
            this.DownloadBtn.TabIndex = 8;
            this.DownloadBtn.Text = "Download";
            this.DownloadBtn.UseVisualStyleBackColor = true;
            // 
            // PluginExtensionsPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.Name = "PluginExtensionsPage";
            this.Size = new System.Drawing.Size(364, 472);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            this.groupBox1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.CheckedListBox AvailableExtscheckedListBox;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.CheckBox AddAddExtscheckBox;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ComboBox ExtendablePlugincomboBox;
        private System.Windows.Forms.Label descriptionLabel;
        private System.Windows.Forms.Button DownloadBtn;
        private System.Windows.Forms.Button AboutBtn;
    }
}
