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
    partial class MultiStringValueEditorDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MultiStringValueEditorDialog));
            this.lblValuename = new System.Windows.Forms.Label();
            this.txtValuename = new System.Windows.Forms.TextBox();
            this.lblValuedata = new System.Windows.Forms.Label();
            this.btnCancel = new System.Windows.Forms.Button();
            this.btnOk = new System.Windows.Forms.Button();
            this.richTextBoxValueData = new System.Windows.Forms.RichTextBox();
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
            this.txtValuename.Size = new System.Drawing.Size(300, 20);
            this.txtValuename.TabIndex = 1;
            this.txtValuename.TextChanged += new System.EventHandler(this.txtValuename_TextChanged);
            //
            // lblValuedata
            //
            this.lblValuedata.AutoSize = true;
            this.lblValuedata.Location = new System.Drawing.Point(5, 55);
            this.lblValuedata.Name = "lblValuedata";
            this.lblValuedata.Size = new System.Drawing.Size(61, 13);
            this.lblValuedata.TabIndex = 2;
            this.lblValuedata.Text = "Value data:";
            //
            // btnCancel
            //
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(227, 257);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 7;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            //
            // btnOk
            //
            this.btnOk.Location = new System.Drawing.Point(146, 257);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(75, 23);
            this.btnOk.TabIndex = 6;
            this.btnOk.Text = "&Ok";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            //
            // richTextBoxValueData
            //
            this.richTextBoxValueData.AcceptsTab = true;
            this.richTextBoxValueData.Location = new System.Drawing.Point(8, 71);
            this.richTextBoxValueData.Name = "richTextBoxValueData";
            this.richTextBoxValueData.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.ForcedVertical;
            this.richTextBoxValueData.Size = new System.Drawing.Size(299, 178);
            this.richTextBoxValueData.TabIndex = 9;
            this.richTextBoxValueData.Text = "";
            //
            // MultiStringValueEditorDialog
            //
            this.AcceptButton = this.btnOk;
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.CancelButton = this.btnCancel;
            this.ClientSize = new System.Drawing.Size(314, 286);
            this.Controls.Add(this.richTextBoxValueData);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.lblValuedata);
            this.Controls.Add(this.txtValuename);
            this.Controls.Add(this.lblValuename);
            this.HelpButton = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "MultiStringValueEditorDialog";
            this.Text = "Edit String Value";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblValuename;
        private System.Windows.Forms.TextBox txtValuename;
        private System.Windows.Forms.Label lblValuedata;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.RichTextBox richTextBoxValueData;
    }
}