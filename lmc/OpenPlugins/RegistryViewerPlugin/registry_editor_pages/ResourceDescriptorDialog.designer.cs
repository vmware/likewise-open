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
    partial class ResourceDescriptorDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ResourceDescriptorDialog));
            this.DMALable = new System.Windows.Forms.Label();
            this.LWlvDMA = new System.Windows.Forms.ListView();
            this.colChannel = new System.Windows.Forms.ColumnHeader();
            this.colPort = new System.Windows.Forms.ColumnHeader();
            this.LWlvInterrupt = new System.Windows.Forms.ListView();
            this.colVector = new System.Windows.Forms.ColumnHeader();
            this.colLevel = new System.Windows.Forms.ColumnHeader();
            this.colAffinity = new System.Windows.Forms.ColumnHeader();
            this.colType = new System.Windows.Forms.ColumnHeader();
            this.InterruptLabel = new System.Windows.Forms.Label();
            this.LWlvMemory = new System.Windows.Forms.ListView();
            this.colAddress = new System.Windows.Forms.ColumnHeader();
            this.colLength = new System.Windows.Forms.ColumnHeader();
            this.colAccess = new System.Windows.Forms.ColumnHeader();
            this.MemoryLabel = new System.Windows.Forms.Label();
            this.LWlvPort = new System.Windows.Forms.ListView();
            this.colPortAddress = new System.Windows.Forms.ColumnHeader();
            this.colPortLength = new System.Windows.Forms.ColumnHeader();
            this.colPortType = new System.Windows.Forms.ColumnHeader();
            this.PortLabel = new System.Windows.Forms.Label();
            this.LWlvData = new System.Windows.Forms.ListView();
            this.colReservedOne = new System.Windows.Forms.ColumnHeader();
            this.colReservedTwo = new System.Windows.Forms.ColumnHeader();
            this.colDataSize = new System.Windows.Forms.ColumnHeader();
            this.DataLebel = new System.Windows.Forms.Label();
            this.ShareGrpBox = new System.Windows.Forms.GroupBox();
            this.DriverLabel = new System.Windows.Forms.Label();
            this.DeviceLabel = new System.Windows.Forms.Label();
            this.SharedLabel = new System.Windows.Forms.Label();
            this.UndeterminedLabel = new System.Windows.Forms.Label();
            this.IntTypeLabel = new System.Windows.Forms.Label();
            this.BusNumLabel = new System.Windows.Forms.Label();
            this.VersionLabel = new System.Windows.Forms.Label();
            this.RevisionLabel = new System.Windows.Forms.Label();
            this.btnOk = new System.Windows.Forms.Button();
            this.btnData = new System.Windows.Forms.Button();
            this.IntTypeAnsLabel = new System.Windows.Forms.Label();
            this.BusNumAnsLabel = new System.Windows.Forms.Label();
            this.VersionAnsLabel = new System.Windows.Forms.Label();
            this.RevisionAnsLabel = new System.Windows.Forms.Label();
            this.ShareGrpBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // DMALable
            // 
            this.DMALable.Location = new System.Drawing.Point(5, 1);
            this.DMALable.Name = "DMALable";
            this.DMALable.Size = new System.Drawing.Size(38, 20);
            this.DMALable.TabIndex = 0;
            this.DMALable.Text = "DMA:";
            this.DMALable.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // LWlvDMA
            // 
            this.LWlvDMA.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colChannel,
            this.colPort});
            this.LWlvDMA.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.LWlvDMA.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.LWlvDMA.Location = new System.Drawing.Point(6, 21);
            this.LWlvDMA.MultiSelect = false;
            this.LWlvDMA.Name = "LWlvDMA";
            this.LWlvDMA.Size = new System.Drawing.Size(407, 62);
            this.LWlvDMA.TabIndex = 1;
            this.LWlvDMA.UseCompatibleStateImageBehavior = false;
            this.LWlvDMA.View = System.Windows.Forms.View.Details;
            this.LWlvDMA.Resize += new System.EventHandler(this.LWlv_Resize);
            this.LWlvDMA.SelectedIndexChanged += new System.EventHandler(this.LWlv_SelectedIndexChanged);
            // 
            // colChannel
            // 
            this.colChannel.Text = "Channel";
            this.colChannel.Width = 200;
            // 
            // colPort
            // 
            this.colPort.Text = "Port";
            this.colPort.Width = 200;
            // 
            // LWlvInterrupt
            // 
            this.LWlvInterrupt.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colVector,
            this.colLevel,
            this.colAffinity,
            this.colType});
            this.LWlvInterrupt.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.LWlvInterrupt.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.LWlvInterrupt.Location = new System.Drawing.Point(6, 108);
            this.LWlvInterrupt.MultiSelect = false;
            this.LWlvInterrupt.Name = "LWlvInterrupt";
            this.LWlvInterrupt.Size = new System.Drawing.Size(407, 62);
            this.LWlvInterrupt.TabIndex = 3;
            this.LWlvInterrupt.UseCompatibleStateImageBehavior = false;
            this.LWlvInterrupt.View = System.Windows.Forms.View.Details;
            this.LWlvInterrupt.Resize += new System.EventHandler(this.LWlv_Resize);
            this.LWlvInterrupt.SelectedIndexChanged += new System.EventHandler(this.LWlv_SelectedIndexChanged);
            // 
            // colVector
            // 
            this.colVector.Text = "Vector";
            this.colVector.Width = 100;
            // 
            // colLevel
            // 
            this.colLevel.Text = "Level";
            this.colLevel.Width = 100;
            // 
            // colAffinity
            // 
            this.colAffinity.Text = "Affinity";
            this.colAffinity.Width = 100;
            // 
            // colType
            // 
            this.colType.Text = "Type";
            this.colType.Width = 100;
            // 
            // InterruptLabel
            // 
            this.InterruptLabel.Location = new System.Drawing.Point(3, 87);
            this.InterruptLabel.Name = "InterruptLabel";
            this.InterruptLabel.Size = new System.Drawing.Size(51, 20);
            this.InterruptLabel.TabIndex = 2;
            this.InterruptLabel.Text = "Interrupt:";
            this.InterruptLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // LWlvMemory
            // 
            this.LWlvMemory.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colAddress,
            this.colLength,
            this.colAccess});
            this.LWlvMemory.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.LWlvMemory.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.LWlvMemory.Location = new System.Drawing.Point(6, 193);
            this.LWlvMemory.MultiSelect = false;
            this.LWlvMemory.Name = "LWlvMemory";
            this.LWlvMemory.Size = new System.Drawing.Size(407, 62);
            this.LWlvMemory.TabIndex = 5;
            this.LWlvMemory.UseCompatibleStateImageBehavior = false;
            this.LWlvMemory.View = System.Windows.Forms.View.Details;
            this.LWlvMemory.Resize += new System.EventHandler(this.LWlv_Resize);
            this.LWlvMemory.SelectedIndexChanged += new System.EventHandler(this.LWlv_SelectedIndexChanged);
            // 
            // colAddress
            // 
            this.colAddress.Text = "Physical Address";
            this.colAddress.Width = 120;
            // 
            // colLength
            // 
            this.colLength.Text = "Length";
            this.colLength.Width = 120;
            // 
            // colAccess
            // 
            this.colAccess.Text = "Access";
            this.colAccess.Width = 160;
            // 
            // MemoryLabel
            // 
            this.MemoryLabel.Location = new System.Drawing.Point(4, 173);
            this.MemoryLabel.Name = "MemoryLabel";
            this.MemoryLabel.Size = new System.Drawing.Size(51, 20);
            this.MemoryLabel.TabIndex = 4;
            this.MemoryLabel.Text = "Memory";
            this.MemoryLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // LWlvPort
            // 
            this.LWlvPort.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colPortAddress,
            this.colPortLength,
            this.colPortType});
            this.LWlvPort.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.LWlvPort.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.LWlvPort.Location = new System.Drawing.Point(6, 276);
            this.LWlvPort.MultiSelect = false;
            this.LWlvPort.Name = "LWlvPort";
            this.LWlvPort.Size = new System.Drawing.Size(407, 62);
            this.LWlvPort.TabIndex = 7;
            this.LWlvPort.UseCompatibleStateImageBehavior = false;
            this.LWlvPort.View = System.Windows.Forms.View.Details;
            this.LWlvPort.Resize += new System.EventHandler(this.LWlv_Resize);
            this.LWlvPort.SelectedIndexChanged += new System.EventHandler(this.LWlv_SelectedIndexChanged);
            // 
            // colPortAddress
            // 
            this.colPortAddress.Text = "Physical Address";
            this.colPortAddress.Width = 120;
            // 
            // colPortLength
            // 
            this.colPortLength.Text = "Length";
            this.colPortLength.Width = 120;
            // 
            // colPortType
            // 
            this.colPortType.Text = "Type";
            this.colPortType.Width = 160;
            // 
            // PortLabel
            // 
            this.PortLabel.Location = new System.Drawing.Point(5, 256);
            this.PortLabel.Name = "PortLabel";
            this.PortLabel.Size = new System.Drawing.Size(51, 20);
            this.PortLabel.TabIndex = 6;
            this.PortLabel.Text = "Port";
            this.PortLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // LWlvData
            // 
            this.LWlvData.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colReservedOne,
            this.colReservedTwo,
            this.colDataSize});
            this.LWlvData.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.LWlvData.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.LWlvData.Location = new System.Drawing.Point(6, 358);
            this.LWlvData.MultiSelect = false;
            this.LWlvData.Name = "LWlvData";
            this.LWlvData.Size = new System.Drawing.Size(407, 62);
            this.LWlvData.TabIndex = 9;
            this.LWlvData.UseCompatibleStateImageBehavior = false;
            this.LWlvData.View = System.Windows.Forms.View.Details;
            this.LWlvData.Resize += new System.EventHandler(this.LWlv_Resize);
            this.LWlvData.SelectedIndexChanged += new System.EventHandler(this.LWlv_SelectedIndexChanged);
            // 
            // colReservedOne
            // 
            this.colReservedOne.Text = "Reserved1";
            this.colReservedOne.Width = 130;
            // 
            // colReservedTwo
            // 
            this.colReservedTwo.Text = "Reserved2";
            this.colReservedTwo.Width = 130;
            // 
            // colDataSize
            // 
            this.colDataSize.Text = "Data Size";
            this.colDataSize.Width = 140;
            // 
            // DataLebel
            // 
            this.DataLebel.Location = new System.Drawing.Point(3, 337);
            this.DataLebel.Name = "DataLebel";
            this.DataLebel.Size = new System.Drawing.Size(114, 20);
            this.DataLebel.TabIndex = 8;
            this.DataLebel.Text = "Device Specific Data:";
            this.DataLebel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // ShareGrpBox
            // 
            this.ShareGrpBox.Controls.Add(this.DriverLabel);
            this.ShareGrpBox.Controls.Add(this.DeviceLabel);
            this.ShareGrpBox.Controls.Add(this.SharedLabel);
            this.ShareGrpBox.Controls.Add(this.UndeterminedLabel);
            this.ShareGrpBox.Location = new System.Drawing.Point(6, 426);
            this.ShareGrpBox.Name = "ShareGrpBox";
            this.ShareGrpBox.Size = new System.Drawing.Size(196, 66);
            this.ShareGrpBox.TabIndex = 10;
            this.ShareGrpBox.TabStop = false;
            this.ShareGrpBox.Text = "Share Disposition";
            // 
            // DriverLabel
            // 
            this.DriverLabel.Enabled = false;
            this.DriverLabel.Location = new System.Drawing.Point(100, 38);
            this.DriverLabel.Name = "DriverLabel";
            this.DriverLabel.Size = new System.Drawing.Size(93, 20);
            this.DriverLabel.TabIndex = 12;
            this.DriverLabel.Text = "Driver Exclusive";
            this.DriverLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // DeviceLabel
            // 
            this.DeviceLabel.Enabled = false;
            this.DeviceLabel.Location = new System.Drawing.Point(100, 17);
            this.DeviceLabel.Name = "DeviceLabel";
            this.DeviceLabel.Size = new System.Drawing.Size(93, 20);
            this.DeviceLabel.TabIndex = 11;
            this.DeviceLabel.Text = "Device Exclusive";
            this.DeviceLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // SharedLabel
            // 
            this.SharedLabel.Enabled = false;
            this.SharedLabel.Location = new System.Drawing.Point(6, 38);
            this.SharedLabel.Name = "SharedLabel";
            this.SharedLabel.Size = new System.Drawing.Size(51, 20);
            this.SharedLabel.TabIndex = 10;
            this.SharedLabel.Text = "Shared";
            this.SharedLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // UndeterminedLabel
            // 
            this.UndeterminedLabel.Enabled = false;
            this.UndeterminedLabel.Location = new System.Drawing.Point(6, 17);
            this.UndeterminedLabel.Name = "UndeterminedLabel";
            this.UndeterminedLabel.Size = new System.Drawing.Size(76, 20);
            this.UndeterminedLabel.TabIndex = 9;
            this.UndeterminedLabel.Text = "Undetermined";
            this.UndeterminedLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // IntTypeLabel
            // 
            this.IntTypeLabel.Location = new System.Drawing.Point(266, 430);
            this.IntTypeLabel.Name = "IntTypeLabel";
            this.IntTypeLabel.Size = new System.Drawing.Size(85, 20);
            this.IntTypeLabel.TabIndex = 11;
            this.IntTypeLabel.Text = "Interface Type:";
            this.IntTypeLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // BusNumLabel
            // 
            this.BusNumLabel.Location = new System.Drawing.Point(274, 450);
            this.BusNumLabel.Name = "BusNumLabel";
            this.BusNumLabel.Size = new System.Drawing.Size(77, 20);
            this.BusNumLabel.TabIndex = 12;
            this.BusNumLabel.Text = "Bus Number:";
            this.BusNumLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // VersionLabel
            // 
            this.VersionLabel.Location = new System.Drawing.Point(300, 470);
            this.VersionLabel.Name = "VersionLabel";
            this.VersionLabel.Size = new System.Drawing.Size(51, 20);
            this.VersionLabel.TabIndex = 13;
            this.VersionLabel.Text = "Version:";
            this.VersionLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // RevisionLabel
            // 
            this.RevisionLabel.Location = new System.Drawing.Point(300, 490);
            this.RevisionLabel.Name = "RevisionLabel";
            this.RevisionLabel.Size = new System.Drawing.Size(51, 20);
            this.RevisionLabel.TabIndex = 14;
            this.RevisionLabel.Text = "Revision:";
            this.RevisionLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // btnOk
            // 
            this.btnOk.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnOk.Location = new System.Drawing.Point(126, 502);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(75, 23);
            this.btnOk.TabIndex = 15;
            this.btnOk.Text = "OK";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            // 
            // btnData
            // 
            this.btnData.Enabled = false;
            this.btnData.Location = new System.Drawing.Point(209, 502);
            this.btnData.Name = "btnData";
            this.btnData.Size = new System.Drawing.Size(75, 23);
            this.btnData.TabIndex = 16;
            this.btnData.Text = "&Data...";
            this.btnData.UseVisualStyleBackColor = true;
            // 
            // IntTypeAnsLabel
            // 
            this.IntTypeAnsLabel.Location = new System.Drawing.Point(357, 430);
            this.IntTypeAnsLabel.Name = "IntTypeAnsLabel";
            this.IntTypeAnsLabel.Size = new System.Drawing.Size(51, 20);
            this.IntTypeAnsLabel.TabIndex = 17;
            this.IntTypeAnsLabel.Text = "Internal";
            this.IntTypeAnsLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // BusNumAnsLabel
            // 
            this.BusNumAnsLabel.Location = new System.Drawing.Point(357, 450);
            this.BusNumAnsLabel.Name = "BusNumAnsLabel";
            this.BusNumAnsLabel.Size = new System.Drawing.Size(51, 20);
            this.BusNumAnsLabel.TabIndex = 18;
            this.BusNumAnsLabel.Text = "0";
            this.BusNumAnsLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // VersionAnsLabel
            // 
            this.VersionAnsLabel.Location = new System.Drawing.Point(357, 470);
            this.VersionAnsLabel.Name = "VersionAnsLabel";
            this.VersionAnsLabel.Size = new System.Drawing.Size(51, 20);
            this.VersionAnsLabel.TabIndex = 19;
            this.VersionAnsLabel.Text = "0";
            this.VersionAnsLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // RevisionAnsLabel
            // 
            this.RevisionAnsLabel.Location = new System.Drawing.Point(357, 490);
            this.RevisionAnsLabel.Name = "RevisionAnsLabel";
            this.RevisionAnsLabel.Size = new System.Drawing.Size(51, 20);
            this.RevisionAnsLabel.TabIndex = 20;
            this.RevisionAnsLabel.Text = "0";
            this.RevisionAnsLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // ResourceDescriptorDialog
            // 
            this.AcceptButton = this.btnData;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.btnOk;
            this.ClientSize = new System.Drawing.Size(425, 533);
            this.Controls.Add(this.RevisionAnsLabel);
            this.Controls.Add(this.VersionAnsLabel);
            this.Controls.Add(this.BusNumAnsLabel);
            this.Controls.Add(this.IntTypeAnsLabel);
            this.Controls.Add(this.btnData);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.RevisionLabel);
            this.Controls.Add(this.VersionLabel);
            this.Controls.Add(this.BusNumLabel);
            this.Controls.Add(this.IntTypeLabel);
            this.Controls.Add(this.ShareGrpBox);
            this.Controls.Add(this.LWlvData);
            this.Controls.Add(this.DataLebel);
            this.Controls.Add(this.LWlvPort);
            this.Controls.Add(this.PortLabel);
            this.Controls.Add(this.LWlvMemory);
            this.Controls.Add(this.MemoryLabel);
            this.Controls.Add(this.LWlvInterrupt);
            this.Controls.Add(this.InterruptLabel);
            this.Controls.Add(this.LWlvDMA);
            this.Controls.Add(this.DMALable);
            this.HelpButton = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ResourceDescriptorDialog";
            this.Text = "Resources";
            this.ShareGrpBox.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label DMALable;
        private System.Windows.Forms.ListView LWlvDMA;
        private System.Windows.Forms.ColumnHeader colChannel;
        private System.Windows.Forms.ColumnHeader colPort;
        private System.Windows.Forms.ListView LWlvInterrupt;
        private System.Windows.Forms.ColumnHeader colVector;
        private System.Windows.Forms.ColumnHeader colLevel;
        private System.Windows.Forms.Label InterruptLabel;
        private System.Windows.Forms.ColumnHeader colAffinity;
        private System.Windows.Forms.ColumnHeader colType;
        private System.Windows.Forms.ListView LWlvMemory;
        private System.Windows.Forms.ColumnHeader colAddress;
        private System.Windows.Forms.ColumnHeader colLength;
        private System.Windows.Forms.ColumnHeader colAccess;
        private System.Windows.Forms.Label MemoryLabel;
        private System.Windows.Forms.ListView LWlvPort;
        private System.Windows.Forms.ColumnHeader colPortAddress;
        private System.Windows.Forms.ColumnHeader colPortLength;
        private System.Windows.Forms.ColumnHeader colPortType;
        private System.Windows.Forms.Label PortLabel;
        private System.Windows.Forms.ListView LWlvData;
        private System.Windows.Forms.ColumnHeader colReservedOne;
        private System.Windows.Forms.ColumnHeader colReservedTwo;
        private System.Windows.Forms.ColumnHeader colDataSize;
        private System.Windows.Forms.Label DataLebel;
        private System.Windows.Forms.GroupBox ShareGrpBox;
        private System.Windows.Forms.Label DriverLabel;
        private System.Windows.Forms.Label DeviceLabel;
        private System.Windows.Forms.Label SharedLabel;
        private System.Windows.Forms.Label UndeterminedLabel;
        private System.Windows.Forms.Label IntTypeLabel;
        private System.Windows.Forms.Label BusNumLabel;
        private System.Windows.Forms.Label VersionLabel;
        private System.Windows.Forms.Label RevisionLabel;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.Button btnData;
        private System.Windows.Forms.Label IntTypeAnsLabel;
        private System.Windows.Forms.Label BusNumAnsLabel;
        private System.Windows.Forms.Label VersionAnsLabel;
        private System.Windows.Forms.Label RevisionAnsLabel;
    }
}