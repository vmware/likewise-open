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

namespace Likewise.LMC.Plugins.EventlogPlugin
{
    partial class EventFilterControl
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
            this.cbFailureAudit = new System.Windows.Forms.CheckBox();
            this.cbSuccessAudit = new System.Windows.Forms.CheckBox();
            this.tbEventSource = new System.Windows.Forms.TextBox();
            this.cbCategory = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.btnDefaults = new System.Windows.Forms.Button();
            this.tbComputer = new System.Windows.Forms.TextBox();
            this.tbUser = new System.Windows.Forms.TextBox();
            this.tbEventID = new System.Windows.Forms.TextBox();
            this.dtEnd = new System.Windows.Forms.DateTimePicker();
            this.dtStart = new System.Windows.Forms.DateTimePicker();
            this.label7 = new System.Windows.Forms.Label();
            this.rbRestrictDates = new System.Windows.Forms.RadioButton();
            this.cbInformation = new System.Windows.Forms.CheckBox();
            this.label6 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.cbError = new System.Windows.Forms.CheckBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.cbWarning = new System.Windows.Forms.CheckBox();
            this.rbShowAll = new System.Windows.Forms.RadioButton();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.tmEnd = new System.Windows.Forms.DateTimePicker();
            this.tmStart = new System.Windows.Forms.DateTimePicker();
            this.tbCustomFilterString = new System.Windows.Forms.TextBox();
            this.filterLabel = new System.Windows.Forms.Label();
            this.pnlData.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.filterLabel);
            this.pnlData.Controls.Add(this.tbCustomFilterString);
            this.pnlData.Controls.Add(this.tbEventSource);
            this.pnlData.Controls.Add(this.cbCategory);
            this.pnlData.Controls.Add(this.label1);
            this.pnlData.Controls.Add(this.btnDefaults);
            this.pnlData.Controls.Add(this.tbComputer);
            this.pnlData.Controls.Add(this.tbUser);
            this.pnlData.Controls.Add(this.tbEventID);
            this.pnlData.Controls.Add(this.label6);
            this.pnlData.Controls.Add(this.label5);
            this.pnlData.Controls.Add(this.label4);
            this.pnlData.Controls.Add(this.label2);
            this.pnlData.Controls.Add(this.groupBox2);
            this.pnlData.Controls.Add(this.groupBox1);
            this.pnlData.Size = new System.Drawing.Size(365, 426);
            this.pnlData.TabIndex = 0;
            // 
            // cbFailureAudit
            // 
            this.cbFailureAudit.Checked = true;
            this.cbFailureAudit.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbFailureAudit.Location = new System.Drawing.Point(162, 42);
            this.cbFailureAudit.Name = "cbFailureAudit";
            this.cbFailureAudit.Size = new System.Drawing.Size(130, 17);
            this.cbFailureAudit.TabIndex = 4;
            this.cbFailureAudit.Text = "Failure Audit";
            this.cbFailureAudit.UseVisualStyleBackColor = true;
            this.cbFailureAudit.CheckedChanged += new System.EventHandler(this.SettingChanged);
            // 
            // cbSuccessAudit
            // 
            this.cbSuccessAudit.Checked = true;
            this.cbSuccessAudit.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbSuccessAudit.Location = new System.Drawing.Point(162, 19);
            this.cbSuccessAudit.Name = "cbSuccessAudit";
            this.cbSuccessAudit.Size = new System.Drawing.Size(130, 17);
            this.cbSuccessAudit.TabIndex = 3;
            this.cbSuccessAudit.Text = "&Success Audit";
            this.cbSuccessAudit.UseVisualStyleBackColor = true;
            this.cbSuccessAudit.CheckedChanged += new System.EventHandler(this.SettingChanged);
            // 
            // tbEventSource
            // 
            this.tbEventSource.Location = new System.Drawing.Point(107, 146);
            this.tbEventSource.Name = "tbEventSource";
            this.tbEventSource.Size = new System.Drawing.Size(250, 20);
            this.tbEventSource.TabIndex = 3;
            this.tbEventSource.TextChanged += new System.EventHandler(this.SettingChanged);
            // 
            // cbCategory
            // 
            this.cbCategory.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbCategory.FormattingEnabled = true;
            this.cbCategory.Items.AddRange(new object[] {
            "(All)",
            "Devices",
            "Disk",
            "Network",
            "None",
            "Printers",
            "Services",
            "Shell",
            "System Event"});
            this.cbCategory.Location = new System.Drawing.Point(107, 175);
            this.cbCategory.Name = "cbCategory";
            this.cbCategory.Size = new System.Drawing.Size(250, 21);
            this.cbCategory.Sorted = true;
            this.cbCategory.TabIndex = 5;
            this.cbCategory.SelectedIndexChanged += new System.EventHandler(this.SettingChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(17, 178);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(52, 13);
            this.label1.TabIndex = 4;
            this.label1.Text = "Categor&y:";
            // 
            // btnDefaults
            // 
            this.btnDefaults.Location = new System.Drawing.Point(238, 8);
            this.btnDefaults.Name = "btnDefaults";
            this.btnDefaults.Size = new System.Drawing.Size(120, 23);
            this.btnDefaults.TabIndex = 0;
            this.btnDefaults.Text = "&Restore Defaults";
            this.btnDefaults.UseVisualStyleBackColor = true;
            this.btnDefaults.Click += new System.EventHandler(this.btnDefaults_Click);
            // 
            // tbComputer
            // 
            this.tbComputer.Location = new System.Drawing.Point(107, 260);
            this.tbComputer.Name = "tbComputer";
            this.tbComputer.Size = new System.Drawing.Size(250, 20);
            this.tbComputer.TabIndex = 11;
            this.tbComputer.TextChanged += new System.EventHandler(this.SettingChanged);
            // 
            // tbUser
            // 
            this.tbUser.Location = new System.Drawing.Point(107, 232);
            this.tbUser.Name = "tbUser";
            this.tbUser.Size = new System.Drawing.Size(250, 20);
            this.tbUser.TabIndex = 9;
            this.tbUser.TextChanged += new System.EventHandler(this.SettingChanged);
            // 
            // tbEventID
            // 
            this.tbEventID.Location = new System.Drawing.Point(107, 205);
            this.tbEventID.Name = "tbEventID";
            this.tbEventID.Size = new System.Drawing.Size(250, 20);
            this.tbEventID.TabIndex = 7;
            this.tbEventID.TextChanged += new System.EventHandler(this.SettingChanged);
            this.tbEventID.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.tbEventID_KeyPress);
            // 
            // dtEnd
            // 
            this.dtEnd.Enabled = false;
            this.dtEnd.Format = System.Windows.Forms.DateTimePickerFormat.Short;
            this.dtEnd.Location = new System.Drawing.Point(101, 73);
            this.dtEnd.Name = "dtEnd";
            this.dtEnd.Size = new System.Drawing.Size(101, 20);
            this.dtEnd.TabIndex = 3;
            this.dtEnd.Value = new System.DateTime(2008, 8, 6, 0, 0, 0, 0);
            this.dtEnd.ValueChanged += new System.EventHandler(this.dtStart_ValueChanged);
            // 
            // dtStart
            // 
            this.dtStart.Enabled = false;
            this.dtStart.Format = System.Windows.Forms.DateTimePickerFormat.Short;
            this.dtStart.Location = new System.Drawing.Point(101, 47);
            this.dtStart.Name = "dtStart";
            this.dtStart.Size = new System.Drawing.Size(101, 20);
            this.dtStart.TabIndex = 2;
            this.dtStart.Value = new System.DateTime(2008, 8, 6, 0, 0, 0, 0);
            this.dtStart.ValueChanged += new System.EventHandler(this.dtStart_ValueChanged);
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(62, 76);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(25, 13);
            this.label7.TabIndex = 3;
            this.label7.Text = "and";
            // 
            // rbRestrictDates
            // 
            this.rbRestrictDates.AutoSize = true;
            this.rbRestrictDates.Location = new System.Drawing.Point(16, 49);
            this.rbRestrictDates.Name = "rbRestrictDates";
            this.rbRestrictDates.Size = new System.Drawing.Size(67, 17);
            this.rbRestrictDates.TabIndex = 1;
            this.rbRestrictDates.Text = "&Between";
            this.rbRestrictDates.UseVisualStyleBackColor = true;
            this.rbRestrictDates.CheckedChanged += new System.EventHandler(this.rbRestrictDates_CheckedChanged);
            // 
            // cbInformation
            // 
            this.cbInformation.Checked = true;
            this.cbInformation.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbInformation.Location = new System.Drawing.Point(15, 19);
            this.cbInformation.Name = "cbInformation";
            this.cbInformation.Size = new System.Drawing.Size(93, 17);
            this.cbInformation.TabIndex = 0;
            this.cbInformation.Text = "&Information";
            this.cbInformation.UseVisualStyleBackColor = true;
            this.cbInformation.CheckedChanged += new System.EventHandler(this.SettingChanged);
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(17, 263);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(55, 13);
            this.label6.TabIndex = 10;
            this.label6.Text = "Co&mputer:";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(17, 235);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(32, 13);
            this.label5.TabIndex = 8;
            this.label5.Text = "Us&er:";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(17, 208);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(52, 13);
            this.label4.TabIndex = 6;
            this.label4.Text = "Event I&D:";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(17, 147);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(73, 13);
            this.label2.TabIndex = 2;
            this.label2.Text = "&Event source:";
            // 
            // cbError
            // 
            this.cbError.Checked = true;
            this.cbError.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbError.Location = new System.Drawing.Point(15, 65);
            this.cbError.Name = "cbError";
            this.cbError.Size = new System.Drawing.Size(93, 17);
            this.cbError.TabIndex = 2;
            this.cbError.Text = "Err&or";
            this.cbError.UseVisualStyleBackColor = true;
            this.cbError.CheckedChanged += new System.EventHandler(this.SettingChanged);
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.cbFailureAudit);
            this.groupBox2.Controls.Add(this.cbSuccessAudit);
            this.groupBox2.Controls.Add(this.cbError);
            this.groupBox2.Controls.Add(this.cbWarning);
            this.groupBox2.Controls.Add(this.cbInformation);
            this.groupBox2.Location = new System.Drawing.Point(17, 40);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(344, 91);
            this.groupBox2.TabIndex = 1;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Event types";
            // 
            // cbWarning
            // 
            this.cbWarning.Checked = true;
            this.cbWarning.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbWarning.Location = new System.Drawing.Point(15, 42);
            this.cbWarning.Name = "cbWarning";
            this.cbWarning.Size = new System.Drawing.Size(93, 17);
            this.cbWarning.TabIndex = 1;
            this.cbWarning.Text = "&Warning";
            this.cbWarning.UseVisualStyleBackColor = true;
            this.cbWarning.CheckedChanged += new System.EventHandler(this.SettingChanged);
            // 
            // rbShowAll
            // 
            this.rbShowAll.AutoSize = true;
            this.rbShowAll.Checked = true;
            this.rbShowAll.Location = new System.Drawing.Point(16, 26);
            this.rbShowAll.Name = "rbShowAll";
            this.rbShowAll.Size = new System.Drawing.Size(65, 17);
            this.rbShowAll.TabIndex = 0;
            this.rbShowAll.TabStop = true;
            this.rbShowAll.Text = "Show &all";
            this.rbShowAll.UseVisualStyleBackColor = true;
            this.rbShowAll.CheckedChanged += new System.EventHandler(this.SettingChanged);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.tmEnd);
            this.groupBox1.Controls.Add(this.tmStart);
            this.groupBox1.Controls.Add(this.dtEnd);
            this.groupBox1.Controls.Add(this.dtStart);
            this.groupBox1.Controls.Add(this.label7);
            this.groupBox1.Controls.Add(this.rbRestrictDates);
            this.groupBox1.Controls.Add(this.rbShowAll);
            this.groupBox1.Location = new System.Drawing.Point(13, 312);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(345, 101);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Time/date range";
            // 
            // tmEnd
            // 
            this.tmEnd.Enabled = false;
            this.tmEnd.Format = System.Windows.Forms.DateTimePickerFormat.Time;
            this.tmEnd.Location = new System.Drawing.Point(214, 73);
            this.tmEnd.Name = "tmEnd";
            this.tmEnd.ShowUpDown = true;
            this.tmEnd.Size = new System.Drawing.Size(102, 20);
            this.tmEnd.TabIndex = 5;
            this.tmEnd.Value = new System.DateTime(2008, 8, 6, 0, 0, 0, 0);
            this.tmEnd.ValueChanged += new System.EventHandler(this.dtStart_ValueChanged);
            // 
            // tmStart
            // 
            this.tmStart.Enabled = false;
            this.tmStart.Format = System.Windows.Forms.DateTimePickerFormat.Time;
            this.tmStart.Location = new System.Drawing.Point(214, 46);
            this.tmStart.Name = "tmStart";
            this.tmStart.ShowUpDown = true;
            this.tmStart.Size = new System.Drawing.Size(102, 20);
            this.tmStart.TabIndex = 4;
            this.tmStart.Value = new System.DateTime(2008, 2, 4, 0, 0, 0, 0);
            this.tmStart.ValueChanged += new System.EventHandler(this.dtStart_ValueChanged);
            // 
            // tbCustomFilterString
            // 
            this.tbCustomFilterString.Location = new System.Drawing.Point(107, 286);
            this.tbCustomFilterString.Name = "tbCustomFilterString";
            this.tbCustomFilterString.Size = new System.Drawing.Size(250, 20);
            this.tbCustomFilterString.TabIndex = 13;
            this.tbCustomFilterString.Visible = false;
            this.tbCustomFilterString.TextChanged += new System.EventHandler(this.SettingChanged);
            // 
            // filterLabel
            // 
            this.filterLabel.AutoSize = true;
            this.filterLabel.Location = new System.Drawing.Point(18, 291);
            this.filterLabel.Name = "filterLabel";
            this.filterLabel.Size = new System.Drawing.Size(59, 13);
            this.filterLabel.TabIndex = 12;
            this.filterLabel.Text = "FilterString:";
            this.filterLabel.Visible = false;
            // 
            // EventFilterControl
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Name = "EventFilterControl";
            this.Size = new System.Drawing.Size(365, 426);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TextBox tbEventSource;
        private System.Windows.Forms.ComboBox cbCategory;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnDefaults;
        private System.Windows.Forms.TextBox tbComputer;
        private System.Windows.Forms.TextBox tbUser;
        private System.Windows.Forms.TextBox tbEventID;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.CheckBox cbFailureAudit;
        private System.Windows.Forms.CheckBox cbSuccessAudit;
        private System.Windows.Forms.CheckBox cbError;
        private System.Windows.Forms.CheckBox cbWarning;
        private System.Windows.Forms.CheckBox cbInformation;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.DateTimePicker dtEnd;
        private System.Windows.Forms.DateTimePicker dtStart;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.RadioButton rbRestrictDates;
        private System.Windows.Forms.RadioButton rbShowAll;
        private System.Windows.Forms.TextBox tbCustomFilterString;
        private System.Windows.Forms.Label filterLabel;
        private System.Windows.Forms.DateTimePicker tmEnd;
        private System.Windows.Forms.DateTimePicker tmStart;

    }
}
