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

namespace Likewise.LMC.Plugins.EventlogPlugin
{
    partial class EventPropertiesPage
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

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.btnNext = new System.Windows.Forms.Button();
            this.btnPrev = new System.Windows.Forms.Button();
            this.tbDescription = new System.Windows.Forms.TextBox();
            this.label9 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label10 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.tbTime = new System.Windows.Forms.TextBox();
            this.tbType = new System.Windows.Forms.TextBox();
            this.tbUser = new System.Windows.Forms.TextBox();
            this.tbComputer = new System.Windows.Forms.TextBox();
            this.tbSource = new System.Windows.Forms.TextBox();
            this.tbCategory = new System.Windows.Forms.TextBox();
            this.tbEventId = new System.Windows.Forms.TextBox();
            this.tbDate = new System.Windows.Forms.TextBox();
            this.btnCopy = new System.Windows.Forms.Button();
            this.pnlData.SuspendLayout();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.BackColor = System.Drawing.SystemColors.Control;
            this.pnlData.Controls.Add(this.btnCopy);
            this.pnlData.Controls.Add(this.tbDate);
            this.pnlData.Controls.Add(this.tbEventId);
            this.pnlData.Controls.Add(this.tbCategory);
            this.pnlData.Controls.Add(this.tbSource);
            this.pnlData.Controls.Add(this.tbComputer);
            this.pnlData.Controls.Add(this.tbDescription);
            this.pnlData.Controls.Add(this.tbUser);
            this.pnlData.Controls.Add(this.tbType);
            this.pnlData.Controls.Add(this.tbTime);
            this.pnlData.Size = new System.Drawing.Size(437, 411);
            this.pnlData.TabIndex = 0;
            // 
            // btnNext
            // 
            this.btnNext.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnNext.Font = new System.Drawing.Font("Wingdings", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(2)));
            this.btnNext.Image = global::Likewise.LMC.Plugins.EventlogPlugin.Properties.Resources.down;
            this.btnNext.Location = new System.Drawing.Point(396, 55);
            this.btnNext.Name = "btnNext";
            this.btnNext.Size = new System.Drawing.Size(31, 27);
            this.btnNext.TabIndex = 10;
            this.btnNext.UseVisualStyleBackColor = true;
            this.btnNext.Click += new System.EventHandler(this.btnNext_Click);
            // 
            // btnPrev
            // 
            this.btnPrev.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnPrev.Font = new System.Drawing.Font("Wingdings", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(2)));
            this.btnPrev.Image = global::Likewise.LMC.Plugins.EventlogPlugin.Properties.Resources.up;
            this.btnPrev.Location = new System.Drawing.Point(396, 27);
            this.btnPrev.Name = "btnPrev";
            this.btnPrev.Size = new System.Drawing.Size(31, 27);
            this.btnPrev.TabIndex = 9;
            this.btnPrev.UseVisualStyleBackColor = true;
            this.btnPrev.Click += new System.EventHandler(this.btnPrev_Click);
            // 
            // tbDescription
            // 
            this.tbDescription.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.tbDescription.BackColor = System.Drawing.SystemColors.Control;
            this.tbDescription.Location = new System.Drawing.Point(13, 176);
            this.tbDescription.Multiline = true;
            this.tbDescription.Name = "tbDescription";
            this.tbDescription.ReadOnly = true;
            this.tbDescription.Size = new System.Drawing.Size(414, 224);
            this.tbDescription.TabIndex = 12;
            this.tbDescription.TabStop = false;
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.BackColor = System.Drawing.SystemColors.Control;
            this.label9.Location = new System.Drawing.Point(10, 158);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(63, 13);
            this.label9.TabIndex = 11;
            this.label9.Text = "&Description:";
            // 
            // label5
            // 
            this.label5.AutoEllipsis = true;
            this.label5.AutoSize = true;
            this.label5.BackColor = System.Drawing.SystemColors.Control;
            this.label5.Location = new System.Drawing.Point(8, 128);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(55, 13);
            this.label5.TabIndex = 5;
            this.label5.Text = "C&omputer:";
            // 
            // label4
            // 
            this.label4.AutoEllipsis = true;
            this.label4.AutoSize = true;
            this.label4.BackColor = System.Drawing.SystemColors.Control;
            this.label4.Location = new System.Drawing.Point(10, 96);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(32, 13);
            this.label4.TabIndex = 4;
            this.label4.Text = "&User:";
            // 
            // label3
            // 
            this.label3.AutoEllipsis = true;
            this.label3.AutoSize = true;
            this.label3.BackColor = System.Drawing.SystemColors.Control;
            this.label3.Location = new System.Drawing.Point(10, 68);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(34, 13);
            this.label3.TabIndex = 3;
            this.label3.Text = "Typ&e:";
            // 
            // label2
            // 
            this.label2.AutoEllipsis = true;
            this.label2.AutoSize = true;
            this.label2.BackColor = System.Drawing.SystemColors.Control;
            this.label2.Location = new System.Drawing.Point(10, 39);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(33, 13);
            this.label2.TabIndex = 2;
            this.label2.Text = "Ti&me:";
            // 
            // label10
            // 
            this.label10.AutoEllipsis = true;
            this.label10.AutoSize = true;
            this.label10.BackColor = System.Drawing.SystemColors.Control;
            this.label10.Location = new System.Drawing.Point(183, 41);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(52, 13);
            this.label10.TabIndex = 7;
            this.label10.Text = "Catego&ry:";
            // 
            // label8
            // 
            this.label8.AutoEllipsis = true;
            this.label8.AutoSize = true;
            this.label8.BackColor = System.Drawing.SystemColors.Control;
            this.label8.Location = new System.Drawing.Point(183, 69);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(52, 13);
            this.label8.TabIndex = 8;
            this.label8.Text = "Event &ID:";
            // 
            // label6
            // 
            this.label6.AutoEllipsis = true;
            this.label6.AutoSize = true;
            this.label6.BackColor = System.Drawing.SystemColors.Control;
            this.label6.Location = new System.Drawing.Point(183, 11);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(44, 13);
            this.label6.TabIndex = 6;
            this.label6.Text = "&Source:";
            // 
            // label1
            // 
            this.label1.AutoEllipsis = true;
            this.label1.AutoSize = true;
            this.label1.BackColor = System.Drawing.SystemColors.Control;
            this.label1.Location = new System.Drawing.Point(10, 11);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(33, 13);
            this.label1.TabIndex = 1;
            this.label1.Text = "D&ate:";
            // 
            // tbTime
            // 
            this.tbTime.BackColor = System.Drawing.SystemColors.Control;
            this.tbTime.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.tbTime.Location = new System.Drawing.Point(70, 39);
            this.tbTime.Multiline = true;
            this.tbTime.Name = "tbTime";
            this.tbTime.ReadOnly = true;
            this.tbTime.Size = new System.Drawing.Size(104, 20);
            this.tbTime.TabIndex = 1;
            // 
            // tbType
            // 
            this.tbType.BackColor = System.Drawing.SystemColors.Control;
            this.tbType.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.tbType.Location = new System.Drawing.Point(70, 67);
            this.tbType.Multiline = true;
            this.tbType.Name = "tbType";
            this.tbType.ReadOnly = true;
            this.tbType.Size = new System.Drawing.Size(104, 20);
            this.tbType.TabIndex = 2;
            // 
            // tbUser
            // 
            this.tbUser.BackColor = System.Drawing.SystemColors.Control;
            this.tbUser.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.tbUser.Location = new System.Drawing.Point(70, 96);
            this.tbUser.Multiline = true;
            this.tbUser.Name = "tbUser";
            this.tbUser.ReadOnly = true;
            this.tbUser.Size = new System.Drawing.Size(318, 20);
            this.tbUser.TabIndex = 3;
            // 
            // tbComputer
            // 
            this.tbComputer.BackColor = System.Drawing.SystemColors.Control;
            this.tbComputer.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.tbComputer.Location = new System.Drawing.Point(83, 127);
            this.tbComputer.Multiline = true;
            this.tbComputer.Name = "tbComputer";
            this.tbComputer.ReadOnly = true;
            this.tbComputer.Size = new System.Drawing.Size(287, 20);
            this.tbComputer.TabIndex = 4;
            // 
            // tbSource
            // 
            this.tbSource.BackColor = System.Drawing.SystemColors.Control;
            this.tbSource.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.tbSource.Location = new System.Drawing.Point(250, 9);
            this.tbSource.Multiline = true;
            this.tbSource.Name = "tbSource";
            this.tbSource.ReadOnly = true;
            this.tbSource.Size = new System.Drawing.Size(177, 20);
            this.tbSource.TabIndex = 5;
            // 
            // tbCategory
            // 
            this.tbCategory.BackColor = System.Drawing.SystemColors.Control;
            this.tbCategory.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.tbCategory.Location = new System.Drawing.Point(250, 40);
            this.tbCategory.Multiline = true;
            this.tbCategory.Name = "tbCategory";
            this.tbCategory.ReadOnly = true;
            this.tbCategory.Size = new System.Drawing.Size(138, 20);
            this.tbCategory.TabIndex = 6;
            // 
            // tbEventId
            // 
            this.tbEventId.BackColor = System.Drawing.SystemColors.Control;
            this.tbEventId.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.tbEventId.Location = new System.Drawing.Point(250, 69);
            this.tbEventId.Multiline = true;
            this.tbEventId.Name = "tbEventId";
            this.tbEventId.ReadOnly = true;
            this.tbEventId.Size = new System.Drawing.Size(139, 20);
            this.tbEventId.TabIndex = 7;
            // 
            // tbDate
            // 
            this.tbDate.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.tbDate.Location = new System.Drawing.Point(70, 9);
            this.tbDate.Multiline = true;
            this.tbDate.Name = "tbDate";
            this.tbDate.ReadOnly = true;
            this.tbDate.Size = new System.Drawing.Size(104, 20);
            this.tbDate.TabIndex = 0;
            // 
            // btnCopy
            // 
            this.btnCopy.Location = new System.Drawing.Point(380, 149);
            this.btnCopy.Name = "btnCopy";
            this.btnCopy.Size = new System.Drawing.Size(47, 22);
            this.btnCopy.TabIndex = 8;
            this.btnCopy.Text = "Co&py";
            this.btnCopy.UseVisualStyleBackColor = true;
            this.btnCopy.Click += new System.EventHandler(this.btnCopy_Click);
            // 
            // EventPropertiesPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.btnNext);
            this.Controls.Add(this.btnPrev);
            this.Controls.Add(this.label9);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label10);
            this.Controls.Add(this.label8);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.label1);
            this.Name = "EventPropertiesPage";
            this.Size = new System.Drawing.Size(437, 411);
            this.Controls.SetChildIndex(this.pnlData, 0);
            this.Controls.SetChildIndex(this.label1, 0);
            this.Controls.SetChildIndex(this.label6, 0);
            this.Controls.SetChildIndex(this.label8, 0);
            this.Controls.SetChildIndex(this.label10, 0);
            this.Controls.SetChildIndex(this.label2, 0);
            this.Controls.SetChildIndex(this.label3, 0);
            this.Controls.SetChildIndex(this.label4, 0);
            this.Controls.SetChildIndex(this.label5, 0);
            this.Controls.SetChildIndex(this.label9, 0);
            this.Controls.SetChildIndex(this.btnPrev, 0);
            this.Controls.SetChildIndex(this.btnNext, 0);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnNext;
        private System.Windows.Forms.Button btnPrev;
        private System.Windows.Forms.TextBox tbDescription;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox tbTime;
        private System.Windows.Forms.TextBox tbSource;
        private System.Windows.Forms.TextBox tbComputer;
        private System.Windows.Forms.TextBox tbUser;
        private System.Windows.Forms.TextBox tbType;
        private System.Windows.Forms.TextBox tbEventId;
        private System.Windows.Forms.TextBox tbCategory;
        private System.Windows.Forms.TextBox tbDate;
        private System.Windows.Forms.Button btnCopy;
    }
}
