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

namespace Likewise.LMC.Plugins.ServiceManagerPlugin
{
    partial class ServiceRecoveryPage
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
            this.label4 = new System.Windows.Forms.Label();
            this.cbFirstfailure = new System.Windows.Forms.ComboBox();
            this.cbSecondFailure = new System.Windows.Forms.ComboBox();
            this.cbSubsquentFailure = new System.Windows.Forms.ComboBox();
            this.label5 = new System.Windows.Forms.Label();
            this.txtDays = new System.Windows.Forms.TextBox();
            this.label7 = new System.Windows.Forms.Label();
            this.pnlResetDays = new System.Windows.Forms.Panel();
            this.pnlResetMinutes = new System.Windows.Forms.Panel();
            this.label8 = new System.Windows.Forms.Label();
            this.txtMinutes = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.txtProgram = new System.Windows.Forms.TextBox();
            this.btnBrowse = new System.Windows.Forms.Button();
            this.label10 = new System.Windows.Forms.Label();
            this.txtCommandlines = new System.Windows.Forms.TextBox();
            this.cbFailCount = new System.Windows.Forms.CheckBox();
            this.groupBoxRunProgram = new System.Windows.Forms.GroupBox();
            this.btnComputerOptions = new System.Windows.Forms.Button();
            this.pnlData.SuspendLayout();
            this.pnlResetDays.SuspendLayout();
            this.pnlResetMinutes.SuspendLayout();
            this.groupBoxRunProgram.SuspendLayout();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.btnComputerOptions);
            this.pnlData.Controls.Add(this.groupBoxRunProgram);
            this.pnlData.Controls.Add(this.pnlResetMinutes);
            this.pnlData.Controls.Add(this.pnlResetDays);
            this.pnlData.Controls.Add(this.cbSubsquentFailure);
            this.pnlData.Controls.Add(this.cbSecondFailure);
            this.pnlData.Controls.Add(this.cbFirstfailure);
            this.pnlData.Controls.Add(this.label4);
            this.pnlData.Controls.Add(this.label3);
            this.pnlData.Controls.Add(this.label2);
            this.pnlData.Controls.Add(this.label1);
            this.pnlData.Size = new System.Drawing.Size(406, 371);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(8, 15);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(243, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Select the computers response is this service fails.";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(10, 43);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(60, 13);
            this.label2.TabIndex = 1;
            this.label2.Text = "&First failure:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(9, 74);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(78, 13);
            this.label3.TabIndex = 2;
            this.label3.Text = "&Second failure:";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(9, 105);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(103, 13);
            this.label4.TabIndex = 3;
            this.label4.Text = "S&ubsequent failures:";
            // 
            // cbFirstfailure
            // 
            this.cbFirstfailure.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbFirstfailure.FormattingEnabled = true;
            this.cbFirstfailure.Location = new System.Drawing.Point(141, 40);
            this.cbFirstfailure.Name = "cbFirstfailure";
            this.cbFirstfailure.Size = new System.Drawing.Size(255, 21);
            this.cbFirstfailure.TabIndex = 4;
            this.cbFirstfailure.SelectedIndexChanged += new System.EventHandler(this.cbFirstfailure_SelectedIndexChanged);
            // 
            // cbSecondFailure
            // 
            this.cbSecondFailure.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbSecondFailure.FormattingEnabled = true;
            this.cbSecondFailure.Location = new System.Drawing.Point(140, 70);
            this.cbSecondFailure.Name = "cbSecondFailure";
            this.cbSecondFailure.Size = new System.Drawing.Size(257, 21);
            this.cbSecondFailure.TabIndex = 5;
            this.cbSecondFailure.SelectedIndexChanged += new System.EventHandler(this.cbFirstfailure_SelectedIndexChanged);
            // 
            // cbSubsquentFailure
            // 
            this.cbSubsquentFailure.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbSubsquentFailure.FormattingEnabled = true;
            this.cbSubsquentFailure.Location = new System.Drawing.Point(141, 102);
            this.cbSubsquentFailure.Name = "cbSubsquentFailure";
            this.cbSubsquentFailure.Size = new System.Drawing.Size(256, 21);
            this.cbSubsquentFailure.TabIndex = 6;
            this.cbSubsquentFailure.SelectedIndexChanged += new System.EventHandler(this.cbFirstfailure_SelectedIndexChanged);
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(4, 7);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(111, 13);
            this.label5.TabIndex = 12;
            this.label5.Text = "Reset fail count after: ";
            // 
            // txtDays
            // 
            this.txtDays.Location = new System.Drawing.Point(134, 5);
            this.txtDays.Name = "txtDays";
            this.txtDays.Size = new System.Drawing.Size(100, 20);
            this.txtDays.TabIndex = 13;
            this.txtDays.Text = "0";
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(236, 8);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(29, 13);
            this.label7.TabIndex = 14;
            this.label7.Text = "days";
            // 
            // pnlResetDays
            // 
            this.pnlResetDays.Controls.Add(this.label7);
            this.pnlResetDays.Controls.Add(this.txtDays);
            this.pnlResetDays.Controls.Add(this.label5);
            this.pnlResetDays.Location = new System.Drawing.Point(7, 131);
            this.pnlResetDays.Name = "pnlResetDays";
            this.pnlResetDays.Size = new System.Drawing.Size(390, 26);
            this.pnlResetDays.TabIndex = 12;
            // 
            // pnlResetMinutes
            // 
            this.pnlResetMinutes.Controls.Add(this.label8);
            this.pnlResetMinutes.Controls.Add(this.txtMinutes);
            this.pnlResetMinutes.Controls.Add(this.label6);
            this.pnlResetMinutes.Enabled = false;
            this.pnlResetMinutes.Location = new System.Drawing.Point(6, 167);
            this.pnlResetMinutes.Name = "pnlResetMinutes";
            this.pnlResetMinutes.Size = new System.Drawing.Size(391, 25);
            this.pnlResetMinutes.TabIndex = 13;
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(236, 6);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(43, 13);
            this.label8.TabIndex = 17;
            this.label8.Text = "minutes";
            // 
            // txtMinutes
            // 
            this.txtMinutes.Location = new System.Drawing.Point(134, 3);
            this.txtMinutes.Name = "txtMinutes";
            this.txtMinutes.Size = new System.Drawing.Size(100, 20);
            this.txtMinutes.TabIndex = 16;
            this.txtMinutes.Text = "1";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(4, 5);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(105, 13);
            this.label6.TabIndex = 15;
            this.label6.Text = "Restart service after:";
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(7, 17);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(49, 13);
            this.label9.TabIndex = 0;
            this.label9.Text = "&Program:";
            // 
            // txtProgram
            // 
            this.txtProgram.Location = new System.Drawing.Point(10, 34);
            this.txtProgram.Name = "txtProgram";
            this.txtProgram.Size = new System.Drawing.Size(274, 20);
            this.txtProgram.TabIndex = 1;
            // 
            // btnBrowse
            // 
            this.btnBrowse.Location = new System.Drawing.Point(292, 31);
            this.btnBrowse.Name = "btnBrowse";
            this.btnBrowse.Size = new System.Drawing.Size(90, 23);
            this.btnBrowse.TabIndex = 2;
            this.btnBrowse.Text = "&Browse";
            this.btnBrowse.UseVisualStyleBackColor = true;
            this.btnBrowse.Click += new System.EventHandler(this.btnBrowse_Click);
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(7, 68);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(131, 13);
            this.label10.TabIndex = 3;
            this.label10.Text = "&Command line parameters:";
            // 
            // txtCommandlines
            // 
            this.txtCommandlines.Location = new System.Drawing.Point(151, 67);
            this.txtCommandlines.Name = "txtCommandlines";
            this.txtCommandlines.Size = new System.Drawing.Size(204, 20);
            this.txtCommandlines.TabIndex = 4;
            // 
            // cbFailCount
            // 
            this.cbFailCount.AutoSize = true;
            this.cbFailCount.Location = new System.Drawing.Point(9, 96);
            this.cbFailCount.Name = "cbFailCount";
            this.cbFailCount.Size = new System.Drawing.Size(282, 17);
            this.cbFailCount.TabIndex = 5;
            this.cbFailCount.Text = "Append fail count to end of command line [/fail/=%1%]";
            this.cbFailCount.UseVisualStyleBackColor = true;
            // 
            // groupBoxRunProgram
            // 
            this.groupBoxRunProgram.Controls.Add(this.cbFailCount);
            this.groupBoxRunProgram.Controls.Add(this.txtCommandlines);
            this.groupBoxRunProgram.Controls.Add(this.label10);
            this.groupBoxRunProgram.Controls.Add(this.btnBrowse);
            this.groupBoxRunProgram.Controls.Add(this.txtProgram);
            this.groupBoxRunProgram.Controls.Add(this.label9);
            this.groupBoxRunProgram.Enabled = false;
            this.groupBoxRunProgram.Location = new System.Drawing.Point(10, 199);
            this.groupBoxRunProgram.Name = "groupBoxRunProgram";
            this.groupBoxRunProgram.Size = new System.Drawing.Size(389, 127);
            this.groupBoxRunProgram.TabIndex = 14;
            this.groupBoxRunProgram.TabStop = false;
            this.groupBoxRunProgram.Text = "Run Program";
            // 
            // btnComputerOptions
            // 
            this.btnComputerOptions.Enabled = false;
            this.btnComputerOptions.Location = new System.Drawing.Point(235, 338);
            this.btnComputerOptions.Name = "btnComputerOptions";
            this.btnComputerOptions.Size = new System.Drawing.Size(164, 23);
            this.btnComputerOptions.TabIndex = 15;
            this.btnComputerOptions.Text = "&Restart Computer Options...";
            this.btnComputerOptions.UseVisualStyleBackColor = true;
            this.btnComputerOptions.Click += new System.EventHandler(this.btnComputerOptions_Click);
            // 
            // ServiceRecoveryPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Name = "ServiceRecoveryPage";
            this.Size = new System.Drawing.Size(406, 371);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            this.pnlResetDays.ResumeLayout(false);
            this.pnlResetDays.PerformLayout();
            this.pnlResetMinutes.ResumeLayout(false);
            this.pnlResetMinutes.PerformLayout();
            this.groupBoxRunProgram.ResumeLayout(false);
            this.groupBoxRunProgram.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button btnComputerOptions;
        private System.Windows.Forms.GroupBox groupBoxRunProgram;
        private System.Windows.Forms.CheckBox cbFailCount;
        private System.Windows.Forms.TextBox txtCommandlines;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.Button btnBrowse;
        private System.Windows.Forms.TextBox txtProgram;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Panel pnlResetMinutes;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.TextBox txtMinutes;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Panel pnlResetDays;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.TextBox txtDays;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.ComboBox cbSubsquentFailure;
        private System.Windows.Forms.ComboBox cbSecondFailure;
        private System.Windows.Forms.ComboBox cbFirstfailure;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;


    }
}
