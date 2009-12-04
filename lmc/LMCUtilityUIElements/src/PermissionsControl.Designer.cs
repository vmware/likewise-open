namespace Likewise.LMC.UtilityUIElements
{
    partial class PermissionsControl
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
            this.btnRemove = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.lblObjectName = new System.Windows.Forms.Label();
            this.btnAdd = new System.Windows.Forms.Button();
            this.label4 = new System.Windows.Forms.Label();
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
            this.lvGroupOrUserNames.Location = new System.Drawing.Point(9, 48);
            this.lvGroupOrUserNames.Name = "lvGroupOrUserNames";
            this.lvGroupOrUserNames.Size = new System.Drawing.Size(345, 115);
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
            this.label1.Location = new System.Drawing.Point(6, 28);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(108, 13);
            this.label1.TabIndex = 2;
            this.label1.Text = "Group or user names:";
            //
            // btnRemove
            //
            this.btnRemove.Location = new System.Drawing.Point(278, 170);
            this.btnRemove.Name = "btnRemove";
            this.btnRemove.Size = new System.Drawing.Size(75, 23);
            this.btnRemove.TabIndex = 3;
            this.btnRemove.Text = "Remove";
            this.btnRemove.UseVisualStyleBackColor = true;
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(6, 176);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(168, 13);
            this.label2.TabIndex = 5;
            this.label2.Text = "To change permissions, click Edit.";
            //
            // label3
            //
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(6, 9);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(70, 13);
            this.label3.TabIndex = 6;
            this.label3.Text = "Object name:";
            //
            // lblObjectName
            //
            this.lblObjectName.AutoSize = true;
            this.lblObjectName.Location = new System.Drawing.Point(82, 9);
            this.lblObjectName.Name = "lblObjectName";
            this.lblObjectName.Size = new System.Drawing.Size(158, 13);
            this.lblObjectName.TabIndex = 7;
            this.lblObjectName.Text = "An Object with Security Settings";
            //
            // btnAdd
            //
            this.btnAdd.Location = new System.Drawing.Point(197, 170);
            this.btnAdd.Name = "btnAdd";
            this.btnAdd.Size = new System.Drawing.Size(75, 23);
            this.btnAdd.TabIndex = 12;
            this.btnAdd.Text = "Add...";
            this.btnAdd.UseVisualStyleBackColor = true;
            //
            // label4
            //
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(8, 203);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(97, 13);
            this.label4.TabIndex = 13;
            this.label4.Text = "Permissions for {0}.";
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
            this.DgPermissions.Location = new System.Drawing.Point(9, 220);
            this.DgPermissions.Name = "DgPermissions";
            this.DgPermissions.Size = new System.Drawing.Size(345, 141);
            this.DgPermissions.TabIndex = 16;
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
            // PermissionsControl
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Controls.Add(this.DgPermissions);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.btnAdd);
            this.Controls.Add(this.lblObjectName);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.btnRemove);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.lvGroupOrUserNames);
            this.Name = "PermissionsControl";
            this.Size = new System.Drawing.Size(363, 370);
            this.Load += new System.EventHandler(this.PermissionsControl_Load);
            ((System.ComponentModel.ISupportInitialize)(this.DgPermissions)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ListView lvGroupOrUserNames;
        private System.Windows.Forms.ColumnHeader chIcon;
        private System.Windows.Forms.ColumnHeader chName;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnRemove;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label lblObjectName;
        private System.Windows.Forms.Button btnAdd;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.DataGridView DgPermissions;
        private System.Windows.Forms.DataGridViewTextBoxColumn colPermission;
        private System.Windows.Forms.DataGridViewCheckBoxColumn colAllow;
        private System.Windows.Forms.DataGridViewCheckBoxColumn colDeny;

    }
}
