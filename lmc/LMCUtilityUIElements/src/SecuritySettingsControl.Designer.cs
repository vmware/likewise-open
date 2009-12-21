namespace Likewise.LMC.UtilityUIElements
{
    partial class SecuritySettingsControl
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
            this.lvGroupOrUserNames = new System.Windows.Forms.ListView();
            this.chIcon = new System.Windows.Forms.ColumnHeader();
            this.chName = new System.Windows.Forms.ColumnHeader();
            this.label1 = new System.Windows.Forms.Label();
            this.btnEdit = new System.Windows.Forms.Button();
            this.btnAdvanced = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.lblObjectName = new System.Windows.Forms.Label();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.lblPermissions = new System.Windows.Forms.Label();
            this.DgPermissions = new System.Windows.Forms.DataGridView();
            this.colPermission = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.colAllow = new System.Windows.Forms.DataGridViewCheckBoxColumn();
            this.colDeny = new System.Windows.Forms.DataGridViewCheckBoxColumn();
            ((System.ComponentModel.ISupportInitialize)(this.DgPermissions)).BeginInit();
            this.SuspendLayout();
            //
            // lvGroupOrUserNames
            //
            this.lvGroupOrUserNames.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.chIcon,
            this.chName});
            this.lvGroupOrUserNames.Location = new System.Drawing.Point(9, 47);
            this.lvGroupOrUserNames.Name = "lvGroupOrUserNames";
            this.lvGroupOrUserNames.Size = new System.Drawing.Size(345, 120);
            this.lvGroupOrUserNames.TabIndex = 1;
            this.lvGroupOrUserNames.UseCompatibleStateImageBehavior = false;
            this.lvGroupOrUserNames.View = System.Windows.Forms.View.List;
            this.lvGroupOrUserNames.SelectedIndexChanged += new System.EventHandler(this.lvGroupOrUserNames_SelectedIndexChanged);
            //
            // chIcon
            //
            this.chIcon.Text = "";
            //
            // chName
            //
            this.chName.Text = "";
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(6, 29);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(108, 13);
            this.label1.TabIndex = 2;
            this.label1.Text = "Group or user names:";
            //
            // btnEdit
            //
            this.btnEdit.Location = new System.Drawing.Point(277, 172);
            this.btnEdit.Name = "btnEdit";
            this.btnEdit.Size = new System.Drawing.Size(75, 23);
            this.btnEdit.TabIndex = 3;
            this.btnEdit.Text = "Edit...";
            this.btnEdit.UseVisualStyleBackColor = true;
            this.btnEdit.Click += new System.EventHandler(this.btnEdit_Click);
            //
            // btnAdvanced
            //
            this.btnAdvanced.Location = new System.Drawing.Point(279, 351);
            this.btnAdvanced.Name = "btnAdvanced";
            this.btnAdvanced.Size = new System.Drawing.Size(75, 23);
            this.btnAdvanced.TabIndex = 4;
            this.btnAdvanced.Text = "Advanced";
            this.btnAdvanced.UseVisualStyleBackColor = true;
            this.btnAdvanced.Click += new System.EventHandler(this.btnAdvanced_Click);
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(6, 174);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(168, 13);
            this.label2.TabIndex = 5;
            this.label2.Text = "To change permissions, click Edit.";
            //
            // label3
            //
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(6, 8);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(70, 13);
            this.label3.TabIndex = 6;
            this.label3.Text = "Object name:";
            //
            // lblObjectName
            //
            this.lblObjectName.AutoSize = true;
            this.lblObjectName.Location = new System.Drawing.Point(82, 8);
            this.lblObjectName.Name = "lblObjectName";
            this.lblObjectName.Size = new System.Drawing.Size(158, 13);
            this.lblObjectName.TabIndex = 7;
            this.lblObjectName.Text = "An Object with Security Settings";
            //
            // textBox1
            //
            this.textBox1.BackColor = System.Drawing.SystemColors.Control;
            this.textBox1.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.textBox1.Location = new System.Drawing.Point(9, 351);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(231, 32);
            this.textBox1.TabIndex = 11;
            this.textBox1.Text = "For special permissions or advanced settings click Advanced.";
            //
            // lblPermissions
            //
            this.lblPermissions.AutoSize = true;
            this.lblPermissions.Location = new System.Drawing.Point(6, 197);
            this.lblPermissions.Name = "lblPermissions";
            this.lblPermissions.Size = new System.Drawing.Size(97, 13);
            this.lblPermissions.TabIndex = 14;
            this.lblPermissions.Text = "Permissions for {0}.";
            //
            // DgPermissions
            //
            this.DgPermissions.AllowUserToAddRows = false;
            this.DgPermissions.AllowUserToDeleteRows = false;
            this.DgPermissions.BackgroundColor = System.Drawing.SystemColors.Window;
            this.DgPermissions.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.DgPermissions.ColumnHeadersVisible = false;
            this.DgPermissions.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.colPermission,
            this.colAllow,
            this.colDeny});
            this.DgPermissions.GridColor = System.Drawing.SystemColors.Window;
            this.DgPermissions.Location = new System.Drawing.Point(9, 216);
            this.DgPermissions.Name = "DgPermissions";
            this.DgPermissions.Size = new System.Drawing.Size(345, 127);
            this.DgPermissions.TabIndex = 15;
            this.DgPermissions.CellMouseUp += new System.Windows.Forms.DataGridViewCellMouseEventHandler(this.DgPermissions_CellMouseUp);
            //
            // colPermission
            //
            this.colPermission.HeaderText = "Column1";
            this.colPermission.Name = "colPermission";
            this.colPermission.ReadOnly = true;
            this.colPermission.Width = 175;
            //
            // colAllow
            //
            this.colAllow.HeaderText = "Column2";
            this.colAllow.Name = "colAllow";
            this.colAllow.ReadOnly = true;
            this.colAllow.Width = 63;
            //
            // colDeny
            //
            this.colDeny.HeaderText = "Column3";
            this.colDeny.Name = "colDeny";
            this.colDeny.ReadOnly = true;
            this.colDeny.Width = 63;
            //
            // SecuritySettingsControl
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Controls.Add(this.DgPermissions);
            this.Controls.Add(this.lblPermissions);
            this.Controls.Add(this.textBox1);
            this.Controls.Add(this.lblObjectName);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.btnAdvanced);
            this.Controls.Add(this.btnEdit);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.lvGroupOrUserNames);
            this.Name = "SecuritySettingsControl";
            this.Size = new System.Drawing.Size(364, 387);
            this.Load += new System.EventHandler(this.SecuritySettingsControl_Load);
            ((System.ComponentModel.ISupportInitialize)(this.DgPermissions)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ListView lvGroupOrUserNames;
        private System.Windows.Forms.ColumnHeader chIcon;
        private System.Windows.Forms.ColumnHeader chName;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnEdit;
        private System.Windows.Forms.Button btnAdvanced;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label lblObjectName;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.Label lblPermissions;
        private System.Windows.Forms.DataGridView DgPermissions;
        private System.Windows.Forms.DataGridViewTextBoxColumn colPermission;
        private System.Windows.Forms.DataGridViewCheckBoxColumn colAllow;
        private System.Windows.Forms.DataGridViewCheckBoxColumn colDeny;

    }
}
