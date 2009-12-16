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
    partial class UserProfilePage
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
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.txtLogonScript = new System.Windows.Forms.TextBox();
            this.txtProfilePath = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.txtConnect = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.cbDrive = new System.Windows.Forms.ComboBox();
            this.rbConnect = new System.Windows.Forms.RadioButton();
            this.txtLocalPath = new System.Windows.Forms.TextBox();
            this.rbLocalPath = new System.Windows.Forms.RadioButton();
            this.pnlData.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            //
            // pnlData
            //
            this.pnlData.Controls.Add(this.groupBox2);
            this.pnlData.Controls.Add(this.groupBox1);
            this.pnlData.Size = new System.Drawing.Size(391, 418);
            //
            // groupBox1
            //
            this.groupBox1.Controls.Add(this.txtLogonScript);
            this.groupBox1.Controls.Add(this.txtProfilePath);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Location = new System.Drawing.Point(18, 13);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(356, 87);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "User profile";
            //
            // txtLogonScript
            //
            this.txtLogonScript.Location = new System.Drawing.Point(97, 52);
            this.txtLogonScript.Name = "txtLogonScript";
            this.txtLogonScript.Size = new System.Drawing.Size(251, 20);
            this.txtLogonScript.TabIndex = 3;
            this.txtLogonScript.TextChanged += new System.EventHandler(this.txtLogonScript_TextChanged);
            //
            // txtProfilePath
            //
            this.txtProfilePath.Location = new System.Drawing.Point(97, 22);
            this.txtProfilePath.Name = "txtProfilePath";
            this.txtProfilePath.Size = new System.Drawing.Size(253, 20);
            this.txtProfilePath.TabIndex = 2;
            this.txtProfilePath.TextChanged += new System.EventHandler(this.txtProfilePath_TextChanged);
            //
            // label2
            //
            this.label2.AutoEllipsis = true;
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(8, 55);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(68, 13);
            this.label2.TabIndex = 1;
            this.label2.Text = "Logon &script:";
            //
            // label1
            //
            this.label1.AutoEllipsis = true;
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(10, 25);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(63, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "&Profile path:";
            //
            // groupBox2
            //
            this.groupBox2.Controls.Add(this.txtConnect);
            this.groupBox2.Controls.Add(this.label3);
            this.groupBox2.Controls.Add(this.cbDrive);
            this.groupBox2.Controls.Add(this.rbConnect);
            this.groupBox2.Controls.Add(this.txtLocalPath);
            this.groupBox2.Controls.Add(this.rbLocalPath);
            this.groupBox2.Location = new System.Drawing.Point(18, 111);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(356, 90);
            this.groupBox2.TabIndex = 1;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Home folder";
            //
            // txtConnect
            //
            this.txtConnect.Location = new System.Drawing.Point(172, 55);
            this.txtConnect.Name = "txtConnect";
            this.txtConnect.Size = new System.Drawing.Size(171, 20);
            this.txtConnect.TabIndex = 8;
            this.txtConnect.TextChanged += new System.EventHandler(this.txtConnect_TextChanged);
            //
            // label3
            //
            this.label3.AutoEllipsis = true;
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(143, 58);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(23, 13);
            this.label3.TabIndex = 7;
            this.label3.Text = "&To:";
            //
            // cbDrive
            //
            this.cbDrive.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbDrive.Enabled = false;
            this.cbDrive.FormattingEnabled = true;
            this.cbDrive.ImeMode = System.Windows.Forms.ImeMode.On;
            this.cbDrive.Location = new System.Drawing.Point(97, 54);
            this.cbDrive.MaxDropDownItems = 12;
            this.cbDrive.Name = "cbDrive";
            this.cbDrive.Size = new System.Drawing.Size(46, 21);
            this.cbDrive.TabIndex = 6;
            this.cbDrive.SelectedIndexChanged += new System.EventHandler(this.cbDrive_SelectedIndexChanged);
            //
            // rbConnect
            //
            this.rbConnect.Location = new System.Drawing.Point(6, 56);
            this.rbConnect.Name = "rbConnect";
            this.rbConnect.Size = new System.Drawing.Size(85, 17);
            this.rbConnect.TabIndex = 5;
            this.rbConnect.Text = "&Connect:";
            this.rbConnect.UseVisualStyleBackColor = true;
            this.rbConnect.CheckedChanged += new System.EventHandler(this.rbConnect_CheckedChanged);
            //
            // txtLocalPath
            //
            this.txtLocalPath.Location = new System.Drawing.Point(97, 25);
            this.txtLocalPath.Name = "txtLocalPath";
            this.txtLocalPath.Size = new System.Drawing.Size(247, 20);
            this.txtLocalPath.TabIndex = 4;
            this.txtLocalPath.TextChanged += new System.EventHandler(this.txtLocalPath_TextChanged);
            //
            // rbLocalPath
            //
            this.rbLocalPath.Location = new System.Drawing.Point(6, 27);
            this.rbLocalPath.Name = "rbLocalPath";
            this.rbLocalPath.Size = new System.Drawing.Size(88, 17);
            this.rbLocalPath.TabIndex = 0;
            this.rbLocalPath.Text = "&Local path:";
            this.rbLocalPath.UseVisualStyleBackColor = true;
            this.rbLocalPath.CheckedChanged += new System.EventHandler(this.rbLocalPath_CheckedChanged);
            //
            // UserProfilePage
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Name = "UserProfilePage";
            this.Size = new System.Drawing.Size(391, 418);
            this.pnlData.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.RadioButton rbConnect;
        private System.Windows.Forms.TextBox txtLocalPath;
        private System.Windows.Forms.RadioButton rbLocalPath;
        private System.Windows.Forms.TextBox txtLogonScript;
        private System.Windows.Forms.TextBox txtProfilePath;
        private System.Windows.Forms.TextBox txtConnect;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.ComboBox cbDrive;
    }
}
