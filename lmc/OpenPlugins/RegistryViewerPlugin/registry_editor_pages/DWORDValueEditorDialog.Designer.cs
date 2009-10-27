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
    partial class DWORDValueEditorDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DWORDValueEditorDialog));
            this.lblValuename = new System.Windows.Forms.Label();
            this.txtValuename = new System.Windows.Forms.TextBox();
            this.lblValuedata = new System.Windows.Forms.Label();
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnOk = new System.Windows.Forms.Button();
            this.txtValuedata = new System.Windows.Forms.TextBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.radioButtonDecimal = new System.Windows.Forms.RadioButton();
            this.radioButtonHexno = new System.Windows.Forms.RadioButton();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // lblValuename
            // 
            this.lblValuename.AutoSize = true;
            this.lblValuename.Location = new System.Drawing.Point(4, 8);
            this.lblValuename.Name = "lblValuename";
            this.lblValuename.Size = new System.Drawing.Size(66, 13);
            this.lblValuename.TabIndex = 0;
            this.lblValuename.Text = "Value name:";
            // 
            // txtValuename
            // 
            this.txtValuename.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.txtValuename.Location = new System.Drawing.Point(7, 25);
            this.txtValuename.MaxLength = 255;
            this.txtValuename.Name = "txtValuename";
            this.txtValuename.ReadOnly = true;
            this.txtValuename.Size = new System.Drawing.Size(287, 20);
            this.txtValuename.TabIndex = 1;
            this.txtValuename.TextChanged += new System.EventHandler(this.txtValuename_TextChanged);
            // 
            // lblValuedata
            // 
            this.lblValuedata.AutoSize = true;
            this.lblValuedata.Location = new System.Drawing.Point(5, 60);
            this.lblValuedata.Name = "lblValuedata";
            this.lblValuedata.Size = new System.Drawing.Size(61, 13);
            this.lblValuedata.TabIndex = 2;
            this.lblValuedata.Text = "Value data:";
            // 
            // btnCancel
            // 
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(219, 149);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 7;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // btnOk
            // 
            this.btnOk.Location = new System.Drawing.Point(138, 149);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(75, 23);
            this.btnOk.TabIndex = 6;
            this.btnOk.Text = "&Ok";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            // 
            // txtValuedata
            // 
            this.txtValuedata.Location = new System.Drawing.Point(8, 78);
            this.txtValuedata.MaxLength = 8;
            this.txtValuedata.Name = "txtValuedata";
            this.txtValuedata.Size = new System.Drawing.Size(113, 20);
            this.txtValuedata.TabIndex = 8;
            this.txtValuedata.KeyUp += new System.Windows.Forms.KeyEventHandler(this.txtValuedata_KeyUp);
            this.txtValuedata.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.txtValuedata_KeyPress);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.radioButtonDecimal);
            this.groupBox1.Controls.Add(this.radioButtonHexno);
            this.groupBox1.Location = new System.Drawing.Point(129, 60);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(165, 77);
            this.groupBox1.TabIndex = 9;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Base";
            // 
            // radioButtonDecimal
            // 
            this.radioButtonDecimal.AutoSize = true;
            this.radioButtonDecimal.Location = new System.Drawing.Point(9, 44);
            this.radioButtonDecimal.Name = "radioButtonDecimal";
            this.radioButtonDecimal.Size = new System.Drawing.Size(63, 17);
            this.radioButtonDecimal.TabIndex = 1;
            this.radioButtonDecimal.Text = "Decimal";
            this.radioButtonDecimal.UseVisualStyleBackColor = true;
            this.radioButtonDecimal.CheckedChanged += new System.EventHandler(this.radioButtonDecimal_CheckedChanged);
            // 
            // radioButtonHexno
            // 
            this.radioButtonHexno.AutoSize = true;
            this.radioButtonHexno.Checked = true;
            this.radioButtonHexno.Location = new System.Drawing.Point(9, 21);
            this.radioButtonHexno.Name = "radioButtonHexno";
            this.radioButtonHexno.Size = new System.Drawing.Size(86, 17);
            this.radioButtonHexno.TabIndex = 0;
            this.radioButtonHexno.TabStop = true;
            this.radioButtonHexno.Text = "Hexadecimal";
            this.radioButtonHexno.UseVisualStyleBackColor = true;
            this.radioButtonHexno.CheckedChanged += new System.EventHandler(this.radioButtonHexno_CheckedChanged);
            // 
            // DWORDValueEditorDialog
            // 
            this.AcceptButton = this.btnOk;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(301, 180);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.txtValuedata);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.lblValuedata);
            this.Controls.Add(this.txtValuename);
            this.Controls.Add(this.lblValuename);
            this.HelpButton = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "DWORDValueEditorDialog";
            this.Text = "Edit DWORD Value";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblValuename;
        private System.Windows.Forms.TextBox txtValuename;
        private System.Windows.Forms.Label lblValuedata;       
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.TextBox txtValuedata;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.RadioButton radioButtonDecimal;
        private System.Windows.Forms.RadioButton radioButtonHexno;
    }
}