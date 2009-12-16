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
    partial class OctetStringAttributeEditor
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
            this.lblselectedAttr = new System.Windows.Forms.Label();
            this.lblValue = new System.Windows.Forms.Label();
            this.btnClear = new System.Windows.Forms.Button();
            this.btnOk = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.rtbAttr = new System.Windows.Forms.RichTextBox();
            this.lbEdit = new System.Windows.Forms.Label();
            this.cbEditValue = new System.Windows.Forms.ComboBox();
            this.SuspendLayout();
            //
            // lblAttr
            //
            this.lblAttr.AutoSize = true;
            this.lblAttr.Location = new System.Drawing.Point(13, 9);
            this.lblAttr.Name = "lblAttr";
            this.lblAttr.Size = new System.Drawing.Size(52, 13);
            this.lblAttr.TabIndex = 0;
            this.lblAttr.Text = "&Attribute :";
            //
            // lblselectedAttr
            //
            this.lblselectedAttr.AutoSize = true;
            this.lblselectedAttr.Location = new System.Drawing.Point(65, 10);
            this.lblselectedAttr.Name = "lblselectedAttr";
            this.lblselectedAttr.Size = new System.Drawing.Size(0, 13);
            this.lblselectedAttr.TabIndex = 1;
            //
            // lblValue
            //
            this.lblValue.AutoSize = true;
            this.lblValue.Location = new System.Drawing.Point(13, 73);
            this.lblValue.Name = "lblValue";
            this.lblValue.Size = new System.Drawing.Size(40, 13);
            this.lblValue.TabIndex = 2;
            this.lblValue.Text = "&Value :";
            //
            // btnClear
            //
            this.btnClear.Location = new System.Drawing.Point(15, 308);
            this.btnClear.Name = "btnClear";
            this.btnClear.Size = new System.Drawing.Size(75, 23);
            this.btnClear.TabIndex = 4;
            this.btnClear.Text = "&Clear";
            this.btnClear.UseVisualStyleBackColor = true;
            this.btnClear.Click += new System.EventHandler(this.btnClear_Click_1);
            //
            // btnOk
            //
            this.btnOk.Location = new System.Drawing.Point(200, 308);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(75, 23);
            this.btnOk.TabIndex = 5;
            this.btnOk.Text = "OK";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click_1);
            //
            // btnCancel
            //
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(282, 308);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 6;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            //
            // rtbAttr
            //
            this.rtbAttr.Location = new System.Drawing.Point(16, 99);
            this.rtbAttr.Name = "rtbAttr";
            this.rtbAttr.Size = new System.Drawing.Size(340, 195);
            this.rtbAttr.TabIndex = 7;
            this.rtbAttr.Text = "";
            //
            // lbEdit
            //
            this.lbEdit.AutoSize = true;
            this.lbEdit.Location = new System.Drawing.Point(12, 41);
            this.lbEdit.Name = "lbEdit";
            this.lbEdit.Size = new System.Drawing.Size(74, 13);
            this.lbEdit.TabIndex = 8;
            this.lbEdit.Text = "E&dit value as :";
            //
            // cbEditValue
            //
            this.cbEditValue.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbEditValue.FormattingEnabled = true;
            this.cbEditValue.Items.AddRange(new object[] {
            "Hexadecimal",
            "Octal",
            "Decimal",
            "Binary"});
            this.cbEditValue.Location = new System.Drawing.Point(83, 38);
            this.cbEditValue.Name = "cbEditValue";
            this.cbEditValue.Size = new System.Drawing.Size(273, 21);
            this.cbEditValue.TabIndex = 9;
            this.cbEditValue.SelectedIndexChanged += new System.EventHandler(this.cbEditValue_SelectedIndexChanged);
            //
            // OctetStringAttributeEditor
            //
            this.AcceptButton = this.btnOk;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(368, 343);
            this.Controls.Add(this.cbEditValue);
            this.Controls.Add(this.lbEdit);
            this.Controls.Add(this.rtbAttr);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.btnClear);
            this.Controls.Add(this.lblValue);
            this.Controls.Add(this.lblselectedAttr);
            this.Controls.Add(this.lblAttr);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "OctetStringAttributeEditor";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Octet String Attribute Editor";
            this.Load += new System.EventHandler(this.OctetStringAttributeEditor_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblAttr;
        private System.Windows.Forms.Label lblselectedAttr;
        private System.Windows.Forms.Label lblValue;
        private System.Windows.Forms.Button btnClear;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.RichTextBox rtbAttr;
        private System.Windows.Forms.Label lbEdit;
        private System.Windows.Forms.ComboBox cbEditValue;
    }
}