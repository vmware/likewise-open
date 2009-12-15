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
    partial class BooleanAttributeEditor
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
            this.lblAttribute = new System.Windows.Forms.Label();
            this.lblValue = new System.Windows.Forms.Label();
            this.rbtnTrue = new System.Windows.Forms.RadioButton();
            this.rbtnFalse = new System.Windows.Forms.RadioButton();
            this.rbtnNotSet = new System.Windows.Forms.RadioButton();
            this.btnOk = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.lblselectedValue = new System.Windows.Forms.Label();
            this.SuspendLayout();
            //
            // lblAttribute
            //
            this.lblAttribute.AutoSize = true;
            this.lblAttribute.BackColor = System.Drawing.Color.Transparent;
            this.lblAttribute.Location = new System.Drawing.Point(13, 13);
            this.lblAttribute.Name = "lblAttribute";
            this.lblAttribute.Size = new System.Drawing.Size(49, 13);
            this.lblAttribute.TabIndex = 0;
            this.lblAttribute.Text = "&Attribute:";
            //
            // lblValue
            //
            this.lblValue.AutoSize = true;
            this.lblValue.BackColor = System.Drawing.Color.Transparent;
            this.lblValue.Location = new System.Drawing.Point(13, 40);
            this.lblValue.Name = "lblValue";
            this.lblValue.Size = new System.Drawing.Size(37, 13);
            this.lblValue.TabIndex = 1;
            this.lblValue.Text = "&Value:";
            //
            // rbtnTrue
            //
            this.rbtnTrue.AutoSize = true;
            this.rbtnTrue.Location = new System.Drawing.Point(16, 68);
            this.rbtnTrue.Name = "rbtnTrue";
            this.rbtnTrue.Size = new System.Drawing.Size(47, 17);
            this.rbtnTrue.TabIndex = 2;
            this.rbtnTrue.Text = "&True";
            this.rbtnTrue.UseVisualStyleBackColor = true;
            //
            // rbtnFalse
            //
            this.rbtnFalse.AutoSize = true;
            this.rbtnFalse.Location = new System.Drawing.Point(16, 91);
            this.rbtnFalse.Name = "rbtnFalse";
            this.rbtnFalse.Size = new System.Drawing.Size(50, 17);
            this.rbtnFalse.TabIndex = 3;
            this.rbtnFalse.Text = "&False";
            this.rbtnFalse.UseVisualStyleBackColor = true;
            //
            // rbtnNotSet
            //
            this.rbtnNotSet.AutoSize = true;
            this.rbtnNotSet.Checked = true;
            this.rbtnNotSet.Location = new System.Drawing.Point(16, 114);
            this.rbtnNotSet.Name = "rbtnNotSet";
            this.rbtnNotSet.Size = new System.Drawing.Size(59, 17);
            this.rbtnNotSet.TabIndex = 4;
            this.rbtnNotSet.TabStop = true;
            this.rbtnNotSet.Text = "&Not set";
            this.rbtnNotSet.UseVisualStyleBackColor = true;
            //
            // btnOk
            //
            this.btnOk.Location = new System.Drawing.Point(101, 138);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(75, 23);
            this.btnOk.TabIndex = 5;
            this.btnOk.Text = "OK";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            //
            // btnCancel
            //
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(182, 138);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 6;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            //
            // lblselectedValue
            //
            this.lblselectedValue.AutoSize = true;
            this.lblselectedValue.Location = new System.Drawing.Point(69, 13);
            this.lblselectedValue.Name = "lblselectedValue";
            this.lblselectedValue.Size = new System.Drawing.Size(0, 13);
            this.lblselectedValue.TabIndex = 7;
            //
            // BooleanAttributeEditor
            //
            this.AcceptButton = this.btnOk;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(265, 175);
            this.Controls.Add(this.lblselectedValue);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.rbtnNotSet);
            this.Controls.Add(this.rbtnFalse);
            this.Controls.Add(this.rbtnTrue);
            this.Controls.Add(this.lblValue);
            this.Controls.Add(this.lblAttribute);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "BooleanAttributeEditor";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Boolean Attribute Editor";
            this.Load += new System.EventHandler(this.BooleanAttributeEditor_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblAttribute;
        private System.Windows.Forms.Label lblValue;
        private System.Windows.Forms.RadioButton rbtnTrue;
        private System.Windows.Forms.RadioButton rbtnFalse;
        private System.Windows.Forms.RadioButton rbtnNotSet;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Label lblselectedValue;
    }
}