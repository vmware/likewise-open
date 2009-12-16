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
    partial class CustomizeViewForm
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
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.cbToolbars = new System.Windows.Forms.CheckBox();
            this.cbMenus = new System.Windows.Forms.CheckBox();
            this.btnOk = new System.Windows.Forms.Button();
            this.lmcCustomView = new Likewise.LMC.ServerControl.LMCCustomView();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            //
            // label1
            //
            this.label1.Location = new System.Drawing.Point(12, 18);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(301, 32);
            this.label1.TabIndex = 0;
            this.label1.Text = "Select or clear the check boxes to show or hide items in the console window.";
            //
            // groupBox2
            //
            this.groupBox2.Controls.Add(this.cbToolbars);
            this.groupBox2.Controls.Add(this.cbMenus);
            this.groupBox2.Location = new System.Drawing.Point(12, 250);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(312, 75);
            this.groupBox2.TabIndex = 2;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Plug-in";
            //
            // cbToolbars
            //
            this.cbToolbars.AutoSize = true;
            this.cbToolbars.Checked = true;
            this.cbToolbars.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbToolbars.Location = new System.Drawing.Point(21, 51);
            this.cbToolbars.Name = "cbToolbars";
            this.cbToolbars.Size = new System.Drawing.Size(67, 17);
            this.cbToolbars.TabIndex = 1;
            this.cbToolbars.Text = "&Toolbars";
            this.cbToolbars.UseVisualStyleBackColor = true;
            this.cbToolbars.CheckedChanged += new System.EventHandler(this.SettingsChanged);
            //
            // cbMenus
            //
            this.cbMenus.AutoSize = true;
            this.cbMenus.Checked = true;
            this.cbMenus.CheckState = System.Windows.Forms.CheckState.Checked;
            this.cbMenus.Location = new System.Drawing.Point(21, 28);
            this.cbMenus.Name = "cbMenus";
            this.cbMenus.Size = new System.Drawing.Size(58, 17);
            this.cbMenus.TabIndex = 0;
            this.cbMenus.Text = "&Menus";
            this.cbMenus.UseVisualStyleBackColor = true;
            this.cbMenus.CheckedChanged += new System.EventHandler(this.SettingsChanged);
            //
            // btnOk
            //
            this.btnOk.Location = new System.Drawing.Point(248, 341);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(76, 22);
            this.btnOk.TabIndex = 3;
            this.btnOk.Text = "OK";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            //
            // lmcCustomView
            //
            this.lmcCustomView.ActionPane = false;
            this.lmcCustomView.BackColor = System.Drawing.Color.Transparent;
            this.lmcCustomView.ConsoleTree = true;
            this.lmcCustomView.ForeColor = System.Drawing.SystemColors.ControlText;
            this.lmcCustomView.IPlugInContainer = null;
            this.lmcCustomView.Location = new System.Drawing.Point(5, 45);
            this.lmcCustomView.Name = "lmcCustomView";
            this.lmcCustomView.ParentContainer = null;
            this.lmcCustomView.Size = new System.Drawing.Size(325, 186);
            this.lmcCustomView.StandardMenus = true;
            this.lmcCustomView.StandardToolbar = true;
            this.lmcCustomView.StatusBar = true;
            this.lmcCustomView.TabIndex = 5;
            this.lmcCustomView.TaskpadNavigationpads = true;
            //
            // CustomizeViewForm
            //
            this.AcceptButton = this.btnOk;
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.ClientSize = new System.Drawing.Size(344, 374);
            this.Controls.Add(this.lmcCustomView);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.label1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "CustomizeViewForm";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "CustomizeView";
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.CheckBox cbToolbars;
        private System.Windows.Forms.CheckBox cbMenus;
        private System.Windows.Forms.Button btnOk;
        private Likewise.LMC.ServerControl.LMCCustomView lmcCustomView;
    }
}