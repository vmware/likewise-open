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

namespace Likewise.LMC.Plugins.ADUCPlugin
{
    partial class IntegerAttributeEditor
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
            this.btnClear = new System.Windows.Forms.Button();
            this.txtAttrValue = new System.Windows.Forms.TextBox();
            this.lblAttrValue = new System.Windows.Forms.Label();
            this.lblSelectedAttr = new System.Windows.Forms.Label();
            this.lblAttr = new System.Windows.Forms.Label();
            this.SuspendLayout();
            //
            // btnCancel
            //
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(271, 81);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 13;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            //
            // btnOk
            //
            this.btnOk.Location = new System.Drawing.Point(190, 81);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(75, 23);
            this.btnOk.TabIndex = 12;
            this.btnOk.Text = "OK";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            //
            // btnClear
            //
            this.btnClear.Location = new System.Drawing.Point(12, 81);
            this.btnClear.Name = "btnClear";
            this.btnClear.Size = new System.Drawing.Size(75, 23);
            this.btnClear.TabIndex = 11;
            this.btnClear.Text = "&Clear";
            this.btnClear.UseVisualStyleBackColor = true;
            this.btnClear.Click += new System.EventHandler(this.btnClear_Click);
            //
            // txtAttrValue
            //
            this.txtAttrValue.Location = new System.Drawing.Point(11, 51);
            this.txtAttrValue.Name = "txtAttrValue";
            this.txtAttrValue.Size = new System.Drawing.Size(335, 20);
            this.txtAttrValue.TabIndex = 10;
            this.txtAttrValue.Text = "<not set>";
            //
            // lblAttrValue
            //
            this.lblAttrValue.AutoSize = true;
            this.lblAttrValue.Location = new System.Drawing.Point(10, 33);
            this.lblAttrValue.Name = "lblAttrValue";
            this.lblAttrValue.Size = new System.Drawing.Size(40, 13);
            this.lblAttrValue.TabIndex = 9;
            this.lblAttrValue.Text = "&Value :";
            //
            // lblSelectedAttr
            //
            this.lblSelectedAttr.AutoSize = true;
            this.lblSelectedAttr.Location = new System.Drawing.Point(63, 9);
            this.lblSelectedAttr.Name = "lblSelectedAttr";
            this.lblSelectedAttr.Size = new System.Drawing.Size(0, 13);
            this.lblSelectedAttr.TabIndex = 8;
            //
            // lblAttr
            //
            this.lblAttr.AutoSize = true;
            this.lblAttr.Location = new System.Drawing.Point(9, 8);
            this.lblAttr.Name = "lblAttr";
            this.lblAttr.Size = new System.Drawing.Size(52, 13);
            this.lblAttr.TabIndex = 7;
            this.lblAttr.Text = "&Attribute :";
            //
            // IntegerAttributeEditor
            //
            this.AcceptButton = this.btnOk;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(359, 112);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.btnClear);
            this.Controls.Add(this.txtAttrValue);
            this.Controls.Add(this.lblAttrValue);
            this.Controls.Add(this.lblSelectedAttr);
            this.Controls.Add(this.lblAttr);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "IntegerAttributeEditor";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Integer Attribute Editor";
            this.Load += new System.EventHandler(this.IntegerAttributeEditor_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.Button btnClear;
        private System.Windows.Forms.TextBox txtAttrValue;
        private System.Windows.Forms.Label lblAttrValue;
        private System.Windows.Forms.Label lblSelectedAttr;
        private System.Windows.Forms.Label lblAttr;
    }
}