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

using System.Collections.Generic;
namespace Likewise.LMC.Plugins.ADUCPlugin
{
    partial class ObjectAddAttributesTab
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
            this.lbPath = new System.Windows.Forms.Label();
            this.label = new System.Windows.Forms.Label();
            this.lbWhichProperty = new System.Windows.Forms.Label();
            this.lbPropertyview = new System.Windows.Forms.Label();
            this.groupBox = new System.Windows.Forms.GroupBox();
            this.txtValues = new System.Windows.Forms.TextBox();
            this.listboxValues = new System.Windows.Forms.ListBox();
            this.txtEditAttribute = new System.Windows.Forms.TextBox();
            this.lbValues = new System.Windows.Forms.Label();
            this.lbEdit = new System.Windows.Forms.Label();
            this.txtSyntax = new System.Windows.Forms.TextBox();
            this.btnClear = new System.Windows.Forms.Button();
            this.btnSet = new System.Windows.Forms.Button();
            this.cbProOptionorMandatory = new System.Windows.Forms.ComboBox();
            this.cbProperty = new System.Windows.Forms.ComboBox();
            this.lbSyntax = new System.Windows.Forms.Label();
            this.lbClass = new System.Windows.Forms.Label();
            this.pnlData.SuspendLayout();
            this.groupBox.SuspendLayout();
            this.SuspendLayout();
            //
            // pnlData
            //
            this.pnlData.AutoSize = true;
            this.pnlData.Controls.Add(this.lbClass);
            this.pnlData.Controls.Add(this.lbSyntax);
            this.pnlData.Controls.Add(this.cbProperty);
            this.pnlData.Controls.Add(this.cbProOptionorMandatory);
            this.pnlData.Controls.Add(this.groupBox);
            this.pnlData.Controls.Add(this.lbPropertyview);
            this.pnlData.Controls.Add(this.lbWhichProperty);
            this.pnlData.Controls.Add(this.label);
            this.pnlData.Controls.Add(this.lbPath);
            this.pnlData.Dock = System.Windows.Forms.DockStyle.None;
            this.pnlData.Size = new System.Drawing.Size(396, 447);
            //
            // lbPath
            //
            this.lbPath.AutoSize = true;
            this.lbPath.Location = new System.Drawing.Point(11, 30);
            this.lbPath.Name = "lbPath";
            this.lbPath.Size = new System.Drawing.Size(32, 13);
            this.lbPath.TabIndex = 0;
            this.lbPath.Text = "&Path:";
            //
            // label
            //
            this.label.AutoSize = true;
            this.label.Location = new System.Drawing.Point(11, 58);
            this.label.Name = "label";
            this.label.Size = new System.Drawing.Size(35, 13);
            this.label.TabIndex = 1;
            this.label.Text = "C&lass:";
            //
            // lbWhichProperty
            //
            this.lbWhichProperty.AutoSize = true;
            this.lbWhichProperty.Location = new System.Drawing.Point(11, 89);
            this.lbWhichProperty.Name = "lbWhichProperty";
            this.lbWhichProperty.Size = new System.Drawing.Size(157, 13);
            this.lbWhichProperty.TabIndex = 2;
            this.lbWhichProperty.Text = "Select &which properties to view:";
            //
            // lbPropertyview
            //
            this.lbPropertyview.AutoSize = true;
            this.lbPropertyview.Location = new System.Drawing.Point(11, 125);
            this.lbPropertyview.Name = "lbPropertyview";
            this.lbPropertyview.Size = new System.Drawing.Size(135, 13);
            this.lbPropertyview.TabIndex = 3;
            this.lbPropertyview.Text = "Select a properties to &view:";
            //
            // groupBox
            //
            this.groupBox.Controls.Add(this.txtValues);
            this.groupBox.Controls.Add(this.listboxValues);
            this.groupBox.Controls.Add(this.txtEditAttribute);
            this.groupBox.Controls.Add(this.lbValues);
            this.groupBox.Controls.Add(this.lbEdit);
            this.groupBox.Controls.Add(this.txtSyntax);
            this.groupBox.Controls.Add(this.btnClear);
            this.groupBox.Controls.Add(this.btnSet);
            this.groupBox.Location = new System.Drawing.Point(10, 155);
            this.groupBox.Name = "groupBox";
            this.groupBox.Size = new System.Drawing.Size(372, 260);
            this.groupBox.TabIndex = 4;
            this.groupBox.TabStop = false;
            this.groupBox.Text = "Attributes Values";
            //
            // txtValues
            //
            this.txtValues.ReadOnly = true;
            this.txtValues.Location = new System.Drawing.Point(104, 117);
            this.txtValues.Name = "txtValues";
            this.txtValues.Size = new System.Drawing.Size(250, 20);
            this.txtValues.TabIndex = 8;
            //
            // listboxValues
            //
            this.listboxValues.FormattingEnabled = true;
            this.listboxValues.Location = new System.Drawing.Point(104, 118);
            this.listboxValues.Name = "listboxValues";
            this.listboxValues.Size = new System.Drawing.Size(249, 82);
            this.listboxValues.TabIndex = 10;
            this.listboxValues.Visible = false;
            this.listboxValues.SelectedIndexChanged += new System.EventHandler(this.listboxValues_SelectedIndexChanged);
            //
            // txtEditAttribute
            //
            this.txtEditAttribute.Location = new System.Drawing.Point(103, 73);
            this.txtEditAttribute.Name = "txtEditAttribute";
            this.txtEditAttribute.Size = new System.Drawing.Size(250, 20);
            this.txtEditAttribute.TabIndex = 8;
            this.txtEditAttribute.TextChanged += new System.EventHandler(this.txtEditAttribute_TextChanged);
            //
            // lbValues
            //
            this.lbValues.AutoSize = true;
            this.lbValues.Location = new System.Drawing.Point(20, 120);
            this.lbValues.Name = "lbValues";
            this.lbValues.Size = new System.Drawing.Size(48, 13);
            this.lbValues.TabIndex = 9;
            this.lbValues.Text = "V&alue(s):";
            //
            // lbEdit
            //
            this.lbEdit.AutoSize = true;
            this.lbEdit.Location = new System.Drawing.Point(20, 76);
            this.lbEdit.Name = "lbEdit";
            this.lbEdit.Size = new System.Drawing.Size(70, 13);
            this.lbEdit.TabIndex = 9;
            this.lbEdit.Text = "&Edit Attribute:";
            //
            // txtSyntax
            //
            this.txtSyntax.Enabled = false;
            this.txtSyntax.Location = new System.Drawing.Point(103, 38);
            this.txtSyntax.Name = "txtSyntax";
            this.txtSyntax.ReadOnly = true;
            this.txtSyntax.Size = new System.Drawing.Size(250, 20);
            this.txtSyntax.TabIndex = 2;
            //
            // btnClear
            //
            this.btnClear.Enabled = false;
            this.btnClear.Location = new System.Drawing.Point(270, 222);
            this.btnClear.Name = "btnClear";
            this.btnClear.Size = new System.Drawing.Size(79, 24);
            this.btnClear.TabIndex = 1;
            this.btnClear.Text = "&Clear";
            this.btnClear.UseVisualStyleBackColor = true;
            this.btnClear.Click += new System.EventHandler(this.btnClear_Click);
            //
            // btnSet
            //
            this.btnSet.Enabled = false;
            this.btnSet.Location = new System.Drawing.Point(179, 222);
            this.btnSet.Name = "btnSet";
            this.btnSet.Size = new System.Drawing.Size(79, 24);
            this.btnSet.TabIndex = 0;
            this.btnSet.Text = "&Set";
            this.btnSet.UseVisualStyleBackColor = true;
            this.btnSet.Click += new System.EventHandler(this.btnSet_Click);
            //
            // cbProOptionorMandatory
            //
            this.cbProOptionorMandatory.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbProOptionorMandatory.FormattingEnabled = true;
            this.cbProOptionorMandatory.Items.AddRange(new object[] {
            "Mandatory",
            "Optional",
            "Both"});
            this.cbProOptionorMandatory.Location = new System.Drawing.Point(179, 86);
            this.cbProOptionorMandatory.Name = "cbProOptionorMandatory";
            this.cbProOptionorMandatory.Size = new System.Drawing.Size(181, 21);
            this.cbProOptionorMandatory.TabIndex = 5;
            this.cbProOptionorMandatory.SelectedIndexChanged += new System.EventHandler(this.cbProOptionorMandatory_SelectedIndexChanged);
            //
            // cbProperty
            //
            this.cbProperty.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cbProperty.FormattingEnabled = true;
            this.cbProperty.Location = new System.Drawing.Point(179, 122);
            this.cbProperty.Name = "cbProperty";
            this.cbProperty.Size = new System.Drawing.Size(184, 21);
            this.cbProperty.TabIndex = 6;
            this.cbProperty.SelectedIndexChanged += new System.EventHandler(this.cbProperty_SelectedIndexChanged);
            //
            // lbSyntax
            //
            this.lbSyntax.AutoSize = true;
            this.lbSyntax.Location = new System.Drawing.Point(50, 201);
            this.lbSyntax.Name = "lbSyntax";
            this.lbSyntax.Size = new System.Drawing.Size(42, 13);
            this.lbSyntax.TabIndex = 7;
            this.lbSyntax.Text = "Synta&x:";
            //
            // lbClass
            //
            this.lbClass.AutoSize = true;
            this.lbClass.Location = new System.Drawing.Point(49, 58);
            this.lbClass.Name = "lbClass";
            this.lbClass.Size = new System.Drawing.Size(0, 13);
            this.lbClass.TabIndex = 8;
            //
            // ObjectAddAttributesTab
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.Name = "ObjectAddAttributesTab";
            this.Size = new System.Drawing.Size(396, 447);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            this.groupBox.ResumeLayout(false);
            this.groupBox.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

            cbMandProperty = new List<string>();
            cbOptionProperty = new List<string>();
            cbBothProperty = new List<string>();
        }

        #endregion

        private System.Windows.Forms.Label lbPath;
        private System.Windows.Forms.Label label;
        private System.Windows.Forms.Label lbWhichProperty;
        private System.Windows.Forms.Label lbPropertyview;
        private System.Windows.Forms.GroupBox groupBox;
        private System.Windows.Forms.Button btnClear;
        private System.Windows.Forms.Button btnSet;
        private System.Windows.Forms.Label lbSyntax;
        private System.Windows.Forms.ComboBox cbProperty;
        private System.Windows.Forms.ComboBox cbProOptionorMandatory;
        private System.Windows.Forms.TextBox txtValues;
        private System.Windows.Forms.TextBox txtEditAttribute;
        private System.Windows.Forms.Label lbValues;
        private System.Windows.Forms.Label lbEdit;
        private System.Windows.Forms.TextBox txtSyntax;
        private System.Windows.Forms.ListBox listboxValues;
        private System.Windows.Forms.Label lbClass;

        private List<string> cbMandProperty;
        private List<string> cbOptionProperty;
        private List<string> cbBothProperty;
    }
}
