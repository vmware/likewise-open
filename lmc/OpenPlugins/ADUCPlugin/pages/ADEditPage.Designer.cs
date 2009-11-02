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
    partial class ADEditPage
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
            this.components = new System.ComponentModel.Container();
            this.errorProvider = new System.Windows.Forms.ErrorProvider(this.components);
            this.pnlAttributes = new System.Windows.Forms.Panel();
            this.lvAttrs = new Likewise.LMC.ServerControl.LWListView();
            this.colAttr = new System.Windows.Forms.ColumnHeader();
            this.colSyntax = new System.Windows.Forms.ColumnHeader();
            this.colValue = new System.Windows.Forms.ColumnHeader();
            this.colAttrType = new System.Windows.Forms.ColumnHeader();
            this.colAttrModified = new System.Windows.Forms.ColumnHeader();
            this.btnEdit = new System.Windows.Forms.Button();
            this.lblAttr = new System.Windows.Forms.Label();
            this.cbValueAttr = new System.Windows.Forms.CheckBox();
            this.cbOptionalAttr = new System.Windows.Forms.CheckBox();
            this.cbMandatoryAttr = new System.Windows.Forms.CheckBox();
            this.pnlData.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.errorProvider)).BeginInit();
            this.pnlAttributes.SuspendLayout();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.btnEdit);
            this.pnlData.Controls.Add(this.pnlAttributes);
            this.pnlData.Controls.Add(this.cbMandatoryAttr);
            this.pnlData.Controls.Add(this.lblAttr);
            this.pnlData.Controls.Add(this.cbOptionalAttr);
            this.pnlData.Controls.Add(this.cbValueAttr);
            this.pnlData.Size = new System.Drawing.Size(373, 411);
            // 
            // errorProvider
            // 
            this.errorProvider.BlinkStyle = System.Windows.Forms.ErrorBlinkStyle.NeverBlink;
            this.errorProvider.ContainerControl = this;
            // 
            // pnlAttributes
            // 
            this.pnlAttributes.Controls.Add(this.lvAttrs);
            this.pnlAttributes.Location = new System.Drawing.Point(15, 105);
            this.pnlAttributes.Name = "pnlAttributes";
            this.pnlAttributes.Size = new System.Drawing.Size(343, 255);
            this.pnlAttributes.TabIndex = 37;
            // 
            // lvAttrs
            // 
            this.lvAttrs.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)));
            this.lvAttrs.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colAttr,
            this.colSyntax,
            this.colValue,
            this.colAttrType,
            this.colAttrModified});
            this.lvAttrs.FullRowSelect = true;
            this.lvAttrs.HideSelection = false;
            this.lvAttrs.Location = new System.Drawing.Point(0, 4);
            this.lvAttrs.MultiSelect = false;
            this.lvAttrs.Name = "lvAttrs";
            this.lvAttrs.Size = new System.Drawing.Size(343, 252);
            this.lvAttrs.Sorting = System.Windows.Forms.SortOrder.Ascending;
            this.lvAttrs.TabIndex = 29;
            this.lvAttrs.UseCompatibleStateImageBehavior = false;
            this.lvAttrs.View = System.Windows.Forms.View.Details;
            this.lvAttrs.SelectedIndexChanged += new System.EventHandler(this.lvAttrs_SelectedIndexChanged);
            this.lvAttrs.DoubleClick += new System.EventHandler(this.btnEdit_Click);
            this.lvAttrs.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.lvAttrs_ColumnClick);
            // 
            // colAttr
            // 
            this.colAttr.Text = "Attribute";
            this.colAttr.Width = 115;
            // 
            // colSyntax
            // 
            this.colSyntax.Text = "Syntax";
            this.colSyntax.Width = 96;
            // 
            // colValue
            // 
            this.colValue.Text = "Value";
            this.colValue.Width = 300;
            // 
            // colAttrType
            // 
            this.colAttrType.Text = "";
            this.colAttrType.Width = 0;
            // 
            // colAttrModified
            // 
            this.colAttrModified.Text = "";
            this.colAttrModified.Width = 0;
            // 
            // btnEdit
            // 
            this.btnEdit.Enabled = false;
            this.btnEdit.Location = new System.Drawing.Point(15, 374);
            this.btnEdit.Name = "btnEdit";
            this.btnEdit.Size = new System.Drawing.Size(75, 23);
            this.btnEdit.TabIndex = 36;
            this.btnEdit.Text = "&Edit";
            this.btnEdit.UseVisualStyleBackColor = true;
            this.btnEdit.Click += new System.EventHandler(this.btnEdit_Click);
            // 
            // lblAttr
            // 
            this.lblAttr.AutoSize = true;
            this.lblAttr.Location = new System.Drawing.Point(12, 89);
            this.lblAttr.Name = "lblAttr";
            this.lblAttr.Size = new System.Drawing.Size(54, 13);
            this.lblAttr.TabIndex = 35;
            this.lblAttr.Text = "Attri&butes:";
            // 
            // cbValueAttr
            // 
            this.cbValueAttr.Location = new System.Drawing.Point(15, 65);
            this.cbValueAttr.Name = "cbValueAttr";
            this.cbValueAttr.Size = new System.Drawing.Size(338, 23);
            this.cbValueAttr.TabIndex = 34;
            this.cbValueAttr.Text = "Show only attributes that have &values                                           " +
                "  ";
            this.cbValueAttr.UseVisualStyleBackColor = true;
            this.cbValueAttr.CheckedChanged += new System.EventHandler(this.cbValueAttr_CheckedChanged);
            // 
            // cbOptionalAttr
            // 
            this.cbOptionalAttr.Checked = true;
            this.cbOptionalAttr.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbOptionalAttr.Location = new System.Drawing.Point(15, 41);
            this.cbOptionalAttr.Name = "cbOptionalAttr";
            this.cbOptionalAttr.Size = new System.Drawing.Size(340, 18);
            this.cbOptionalAttr.TabIndex = 33;
            this.cbOptionalAttr.Text = "Show &optional attributes                                                        " +
                "           ";
            this.cbOptionalAttr.UseVisualStyleBackColor = true;
            this.cbOptionalAttr.CheckedChanged += new System.EventHandler(this.cbOptionalAttr_CheckedChanged);
            // 
            // cbMandatoryAttr
            // 
            this.cbMandatoryAttr.Checked = true;
            this.cbMandatoryAttr.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbMandatoryAttr.Location = new System.Drawing.Point(15, 13);
            this.cbMandatoryAttr.Name = "cbMandatoryAttr";
            this.cbMandatoryAttr.Size = new System.Drawing.Size(343, 29);
            this.cbMandatoryAttr.TabIndex = 32;
            this.cbMandatoryAttr.Text = "Show &mandatory attributes                                                       " +
                "         ";
            this.cbMandatoryAttr.UseVisualStyleBackColor = true;
            this.cbMandatoryAttr.CheckedChanged += new System.EventHandler(this.cbMandatoryAttr_CheckedChanged);
            // 
            // ADEditPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.Name = "ADEditPage";
            this.Size = new System.Drawing.Size(373, 411);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.errorProvider)).EndInit();
            this.pnlAttributes.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ErrorProvider errorProvider;
        private System.Windows.Forms.Panel pnlAttributes;
        private Likewise.LMC.ServerControl.LWListView lvAttrs;
        private System.Windows.Forms.ColumnHeader colAttr;
        private System.Windows.Forms.ColumnHeader colSyntax;
        private System.Windows.Forms.ColumnHeader colValue;
        private System.Windows.Forms.ColumnHeader colAttrType;
        private System.Windows.Forms.ColumnHeader colAttrModified;
        private System.Windows.Forms.Button btnEdit;
        private System.Windows.Forms.Label lblAttr;
        private System.Windows.Forms.CheckBox cbValueAttr;
        private System.Windows.Forms.CheckBox cbOptionalAttr;
        private System.Windows.Forms.CheckBox cbMandatoryAttr;
    }
}
