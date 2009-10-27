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
    partial class LMCCustomView
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
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.cbActionPane = new System.Windows.Forms.CheckBox();
            this.cbTaskPad = new System.Windows.Forms.CheckBox();
            this.cbStatusbar = new System.Windows.Forms.CheckBox();
            this.cbToolBar = new System.Windows.Forms.CheckBox();
            this.cbStandardMenus = new System.Windows.Forms.CheckBox();
            this.cbConsoleTree = new System.Windows.Forms.CheckBox();
            this.pnlData.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // pnlData
            // 
            this.pnlData.Controls.Add(this.groupBox2);
            this.pnlData.Size = new System.Drawing.Size(325, 186);
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.cbActionPane);
            this.groupBox2.Controls.Add(this.cbTaskPad);
            this.groupBox2.Controls.Add(this.cbStatusbar);
            this.groupBox2.Controls.Add(this.cbToolBar);
            this.groupBox2.Controls.Add(this.cbStandardMenus);
            this.groupBox2.Controls.Add(this.cbConsoleTree);
            this.groupBox2.Location = new System.Drawing.Point(8, 9);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(309, 166);
            this.groupBox2.TabIndex = 2;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "LMC";
            // 
            // cbActionPane
            // 
            this.cbActionPane.AutoSize = true;
            this.cbActionPane.Location = new System.Drawing.Point(21, 136);
            this.cbActionPane.Name = "cbActionPane";
            this.cbActionPane.Size = new System.Drawing.Size(83, 17);
            this.cbActionPane.TabIndex = 6;
            this.cbActionPane.Text = "&Action pane";
            this.cbActionPane.UseVisualStyleBackColor = true;
            this.cbActionPane.CheckedChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // cbTaskPad
            // 
            this.cbTaskPad.AutoSize = true;
            this.cbTaskPad.Checked = true;
            this.cbTaskPad.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbTaskPad.Location = new System.Drawing.Point(21, 113);
            this.cbTaskPad.Name = "cbTaskPad";
            this.cbTaskPad.Size = new System.Drawing.Size(143, 17);
            this.cbTaskPad.TabIndex = 5;
            this.cbTaskPad.Text = "Taskpad &navigation tabs";
            this.cbTaskPad.UseVisualStyleBackColor = true;
            this.cbTaskPad.CheckedChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // cbStatusbar
            // 
            this.cbStatusbar.AutoSize = true;
            this.cbStatusbar.Checked = true;
            this.cbStatusbar.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbStatusbar.Location = new System.Drawing.Point(21, 89);
            this.cbStatusbar.Name = "cbStatusbar";
            this.cbStatusbar.Size = new System.Drawing.Size(74, 17);
            this.cbStatusbar.TabIndex = 3;
            this.cbStatusbar.Text = "&Status bar";
            this.cbStatusbar.UseVisualStyleBackColor = true;
            this.cbStatusbar.CheckedChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // cbToolBar
            // 
            this.cbToolBar.AutoSize = true;
            this.cbToolBar.Checked = true;
            this.cbToolBar.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbToolBar.Location = new System.Drawing.Point(21, 66);
            this.cbToolBar.Name = "cbToolBar";
            this.cbToolBar.Size = new System.Drawing.Size(104, 17);
            this.cbToolBar.TabIndex = 2;
            this.cbToolBar.Text = "Standard tool&bar";
            this.cbToolBar.UseVisualStyleBackColor = true;
            this.cbToolBar.CheckedChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // cbStandardMenus
            // 
            this.cbStandardMenus.AutoSize = true;
            this.cbStandardMenus.Checked = true;
            this.cbStandardMenus.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbStandardMenus.Location = new System.Drawing.Point(21, 43);
            this.cbStandardMenus.Name = "cbStandardMenus";
            this.cbStandardMenus.Size = new System.Drawing.Size(189, 17);
            this.cbStandardMenus.TabIndex = 1;
            this.cbStandardMenus.Text = "Standard m&enus (Action and View)";
            this.cbStandardMenus.UseVisualStyleBackColor = true;
            this.cbStandardMenus.CheckedChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // cbConsoleTree
            // 
            this.cbConsoleTree.AutoSize = true;
            this.cbConsoleTree.Checked = true;
            this.cbConsoleTree.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbConsoleTree.Location = new System.Drawing.Point(21, 20);
            this.cbConsoleTree.Name = "cbConsoleTree";
            this.cbConsoleTree.Size = new System.Drawing.Size(85, 17);
            this.cbConsoleTree.TabIndex = 0;
            this.cbConsoleTree.Text = "&Console tree";
            this.cbConsoleTree.UseVisualStyleBackColor = true;
            this.cbConsoleTree.CheckedChanged += new System.EventHandler(this.SettingsChanged);
            // 
            // LMCCustomView
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Name = "LMCCustomView";
            this.Size = new System.Drawing.Size(325, 186);
            this.pnlData.ResumeLayout(false);
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion
              
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.CheckBox cbActionPane;
        private System.Windows.Forms.CheckBox cbTaskPad;
        private System.Windows.Forms.CheckBox cbStatusbar;
        private System.Windows.Forms.CheckBox cbToolBar;
        private System.Windows.Forms.CheckBox cbStandardMenus;
        private System.Windows.Forms.CheckBox cbConsoleTree;
    }
}
