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

namespace Likewise.LMC.Plugins.FileShareManager
{
    partial class NewShareFinishPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(NewShareFinishPage));
            this.label1 = new System.Windows.Forms.Label();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.textBoxFoldername = new System.Windows.Forms.TextBox();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.checkBox = new System.Windows.Forms.CheckBox();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Verdana", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(155, 12);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(295, 36);
            this.label1.TabIndex = 10;
            this.label1.Text = "Completing the Create A Shared \r\nFolder Wizard\r\n";
            //
            // pictureBox1
            //
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.InitialImage = global::Likewise.LMC.Plugins.FileShareManager.Properties.Resources.likewise_splash_screen;
            this.pictureBox1.Location = new System.Drawing.Point(0, 0);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(151, 313);
            this.pictureBox1.TabIndex = 9;
            this.pictureBox1.TabStop = false;
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(158, 73);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(261, 13);
            this.label2.TabIndex = 19;
            this.label2.Text = "You successfully shared the following folder:";
            //
            // label3
            //
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(158, 137);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(273, 13);
            this.label3.TabIndex = 20;
            this.label3.Text = "The folder is shared with these network users:";
            //
            // label4
            //
            this.label4.Location = new System.Drawing.Point(162, 292);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(257, 18);
            this.label4.TabIndex = 22;
            this.label4.Text = "To close this wizard, click Finish.";
            //
            // textBoxFoldername
            //
            this.textBoxFoldername.BackColor = System.Drawing.SystemColors.Window;
            this.textBoxFoldername.Location = new System.Drawing.Point(162, 92);
            this.textBoxFoldername.Name = "textBoxFoldername";
            this.textBoxFoldername.ReadOnly = true;
            this.textBoxFoldername.Size = new System.Drawing.Size(288, 21);
            this.textBoxFoldername.TabIndex = 23;
            //
            // textBox1
            //
            this.textBox1.BackColor = System.Drawing.SystemColors.Window;
            this.textBox1.Location = new System.Drawing.Point(162, 157);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.ReadOnly = true;
            this.textBox1.Size = new System.Drawing.Size(288, 68);
            this.textBox1.TabIndex = 24;
            this.textBox1.Text = "All the Windows and Non-Windows clients";
            //
            // checkBox
            //
            this.checkBox.Location = new System.Drawing.Point(161, 248);
            this.checkBox.Name = "checkBox";
            this.checkBox.Size = new System.Drawing.Size(289, 36);
            this.checkBox.TabIndex = 25;
            this.checkBox.Text = "&Run this wizard again and create another \r\nshared folder.";
            this.checkBox.UseVisualStyleBackColor = true;
            //
            // NewShareFinishPage
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.BackColor = System.Drawing.SystemColors.Window;
            this.Controls.Add(this.checkBox);
            this.Controls.Add(this.textBox1);
            this.Controls.Add(this.textBoxFoldername);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.pictureBox1);
            this.MaximumSize = new System.Drawing.Size(490, 313);
            this.MinimumSize = new System.Drawing.Size(490, 313);
            this.Name = "NewShareFinishPage";
            this.Size = new System.Drawing.Size(490, 313);
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox textBoxFoldername;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.CheckBox checkBox;


    }
}
