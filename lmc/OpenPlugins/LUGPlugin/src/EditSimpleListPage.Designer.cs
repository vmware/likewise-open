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

namespace Likewise.LMC.Plugins.LUG
{
    partial class EditSimpleListPage
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(EditSimpleListPage));
            this.errorProvider = new System.Windows.Forms.ErrorProvider(this.components);
            this.pbSubjectImage = new System.Windows.Forms.PictureBox();
            this.gbSeparator = new System.Windows.Forms.GroupBox();
            this.lbSubject = new System.Windows.Forms.Label();
            this.lbDescription = new System.Windows.Forms.Label();
            this.tbDescription = new System.Windows.Forms.TextBox();
            this.lvMembers = new Likewise.LMC.ServerControl.LWListView();
            this.btnAdd = new System.Windows.Forms.Button();
            this.btnRemove = new System.Windows.Forms.Button();
            this.lbSubjectListRelationship = new System.Windows.Forms.Label();
            this.pnlData.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.errorProvider)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pbSubjectImage)).BeginInit();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.gbSeparator);
            this.pnlData.Controls.Add(this.lbSubjectListRelationship);
            this.pnlData.Controls.Add(this.btnRemove);
            this.pnlData.Controls.Add(this.btnAdd);
            this.pnlData.Controls.Add(this.lvMembers);
            this.pnlData.Controls.Add(this.tbDescription);
            this.pnlData.Controls.Add(this.lbDescription);
            this.pnlData.Controls.Add(this.lbSubject);
            this.pnlData.Controls.Add(this.pbSubjectImage);
            this.pnlData.Dock = System.Windows.Forms.DockStyle.None;
            this.pnlData.Size = new System.Drawing.Size(386, 380);
            this.pnlData.TabIndex = 0;
            // 
            // errorProvider
            // 
            this.errorProvider.BlinkStyle = System.Windows.Forms.ErrorBlinkStyle.NeverBlink;
            this.errorProvider.ContainerControl = this;
            // 
            // pbSubjectImage
            // 
            this.pbSubjectImage.Image = ((System.Drawing.Image)(resources.GetObject("pbSubjectImage.Image")));
            this.pbSubjectImage.Location = new System.Drawing.Point(17, 8);
            this.pbSubjectImage.Name = "pbSubjectImage";
            this.pbSubjectImage.Size = new System.Drawing.Size(32, 32);
            this.pbSubjectImage.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
            this.pbSubjectImage.TabIndex = 0;
            this.pbSubjectImage.TabStop = false;
            // 
            // gbSeparator
            // 
            this.gbSeparator.BackColor = System.Drawing.Color.Black;
            this.gbSeparator.Location = new System.Drawing.Point(17, 50);
            this.gbSeparator.Margin = new System.Windows.Forms.Padding(1);
            this.gbSeparator.Name = "gbSeparator";
            this.gbSeparator.Size = new System.Drawing.Size(352, 1);
            this.gbSeparator.TabIndex = 1;
            this.gbSeparator.TabStop = false;
            // 
            // lbSubject
            // 
            this.lbSubject.AutoSize = true;
            this.lbSubject.Location = new System.Drawing.Point(64, 18);
            this.lbSubject.Name = "lbSubject";
            this.lbSubject.Size = new System.Drawing.Size(21, 13);
            this.lbSubject.TabIndex = 0;
            this.lbSubject.Text = "{0}";
            // 
            // lbDescription
            // 
            this.lbDescription.AutoSize = true;
            this.lbDescription.Location = new System.Drawing.Point(14, 67);
            this.lbDescription.Name = "lbDescription";
            this.lbDescription.Size = new System.Drawing.Size(63, 13);
            this.lbDescription.TabIndex = 2;
            this.lbDescription.Text = "D&escription:";
            // 
            // tbDescription
            // 
            this.tbDescription.Location = new System.Drawing.Point(98, 64);
            this.tbDescription.Name = "tbDescription";
            this.tbDescription.Size = new System.Drawing.Size(271, 20);
            this.tbDescription.TabIndex = 3;
            // 
            // lvMembers
            // 
            this.lvMembers.FullRowSelect = true;
            this.lvMembers.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.None;
            this.lvMembers.HideSelection = false;
            this.lvMembers.Location = new System.Drawing.Point(17, 118);
            this.lvMembers.MultiSelect = false;
            this.lvMembers.Name = "lvMembers";
            this.lvMembers.ShowGroups = false;
            this.lvMembers.Size = new System.Drawing.Size(350, 219);
            this.lvMembers.Sorting = System.Windows.Forms.SortOrder.Ascending;
            this.lvMembers.TabIndex = 5;
            this.lvMembers.UseCompatibleStateImageBehavior = false;
            this.lvMembers.View = System.Windows.Forms.View.List;
            this.lvMembers.SelectedIndexChanged += new System.EventHandler(this.lvMembers_SelectedIndexChanged);
            // 
            // btnAdd
            // 
            this.btnAdd.Location = new System.Drawing.Point(17, 343);
            this.btnAdd.Name = "btnAdd";
            this.btnAdd.Size = new System.Drawing.Size(75, 23);
            this.btnAdd.TabIndex = 6;
            this.btnAdd.Text = "A&dd...";
            this.btnAdd.UseVisualStyleBackColor = true;
            this.btnAdd.Click += new System.EventHandler(this.btnAdd_Click);
            // 
            // btnRemove
            // 
            this.btnRemove.Enabled = false;
            this.btnRemove.Location = new System.Drawing.Point(98, 343);
            this.btnRemove.Name = "btnRemove";
            this.btnRemove.Size = new System.Drawing.Size(75, 23);
            this.btnRemove.TabIndex = 7;
            this.btnRemove.Text = "&Remove";
            this.btnRemove.UseVisualStyleBackColor = true;
            this.btnRemove.Click += new System.EventHandler(this.btnRemove_Click);
            // 
            // lbSubjectListRelationship
            // 
            this.lbSubjectListRelationship.AutoSize = true;
            this.lbSubjectListRelationship.Location = new System.Drawing.Point(14, 94);
            this.lbSubjectListRelationship.Name = "lbSubjectListRelationship";
            this.lbSubjectListRelationship.Size = new System.Drawing.Size(21, 13);
            this.lbSubjectListRelationship.TabIndex = 4;
            this.lbSubjectListRelationship.Text = "{0}";
            // 
            // EditSimpleListPage
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Name = "EditSimpleListPage";
            this.Size = new System.Drawing.Size(386, 380);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.errorProvider)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pbSubjectImage)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ErrorProvider errorProvider;
        private System.Windows.Forms.Button btnRemove;
        private System.Windows.Forms.Button btnAdd;
        private Likewise.LMC.ServerControl.LWListView lvMembers;
        private System.Windows.Forms.TextBox tbDescription;
        private System.Windows.Forms.Label lbDescription;
        private System.Windows.Forms.Label lbSubject;
        private System.Windows.Forms.GroupBox gbSeparator;
        private System.Windows.Forms.PictureBox pbSubjectImage;
        private System.Windows.Forms.Label lbSubjectListRelationship;
    }
}
