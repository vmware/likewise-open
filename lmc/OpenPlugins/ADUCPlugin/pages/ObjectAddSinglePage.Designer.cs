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
    partial class ObjectAddSinglePage
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
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.cnSyntaxlabel = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.cntextBox = new System.Windows.Forms.TextBox();
            this.attrNameLabel = new System.Windows.Forms.Label();
            this.SuspendLayout();
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(28, 38);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(65, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Attribute: ";
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(31, 74);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(56, 13);
            this.label2.TabIndex = 1;
            this.label2.Text = "Syntax: ";
            //
            // cnSyntaxlabel
            //
            this.cnSyntaxlabel.AutoSize = true;
            this.cnSyntaxlabel.Location = new System.Drawing.Point(99, 74);
            this.cnSyntaxlabel.Name = "cnSyntaxlabel";
            this.cnSyntaxlabel.Size = new System.Drawing.Size(47, 13);
            this.cnSyntaxlabel.TabIndex = 2;
            this.cnSyntaxlabel.Text = "Syntax";
            //
            // label5
            //
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(34, 117);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(44, 13);
            this.label5.TabIndex = 4;
            this.label5.Text = "Value:";
            //
            // cntextBox
            //
            this.cntextBox.Location = new System.Drawing.Point(99, 114);
            this.cntextBox.Name = "cntextBox";
            this.cntextBox.Size = new System.Drawing.Size(233, 21);
            this.cntextBox.TabIndex = 5;
            this.cntextBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.cntextBox_KeyPress);
            this.cntextBox.TextChanged += new System.EventHandler(this.cntextBox_TextChanged);
            //
            // attrNameLabel
            //
            this.attrNameLabel.AutoSize = true;
            this.attrNameLabel.Location = new System.Drawing.Point(99, 38);
            this.attrNameLabel.Name = "attrNameLabel";
            this.attrNameLabel.Size = new System.Drawing.Size(92, 13);
            this.attrNameLabel.TabIndex = 6;
            this.attrNameLabel.Text = "Attribute name";
            //
            // ObjectAddSinglePage
            //
            this.Controls.Add(this.attrNameLabel);
            this.Controls.Add(this.cntextBox);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.cnSyntaxlabel);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.MaximumSize = new System.Drawing.Size(430, 270);
            this.MinimumSize = new System.Drawing.Size(430, 270);
            this.Name = "ObjectAddSinglePage";
            this.Size = new System.Drawing.Size(430, 270);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label cnSyntaxlabel;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox cntextBox;
        private System.Windows.Forms.Label attrNameLabel;
    }
}
