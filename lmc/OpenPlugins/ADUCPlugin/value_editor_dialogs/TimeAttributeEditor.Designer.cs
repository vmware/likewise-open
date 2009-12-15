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
    partial class TimeAttributeEditor
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
            this.lblTimeAttribute = new System.Windows.Forms.Label();
            this.lblSelectedValue = new System.Windows.Forms.Label();
            this.lblGMTDisplay = new System.Windows.Forms.Label();
            this.lblDate = new System.Windows.Forms.Label();
            this.dtDate = new System.Windows.Forms.DateTimePicker();
            this.lblTime = new System.Windows.Forms.Label();
            this.dtTime = new System.Windows.Forms.DateTimePicker();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.SuspendLayout();
            //
            // lblTimeAttribute
            //
            this.lblTimeAttribute.AutoSize = true;
            this.lblTimeAttribute.Location = new System.Drawing.Point(3, 9);
            this.lblTimeAttribute.Name = "lblTimeAttribute";
            this.lblTimeAttribute.Size = new System.Drawing.Size(49, 13);
            this.lblTimeAttribute.TabIndex = 0;
            this.lblTimeAttribute.Text = "&Attribute:";
            //
            // lblSelectedValue
            //
            this.lblSelectedValue.AutoSize = true;
            this.lblSelectedValue.Location = new System.Drawing.Point(58, 9);
            this.lblSelectedValue.Name = "lblSelectedValue";
            this.lblSelectedValue.Size = new System.Drawing.Size(0, 13);
            this.lblSelectedValue.TabIndex = 1;
            //
            // lblGMTDisplay
            //
            this.lblGMTDisplay.AutoSize = true;
            this.lblGMTDisplay.Location = new System.Drawing.Point(3, 33);
            this.lblGMTDisplay.Name = "lblGMTDisplay";
            this.lblGMTDisplay.Size = new System.Drawing.Size(217, 13);
            this.lblGMTDisplay.TabIndex = 2;
            this.lblGMTDisplay.Text = "All times are in Greenwich Mean Time (GMT)";
            //
            // lblDate
            //
            this.lblDate.AutoSize = true;
            this.lblDate.Location = new System.Drawing.Point(3, 52);
            this.lblDate.Name = "lblDate";
            this.lblDate.Size = new System.Drawing.Size(33, 13);
            this.lblDate.TabIndex = 3;
            this.lblDate.Text = "&Date:";
            //
            // dtDate
            //
            this.dtDate.DropDownAlign = System.Windows.Forms.LeftRightAlignment.Right;
            this.dtDate.Format = System.Windows.Forms.DateTimePickerFormat.Short;
            this.dtDate.Location = new System.Drawing.Point(6, 68);
            this.dtDate.Name = "dtDate";
            this.dtDate.ShowCheckBox = true;
            this.dtDate.Size = new System.Drawing.Size(237, 20);
            this.dtDate.TabIndex = 4;
            //
            // lblTime
            //
            this.lblTime.AutoSize = true;
            this.lblTime.Location = new System.Drawing.Point(3, 123);
            this.lblTime.Name = "lblTime";
            this.lblTime.Size = new System.Drawing.Size(33, 13);
            this.lblTime.TabIndex = 5;
            this.lblTime.Text = "&Time:";
            //
            // dtTime
            //
            this.dtTime.Format = System.Windows.Forms.DateTimePickerFormat.Time;
            this.dtTime.Location = new System.Drawing.Point(6, 139);
            this.dtTime.Name = "dtTime";
            this.dtTime.ShowCheckBox = true;
            this.dtTime.ShowUpDown = true;
            this.dtTime.Size = new System.Drawing.Size(237, 20);
            this.dtTime.TabIndex = 6;
            //
            // btnOK
            //
            this.btnOK.Location = new System.Drawing.Point(89, 211);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(75, 23);
            this.btnOK.TabIndex = 7;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            //
            // btnCancel
            //
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(168, 211);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 8;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            //
            // TimeAttributeEditor
            //
            this.AcceptButton = this.btnOK;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(256, 255);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.dtTime);
            this.Controls.Add(this.lblTime);
            this.Controls.Add(this.dtDate);
            this.Controls.Add(this.lblDate);
            this.Controls.Add(this.lblGMTDisplay);
            this.Controls.Add(this.lblSelectedValue);
            this.Controls.Add(this.lblTimeAttribute);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "TimeAttributeEditor";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Time Attribute Editor";
            this.Load += new System.EventHandler(this.TimeAttributeEditor_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblTimeAttribute;
        private System.Windows.Forms.Label lblSelectedValue;
        private System.Windows.Forms.Label lblGMTDisplay;
        private System.Windows.Forms.Label lblDate;
        private System.Windows.Forms.DateTimePicker dtDate;
        private System.Windows.Forms.Label lblTime;
        private System.Windows.Forms.DateTimePicker dtTime;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnCancel;
    }
}