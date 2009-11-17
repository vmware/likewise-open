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
    partial class ResourceListDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ResourceListDialog));
            this.LWlvResourceList = new System.Windows.Forms.ListView();
            this.colBusNum = new System.Windows.Forms.ColumnHeader();
            this.colIntType = new System.Windows.Forms.ColumnHeader();
            this.colResourceList = new System.Windows.Forms.ColumnHeader();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnDisplay = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // LWlvResourceList
            // 
            this.LWlvResourceList.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colBusNum,
            this.colIntType,
            this.colResourceList});
            this.LWlvResourceList.Location = new System.Drawing.Point(5, 8);
            this.LWlvResourceList.MultiSelect = false;
            this.LWlvResourceList.Name = "LWlvResourceList";
            this.LWlvResourceList.Size = new System.Drawing.Size(228, 142);
            this.LWlvResourceList.TabIndex = 0;
            this.LWlvResourceList.UseCompatibleStateImageBehavior = false;
            this.LWlvResourceList.View = System.Windows.Forms.View.Details;
            // 
            // colBusNum
            // 
            this.colBusNum.Text = "Bus Number";
            this.colBusNum.Width = 95;
            // 
            // colIntType
            // 
            this.colIntType.Text = "Interface Type";
            this.colIntType.Width = 118;
            // 
            // colResourceList
            // 
            this.colResourceList.Text = "";
            // 
            // btnOK
            // 
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnOK.Location = new System.Drawing.Point(76, 161);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(75, 23);
            this.btnOK.TabIndex = 1;
            this.btnOK.Text = "&OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnDisplay
            // 
            this.btnDisplay.Enabled = false;
            this.btnDisplay.Location = new System.Drawing.Point(160, 161);
            this.btnDisplay.Name = "btnDisplay";
            this.btnDisplay.Size = new System.Drawing.Size(75, 23);
            this.btnDisplay.TabIndex = 2;
            this.btnDisplay.Text = "&Display...";
            this.btnDisplay.UseVisualStyleBackColor = true;
            this.btnDisplay.Click += new System.EventHandler(this.btnDisplay_Click);
            // 
            // ResourceListDialog
            // 
            this.AcceptButton = this.btnDisplay;
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.CancelButton = this.btnOK;
            this.ClientSize = new System.Drawing.Size(239, 190);
            this.Controls.Add(this.btnDisplay);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.LWlvResourceList);
            this.HelpButton = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ResourceListDialog";
            this.Text = "Resource Lists";
            this.Load += new System.EventHandler(this.ResourceListDialog_Load);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ListView LWlvResourceList;
        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnDisplay;
        private System.Windows.Forms.ColumnHeader colBusNum;
        private System.Windows.Forms.ColumnHeader colIntType;
        private System.Windows.Forms.ColumnHeader colResourceList;
    }
}