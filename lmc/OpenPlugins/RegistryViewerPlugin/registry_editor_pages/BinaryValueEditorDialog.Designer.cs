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

namespace Likewise.LMC.Plugins.RegistryViewerPlugin
{
    partial class BinaryValueEditorDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(BinaryValueEditorDialog));
            this.lblValuename = new System.Windows.Forms.Label();
            this.txtValuename = new System.Windows.Forms.TextBox();
            this.lblValuedata = new System.Windows.Forms.Label();
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnOk = new System.Windows.Forms.Button();
            this.LWDataBinarydata = new System.Windows.Forms.DataGridView();
            this.datacol1 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol2 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol3 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol4 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol5 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol6 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol7 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol8 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol9 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol10 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol11 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol12 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol13 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol14 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol15 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol16 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.dataCol17 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            ((System.ComponentModel.ISupportInitialize)(this.LWDataBinarydata)).BeginInit();
            this.SuspendLayout();
            // 
            // lblValuename
            // 
            this.lblValuename.AutoSize = true;
            this.lblValuename.Location = new System.Drawing.Point(4, 4);
            this.lblValuename.Name = "lblValuename";
            this.lblValuename.Size = new System.Drawing.Size(66, 13);
            this.lblValuename.TabIndex = 0;
            this.lblValuename.Text = "Value name:";
            // 
            // txtValuename
            // 
            this.txtValuename.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.txtValuename.Location = new System.Drawing.Point(6, 22);
            this.txtValuename.MaxLength = 255;
            this.txtValuename.Name = "txtValuename";
            this.txtValuename.ReadOnly = true;
            this.txtValuename.Size = new System.Drawing.Size(383, 20);
            this.txtValuename.TabIndex = 1;
            this.txtValuename.TextChanged += new System.EventHandler(this.txtValuename_TextChanged);
            // 
            // lblValuedata
            // 
            this.lblValuedata.AutoSize = true;
            this.lblValuedata.Location = new System.Drawing.Point(5, 54);
            this.lblValuedata.Name = "lblValuedata";
            this.lblValuedata.Size = new System.Drawing.Size(61, 13);
            this.lblValuedata.TabIndex = 2;
            this.lblValuedata.Text = "Value data:";
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(314, 241);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 5;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // btnOk
            // 
            this.btnOk.Location = new System.Drawing.Point(228, 241);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(75, 23);
            this.btnOk.TabIndex = 4;
            this.btnOk.Text = "&Ok";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            // 
            // LWDataBinarydata
            // 
            this.LWDataBinarydata.AllowUserToDeleteRows = false;
            this.LWDataBinarydata.AllowUserToResizeColumns = false;
            this.LWDataBinarydata.AllowUserToResizeRows = false;
            this.LWDataBinarydata.BackgroundColor = System.Drawing.SystemColors.Window;
            this.LWDataBinarydata.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.LWDataBinarydata.ColumnHeadersBorderStyle = System.Windows.Forms.DataGridViewHeaderBorderStyle.None;
            this.LWDataBinarydata.ColumnHeadersHeight = 4;
            this.LWDataBinarydata.ColumnHeadersVisible = false;
            this.LWDataBinarydata.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.datacol1,
            this.dataCol2,
            this.dataCol3,
            this.dataCol4,
            this.dataCol5,
            this.dataCol6,
            this.dataCol7,
            this.dataCol8,
            this.dataCol9,
            this.dataCol10,
            this.dataCol11,
            this.dataCol12,
            this.dataCol13,
            this.dataCol14,
            this.dataCol15,
            this.dataCol16,
            this.dataCol17});
            this.LWDataBinarydata.EditMode = System.Windows.Forms.DataGridViewEditMode.EditOnEnter;
            this.LWDataBinarydata.GridColor = System.Drawing.SystemColors.Window;
            this.LWDataBinarydata.Location = new System.Drawing.Point(7, 70);
            this.LWDataBinarydata.Name = "LWDataBinarydata";
            this.LWDataBinarydata.RowHeadersBorderStyle = System.Windows.Forms.DataGridViewHeaderBorderStyle.None;
            this.LWDataBinarydata.RowHeadersVisible = false;
            this.LWDataBinarydata.RowHeadersWidthSizeMode = System.Windows.Forms.DataGridViewRowHeadersWidthSizeMode.DisableResizing;
            this.LWDataBinarydata.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.CellSelect;
            this.LWDataBinarydata.Size = new System.Drawing.Size(384, 158);
            this.LWDataBinarydata.TabIndex = 3;
            this.LWDataBinarydata.CellMouseUp += new System.Windows.Forms.DataGridViewCellMouseEventHandler(this.LWDataBinarydata_CellMouseUp);
            this.LWDataBinarydata.CellBeginEdit += new System.Windows.Forms.DataGridViewCellCancelEventHandler(this.LWDataBinarydata_CellBeginEdit);
            this.LWDataBinarydata.EditingControlShowing += new System.Windows.Forms.DataGridViewEditingControlShowingEventHandler(this.LWDataBinarydata_EditingControlShowing);
            this.LWDataBinarydata.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.LWDataBinarydata_KeyPress);
            // 
            // datacol1
            // 
            this.datacol1.DividerWidth = 3;
            this.datacol1.HeaderText = "Col1";
            this.datacol1.MaxInputLength = 4;
            this.datacol1.Name = "datacol1";
            this.datacol1.ReadOnly = true;
            this.datacol1.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.datacol1.Width = 45;
            // 
            // dataCol2
            // 
            this.dataCol2.HeaderText = "Col2";
            this.dataCol2.MaxInputLength = 2;
            this.dataCol2.Name = "dataCol2";
            this.dataCol2.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol2.Width = 25;
            // 
            // dataCol3
            // 
            this.dataCol3.HeaderText = "Col3";
            this.dataCol3.MaxInputLength = 2;
            this.dataCol3.Name = "dataCol3";
            this.dataCol3.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol3.Width = 25;
            // 
            // dataCol4
            // 
            this.dataCol4.HeaderText = "Col4";
            this.dataCol4.MaxInputLength = 2;
            this.dataCol4.Name = "dataCol4";
            this.dataCol4.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol4.Width = 25;
            // 
            // dataCol5
            // 
            this.dataCol5.HeaderText = "Col5";
            this.dataCol5.MaxInputLength = 2;
            this.dataCol5.Name = "dataCol5";
            this.dataCol5.Width = 25;
            // 
            // dataCol6
            // 
            this.dataCol6.HeaderText = "Col6";
            this.dataCol6.MaxInputLength = 2;
            this.dataCol6.Name = "dataCol6";
            this.dataCol6.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol6.Width = 25;
            // 
            // dataCol7
            // 
            this.dataCol7.HeaderText = "Col7";
            this.dataCol7.MaxInputLength = 2;
            this.dataCol7.Name = "dataCol7";
            this.dataCol7.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol7.Width = 25;
            // 
            // dataCol8
            // 
            this.dataCol8.HeaderText = "Col8";
            this.dataCol8.MaxInputLength = 2;
            this.dataCol8.Name = "dataCol8";
            this.dataCol8.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol8.Width = 25;
            // 
            // dataCol9
            // 
            this.dataCol9.HeaderText = "Col9";
            this.dataCol9.MaxInputLength = 2;
            this.dataCol9.Name = "dataCol9";
            this.dataCol9.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol9.Width = 39;
            // 
            // dataCol10
            // 
            this.dataCol10.HeaderText = "Col10";
            this.dataCol10.MaxInputLength = 1;
            this.dataCol10.Name = "dataCol10";
            this.dataCol10.ReadOnly = true;
            this.dataCol10.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol10.Width = 15;
            // 
            // dataCol11
            // 
            this.dataCol11.HeaderText = "Col11";
            this.dataCol11.MaxInputLength = 1;
            this.dataCol11.Name = "dataCol11";
            this.dataCol11.ReadOnly = true;
            this.dataCol11.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol11.Width = 15;
            // 
            // dataCol12
            // 
            this.dataCol12.HeaderText = "Col12";
            this.dataCol12.MaxInputLength = 1;
            this.dataCol12.Name = "dataCol12";
            this.dataCol12.ReadOnly = true;
            this.dataCol12.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol12.Width = 15;
            // 
            // dataCol13
            // 
            this.dataCol13.HeaderText = "Col13";
            this.dataCol13.MaxInputLength = 1;
            this.dataCol13.Name = "dataCol13";
            this.dataCol13.ReadOnly = true;
            this.dataCol13.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol13.Width = 15;
            // 
            // dataCol14
            // 
            this.dataCol14.HeaderText = "Col14";
            this.dataCol14.MaxInputLength = 1;
            this.dataCol14.Name = "dataCol14";
            this.dataCol14.ReadOnly = true;
            this.dataCol14.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol14.Width = 15;
            // 
            // dataCol15
            // 
            this.dataCol15.HeaderText = "Col15";
            this.dataCol15.MaxInputLength = 1;
            this.dataCol15.Name = "dataCol15";
            this.dataCol15.ReadOnly = true;
            this.dataCol15.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol15.Width = 15;
            // 
            // dataCol16
            // 
            this.dataCol16.HeaderText = "Col16";
            this.dataCol16.MaxInputLength = 1;
            this.dataCol16.Name = "dataCol16";
            this.dataCol16.ReadOnly = true;
            this.dataCol16.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol16.Width = 15;
            // 
            // dataCol17
            // 
            this.dataCol17.HeaderText = "Col17";
            this.dataCol17.MaxInputLength = 1;
            this.dataCol17.Name = "dataCol17";
            this.dataCol17.ReadOnly = true;
            this.dataCol17.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.dataCol17.Width = 15;
            // 
            // BinaryValueEditorDialog
            // 
            this.AcceptButton = this.btnOk;
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(399, 274);
            this.Controls.Add(this.LWDataBinarydata);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.lblValuedata);
            this.Controls.Add(this.txtValuename);
            this.Controls.Add(this.lblValuename);
            this.HelpButton = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "BinaryValueEditorDialog";
            this.Text = "Edit Binary Value";
            ((System.ComponentModel.ISupportInitialize)(this.LWDataBinarydata)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblValuename;
        private System.Windows.Forms.TextBox txtValuename;
        private System.Windows.Forms.Label lblValuedata;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.DataGridView LWDataBinarydata;
        private System.Windows.Forms.DataGridViewTextBoxColumn datacol1;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol2;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol3;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol4;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol5;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol6;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol7;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol8;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol9;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol10;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol11;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol12;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol13;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol14;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol15;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol16;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataCol17;
    }
}