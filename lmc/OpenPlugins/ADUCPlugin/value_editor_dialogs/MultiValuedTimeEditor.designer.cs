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
    partial class MultiValuedTimeEditor
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
            this.lblAttr = new System.Windows.Forms.Label();
            this.AttributeSelectionLabel = new System.Windows.Forms.Label();
            this.lblDate = new System.Windows.Forms.Label();
            this.dtPickerDate = new System.Windows.Forms.DateTimePicker();
            this.lblTime = new System.Windows.Forms.Label();
            this.dtPickerTime = new System.Windows.Forms.DateTimePicker();
            this.btnAdd = new System.Windows.Forms.Button();
            this.ValueTextLabel = new System.Windows.Forms.Label();
            this.btnRemove = new System.Windows.Forms.Button();
            this.btnOk = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.lblGMTTime = new System.Windows.Forms.Label();
            this.lblselectedAttr = new System.Windows.Forms.Label();
            this.listViewAttrValues = new Likewise.LMC.ServerControl.LWListView();
            this.chAttributeValues = new System.Windows.Forms.ColumnHeader();
            this.SuspendLayout();
            // 
            // lblAttr
            // 
            this.lblAttr.AutoSize = true;
            this.lblAttr.Location = new System.Drawing.Point(13, 12);
            this.lblAttr.Name = "lblAttr";
            this.lblAttr.Size = new System.Drawing.Size(49, 13);
            this.lblAttr.TabIndex = 1;
            this.lblAttr.Text = "&Attribute:";
            // 
            // AttributeSelectionLabel
            // 
            this.AttributeSelectionLabel.AutoSize = true;
            this.AttributeSelectionLabel.Location = new System.Drawing.Point(61, 14);
            this.AttributeSelectionLabel.Name = "AttributeSelectionLabel";
            this.AttributeSelectionLabel.Size = new System.Drawing.Size(0, 13);
            this.AttributeSelectionLabel.TabIndex = 2;
            // 
            // lblDate
            // 
            this.lblDate.AutoSize = true;
            this.lblDate.Location = new System.Drawing.Point(12, 56);
            this.lblDate.Name = "lblDate";
            this.lblDate.Size = new System.Drawing.Size(33, 13);
            this.lblDate.TabIndex = 4;
            this.lblDate.Text = "Dat&e:";
            // 
            // dtPickerDate
            // 
            this.dtPickerDate.Location = new System.Drawing.Point(15, 74);
            this.dtPickerDate.Name = "dtPickerDate";
            this.dtPickerDate.Size = new System.Drawing.Size(240, 20);
            this.dtPickerDate.TabIndex = 5;
            // 
            // lblTime
            // 
            this.lblTime.AutoSize = true;
            this.lblTime.Location = new System.Drawing.Point(12, 106);
            this.lblTime.Name = "lblTime";
            this.lblTime.Size = new System.Drawing.Size(33, 13);
            this.lblTime.TabIndex = 6;
            this.lblTime.Text = "Ti&me:";
            // 
            // dtPickerTime
            // 
            this.dtPickerTime.Format = System.Windows.Forms.DateTimePickerFormat.Time;
            this.dtPickerTime.Location = new System.Drawing.Point(15, 123);
            this.dtPickerTime.Name = "dtPickerTime";
            this.dtPickerTime.ShowUpDown = true;
            this.dtPickerTime.Size = new System.Drawing.Size(240, 20);
            this.dtPickerTime.TabIndex = 7;
            // 
            // btnAdd
            // 
            this.btnAdd.Location = new System.Drawing.Point(265, 122);
            this.btnAdd.Name = "btnAdd";
            this.btnAdd.Size = new System.Drawing.Size(75, 22);
            this.btnAdd.TabIndex = 8;
            this.btnAdd.Text = "A&dd";
            this.btnAdd.UseVisualStyleBackColor = true;
            this.btnAdd.Click += new System.EventHandler(this.btnAdd_Click);
            // 
            // ValueTextLabel
            // 
            this.ValueTextLabel.AutoSize = true;
            this.ValueTextLabel.Location = new System.Drawing.Point(14, 155);
            this.ValueTextLabel.Name = "ValueTextLabel";
            this.ValueTextLabel.Size = new System.Drawing.Size(42, 13);
            this.ValueTextLabel.TabIndex = 9;
            this.ValueTextLabel.Text = "Val&ues:";
            // 
            // btnRemove
            // 
            this.btnRemove.Enabled = false;
            this.btnRemove.Location = new System.Drawing.Point(265, 172);
            this.btnRemove.Name = "btnRemove";
            this.btnRemove.Size = new System.Drawing.Size(75, 22);
            this.btnRemove.TabIndex = 11;
            this.btnRemove.Text = "&Remove";
            this.btnRemove.UseVisualStyleBackColor = true;
            this.btnRemove.Click += new System.EventHandler(this.btnRemove_Click);
            // 
            // btnOk
            // 
            this.btnOk.Location = new System.Drawing.Point(180, 328);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(75, 22);
            this.btnOk.TabIndex = 12;
            this.btnOk.Text = "&OK";
            this.btnOk.UseVisualStyleBackColor = true;
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(265, 328);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 22);
            this.btnCancel.TabIndex = 13;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // lblGMTTime
            // 
            this.lblGMTTime.AutoSize = true;
            this.lblGMTTime.Location = new System.Drawing.Point(12, 34);
            this.lblGMTTime.Name = "lblGMTTime";
            this.lblGMTTime.Size = new System.Drawing.Size(214, 13);
            this.lblGMTTime.TabIndex = 3;
            this.lblGMTTime.Text = "All times are in Greenwich Mean Time[GMT]";
            // 
            // lblselectedAttr
            // 
            this.lblselectedAttr.AutoSize = true;
            this.lblselectedAttr.Location = new System.Drawing.Point(67, 14);
            this.lblselectedAttr.Name = "lblselectedAttr";
            this.lblselectedAttr.Size = new System.Drawing.Size(0, 13);
            this.lblselectedAttr.TabIndex = 14;
            // 
            // listViewAttrValues
            // 
            this.listViewAttrValues.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.chAttributeValues});
            this.listViewAttrValues.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.None;
            this.listViewAttrValues.HideSelection = false;
            this.listViewAttrValues.Location = new System.Drawing.Point(16, 172);
            this.listViewAttrValues.MultiSelect = false;
            this.listViewAttrValues.Name = "listViewAttrValues";
            this.listViewAttrValues.Size = new System.Drawing.Size(239, 137);
            this.listViewAttrValues.TabIndex = 10;
            this.listViewAttrValues.UseCompatibleStateImageBehavior = false;
            this.listViewAttrValues.View = System.Windows.Forms.View.Details;
            this.listViewAttrValues.SelectedIndexChanged += new System.EventHandler(this.listViewAttrValues_SelectedIndexChanged);
            // 
            // chAttributeValues
            // 
            this.chAttributeValues.Text = "AttributeValues";
            this.chAttributeValues.Width = 232;
            // 
            // MultiValuedTimeEditor
            // 
            this.AcceptButton = this.btnOk;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(354, 360);
            this.Controls.Add(this.listViewAttrValues);
            this.Controls.Add(this.lblselectedAttr);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.btnRemove);
            this.Controls.Add(this.ValueTextLabel);
            this.Controls.Add(this.btnAdd);
            this.Controls.Add(this.dtPickerTime);
            this.Controls.Add(this.lblTime);
            this.Controls.Add(this.dtPickerDate);
            this.Controls.Add(this.lblDate);
            this.Controls.Add(this.lblGMTTime);
            this.Controls.Add(this.AttributeSelectionLabel);
            this.Controls.Add(this.lblAttr);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "MultiValuedTimeEditor";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Multi-valued Time Editor";
            this.Load += new System.EventHandler(this.MultiValuedTimeEditor_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblAttr;
        private System.Windows.Forms.Label AttributeSelectionLabel;
        private System.Windows.Forms.Label lblDate;
        private System.Windows.Forms.DateTimePicker dtPickerDate;
        private System.Windows.Forms.Label lblTime;
        private System.Windows.Forms.DateTimePicker dtPickerTime;
        private System.Windows.Forms.Button btnAdd;
        private System.Windows.Forms.Label ValueTextLabel;
        private System.Windows.Forms.Button btnRemove;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Label lblGMTTime;
        private System.Windows.Forms.Label lblselectedAttr;
        private Likewise.LMC.ServerControl.LWListView listViewAttrValues;
        private System.Windows.Forms.ColumnHeader chAttributeValues;
    }
}