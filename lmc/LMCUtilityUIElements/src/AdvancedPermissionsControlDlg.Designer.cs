namespace Likewise.LMC.UtilityUIElements
{
    partial class AdvancedPermissionsControlDlg
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AdvancedPermissionsControlDlg));
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.tabPagePermissions = new System.Windows.Forms.TabPage();
            this.tabPageAuditing = new System.Windows.Forms.TabPage();
            this.OKBtn = new System.Windows.Forms.Button();
            this.CancelBtn = new System.Windows.Forms.Button();
            this.tabPageOwner = new System.Windows.Forms.TabPage();
            this.tabPageEffectivePermissions = new System.Windows.Forms.TabPage();
            this.panel1 = new System.Windows.Forms.Panel();
            this.label1 = new System.Windows.Forms.Label();
            this.listView1 = new System.Windows.Forms.ListView();
            this.colType = new System.Windows.Forms.ColumnHeader();
            this.colName = new System.Windows.Forms.ColumnHeader();
            this.colPermissions = new System.Windows.Forms.ColumnHeader();
            this.colinheritedForm = new System.Windows.Forms.ColumnHeader();
            this.btnAdd = new System.Windows.Forms.Button();
            this.button1 = new System.Windows.Forms.Button();
            this.button2 = new System.Windows.Forms.Button();
            this.cbInherit = new System.Windows.Forms.CheckBox();
            this.label2 = new System.Windows.Forms.Label();
            this.panel2 = new System.Windows.Forms.Panel();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.listView2 = new System.Windows.Forms.ListView();
            this.colAuditType = new System.Windows.Forms.ColumnHeader();
            this.colAuditName = new System.Windows.Forms.ColumnHeader();
            this.colAuditAccess = new System.Windows.Forms.ColumnHeader();
            this.colAuditInheritedFrom = new System.Windows.Forms.ColumnHeader();
            this.cbAudit = new System.Windows.Forms.CheckBox();
            this.btnAuditRemove = new System.Windows.Forms.Button();
            this.btnAuditEdit = new System.Windows.Forms.Button();
            this.btnAuditAdd = new System.Windows.Forms.Button();
            this.label5 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.lblUsername = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.listView3 = new System.Windows.Forms.ListView();
            this.colOwnerName = new System.Windows.Forms.ColumnHeader();
            this.label8 = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.lblUserorGroup = new System.Windows.Forms.Label();
            this.button6 = new System.Windows.Forms.Button();
            this.label11 = new System.Windows.Forms.Label();
            this.checkedListBox1 = new System.Windows.Forms.CheckedListBox();
            this.label10 = new System.Windows.Forms.Label();
            this.btnApply = new System.Windows.Forms.Button();
            this.tabControl1.SuspendLayout();
            this.tabPagePermissions.SuspendLayout();
            this.tabPageAuditing.SuspendLayout();
            this.tabPageOwner.SuspendLayout();
            this.tabPageEffectivePermissions.SuspendLayout();
            this.panel1.SuspendLayout();
            this.panel2.SuspendLayout();
            this.SuspendLayout();
            //
            // tabControl1
            //
            this.tabControl1.Controls.Add(this.tabPagePermissions);
            this.tabControl1.Controls.Add(this.tabPageAuditing);
            this.tabControl1.Controls.Add(this.tabPageOwner);
            this.tabControl1.Controls.Add(this.tabPageEffectivePermissions);
            this.tabControl1.Location = new System.Drawing.Point(4, 6);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(621, 382);
            this.tabControl1.TabIndex = 0;
            //
            // tabPagePermissions
            //
            this.tabPagePermissions.Controls.Add(this.panel1);
            this.tabPagePermissions.Location = new System.Drawing.Point(4, 22);
            this.tabPagePermissions.Name = "tabPagePermissions";
            this.tabPagePermissions.Padding = new System.Windows.Forms.Padding(3);
            this.tabPagePermissions.Size = new System.Drawing.Size(613, 356);
            this.tabPagePermissions.TabIndex = 0;
            this.tabPagePermissions.Text = "Permissions";
            this.tabPagePermissions.UseVisualStyleBackColor = true;
            //
            // tabPageAuditing
            //
            this.tabPageAuditing.Controls.Add(this.panel2);
            this.tabPageAuditing.Location = new System.Drawing.Point(4, 22);
            this.tabPageAuditing.Name = "tabPageAuditing";
            this.tabPageAuditing.Padding = new System.Windows.Forms.Padding(3);
            this.tabPageAuditing.Size = new System.Drawing.Size(613, 356);
            this.tabPageAuditing.TabIndex = 1;
            this.tabPageAuditing.Text = "Auditing";
            this.tabPageAuditing.UseVisualStyleBackColor = true;
            //
            // OKBtn
            //
            this.OKBtn.Location = new System.Drawing.Point(384, 396);
            this.OKBtn.Name = "OKBtn";
            this.OKBtn.Size = new System.Drawing.Size(75, 23);
            this.OKBtn.TabIndex = 7;
            this.OKBtn.Text = "&OK";
            this.OKBtn.UseVisualStyleBackColor = true;
            //
            // CancelBtn
            //
            this.CancelBtn.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.CancelBtn.Location = new System.Drawing.Point(465, 396);
            this.CancelBtn.Name = "CancelBtn";
            this.CancelBtn.Size = new System.Drawing.Size(75, 23);
            this.CancelBtn.TabIndex = 6;
            this.CancelBtn.Text = "&Cancel";
            this.CancelBtn.UseVisualStyleBackColor = true;
            //
            // tabPageOwner
            //
            this.tabPageOwner.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.tabPageOwner.Controls.Add(this.listView3);
            this.tabPageOwner.Controls.Add(this.label7);
            this.tabPageOwner.Controls.Add(this.lblUsername);
            this.tabPageOwner.Controls.Add(this.label6);
            this.tabPageOwner.Controls.Add(this.label5);
            this.tabPageOwner.Location = new System.Drawing.Point(4, 22);
            this.tabPageOwner.Name = "tabPageOwner";
            this.tabPageOwner.Size = new System.Drawing.Size(613, 356);
            this.tabPageOwner.TabIndex = 2;
            this.tabPageOwner.Text = "Owner";
            this.tabPageOwner.UseVisualStyleBackColor = true;
            //
            // tabPageEffectivePermissions
            //
            this.tabPageEffectivePermissions.Controls.Add(this.checkedListBox1);
            this.tabPageEffectivePermissions.Controls.Add(this.label11);
            this.tabPageEffectivePermissions.Controls.Add(this.button6);
            this.tabPageEffectivePermissions.Controls.Add(this.lblUserorGroup);
            this.tabPageEffectivePermissions.Controls.Add(this.label10);
            this.tabPageEffectivePermissions.Controls.Add(this.label9);
            this.tabPageEffectivePermissions.Controls.Add(this.label8);
            this.tabPageEffectivePermissions.Location = new System.Drawing.Point(4, 22);
            this.tabPageEffectivePermissions.Name = "tabPageEffectivePermissions";
            this.tabPageEffectivePermissions.Size = new System.Drawing.Size(613, 356);
            this.tabPageEffectivePermissions.TabIndex = 3;
            this.tabPageEffectivePermissions.Text = "Effective Permissions";
            this.tabPageEffectivePermissions.UseVisualStyleBackColor = true;
            //
            // panel1
            //
            this.panel1.Controls.Add(this.label2);
            this.panel1.Controls.Add(this.cbInherit);
            this.panel1.Controls.Add(this.button2);
            this.panel1.Controls.Add(this.button1);
            this.panel1.Controls.Add(this.btnAdd);
            this.panel1.Controls.Add(this.listView1);
            this.panel1.Controls.Add(this.label1);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point(3, 3);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(607, 350);
            this.panel1.TabIndex = 0;
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(9, 15);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(468, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "To view more information about Special permissions, select a permission entry, an" +
                "d then click Edit.";
            //
            // listView1
            //
            this.listView1.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colType,
            this.colName,
            this.colPermissions,
            this.colinheritedForm});
            this.listView1.Location = new System.Drawing.Point(12, 68);
            this.listView1.Name = "listView1";
            this.listView1.Size = new System.Drawing.Size(584, 165);
            this.listView1.TabIndex = 1;
            this.listView1.UseCompatibleStateImageBehavior = false;
            this.listView1.View = System.Windows.Forms.View.Details;
            //
            // colType
            //
            this.colType.Text = "Type";
            this.colType.Width = 75;
            //
            // colName
            //
            this.colName.Text = "Name";
            this.colName.Width = 250;
            //
            // colPermissions
            //
            this.colPermissions.Text = "Permission";
            this.colPermissions.Width = 120;
            //
            // colinheritedForm
            //
            this.colinheritedForm.Text = "Inherited From";
            this.colinheritedForm.Width = 135;
            //
            // btnAdd
            //
            this.btnAdd.Location = new System.Drawing.Point(12, 242);
            this.btnAdd.Name = "btnAdd";
            this.btnAdd.Size = new System.Drawing.Size(102, 23);
            this.btnAdd.TabIndex = 2;
            this.btnAdd.Text = "A&dd...";
            this.btnAdd.UseVisualStyleBackColor = true;
            //
            // button1
            //
            this.button1.Location = new System.Drawing.Point(122, 242);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(102, 23);
            this.button1.TabIndex = 3;
            this.button1.Text = "&Edit...";
            this.button1.UseVisualStyleBackColor = true;
            //
            // button2
            //
            this.button2.Location = new System.Drawing.Point(233, 242);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(102, 23);
            this.button2.TabIndex = 4;
            this.button2.Text = "&Remove...";
            this.button2.UseVisualStyleBackColor = true;
            //
            // cbInherit
            //
            this.cbInherit.AutoSize = true;
            this.cbInherit.Location = new System.Drawing.Point(12, 277);
            this.cbInherit.Name = "cbInherit";
            this.cbInherit.Size = new System.Drawing.Size(571, 17);
            this.cbInherit.TabIndex = 5;
            this.cbInherit.Text = "Inherit from parent the permisison entries that apply to child objects. Include t" +
                "hese with entries explicitly defined here.";
            this.cbInherit.UseVisualStyleBackColor = true;
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(11, 48);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(99, 13);
            this.label2.TabIndex = 6;
            this.label2.Text = "Permissions en&tries:";
            //
            // panel2
            //
            this.panel2.Controls.Add(this.cbAudit);
            this.panel2.Controls.Add(this.btnAuditRemove);
            this.panel2.Controls.Add(this.btnAuditEdit);
            this.panel2.Controls.Add(this.btnAuditAdd);
            this.panel2.Controls.Add(this.label4);
            this.panel2.Controls.Add(this.listView2);
            this.panel2.Controls.Add(this.label3);
            this.panel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel2.Location = new System.Drawing.Point(3, 3);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(607, 350);
            this.panel2.TabIndex = 0;
            //
            // label3
            //
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(8, 16);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(479, 13);
            this.label3.TabIndex = 1;
            this.label3.Text = "To view more information about Special auditing entries, select an auditing entry" +
                ", and then click Edit.";
            //
            // label4
            //
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(10, 46);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(82, 13);
            this.label4.TabIndex = 8;
            this.label4.Text = "Auditing en&tries:";
            //
            // listView2
            //
            this.listView2.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colAuditType,
            this.colAuditName,
            this.colAuditAccess,
            this.colAuditInheritedFrom});
            this.listView2.Location = new System.Drawing.Point(11, 64);
            this.listView2.Name = "listView2";
            this.listView2.Size = new System.Drawing.Size(584, 165);
            this.listView2.TabIndex = 7;
            this.listView2.UseCompatibleStateImageBehavior = false;
            this.listView2.View = System.Windows.Forms.View.Details;
            //
            // colAuditType
            //
            this.colAuditType.Text = "Type";
            this.colAuditType.Width = 75;
            //
            // colAuditName
            //
            this.colAuditName.Text = "Name";
            this.colAuditName.Width = 250;
            //
            // colAuditAccess
            //
            this.colAuditAccess.Text = "Access";
            this.colAuditAccess.Width = 120;
            //
            // colAuditInheritedFrom
            //
            this.colAuditInheritedFrom.Text = "Inherited From";
            this.colAuditInheritedFrom.Width = 135;
            //
            // cbAudit
            //
            this.cbAudit.AutoSize = true;
            this.cbAudit.Location = new System.Drawing.Point(12, 277);
            this.cbAudit.Name = "cbAudit";
            this.cbAudit.Size = new System.Drawing.Size(559, 17);
            this.cbAudit.TabIndex = 12;
            this.cbAudit.Text = "Inherit from parent the auditing entries that apply to child objects. Include the" +
                "se with entries explicitly defined here.";
            this.cbAudit.UseVisualStyleBackColor = true;
            //
            // btnAuditRemove
            //
            this.btnAuditRemove.Location = new System.Drawing.Point(232, 238);
            this.btnAuditRemove.Name = "btnAuditRemove";
            this.btnAuditRemove.Size = new System.Drawing.Size(102, 23);
            this.btnAuditRemove.TabIndex = 11;
            this.btnAuditRemove.Text = "&Remove...";
            this.btnAuditRemove.UseVisualStyleBackColor = true;
            //
            // btnAuditEdit
            //
            this.btnAuditEdit.Location = new System.Drawing.Point(121, 238);
            this.btnAuditEdit.Name = "btnAuditEdit";
            this.btnAuditEdit.Size = new System.Drawing.Size(102, 23);
            this.btnAuditEdit.TabIndex = 10;
            this.btnAuditEdit.Text = "&Edit...";
            this.btnAuditEdit.UseVisualStyleBackColor = true;
            //
            // btnAuditAdd
            //
            this.btnAuditAdd.Location = new System.Drawing.Point(11, 238);
            this.btnAuditAdd.Name = "btnAuditAdd";
            this.btnAuditAdd.Size = new System.Drawing.Size(102, 23);
            this.btnAuditAdd.TabIndex = 9;
            this.btnAuditAdd.Text = "A&dd...";
            this.btnAuditAdd.UseVisualStyleBackColor = true;
            //
            // label5
            //
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(10, 15);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(370, 13);
            this.label5.TabIndex = 0;
            this.label5.Text = "You can take ownership of an object if you have the appropriate permissions.";
            //
            // label6
            //
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(10, 46);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(129, 13);
            this.label6.TabIndex = 2;
            this.label6.Text = "Current owner of this item:";
            //
            // lblUsername
            //
            this.lblUsername.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.lblUsername.Location = new System.Drawing.Point(13, 65);
            this.lblUsername.Name = "lblUsername";
            this.lblUsername.Size = new System.Drawing.Size(563, 23);
            this.lblUsername.TabIndex = 3;
            //
            // label7
            //
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(10, 108);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(91, 13);
            this.label7.TabIndex = 4;
            this.label7.Text = "Change &owner to:";
            //
            // listView3
            //
            this.listView3.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colOwnerName});
            this.listView3.Location = new System.Drawing.Point(13, 127);
            this.listView3.Name = "listView3";
            this.listView3.Size = new System.Drawing.Size(585, 175);
            this.listView3.TabIndex = 5;
            this.listView3.UseCompatibleStateImageBehavior = false;
            this.listView3.View = System.Windows.Forms.View.Details;
            //
            // colOwnerName
            //
            this.colOwnerName.Text = "Name";
            this.colOwnerName.Width = 580;
            //
            // label8
            //
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(11, 14);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(551, 26);
            this.label8.TabIndex = 0;
            this.label8.Text = "The following list displays the permissions that would be granted to the selected" +
                " group or user, based on all relevent \r\npermissions.";
            //
            // label9
            //
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(12, 53);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(103, 13);
            this.label9.TabIndex = 1;
            this.label9.Text = "&Group or user name:";
            //
            // lblUserorGroup
            //
            this.lblUserorGroup.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.lblUserorGroup.Location = new System.Drawing.Point(14, 70);
            this.lblUserorGroup.Name = "lblUserorGroup";
            this.lblUserorGroup.Size = new System.Drawing.Size(503, 23);
            this.lblUserorGroup.TabIndex = 4;
            //
            // button6
            //
            this.button6.Location = new System.Drawing.Point(523, 69);
            this.button6.Name = "button6";
            this.button6.Size = new System.Drawing.Size(75, 25);
            this.button6.TabIndex = 8;
            this.button6.Text = "&Select...";
            this.button6.UseVisualStyleBackColor = true;
            //
            // label11
            //
            this.label11.AutoSize = true;
            this.label11.Location = new System.Drawing.Point(12, 107);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(109, 13);
            this.label11.TabIndex = 9;
            this.label11.Text = "&Effective permissions:";
            //
            // checkedListBox1
            //
            this.checkedListBox1.FormattingEnabled = true;
            this.checkedListBox1.Items.AddRange(new object[] {
            "Full Control",
            "Traverse Folder / Execute File",
            "List Folder / Read Data",
            "Read Attributes",
            "Read Extended Attributes",
            "Create Files / Write Data",
            "Create Folders / Append Data",
            "Write Attributes",
            "Write Extended Attributes",
            "Delete",
            "Read Permissions",
            "Change Permissions",
            "Take Ownership"});
            this.checkedListBox1.Location = new System.Drawing.Point(14, 125);
            this.checkedListBox1.Name = "checkedListBox1";
            this.checkedListBox1.Size = new System.Drawing.Size(584, 214);
            this.checkedListBox1.TabIndex = 10;
            //
            // label10
            //
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(12, 14);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(41, 13);
            this.label10.TabIndex = 2;
            this.label10.Text = "label10";
            //
            // btnApply
            //
            this.btnApply.Location = new System.Drawing.Point(546, 395);
            this.btnApply.Name = "btnApply";
            this.btnApply.Size = new System.Drawing.Size(75, 23);
            this.btnApply.TabIndex = 8;
            this.btnApply.Text = "&Apply";
            this.btnApply.UseVisualStyleBackColor = true;
            //
            // AdvancedPermissionsControl
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.ClientSize = new System.Drawing.Size(627, 430);
            this.Controls.Add(this.btnApply);
            this.Controls.Add(this.OKBtn);
            this.Controls.Add(this.CancelBtn);
            this.Controls.Add(this.tabControl1);
            this.HelpButton = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "AdvancedPermissionsControl";
            this.Text = "Advanced Security Settings for {0}";
            this.tabControl1.ResumeLayout(false);
            this.tabPagePermissions.ResumeLayout(false);
            this.tabPageAuditing.ResumeLayout(false);
            this.tabPageOwner.ResumeLayout(false);
            this.tabPageOwner.PerformLayout();
            this.tabPageEffectivePermissions.ResumeLayout(false);
            this.tabPageEffectivePermissions.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.panel2.ResumeLayout(false);
            this.panel2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabControl tabControl1;
        private System.Windows.Forms.TabPage tabPagePermissions;
        private System.Windows.Forms.TabPage tabPageAuditing;
        public System.Windows.Forms.Button OKBtn;
        private System.Windows.Forms.Button CancelBtn;
        private System.Windows.Forms.TabPage tabPageOwner;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.ListView listView1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TabPage tabPageEffectivePermissions;
        private System.Windows.Forms.ColumnHeader colType;
        private System.Windows.Forms.ColumnHeader colName;
        private System.Windows.Forms.ColumnHeader colPermissions;
        private System.Windows.Forms.ColumnHeader colinheritedForm;
        private System.Windows.Forms.CheckBox cbInherit;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.Button btnAdd;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.ListView listView2;
        private System.Windows.Forms.ColumnHeader colAuditType;
        private System.Windows.Forms.ColumnHeader colAuditName;
        private System.Windows.Forms.ColumnHeader colAuditAccess;
        private System.Windows.Forms.ColumnHeader colAuditInheritedFrom;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.CheckBox cbAudit;
        private System.Windows.Forms.Button btnAuditRemove;
        private System.Windows.Forms.Button btnAuditEdit;
        private System.Windows.Forms.Button btnAuditAdd;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.ListView listView3;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label lblUsername;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.ColumnHeader colOwnerName;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label label11;
        public System.Windows.Forms.Button button6;
        private System.Windows.Forms.Label lblUserorGroup;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.CheckedListBox checkedListBox1;
        private System.Windows.Forms.Label label10;
        public System.Windows.Forms.Button btnApply;
    }
}