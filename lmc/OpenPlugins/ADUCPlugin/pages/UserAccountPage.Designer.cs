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
    partial class UserAccountPage
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
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.txtlogon = new System.Windows.Forms.TextBox();
            this.cbDomain = new System.Windows.Forms.ComboBox();
            this.txtpreLogonname = new System.Windows.Forms.TextBox();
            this.txtDomian = new System.Windows.Forms.TextBox();
            this.btnLogonHours = new System.Windows.Forms.Button();
            this.btnLogonTo = new System.Windows.Forms.Button();
            this.cbAccLocked = new System.Windows.Forms.CheckBox();
            this.ListUserOptions = new System.Windows.Forms.CheckedListBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.dateTimePicker = new System.Windows.Forms.DateTimePicker();
            this.rbEndOf = new System.Windows.Forms.RadioButton();
            this.rbNever = new System.Windows.Forms.RadioButton();
            this.pnlData.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            //
            // pnlData
            //
            this.pnlData.Controls.Add(this.groupBox1);
            this.pnlData.Controls.Add(this.ListUserOptions);
            this.pnlData.Controls.Add(this.cbAccLocked);
            this.pnlData.Controls.Add(this.btnLogonTo);
            this.pnlData.Controls.Add(this.btnLogonHours);
            this.pnlData.Controls.Add(this.txtDomian);
            this.pnlData.Controls.Add(this.txtpreLogonname);
            this.pnlData.Controls.Add(this.cbDomain);
            this.pnlData.Controls.Add(this.txtlogon);
            this.pnlData.Controls.Add(this.label3);
            this.pnlData.Controls.Add(this.label2);
            this.pnlData.Controls.Add(this.label1);
            this.pnlData.Size = new System.Drawing.Size(374, 444);
            //
            // label1
            //
            this.label1.AutoEllipsis = true;
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 14);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(90, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "&User logon name:";
            //
            // label2
            //
            this.label2.AutoEllipsis = true;
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(12, 61);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(188, 13);
            this.label2.TabIndex = 1;
            this.label2.Text = "User logon name (pre-&Windows 2000):";
            //
            // label3
            //
            this.label3.AutoEllipsis = true;
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(15, 169);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(87, 13);
            this.label3.TabIndex = 2;
            this.label3.Text = "Account &options.";
            //
            // txtlogon
            //
            this.txtlogon.Location = new System.Drawing.Point(15, 30);
            this.txtlogon.Name = "txtlogon";
            this.txtlogon.Size = new System.Drawing.Size(165, 20);
            this.txtlogon.TabIndex = 3;
            this.txtlogon.TextChanged += new System.EventHandler(this.txtlogon_TextChanged);
            //
            // cbDomain
            //
            this.cbDomain.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbDomain.FormattingEnabled = true;
            this.cbDomain.Location = new System.Drawing.Point(186, 30);
            this.cbDomain.Name = "cbDomain";
            this.cbDomain.Size = new System.Drawing.Size(170, 21);
            this.cbDomain.TabIndex = 4;
            //
            // txtpreLogonname
            //
            this.txtpreLogonname.Location = new System.Drawing.Point(186, 77);
            this.txtpreLogonname.Name = "txtpreLogonname";
            this.txtpreLogonname.Size = new System.Drawing.Size(170, 20);
            this.txtpreLogonname.TabIndex = 5;
            this.txtpreLogonname.TextChanged += new System.EventHandler(this.txtpreLogonname_TextChanged);
            //
            // txtDomian
            //
            this.txtDomian.Location = new System.Drawing.Point(15, 77);
            this.txtDomian.Name = "txtDomian";
            this.txtDomian.ReadOnly = true;
            this.txtDomian.Size = new System.Drawing.Size(165, 20);
            this.txtDomian.TabIndex = 6;
            //
            // btnLogonHours
            //
            this.btnLogonHours.AutoEllipsis = true;
            this.btnLogonHours.Location = new System.Drawing.Point(15, 106);
            this.btnLogonHours.Name = "btnLogonHours";
            this.btnLogonHours.Size = new System.Drawing.Size(96, 28);
            this.btnLogonHours.TabIndex = 7;
            this.btnLogonHours.Text = "Logon Hours...";
            this.btnLogonHours.UseVisualStyleBackColor = true;
            this.btnLogonHours.Click += new System.EventHandler(this.btnLogonHours_Click);
            //
            // btnLogonTo
            //
            this.btnLogonTo.AutoEllipsis = true;
            this.btnLogonTo.Location = new System.Drawing.Point(116, 107);
            this.btnLogonTo.Name = "btnLogonTo";
            this.btnLogonTo.Size = new System.Drawing.Size(93, 26);
            this.btnLogonTo.TabIndex = 8;
            this.btnLogonTo.Text = "Log On &To...";
            this.btnLogonTo.UseVisualStyleBackColor = true;
            this.btnLogonTo.Click += new System.EventHandler(this.btnLogonTo_Click);
            //
            // cbAccLocked
            //
            this.cbAccLocked.AutoSize = true;
            this.cbAccLocked.Enabled = false;
            this.cbAccLocked.Location = new System.Drawing.Point(15, 144);
            this.cbAccLocked.Name = "cbAccLocked";
            this.cbAccLocked.Size = new System.Drawing.Size(115, 17);
            this.cbAccLocked.TabIndex = 9;
            this.cbAccLocked.Text = "A&ccount is Locked";
            this.cbAccLocked.UseVisualStyleBackColor = true;
            this.cbAccLocked.CheckedChanged += new System.EventHandler(this.cbAccLocked_CheckedChanged);
            //
            // ListUserOptions
            //
            this.ListUserOptions.BackColor = System.Drawing.SystemColors.Control;
            this.ListUserOptions.FormattingEnabled = true;
            this.ListUserOptions.HorizontalScrollbar = true;
            this.ListUserOptions.Location = new System.Drawing.Point(15, 186);
            this.ListUserOptions.Name = "ListUserOptions";
            this.ListUserOptions.Size = new System.Drawing.Size(341, 94);
            this.ListUserOptions.TabIndex = 10;
            this.ListUserOptions.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.ListUserOptions_ItemCheck);
            //
            // groupBox1
            //
            this.groupBox1.Controls.Add(this.dateTimePicker);
            this.groupBox1.Controls.Add(this.rbEndOf);
            this.groupBox1.Controls.Add(this.rbNever);
            this.groupBox1.Location = new System.Drawing.Point(15, 286);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(341, 77);
            this.groupBox1.TabIndex = 11;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Account expires";
            //
            // dateTimePicker
            //
            this.dateTimePicker.Enabled = false;
            this.dateTimePicker.Location = new System.Drawing.Point(101, 37);
            this.dateTimePicker.Name = "dateTimePicker";
            this.dateTimePicker.Size = new System.Drawing.Size(232, 20);
            this.dateTimePicker.TabIndex = 2;
            this.dateTimePicker.ValueChanged += new System.EventHandler(this.dateTimePicker_ValueChanged);
            //
            // rbEndOf
            //
            this.rbEndOf.AutoSize = true;
            this.rbEndOf.Location = new System.Drawing.Point(10, 39);
            this.rbEndOf.Name = "rbEndOf";
            this.rbEndOf.Size = new System.Drawing.Size(62, 17);
            this.rbEndOf.TabIndex = 1;
            this.rbEndOf.Text = "&End of..";
            this.rbEndOf.UseVisualStyleBackColor = true;
            this.rbEndOf.CheckedChanged += new System.EventHandler(this.rbEndOf_CheckedChanged);
            //
            // rbNever
            //
            this.rbNever.AutoSize = true;
            this.rbNever.Checked = true;
            this.rbNever.Location = new System.Drawing.Point(10, 19);
            this.rbNever.Name = "rbNever";
            this.rbNever.Size = new System.Drawing.Size(54, 17);
            this.rbNever.TabIndex = 0;
            this.rbNever.TabStop = true;
            this.rbNever.Text = "Ne&ver";
            this.rbNever.UseVisualStyleBackColor = true;
            this.rbNever.CheckedChanged += new System.EventHandler(this.rbNever_CheckedChanged);
            //
            // UserAccountPage
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Name = "UserAccountPage";
            this.Size = new System.Drawing.Size(374, 444);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ComboBox cbDomain;
        private System.Windows.Forms.TextBox txtlogon;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox txtDomian;
        private System.Windows.Forms.TextBox txtpreLogonname;
        private System.Windows.Forms.Button btnLogonHours;
        private System.Windows.Forms.CheckedListBox ListUserOptions;
        private System.Windows.Forms.CheckBox cbAccLocked;
        private System.Windows.Forms.Button btnLogonTo;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.RadioButton rbNever;
        private System.Windows.Forms.RadioButton rbEndOf;
        private System.Windows.Forms.DateTimePicker dateTimePicker;
    }
}
