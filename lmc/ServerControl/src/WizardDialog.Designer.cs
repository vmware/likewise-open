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

namespace Likewise.LMC.ServerControl
{
    partial class WizardDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WizardDialog));
            this.panel_content = new System.Windows.Forms.Panel();
            this.panel_controls = new System.Windows.Forms.Panel();
            this.m_buttonStart = new System.Windows.Forms.Button();
            this.m_buttonFinish = new System.Windows.Forms.Button();
            this.m_buttonNext = new System.Windows.Forms.Button();
            this.m_buttonBack = new System.Windows.Forms.Button();
            this.m_buttonCancel = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.panel_controls.SuspendLayout();
            this.SuspendLayout();
            //
            // panel_content
            //
            this.panel_content.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel_content.Location = new System.Drawing.Point(0, 0);
            this.panel_content.Name = "panel_content";
            this.panel_content.Size = new System.Drawing.Size(495, 358);
            this.panel_content.TabIndex = 0;
            //
            // panel_controls
            //
            this.panel_controls.Controls.Add(this.m_buttonStart);
            this.panel_controls.Controls.Add(this.m_buttonFinish);
            this.panel_controls.Controls.Add(this.m_buttonNext);
            this.panel_controls.Controls.Add(this.m_buttonBack);
            this.panel_controls.Controls.Add(this.m_buttonCancel);
            this.panel_controls.Controls.Add(this.groupBox1);
            this.panel_controls.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel_controls.Location = new System.Drawing.Point(0, 304);
            this.panel_controls.Name = "panel_controls";
            this.panel_controls.Size = new System.Drawing.Size(495, 54);
            this.panel_controls.TabIndex = 1;
            //
            // m_buttonStart
            //
            this.m_buttonStart.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.m_buttonStart.Location = new System.Drawing.Point(296, 19);
            this.m_buttonStart.Name = "m_buttonStart";
            this.m_buttonStart.Size = new System.Drawing.Size(87, 23);
            this.m_buttonStart.TabIndex = 6;
            this.m_buttonStart.Text = "Start";
            this.m_buttonStart.UseVisualStyleBackColor = true;
            this.m_buttonStart.Click += new System.EventHandler(this.m_buttonStart_Click);
            //
            // m_buttonFinish
            //
            this.m_buttonFinish.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.m_buttonFinish.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.m_buttonFinish.Location = new System.Drawing.Point(296, 19);
            this.m_buttonFinish.Name = "m_buttonFinish";
            this.m_buttonFinish.Size = new System.Drawing.Size(87, 23);
            this.m_buttonFinish.TabIndex = 3;
            this.m_buttonFinish.Text = "&Finish";
            this.m_buttonFinish.UseVisualStyleBackColor = true;
            this.m_buttonFinish.Click += new System.EventHandler(this.OnClickFinish);
            //
            // m_buttonNext
            //
            this.m_buttonNext.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.m_buttonNext.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.m_buttonNext.Location = new System.Drawing.Point(296, 19);
            this.m_buttonNext.Name = "m_buttonNext";
            this.m_buttonNext.Size = new System.Drawing.Size(87, 23);
            this.m_buttonNext.TabIndex = 2;
            this.m_buttonNext.Text = "&Next >";
            this.m_buttonNext.UseVisualStyleBackColor = true;
            this.m_buttonNext.Click += new System.EventHandler(this.OnClickNext);
            //
            // m_buttonBack
            //
            this.m_buttonBack.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.m_buttonBack.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.m_buttonBack.Location = new System.Drawing.Point(203, 19);
            this.m_buttonBack.Name = "m_buttonBack";
            this.m_buttonBack.Size = new System.Drawing.Size(87, 23);
            this.m_buttonBack.TabIndex = 1;
            this.m_buttonBack.Text = "< &Back";
            this.m_buttonBack.UseVisualStyleBackColor = true;
            this.m_buttonBack.Click += new System.EventHandler(this.OnClickBack);
            //
            // m_buttonCancel
            //
            this.m_buttonCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.m_buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.m_buttonCancel.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.m_buttonCancel.Location = new System.Drawing.Point(389, 19);
            this.m_buttonCancel.Name = "m_buttonCancel";
            this.m_buttonCancel.Size = new System.Drawing.Size(87, 23);
            this.m_buttonCancel.TabIndex = 4;
            this.m_buttonCancel.Text = "&Cancel";
            this.m_buttonCancel.UseVisualStyleBackColor = true;
            this.m_buttonCancel.Click += new System.EventHandler(this.OnClickCancel);
            //
            // groupBox1
            //
            this.groupBox1.Dock = System.Windows.Forms.DockStyle.Top;
            this.groupBox1.Location = new System.Drawing.Point(0, 0);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(495, 2);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "groupBox1";
            //
            // WizardDialog
            //
            this.AcceptButton = this.m_buttonNext;
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.CancelButton = this.m_buttonCancel;
            this.ClientSize = new System.Drawing.Size(495, 366);
            this.Controls.Add(this.panel_controls);
            this.Controls.Add(this.panel_content);
            this.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MaximumSize = new System.Drawing.Size(503, 392);
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(503, 392);
            this.Name = "WizardDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "WizardForm";
            this.Load += new System.EventHandler(this.WizardForm_Load);
            this.panel_controls.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel panel_controls;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Button m_buttonFinish;
        private System.Windows.Forms.Button m_buttonNext;
        private System.Windows.Forms.Button m_buttonBack;
        private System.Windows.Forms.Button m_buttonCancel;
        private System.Windows.Forms.Button m_buttonStart;
        private System.Windows.Forms.Panel panel_content;
    }
}