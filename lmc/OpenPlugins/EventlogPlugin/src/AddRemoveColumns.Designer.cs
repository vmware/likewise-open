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
    partial class AddRemoveColumnsForm
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
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnOk = new System.Windows.Forms.Button();
            this.lbAvailableColumns = new System.Windows.Forms.ListBox();
            this.lbDisplayedColumns = new System.Windows.Forms.ListBox();
            this.btnAdd = new System.Windows.Forms.Button();
            this.btnMoveup = new System.Windows.Forms.Button();
            this.btnRestoredefaults = new System.Windows.Forms.Button();
            this.btnMovedown = new System.Windows.Forms.Button();
            this.labelAvacol = new System.Windows.Forms.Label();
            this.lableDiscol = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.btnRemove = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(435, 266);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(89, 28);
            this.btnCancel.TabIndex = 16;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // btnOk
            // 
            this.btnOk.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnOk.Location = new System.Drawing.Point(338, 266);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(89, 28);
            this.btnOk.TabIndex = 15;
            this.btnOk.Text = "&OK";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            // 
            // lbAvailableColumns
            // 
            this.lbAvailableColumns.FormattingEnabled = true;
            this.lbAvailableColumns.HorizontalScrollbar = true;
            this.lbAvailableColumns.Location = new System.Drawing.Point(16, 39);
            this.lbAvailableColumns.Name = "lbAvailableColumns";
            this.lbAvailableColumns.ScrollAlwaysVisible = true;
            this.lbAvailableColumns.Size = new System.Drawing.Size(137, 199);
            this.lbAvailableColumns.TabIndex = 17;
            this.lbAvailableColumns.DoubleClick += new System.EventHandler(this.lbAvailableColumns_DoubleClick);
            // 
            // lbDisplayedColumns
            // 
            this.lbDisplayedColumns.FormattingEnabled = true;
            this.lbDisplayedColumns.HorizontalScrollbar = true;
            this.lbDisplayedColumns.Location = new System.Drawing.Point(258, 39);
            this.lbDisplayedColumns.Name = "lbDisplayedColumns";
            this.lbDisplayedColumns.ScrollAlwaysVisible = true;
            this.lbDisplayedColumns.Size = new System.Drawing.Size(137, 199);
            this.lbDisplayedColumns.TabIndex = 18;
            this.lbDisplayedColumns.SelectedIndexChanged += new System.EventHandler(this.lbDisplayedColumns_SelectedIndexChanged);
            this.lbDisplayedColumns.DoubleClick += new System.EventHandler(this.lbDisplayedColumns_DoubleClick);
            // 
            // btnAdd
            // 
            this.btnAdd.Location = new System.Drawing.Point(161, 92);
            this.btnAdd.Name = "btnAdd";
            this.btnAdd.Size = new System.Drawing.Size(91, 28);
            this.btnAdd.TabIndex = 19;
            this.btnAdd.Text = "A&dd ->";
            this.btnAdd.UseVisualStyleBackColor = true;
            this.btnAdd.Click += new System.EventHandler(this.btnAdd_Click);
            // 
            // btnMoveup
            // 
            this.btnMoveup.Location = new System.Drawing.Point(406, 53);
            this.btnMoveup.Name = "btnMoveup";
            this.btnMoveup.Size = new System.Drawing.Size(118, 28);
            this.btnMoveup.TabIndex = 21;
            this.btnMoveup.Text = "M&ove Up";
            this.btnMoveup.UseVisualStyleBackColor = true;
            this.btnMoveup.Click += new System.EventHandler(this.btnMoveup_Click);
            // 
            // btnRestoredefaults
            // 
            this.btnRestoredefaults.Location = new System.Drawing.Point(406, 130);
            this.btnRestoredefaults.Name = "btnRestoredefaults";
            this.btnRestoredefaults.Size = new System.Drawing.Size(118, 28);
            this.btnRestoredefaults.TabIndex = 22;
            this.btnRestoredefaults.Text = "Re&store Defaults";
            this.btnRestoredefaults.UseVisualStyleBackColor = true;
            this.btnRestoredefaults.Click += new System.EventHandler(this.btnRestoredefaults_Click);
            // 
            // btnMovedown
            // 
            this.btnMovedown.Location = new System.Drawing.Point(406, 92);
            this.btnMovedown.Name = "btnMovedown";
            this.btnMovedown.Size = new System.Drawing.Size(118, 28);
            this.btnMovedown.TabIndex = 23;
            this.btnMovedown.Text = "Mo&ve Down";
            this.btnMovedown.UseVisualStyleBackColor = true;
            this.btnMovedown.Click += new System.EventHandler(this.btnMovedown_Click);
            // 
            // labelAvacol
            // 
            this.labelAvacol.AutoSize = true;
            this.labelAvacol.Location = new System.Drawing.Point(17, 21);
            this.labelAvacol.Name = "labelAvacol";
            this.labelAvacol.Size = new System.Drawing.Size(95, 13);
            this.labelAvacol.TabIndex = 24;
            this.labelAvacol.Text = "&Available columns:";
            // 
            // lableDiscol
            // 
            this.lableDiscol.AutoSize = true;
            this.lableDiscol.Location = new System.Drawing.Point(258, 22);
            this.lableDiscol.Name = "lableDiscol";
            this.lableDiscol.Size = new System.Drawing.Size(98, 13);
            this.lableDiscol.TabIndex = 25;
            this.lableDiscol.Text = "Display&ed columns:";
            // 
            // groupBox1
            // 
            this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox1.BackColor = System.Drawing.Color.Transparent;
            this.groupBox1.Location = new System.Drawing.Point(16, 251);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(508, 5);
            this.groupBox1.TabIndex = 26;
            this.groupBox1.TabStop = false;
            // 
            // btnRemove
            // 
            this.btnRemove.Location = new System.Drawing.Point(162, 128);
            this.btnRemove.Name = "btnRemove";
            this.btnRemove.Size = new System.Drawing.Size(91, 28);
            this.btnRemove.TabIndex = 27;
            this.btnRemove.Text = "<- &Remove";
            this.btnRemove.UseVisualStyleBackColor = true;
            this.btnRemove.Click += new System.EventHandler(this.btnRemove_Click);
            // 
            // AddRemoveColumnsForm
            // 
            this.AcceptButton = this.btnOk;
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(536, 304);
            this.Controls.Add(this.btnRemove);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.lableDiscol);
            this.Controls.Add(this.labelAvacol);
            this.Controls.Add(this.btnMovedown);
            this.Controls.Add(this.btnRestoredefaults);
            this.Controls.Add(this.btnMoveup);
            this.Controls.Add(this.btnAdd);
            this.Controls.Add(this.lbDisplayedColumns);
            this.Controls.Add(this.lbAvailableColumns);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOk);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "AddRemoveColumnsForm";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Add/RemoveColumns";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.ListBox lbAvailableColumns;
        private System.Windows.Forms.ListBox lbDisplayedColumns;
        private System.Windows.Forms.Button btnAdd;
        private System.Windows.Forms.Button btnMoveup;
        private System.Windows.Forms.Button btnRestoredefaults;
        private System.Windows.Forms.Button btnMovedown;
        private System.Windows.Forms.Label labelAvacol;
        private System.Windows.Forms.Label lableDiscol;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Button btnRemove;
    }
}