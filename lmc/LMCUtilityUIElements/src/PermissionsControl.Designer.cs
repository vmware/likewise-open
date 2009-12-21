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
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle2 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle3 = new System.Windows.Forms.DataGridViewCellStyle();
            this.lvGroupOrUserNames = new System.Windows.Forms.ListView();
            this.chName = new System.Windows.Forms.ColumnHeader();
            this.label1 = new System.Windows.Forms.Label();
            this.btnRemove = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.lblObjectName = new System.Windows.Forms.Label();
            this.btnAdd = new System.Windows.Forms.Button();
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
            this.chName});
            this.lvGroupOrUserNames.Location = new System.Drawing.Point(9, 48);
            this.lvGroupOrUserNames.MultiSelect = false;
            this.lvGroupOrUserNames.Name = "lvGroupOrUserNames";
            this.lvGroupOrUserNames.Size = new System.Drawing.Size(345, 115);
            this.lvGroupOrUserNames.TabIndex = 1;
            this.lvGroupOrUserNames.UseCompatibleStateImageBehavior = false;
            this.lvGroupOrUserNames.View = System.Windows.Forms.View.List;
            this.lvGroupOrUserNames.SelectedIndexChanged += new System.EventHandler(this.lvGroupOrUserNames_SelectedIndexChanged);
            //
            // chName
            //
            this.chName.Text = "";
            this.chName.Width = 300;
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
            this.btnRemove.Enabled = false;
            this.btnRemove.Location = new System.Drawing.Point(278, 170);
            this.btnRemove.Name = "btnRemove";
            this.btnRemove.Size = new System.Drawing.Size(75, 23);
            this.btnRemove.TabIndex = 3;
            this.btnRemove.Text = "Remove";
            this.btnRemove.UseVisualStyleBackColor = true;
            this.btnRemove.Click += new System.EventHandler(this.btnRemove_Click);
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
            this.btnAdd.Click += new System.EventHandler(this.btnAdd_Click);
            //
            // lblPermissions
            //
            this.lblPermissions.AutoSize = true;
            this.lblPermissions.Location = new System.Drawing.Point(8, 203);
            this.lblPermissions.Name = "lblPermissions";
            this.lblPermissions.Size = new System.Drawing.Size(97, 13);
            this.lblPermissions.TabIndex = 13;
            this.lblPermissions.Text = "Permissions for {0}.";
            //
            // DgPermissions
            //
            this.DgPermissions.AllowUserToAddRows = false;
            this.DgPermissions.AllowUserToDeleteRows = false;
            this.DgPermissions.BackgroundColor = System.Drawing.SystemColors.Window;
            this.DgPermissions.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            dataGridViewCellStyle1.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle1.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle1.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle1.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle1.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle1.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle1.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.DgPermissions.ColumnHeadersDefaultCellStyle = dataGridViewCellStyle1;
            this.DgPermissions.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.DgPermissions.ColumnHeadersVisible = false;
            this.DgPermissions.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.colPermission,
            this.colAllow,
            this.colDeny});
            dataGridViewCellStyle2.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle2.BackColor = System.Drawing.SystemColors.Window;
            dataGridViewCellStyle2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle2.ForeColor = System.Drawing.SystemColors.ControlText;
            dataGridViewCellStyle2.SelectionBackColor = System.Drawing.Color.Transparent;
            dataGridViewCellStyle2.SelectionForeColor = System.Drawing.SystemColors.ControlText;
            dataGridViewCellStyle2.WrapMode = System.Windows.Forms.DataGridViewTriState.False;
            this.DgPermissions.DefaultCellStyle = dataGridViewCellStyle2;
            this.DgPermissions.GridColor = System.Drawing.SystemColors.Window;
            this.DgPermissions.Location = new System.Drawing.Point(9, 220);
            this.DgPermissions.Name = "DgPermissions";
            dataGridViewCellStyle3.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle3.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle3.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle3.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle3.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle3.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle3.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.DgPermissions.RowHeadersDefaultCellStyle = dataGridViewCellStyle3;
            this.DgPermissions.RowHeadersVisible = false;
            this.DgPermissions.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.DgPermissions.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.CellSelect;
            this.DgPermissions.Size = new System.Drawing.Size(345, 141);
            this.DgPermissions.TabIndex = 16;
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
            this.colAllow.Width = 63;
            //
            // colDeny
            //
            this.colDeny.HeaderText = "Column3";
            this.colDeny.Name = "colDeny";
            this.colDeny.Width = 63;
            //
            // PermissionsControl
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Controls.Add(this.DgPermissions);
            this.Controls.Add(this.lblPermissions);
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
        private System.Windows.Forms.ColumnHeader chName;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnRemove;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label lblObjectName;
        private System.Windows.Forms.Button btnAdd;
        private System.Windows.Forms.Label lblPermissions;
        private System.Windows.Forms.DataGridView DgPermissions;
        private System.Windows.Forms.DataGridViewTextBoxColumn colPermission;
        private System.Windows.Forms.DataGridViewCheckBoxColumn colAllow;
        private System.Windows.Forms.DataGridViewCheckBoxColumn colDeny;

    }
}
