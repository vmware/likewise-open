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

namespace System.DirectoryServices.Misc
{
    partial class DsPicker
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
            this.CancelBtn = new System.Windows.Forms.Button();
            this.Okbtn = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.textBoxObjectType = new System.Windows.Forms.TextBox();
            this.textBoxDomain = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.lvUserToGroup = new System.Windows.Forms.ListView();
            this.colName = new System.Windows.Forms.ColumnHeader();
            this.colEmail = new System.Windows.Forms.ColumnHeader();
            this.colDesc = new System.Windows.Forms.ColumnHeader();
            this.colPath = new System.Windows.Forms.ColumnHeader();
            this.backgroundWorker = new System.ComponentModel.BackgroundWorker();
            this.timer = new System.Windows.Forms.Timer(this.components);
            this.SuspendLayout();
            // 
            // CancelBtn
            // 
            this.CancelBtn.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.CancelBtn.Enabled = false;
            this.CancelBtn.Location = new System.Drawing.Point(388, 247);
            this.CancelBtn.Name = "CancelBtn";
            this.CancelBtn.Size = new System.Drawing.Size(74, 23);
            this.CancelBtn.TabIndex = 6;
            this.CancelBtn.Text = "&Cancel";
            this.CancelBtn.UseVisualStyleBackColor = true;
            this.CancelBtn.Click += new System.EventHandler(this.CancelBtn_Click);
            // 
            // Okbtn
            // 
            this.Okbtn.Enabled = false;
            this.Okbtn.Location = new System.Drawing.Point(307, 247);
            this.Okbtn.Name = "Okbtn";
            this.Okbtn.Size = new System.Drawing.Size(75, 23);
            this.Okbtn.TabIndex = 5;
            this.Okbtn.Text = "&OK";
            this.Okbtn.UseVisualStyleBackColor = true;
            this.Okbtn.Click += new System.EventHandler(this.Okbtn_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(3, 12);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(110, 13);
            this.label1.TabIndex = 7;
            this.label1.Text = "Selected object type :";
            // 
            // textBoxObjectType
            // 
            this.textBoxObjectType.Location = new System.Drawing.Point(136, 9);
            this.textBoxObjectType.Name = "textBoxObjectType";
            this.textBoxObjectType.ReadOnly = true;
            this.textBoxObjectType.Size = new System.Drawing.Size(335, 20);
            this.textBoxObjectType.TabIndex = 8;
            // 
            // textBoxDomain
            // 
            this.textBoxDomain.Location = new System.Drawing.Point(136, 38);
            this.textBoxDomain.Name = "textBoxDomain";
            this.textBoxDomain.ReadOnly = true;
            this.textBoxDomain.Size = new System.Drawing.Size(335, 20);
            this.textBoxDomain.TabIndex = 9;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(3, 40);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(95, 13);
            this.label2.TabIndex = 10;
            this.label2.Text = "From the Location:";
            // 
            // lvUserToGroup
            // 
            this.lvUserToGroup.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colName,
            this.colEmail,
            this.colDesc,
            this.colPath});
            this.lvUserToGroup.FullRowSelect = true;
            this.lvUserToGroup.HideSelection = false;
            this.lvUserToGroup.Location = new System.Drawing.Point(5, 72);
            this.lvUserToGroup.Name = "lvUserToGroup";
            this.lvUserToGroup.Size = new System.Drawing.Size(465, 169);
            this.lvUserToGroup.Sorting = System.Windows.Forms.SortOrder.Ascending;
            this.lvUserToGroup.TabIndex = 11;
            this.lvUserToGroup.UseCompatibleStateImageBehavior = false;
            this.lvUserToGroup.View = System.Windows.Forms.View.Details;
            this.lvUserToGroup.SelectedIndexChanged += new System.EventHandler(this.lvUserToGroup_SelectedIndexChanged);
            this.lvUserToGroup.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.lvUserToGroup_ColumnClick);
            // 
            // colName
            // 
            this.colName.Text = "Name [RDN]";
            this.colName.Width = 110;
            // 
            // colEmail
            // 
            this.colEmail.Text = "E-Mail Address";
            this.colEmail.Width = 110;
            // 
            // colDesc
            // 
            this.colDesc.Text = "Description";
            this.colDesc.Width = 110;
            // 
            // colPath
            // 
            this.colPath.Text = "In Folder";
            this.colPath.Width = 130;
            // 
            // backgroundWorker
            // 
            this.backgroundWorker.WorkerReportsProgress = true;
            this.backgroundWorker.WorkerSupportsCancellation = true;
            this.backgroundWorker.DoWork += new System.ComponentModel.DoWorkEventHandler(this.backgroundWorker_DoWork);
            this.backgroundWorker.RunWorkerCompleted += new System.ComponentModel.RunWorkerCompletedEventHandler(this.backgroundWorker_RunWorkerCompleted);
            this.backgroundWorker.ProgressChanged += new System.ComponentModel.ProgressChangedEventHandler(this.backgroundWorker_ProgressChanged);
            // 
            // timer
            // 
            this.timer.Interval = 300;
            this.timer.Tick += new System.EventHandler(this.timer_Tick);
            // 
            // DsPicker
            // 
            this.AcceptButton = this.Okbtn;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.CancelBtn;
            this.ClientSize = new System.Drawing.Size(476, 300);
            this.ControlBox = false;
            this.Controls.Add(this.lvUserToGroup);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.textBoxDomain);
            this.Controls.Add(this.textBoxObjectType);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.CancelBtn);
            this.Controls.Add(this.Okbtn);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "DsPicker";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "DsPicker";            
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button CancelBtn;
        private System.Windows.Forms.Button Okbtn;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox textBoxObjectType;
        private System.Windows.Forms.TextBox textBoxDomain;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ListView lvUserToGroup;
        private System.Windows.Forms.ColumnHeader colName;
        private System.Windows.Forms.ColumnHeader colEmail;
        private System.Windows.Forms.ColumnHeader colDesc;
        private System.Windows.Forms.ColumnHeader colPath;
        private System.ComponentModel.BackgroundWorker backgroundWorker;
        private System.Windows.Forms.Timer timer;
    }
}