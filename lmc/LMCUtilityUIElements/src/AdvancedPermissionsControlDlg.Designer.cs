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
            this.tabControl = new System.Windows.Forms.TabControl();
            this.tabPagePermissions = new System.Windows.Forms.TabPage();
            this.panel1 = new System.Windows.Forms.Panel();
            this.label2 = new System.Windows.Forms.Label();
            this.cbInherit = new System.Windows.Forms.CheckBox();
            this.btnPermissionsRemove = new System.Windows.Forms.Button();
            this.btnPermissionsEdit = new System.Windows.Forms.Button();
            this.btnPermissionsAdd = new System.Windows.Forms.Button();
            this.lvPermissions = new System.Windows.Forms.ListView();
            this.colType = new System.Windows.Forms.ColumnHeader();
            this.colName = new System.Windows.Forms.ColumnHeader();
            this.colPermissions = new System.Windows.Forms.ColumnHeader();
            this.colinheritedForm = new System.Windows.Forms.ColumnHeader();
            this.colApplyTo = new System.Windows.Forms.ColumnHeader();
            this.label1 = new System.Windows.Forms.Label();
            this.tabPageAuditing = new System.Windows.Forms.TabPage();
            this.panel2 = new System.Windows.Forms.Panel();
            this.cbAudit = new System.Windows.Forms.CheckBox();
            this.btnAuditRemove = new System.Windows.Forms.Button();
            this.btnAuditEdit = new System.Windows.Forms.Button();
            this.btnAuditAdd = new System.Windows.Forms.Button();
            this.label4 = new System.Windows.Forms.Label();
            this.lvAuditing = new System.Windows.Forms.ListView();
            this.colAuditType = new System.Windows.Forms.ColumnHeader();
            this.colAuditName = new System.Windows.Forms.ColumnHeader();
            this.colAuditAccess = new System.Windows.Forms.ColumnHeader();
            this.colAuditInheritedFrom = new System.Windows.Forms.ColumnHeader();
            this.label3 = new System.Windows.Forms.Label();
            this.tabPageOwner = new System.Windows.Forms.TabPage();
            this.LWlvOwner = new System.Windows.Forms.ListView();
            this.colOwnerName = new System.Windows.Forms.ColumnHeader();
            this.label7 = new System.Windows.Forms.Label();
            this.lblUsername = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.tabPageEffectivePermissions = new System.Windows.Forms.TabPage();
            this.checkedListviewPermissions = new System.Windows.Forms.CheckedListBox();
            this.label11 = new System.Windows.Forms.Label();
            this.btnEffectivePermissions = new System.Windows.Forms.Button();
            this.lblUserorGroup = new System.Windows.Forms.Label();
            this.label10 = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnApply = new System.Windows.Forms.Button();
            this.tabControl.SuspendLayout();
            this.tabPagePermissions.SuspendLayout();
            this.panel1.SuspendLayout();
            this.tabPageAuditing.SuspendLayout();
            this.panel2.SuspendLayout();
            this.tabPageOwner.SuspendLayout();
            this.tabPageEffectivePermissions.SuspendLayout();
            this.SuspendLayout();
            //
            // tabControl
            //
            this.tabControl.Controls.Add(this.tabPagePermissions);
            this.tabControl.Controls.Add(this.tabPageAuditing);
            this.tabControl.Controls.Add(this.tabPageOwner);
            this.tabControl.Controls.Add(this.tabPageEffectivePermissions);
            this.tabControl.Location = new System.Drawing.Point(4, 6);
            this.tabControl.Name = "tabControl";
            this.tabControl.SelectedIndex = 0;
            this.tabControl.Size = new System.Drawing.Size(621, 382);
            this.tabControl.TabIndex = 0;
            this.tabControl.SelectedIndexChanged += new System.EventHandler(this.tabControl_SelectedIndexChanged);
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
            // panel1
            //
            this.panel1.Controls.Add(this.label2);
            this.panel1.Controls.Add(this.cbInherit);
            this.panel1.Controls.Add(this.btnPermissionsRemove);
            this.panel1.Controls.Add(this.btnPermissionsEdit);
            this.panel1.Controls.Add(this.btnPermissionsAdd);
            this.panel1.Controls.Add(this.lvPermissions);
            this.panel1.Controls.Add(this.label1);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point(3, 3);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(607, 350);
            this.panel1.TabIndex = 0;
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
            // btnPermissionsRemove
            //
            this.btnPermissionsRemove.Location = new System.Drawing.Point(233, 242);
            this.btnPermissionsRemove.Name = "btnPermissionsRemove";
            this.btnPermissionsRemove.Size = new System.Drawing.Size(102, 23);
            this.btnPermissionsRemove.TabIndex = 4;
            this.btnPermissionsRemove.Text = "&Remove...";
            this.btnPermissionsRemove.UseVisualStyleBackColor = true;
            this.btnPermissionsRemove.Click += new System.EventHandler(this.btnPermissionsRemove_Click);
            //
            // btnPermissionsEdit
            //
            this.btnPermissionsEdit.Location = new System.Drawing.Point(122, 242);
            this.btnPermissionsEdit.Name = "btnPermissionsEdit";
            this.btnPermissionsEdit.Size = new System.Drawing.Size(102, 23);
            this.btnPermissionsEdit.TabIndex = 3;
            this.btnPermissionsEdit.Text = "&Edit...";
            this.btnPermissionsEdit.UseVisualStyleBackColor = true;
            this.btnPermissionsEdit.Click += new System.EventHandler(this.btnPermissionsEdit_Click);
            //
            // btnPermissionsAdd
            //
            this.btnPermissionsAdd.Location = new System.Drawing.Point(12, 242);
            this.btnPermissionsAdd.Name = "btnPermissionsAdd";
            this.btnPermissionsAdd.Size = new System.Drawing.Size(102, 23);
            this.btnPermissionsAdd.TabIndex = 2;
            this.btnPermissionsAdd.Text = "A&dd...";
            this.btnPermissionsAdd.UseVisualStyleBackColor = true;
            this.btnPermissionsAdd.Click += new System.EventHandler(this.btnPermissionsAdd_Click);
            //
            // lvPermissions
            //
            this.lvPermissions.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colType,
            this.colName,
            this.colPermissions,
            this.colinheritedForm,
            this.colApplyTo});
            this.lvPermissions.Location = new System.Drawing.Point(12, 68);
            this.lvPermissions.Name = "lvPermissions";
            this.lvPermissions.Size = new System.Drawing.Size(584, 165);
            this.lvPermissions.TabIndex = 1;
            this.lvPermissions.UseCompatibleStateImageBehavior = false;
            this.lvPermissions.View = System.Windows.Forms.View.Details;
            this.lvPermissions.SelectedIndexChanged += new System.EventHandler(this.lvPermissions_SelectedIndexChanged);
            //
            // colType
            //
            this.colType.Text = "Type";
            this.colType.Width = 50;
            //
            // colName
            //
            this.colName.Text = "Name";
            this.colName.Width = 225;
            //
            // colPermissions
            //
            this.colPermissions.Text = "Permission";
            this.colPermissions.Width = 100;
            //
            // colinheritedForm
            //
            this.colinheritedForm.Text = "Inherited From";
            this.colinheritedForm.Width = 135;
            //
            // colApplyTo
            //
            this.colApplyTo.Text = "Apply To";
            this.colApplyTo.Width = 70;
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
            // panel2
            //
            this.panel2.Controls.Add(this.cbAudit);
            this.panel2.Controls.Add(this.btnAuditRemove);
            this.panel2.Controls.Add(this.btnAuditEdit);
            this.panel2.Controls.Add(this.btnAuditAdd);
            this.panel2.Controls.Add(this.label4);
            this.panel2.Controls.Add(this.lvAuditing);
            this.panel2.Controls.Add(this.label3);
            this.panel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel2.Location = new System.Drawing.Point(3, 3);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(607, 350);
            this.panel2.TabIndex = 0;
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
            // label4
            //
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(10, 46);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(82, 13);
            this.label4.TabIndex = 8;
            this.label4.Text = "Auditing en&tries:";
            //
            // lvAuditing
            //
            this.lvAuditing.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colAuditType,
            this.colAuditName,
            this.colAuditAccess,
            this.colAuditInheritedFrom});
            this.lvAuditing.Location = new System.Drawing.Point(11, 64);
            this.lvAuditing.Name = "lvAuditing";
            this.lvAuditing.Size = new System.Drawing.Size(584, 165);
            this.lvAuditing.TabIndex = 7;
            this.lvAuditing.UseCompatibleStateImageBehavior = false;
            this.lvAuditing.View = System.Windows.Forms.View.Details;
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
            // tabPageOwner
            //
            this.tabPageOwner.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.tabPageOwner.Controls.Add(this.LWlvOwner);
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
            // LWlvOwner
            //
            this.LWlvOwner.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colOwnerName});
            this.LWlvOwner.Location = new System.Drawing.Point(13, 127);
            this.LWlvOwner.Name = "LWlvOwner";
            this.LWlvOwner.Size = new System.Drawing.Size(585, 175);
            this.LWlvOwner.TabIndex = 5;
            this.LWlvOwner.UseCompatibleStateImageBehavior = false;
            this.LWlvOwner.View = System.Windows.Forms.View.Details;
            //
            // colOwnerName
            //
            this.colOwnerName.Text = "Name";
            this.colOwnerName.Width = 580;
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
            // lblUsername
            //
            this.lblUsername.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.lblUsername.Location = new System.Drawing.Point(13, 65);
            this.lblUsername.Name = "lblUsername";
            this.lblUsername.Size = new System.Drawing.Size(563, 23);
            this.lblUsername.TabIndex = 3;
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
            // label5
            //
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(10, 15);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(370, 13);
            this.label5.TabIndex = 0;
            this.label5.Text = "You can take ownership of an object if you have the appropriate permissions.";
            //
            // tabPageEffectivePermissions
            //
            this.tabPageEffectivePermissions.Controls.Add(this.checkedListviewPermissions);
            this.tabPageEffectivePermissions.Controls.Add(this.label11);
            this.tabPageEffectivePermissions.Controls.Add(this.btnEffectivePermissions);
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
            // checkedListviewPermissions
            //
            this.checkedListviewPermissions.FormattingEnabled = true;
            this.checkedListviewPermissions.Location = new System.Drawing.Point(14, 125);
            this.checkedListviewPermissions.Name = "checkedListviewPermissions";
            this.checkedListviewPermissions.Size = new System.Drawing.Size(584, 214);
            this.checkedListviewPermissions.TabIndex = 10;
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
            // btnEffectivePermissions
            //
            this.btnEffectivePermissions.Location = new System.Drawing.Point(523, 69);
            this.btnEffectivePermissions.Name = "btnEffectivePermissions";
            this.btnEffectivePermissions.Size = new System.Drawing.Size(75, 25);
            this.btnEffectivePermissions.TabIndex = 8;
            this.btnEffectivePermissions.Text = "&Select...";
            this.btnEffectivePermissions.UseVisualStyleBackColor = true;
            this.btnEffectivePermissions.Click += new System.EventHandler(this.btnEffectivePermissions_Click);
            //
            // lblUserorGroup
            //
            this.lblUserorGroup.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.lblUserorGroup.Location = new System.Drawing.Point(14, 70);
            this.lblUserorGroup.Name = "lblUserorGroup";
            this.lblUserorGroup.Size = new System.Drawing.Size(503, 23);
            this.lblUserorGroup.TabIndex = 4;
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
            // label9
            //
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(12, 53);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(103, 13);
            this.label9.TabIndex = 1;
            this.label9.Text = "&Group or user name:";
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
            // btnOK
            //
            this.btnOK.Location = new System.Drawing.Point(384, 396);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(75, 23);
            this.btnOK.TabIndex = 7;
            this.btnOK.Text = "&OK";
            this.btnOK.UseVisualStyleBackColor = true;
            //
            // btnCancel
            //
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(465, 396);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 6;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
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
            // AdvancedPermissionsControlDlg
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.ClientSize = new System.Drawing.Size(627, 430);
            this.Controls.Add(this.btnApply);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.tabControl);
            this.HelpButton = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "AdvancedPermissionsControlDlg";
            this.Text = "Advanced Security Settings for {0}";
            this.Load += new System.EventHandler(this.AdvancedPermissionsControlDlg_Load);
            this.tabControl.ResumeLayout(false);
            this.tabPagePermissions.ResumeLayout(false);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.tabPageAuditing.ResumeLayout(false);
            this.panel2.ResumeLayout(false);
            this.panel2.PerformLayout();
            this.tabPageOwner.ResumeLayout(false);
            this.tabPageOwner.PerformLayout();
            this.tabPageEffectivePermissions.ResumeLayout(false);
            this.tabPageEffectivePermissions.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabControl tabControl;
        private System.Windows.Forms.TabPage tabPagePermissions;
        private System.Windows.Forms.TabPage tabPageAuditing;
        public System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.TabPage tabPageOwner;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.ListView lvPermissions;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TabPage tabPageEffectivePermissions;
        private System.Windows.Forms.ColumnHeader colType;
        private System.Windows.Forms.ColumnHeader colName;
        private System.Windows.Forms.ColumnHeader colPermissions;
        private System.Windows.Forms.ColumnHeader colinheritedForm;
        private System.Windows.Forms.CheckBox cbInherit;
        private System.Windows.Forms.Button btnPermissionsRemove;
        private System.Windows.Forms.Button btnPermissionsEdit;
        private System.Windows.Forms.Button btnPermissionsAdd;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.ListView lvAuditing;
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
        private System.Windows.Forms.ListView LWlvOwner;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label lblUsername;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.ColumnHeader colOwnerName;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label label11;
        public System.Windows.Forms.Button btnEffectivePermissions;
        private System.Windows.Forms.Label lblUserorGroup;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.CheckedListBox checkedListviewPermissions;
        private System.Windows.Forms.Label label10;
        public System.Windows.Forms.Button btnApply;
        private System.Windows.Forms.ColumnHeader colApplyTo;
    }
}