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

using Likewise.LMC.Utilities;

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    partial class OthersStringEditor
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
            this.AttributeSelectionLabel = new System.Windows.Forms.Label();
            this.lblAttrValue = new System.Windows.Forms.Label();
            this.txtAttrValue = new System.Windows.Forms.TextBox();
            this.btnAdd = new System.Windows.Forms.Button();
            this.lblValues = new System.Windows.Forms.Label();
            this.buttonOk = new System.Windows.Forms.Button();
            this.buttonCancel = new System.Windows.Forms.Button();
            this.btnRemove = new System.Windows.Forms.Button();
            this.lblselectedAttr = new System.Windows.Forms.Label();
            this.listViewAttrValues = new Likewise.LMC.ServerControl.LWListView();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.btnEdit = new System.Windows.Forms.Button();
            this.chValues = new System.Windows.Forms.ColumnHeader();
            this.SuspendLayout();
            // 
            // AttributeSelectionLabel
            // 
            this.AttributeSelectionLabel.AutoSize = true;
            this.AttributeSelectionLabel.Location = new System.Drawing.Point(61, 13);
            this.AttributeSelectionLabel.Name = "AttributeSelectionLabel";
            this.AttributeSelectionLabel.Size = new System.Drawing.Size(0, 13);
            this.AttributeSelectionLabel.TabIndex = 1;
            // 
            // lblAttrValue
            // 
            this.lblAttrValue.AutoSize = true;
            this.lblAttrValue.Location = new System.Drawing.Point(6, 10);
            this.lblAttrValue.Name = "lblAttrValue";
            this.lblAttrValue.Size = new System.Drawing.Size(62, 13);
            this.lblAttrValue.TabIndex = 3;
            this.lblAttrValue.Text = "&New Value:";
            // 
            // txtAttrValue
            // 
            this.txtAttrValue.Location = new System.Drawing.Point(9, 29);
            this.txtAttrValue.Name = "txtAttrValue";
            this.txtAttrValue.Size = new System.Drawing.Size(262, 20);
            this.txtAttrValue.TabIndex = 4;
            this.txtAttrValue.TextChanged += new System.EventHandler(this.txtAttrValue_TextChanged);
            // 
            // btnAdd
            // 
            this.btnAdd.Enabled = false;
            this.btnAdd.Location = new System.Drawing.Point(279, 27);
            this.btnAdd.Name = "btnAdd";
            this.btnAdd.Size = new System.Drawing.Size(72, 22);
            this.btnAdd.TabIndex = 5;
            this.btnAdd.Text = "A&dd";
            this.btnAdd.UseVisualStyleBackColor = true;
            this.btnAdd.Click += new System.EventHandler(this.btnAdd_Click);
            // 
            // lblValues
            // 
            this.lblValues.AutoSize = true;
            this.lblValues.Location = new System.Drawing.Point(8, 58);
            this.lblValues.Name = "lblValues";
            this.lblValues.Size = new System.Drawing.Size(45, 13);
            this.lblValues.TabIndex = 6;
            this.lblValues.Text = "Val&ues :";
            // 
            // buttonOk
            // 
            this.buttonOk.Enabled = false;
            this.buttonOk.Location = new System.Drawing.Point(202, 263);
            this.buttonOk.Name = "buttonOk";
            this.buttonOk.Size = new System.Drawing.Size(71, 22);
            this.buttonOk.TabIndex = 9;
            this.buttonOk.Text = "&OK";
            this.buttonOk.UseVisualStyleBackColor = true;
            this.buttonOk.Click += new System.EventHandler(this.buttonOk_Click);
            // 
            // buttonCancel
            // 
            this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.buttonCancel.Location = new System.Drawing.Point(279, 264);
            this.buttonCancel.Name = "buttonCancel";
            this.buttonCancel.Size = new System.Drawing.Size(73, 22);
            this.buttonCancel.TabIndex = 10;
            this.buttonCancel.Text = "&Cancel";
            this.buttonCancel.UseVisualStyleBackColor = true;
            this.buttonCancel.Click += new System.EventHandler(this.buttonCancel_Click);
            // 
            // btnRemove
            // 
            this.btnRemove.Enabled = false;
            this.btnRemove.Location = new System.Drawing.Point(279, 102);
            this.btnRemove.Name = "btnRemove";
            this.btnRemove.Size = new System.Drawing.Size(72, 22);
            this.btnRemove.TabIndex = 8;
            this.btnRemove.Text = "&Remove";
            this.btnRemove.UseVisualStyleBackColor = true;
            this.btnRemove.Click += new System.EventHandler(this.btnRemove_Click);
            // 
            // lblselectedAttr
            // 
            this.lblselectedAttr.AutoSize = true;
            this.lblselectedAttr.Location = new System.Drawing.Point(65, 10);
            this.lblselectedAttr.Name = "lblselectedAttr";
            this.lblselectedAttr.Size = new System.Drawing.Size(0, 13);
            this.lblselectedAttr.TabIndex = 2;
            // 
            // listViewAttrValues
            // 
            this.listViewAttrValues.AutoArrange = false;
            this.listViewAttrValues.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.chValues});
            this.listViewAttrValues.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.None;
            this.listViewAttrValues.HideSelection = false;
            this.listViewAttrValues.Location = new System.Drawing.Point(9, 74);
            this.listViewAttrValues.MultiSelect = false;
            this.listViewAttrValues.Name = "listViewAttrValues";
            this.listViewAttrValues.Size = new System.Drawing.Size(262, 167);
            this.listViewAttrValues.TabIndex = 7;
            this.listViewAttrValues.UseCompatibleStateImageBehavior = false;
            this.listViewAttrValues.View = System.Windows.Forms.View.Details;
            this.listViewAttrValues.AfterLabelEdit += new System.Windows.Forms.LabelEditEventHandler(this.listViewAttrValues_AfterLabelEdit);
            this.listViewAttrValues.SelectedIndexChanged += new System.EventHandler(this.listViewAttrValues_SelectedIndexChanged);
            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(9, 249);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(343, 5);
            this.groupBox1.TabIndex = 11;
            this.groupBox1.TabStop = false;
            // 
            // btnEdit
            // 
            this.btnEdit.Enabled = false;
            this.btnEdit.Location = new System.Drawing.Point(279, 74);
            this.btnEdit.Name = "btnEdit";
            this.btnEdit.Size = new System.Drawing.Size(72, 22);
            this.btnEdit.TabIndex = 12;
            this.btnEdit.Text = "&Edit";
            this.btnEdit.UseVisualStyleBackColor = true;
            this.btnEdit.Click += new System.EventHandler(this.btnEdit_Click);
            // 
            // chValues
            // 
            this.chValues.Text = "values";
            this.chValues.Width = 250;
            // 
            // OthersStringEditor
            // 
            this.AcceptButton = this.buttonOk;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.buttonCancel;
            this.ClientSize = new System.Drawing.Size(361, 295);
            this.Controls.Add(this.btnEdit);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.listViewAttrValues);
            this.Controls.Add(this.lblselectedAttr);
            this.Controls.Add(this.btnRemove);
            this.Controls.Add(this.buttonCancel);
            this.Controls.Add(this.buttonOk);
            this.Controls.Add(this.lblValues);
            this.Controls.Add(this.btnAdd);
            this.Controls.Add(this.txtAttrValue);
            this.Controls.Add(this.lblAttrValue);
            this.Controls.Add(this.AttributeSelectionLabel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "OthersStringEditor";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Multi-valued String Editor";
            this.Load += new System.EventHandler(this.MultiValuedStringEditor_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label AttributeSelectionLabel;
        private System.Windows.Forms.Label lblAttrValue;
        private System.Windows.Forms.TextBox txtAttrValue;
        private System.Windows.Forms.Button btnAdd;
        private System.Windows.Forms.Label lblValues;
        private System.Windows.Forms.Button buttonOk;
        private System.Windows.Forms.Button buttonCancel;
        private System.Windows.Forms.Button btnRemove;
        private System.Windows.Forms.Label lblselectedAttr;
        private Likewise.LMC.ServerControl.LWListView listViewAttrValues;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Button btnEdit;
        private System.Windows.Forms.ColumnHeader chValues;
    }
}