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
    partial class ComputerAddHostServerPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ComputerAddHostServerPage));
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.label2 = new System.Windows.Forms.Label();
            this.rbRemoteDefault = new System.Windows.Forms.RadioButton();
            this.rbRemoteSvrSelected = new System.Windows.Forms.RadioButton();
            this.textBox = new System.Windows.Forms.TextBox();
            this.btnSearch = new System.Windows.Forms.Button();
            this.label3 = new System.Windows.Forms.Label();
            this.txtCreatein = new System.Windows.Forms.TextBox();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(39, 83);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(410, 2);
            this.groupBox1.TabIndex = 32;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "groupBox1";
            // 
            // pictureBox1
            // 
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(42, 30);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(47, 48);
            this.pictureBox1.TabIndex = 31;
            this.pictureBox1.TabStop = false;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(39, 99);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(340, 13);
            this.label2.TabIndex = 33;
            this.label2.Text = "Specify the remote installation client to support this client:";
            // 
            // rbRemoteDefault
            // 
            this.rbRemoteDefault.AutoSize = true;
            this.rbRemoteDefault.Checked = true;
            this.rbRemoteDefault.Location = new System.Drawing.Point(42, 124);
            this.rbRemoteDefault.Name = "rbRemoteDefault";
            this.rbRemoteDefault.Size = new System.Drawing.Size(253, 17);
            this.rbRemoteDefault.TabIndex = 34;
            this.rbRemoteDefault.TabStop = true;
            this.rbRemoteDefault.Text = "&Any available remote installation server";
            this.rbRemoteDefault.UseVisualStyleBackColor = true;
            this.rbRemoteDefault.CheckedChanged += new System.EventHandler(this.rbRemoteDefault_CheckedChanged);
            // 
            // rbRemoteSvrSelected
            // 
            this.rbRemoteSvrSelected.AutoSize = true;
            this.rbRemoteSvrSelected.Location = new System.Drawing.Point(42, 147);
            this.rbRemoteSvrSelected.Name = "rbRemoteSvrSelected";
            this.rbRemoteSvrSelected.Size = new System.Drawing.Size(256, 17);
            this.rbRemoteSvrSelected.TabIndex = 35;
            this.rbRemoteSvrSelected.Text = "The following remote installation server:";
            this.rbRemoteSvrSelected.UseVisualStyleBackColor = true;
            this.rbRemoteSvrSelected.CheckedChanged += new System.EventHandler(this.rbRemoteSvrSelected_CheckedChanged);
            // 
            // textBox
            // 
            this.textBox.Enabled = false;
            this.textBox.Location = new System.Drawing.Point(60, 170);
            this.textBox.Name = "textBox";
            this.textBox.Size = new System.Drawing.Size(307, 21);
            this.textBox.TabIndex = 36;
            // 
            // btnSearch
            // 
            this.btnSearch.Enabled = false;
            this.btnSearch.Location = new System.Drawing.Point(374, 168);
            this.btnSearch.Name = "btnSearch";
            this.btnSearch.Size = new System.Drawing.Size(75, 23);
            this.btnSearch.TabIndex = 37;
            this.btnSearch.Text = "S&earch...";
            this.btnSearch.UseVisualStyleBackColor = true;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(58, 205);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(407, 13);
            this.label3.TabIndex = 38;
            this.label3.Text = "The server name you enter should be a fully qualified DNS hostname.";
            // 
            // txtCreatein
            // 
            this.txtCreatein.BackColor = System.Drawing.SystemColors.Control;
            this.txtCreatein.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.txtCreatein.Location = new System.Drawing.Point(99, 48);
            this.txtCreatein.Name = "txtCreatein";
            this.txtCreatein.ReadOnly = true;
            this.txtCreatein.Size = new System.Drawing.Size(362, 14);
            this.txtCreatein.TabIndex = 43;
            // 
            // ComputerAddHostServerPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.txtCreatein);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.btnSearch);
            this.Controls.Add(this.textBox);
            this.Controls.Add(this.rbRemoteSvrSelected);
            this.Controls.Add(this.rbRemoteDefault);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.pictureBox1);
            this.Name = "ComputerAddHostServerPage";
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.RadioButton rbRemoteDefault;
        private System.Windows.Forms.RadioButton rbRemoteSvrSelected;
        private System.Windows.Forms.TextBox textBox;
        private System.Windows.Forms.Button btnSearch;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox txtCreatein;
    }
}
