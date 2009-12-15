namespace Likewise.LMC.UtilityUIElements
{
    partial class PermissionEntry
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(PermissionEntry));
            this.tabPageObject = new System.Windows.Forms.TabPage();
            this.btnClearAll = new System.Windows.Forms.Button();
            this.chkInherit = new System.Windows.Forms.CheckBox();
            this.label6 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.dgPermissions = new System.Windows.Forms.DataGridView();
            this.colPermissionName = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.colAllow = new System.Windows.Forms.DataGridViewCheckBoxColumn();
            this.colDeny = new System.Windows.Forms.DataGridViewCheckBoxColumn();
            this.comboApplyTo = new System.Windows.Forms.ComboBox();
            this.label3 = new System.Windows.Forms.Label();
            this.btnChange = new System.Windows.Forms.Button();
            this.txtObjectName = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.tabControl = new System.Windows.Forms.TabControl();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.tabPageObject.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dgPermissions)).BeginInit();
            this.tabControl.SuspendLayout();
            this.SuspendLayout();
            //
            // tabPageObject
            //
            this.tabPageObject.Controls.Add(this.btnClearAll);
            this.tabPageObject.Controls.Add(this.chkInherit);
            this.tabPageObject.Controls.Add(this.label6);
            this.tabPageObject.Controls.Add(this.label5);
            this.tabPageObject.Controls.Add(this.label4);
            this.tabPageObject.Controls.Add(this.dgPermissions);
            this.tabPageObject.Controls.Add(this.comboApplyTo);
            this.tabPageObject.Controls.Add(this.label3);
            this.tabPageObject.Controls.Add(this.btnChange);
            this.tabPageObject.Controls.Add(this.txtObjectName);
            this.tabPageObject.Controls.Add(this.label2);
            this.tabPageObject.Controls.Add(this.label1);
            this.tabPageObject.Location = new System.Drawing.Point(4, 22);
            this.tabPageObject.Name = "tabPageObject";
            this.tabPageObject.Padding = new System.Windows.Forms.Padding(3);
            this.tabPageObject.Size = new System.Drawing.Size(357, 384);
            this.tabPageObject.TabIndex = 0;
            this.tabPageObject.Text = "Object";
            this.tabPageObject.UseVisualStyleBackColor = true;
            //
            // btnClearAll
            //
            this.btnClearAll.Enabled = false;
            this.btnClearAll.Location = new System.Drawing.Point(271, 350);
            this.btnClearAll.Name = "btnClearAll";
            this.btnClearAll.Size = new System.Drawing.Size(75, 23);
            this.btnClearAll.TabIndex = 11;
            this.btnClearAll.Text = "Clear All";
            this.btnClearAll.UseVisualStyleBackColor = true;
            //
            // chkInherit
            //
            this.chkInherit.AutoSize = true;
            this.chkInherit.Enabled = false;
            this.chkInherit.Location = new System.Drawing.Point(12, 346);
            this.chkInherit.Name = "chkInherit";
            this.chkInherit.Size = new System.Drawing.Size(225, 30);
            this.chkInherit.TabIndex = 10;
            this.chkInherit.Text = "Apply these permissions to objects and/or \r\ncontainers within this container only" +
                ".";
            this.chkInherit.UseVisualStyleBackColor = true;
            //
            // label6
            //
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(299, 85);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(32, 13);
            this.label6.TabIndex = 9;
            this.label6.Text = "Deny";
            //
            // label5
            //
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(231, 85);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(32, 13);
            this.label5.TabIndex = 8;
            this.label5.Text = "Allow";
            //
            // label4
            //
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(11, 85);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(62, 13);
            this.label4.TabIndex = 7;
            this.label4.Text = "Permissions";
            //
            // dgPermissions
            //
            this.dgPermissions.AllowUserToAddRows = false;
            this.dgPermissions.AllowUserToDeleteRows = false;
            this.dgPermissions.AllowUserToResizeRows = false;
            this.dgPermissions.BackgroundColor = System.Drawing.SystemColors.Window;
            this.dgPermissions.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dgPermissions.ColumnHeadersVisible = false;
            this.dgPermissions.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.colPermissionName,
            this.colAllow,
            this.colDeny});
            this.dgPermissions.Location = new System.Drawing.Point(11, 103);
            this.dgPermissions.MultiSelect = false;
            this.dgPermissions.Name = "dgPermissions";
            this.dgPermissions.RowHeadersVisible = false;
            this.dgPermissions.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.CellSelect;
            this.dgPermissions.Size = new System.Drawing.Size(336, 237);
            this.dgPermissions.TabIndex = 6;
            //
            // colPermissionName
            //
            this.colPermissionName.HeaderText = "Column1";
            this.colPermissionName.Name = "colPermissionName";
            this.colPermissionName.ReadOnly = true;
            this.colPermissionName.Width = 213;
            //
            // colAllow
            //
            this.colAllow.HeaderText = "Column1";
            this.colAllow.Name = "colAllow";
            this.colAllow.Width = 60;
            //
            // colDeny
            //
            this.colDeny.HeaderText = "Column1";
            this.colDeny.Name = "colDeny";
            this.colDeny.Width = 60;
            //
            // comboApplyTo
            //
            this.comboApplyTo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboApplyTo.Enabled = false;
            this.comboApplyTo.FormattingEnabled = true;
            this.comboApplyTo.Items.AddRange(new object[] {
            "This folders, subfolders and files"});
            this.comboApplyTo.Location = new System.Drawing.Point(72, 54);
            this.comboApplyTo.Name = "comboApplyTo";
            this.comboApplyTo.Size = new System.Drawing.Size(275, 21);
            this.comboApplyTo.TabIndex = 5;
            //
            // label3
            //
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(8, 58);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(60, 13);
            this.label3.TabIndex = 4;
            this.label3.Text = "Apply onto:";
            //
            // btnChange
            //
            this.btnChange.Enabled = false;
            this.btnChange.Location = new System.Drawing.Point(269, 25);
            this.btnChange.Name = "btnChange";
            this.btnChange.Size = new System.Drawing.Size(78, 23);
            this.btnChange.TabIndex = 3;
            this.btnChange.Text = "Change";
            this.btnChange.UseVisualStyleBackColor = true;
            //
            // txtObjectName
            //
            this.txtObjectName.BackColor = System.Drawing.SystemColors.Control;
            this.txtObjectName.Location = new System.Drawing.Point(49, 26);
            this.txtObjectName.Name = "txtObjectName";
            this.txtObjectName.ReadOnly = true;
            this.txtObjectName.Size = new System.Drawing.Size(210, 20);
            this.txtObjectName.TabIndex = 2;
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(7, 29);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(38, 13);
            this.label2.TabIndex = 1;
            this.label2.Text = "Name:";
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(8, 6);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(241, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "This permission is inherited from the parent object.";
            //
            // tabControl
            //
            this.tabControl.Controls.Add(this.tabPageObject);
            this.tabControl.Location = new System.Drawing.Point(5, 6);
            this.tabControl.Name = "tabControl";
            this.tabControl.SelectedIndex = 0;
            this.tabControl.Size = new System.Drawing.Size(365, 410);
            this.tabControl.TabIndex = 0;
            //
            // btnOK
            //
            this.btnOK.Location = new System.Drawing.Point(208, 423);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(75, 23);
            this.btnOK.TabIndex = 1;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            //
            // btnCancel
            //
            this.btnCancel.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(294, 423);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 2;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            //
            // PermissionEntry
            //
            this.AcceptButton = this.btnOK;
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(374, 452);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.tabControl);
            this.HelpButton = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "PermissionEntry";
            this.Text = "PermissionEntry ";
            this.tabPageObject.ResumeLayout(false);
            this.tabPageObject.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dgPermissions)).EndInit();
            this.tabControl.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabPage tabPageObject;
        private System.Windows.Forms.TabControl tabControl;
        private System.Windows.Forms.Button btnChange;
        private System.Windows.Forms.TextBox txtObjectName;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox comboApplyTo;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.DataGridView dgPermissions;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.CheckBox chkInherit;
        private System.Windows.Forms.Button btnClearAll;
        private System.Windows.Forms.DataGridViewTextBoxColumn colPermissionName;
        private System.Windows.Forms.DataGridViewCheckBoxColumn colAllow;
        private System.Windows.Forms.DataGridViewCheckBoxColumn colDeny;

    }
}