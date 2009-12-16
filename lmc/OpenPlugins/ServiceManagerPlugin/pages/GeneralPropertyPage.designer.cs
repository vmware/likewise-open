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

namespace Likewise.LMC.Plugins.ServiceManagerPlugin
{
    partial class GeneralPropertyPage
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
            this.lblServiceName = new System.Windows.Forms.Label();
            this.lblServiceNameValue = new System.Windows.Forms.Label();
            this.lblDisplayName = new System.Windows.Forms.Label();
            this.txtDisplayName = new System.Windows.Forms.TextBox();
            this.lblDescription = new System.Windows.Forms.Label();
            this.txtDescription = new System.Windows.Forms.TextBox();
            this.lblPath = new System.Windows.Forms.Label();
            this.txtPathToExecute = new System.Windows.Forms.TextBox();
            this.lblStartupType = new System.Windows.Forms.Label();
            this.cmbStartupType = new System.Windows.Forms.ComboBox();
            this.lblServiceStatusValue = new System.Windows.Forms.Label();
            this.lblServiceStatus = new System.Windows.Forms.Label();
            this.btnStart = new System.Windows.Forms.Button();
            this.btnStop = new System.Windows.Forms.Button();
            this.btnPause = new System.Windows.Forms.Button();
            this.btnResume = new System.Windows.Forms.Button();
            this.lblMessage = new System.Windows.Forms.Label();
            this.lblStartParameters = new System.Windows.Forms.Label();
            this.txtStartParameters = new System.Windows.Forms.TextBox();
            this.pnlData.SuspendLayout();
            this.SuspendLayout();
            //
            // pnlData
            //
            this.pnlData.Controls.Add(this.txtStartParameters);
            this.pnlData.Controls.Add(this.lblStartParameters);
            this.pnlData.Controls.Add(this.lblMessage);
            this.pnlData.Controls.Add(this.btnResume);
            this.pnlData.Controls.Add(this.btnPause);
            this.pnlData.Controls.Add(this.btnStop);
            this.pnlData.Controls.Add(this.btnStart);
            this.pnlData.Controls.Add(this.lblServiceStatusValue);
            this.pnlData.Controls.Add(this.lblServiceStatus);
            this.pnlData.Controls.Add(this.cmbStartupType);
            this.pnlData.Controls.Add(this.lblStartupType);
            this.pnlData.Controls.Add(this.txtPathToExecute);
            this.pnlData.Controls.Add(this.lblPath);
            this.pnlData.Controls.Add(this.txtDescription);
            this.pnlData.Controls.Add(this.lblDescription);
            this.pnlData.Controls.Add(this.txtDisplayName);
            this.pnlData.Controls.Add(this.lblDisplayName);
            this.pnlData.Controls.Add(this.lblServiceNameValue);
            this.pnlData.Controls.Add(this.lblServiceName);
            this.pnlData.Size = new System.Drawing.Size(406, 371);
            //
            // lblServiceName
            //
            this.lblServiceName.Location = new System.Drawing.Point(13, 10);
            this.lblServiceName.Name = "lblServiceName";
            this.lblServiceName.Size = new System.Drawing.Size(100, 23);
            this.lblServiceName.TabIndex = 0;
            this.lblServiceName.Text = "Service name:";
            this.lblServiceName.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            //
            // lblServiceNameValue
            //
            this.lblServiceNameValue.Location = new System.Drawing.Point(120, 9);
            this.lblServiceNameValue.Name = "lblServiceNameValue";
            this.lblServiceNameValue.Size = new System.Drawing.Size(111, 23);
            this.lblServiceNameValue.TabIndex = 1;
            this.lblServiceNameValue.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            //
            // lblDisplayName
            //
            this.lblDisplayName.Location = new System.Drawing.Point(13, 44);
            this.lblDisplayName.Name = "lblDisplayName";
            this.lblDisplayName.Size = new System.Drawing.Size(81, 23);
            this.lblDisplayName.TabIndex = 2;
            this.lblDisplayName.Text = "Display name:";
            this.lblDisplayName.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            //
            // txtDisplayName
            //
            this.txtDisplayName.Location = new System.Drawing.Point(103, 45);
            this.txtDisplayName.Name = "txtDisplayName";
            this.txtDisplayName.ReadOnly = true;
            this.txtDisplayName.Size = new System.Drawing.Size(293, 20);
            this.txtDisplayName.TabIndex = 3;
            this.txtDisplayName.TextChanged += new System.EventHandler(this.txtDisplayName_TextChanged);
            //
            // lblDescription
            //
            this.lblDescription.Location = new System.Drawing.Point(13, 74);
            this.lblDescription.Name = "lblDescription";
            this.lblDescription.Size = new System.Drawing.Size(81, 23);
            this.lblDescription.TabIndex = 4;
            this.lblDescription.Text = "Description:";
            this.lblDescription.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            //
            // txtDescription
            //
            this.txtDescription.Location = new System.Drawing.Point(103, 76);
            this.txtDescription.Multiline = true;
            this.txtDescription.Name = "txtDescription";
            this.txtDescription.ReadOnly = true;
            this.txtDescription.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.txtDescription.Size = new System.Drawing.Size(293, 41);
            this.txtDescription.TabIndex = 5;
            //
            // lblPath
            //
            this.lblPath.Location = new System.Drawing.Point(13, 122);
            this.lblPath.Name = "lblPath";
            this.lblPath.Size = new System.Drawing.Size(100, 23);
            this.lblPath.TabIndex = 6;
            this.lblPath.Text = "Path to execute:";
            this.lblPath.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            //
            // txtPathToExecute
            //
            this.txtPathToExecute.Location = new System.Drawing.Point(16, 146);
            this.txtPathToExecute.Name = "txtPathToExecute";
            this.txtPathToExecute.ReadOnly = true;
            this.txtPathToExecute.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.txtPathToExecute.Size = new System.Drawing.Size(377, 20);
            this.txtPathToExecute.TabIndex = 7;
            //
            // lblStartupType
            //
            this.lblStartupType.Location = new System.Drawing.Point(13, 183);
            this.lblStartupType.Name = "lblStartupType";
            this.lblStartupType.Size = new System.Drawing.Size(81, 23);
            this.lblStartupType.TabIndex = 8;
            this.lblStartupType.Text = "Startup Type";
            this.lblStartupType.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            //
            // cmbStartupType
            //
            this.cmbStartupType.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbStartupType.FormattingEnabled = true;
            this.cmbStartupType.Items.AddRange(new object[] {
            "Automatic",
            "Manual",
            "Disabled"});
            this.cmbStartupType.Location = new System.Drawing.Point(100, 183);
            this.cmbStartupType.Name = "cmbStartupType";
            this.cmbStartupType.Size = new System.Drawing.Size(293, 21);
            this.cmbStartupType.TabIndex = 9;
            //
            // lblServiceStatusValue
            //
            this.lblServiceStatusValue.Location = new System.Drawing.Point(123, 225);
            this.lblServiceStatusValue.Name = "lblServiceStatusValue";
            this.lblServiceStatusValue.Size = new System.Drawing.Size(198, 23);
            this.lblServiceStatusValue.TabIndex = 11;
            this.lblServiceStatusValue.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            //
            // lblServiceStatus
            //
            this.lblServiceStatus.Location = new System.Drawing.Point(13, 222);
            this.lblServiceStatus.Name = "lblServiceStatus";
            this.lblServiceStatus.Size = new System.Drawing.Size(100, 23);
            this.lblServiceStatus.TabIndex = 10;
            this.lblServiceStatus.Text = "Service Status";
            this.lblServiceStatus.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            //
            // btnStart
            //
            this.btnStart.Enabled = false;
            this.btnStart.Location = new System.Drawing.Point(16, 255);
            this.btnStart.Name = "btnStart";
            this.btnStart.Size = new System.Drawing.Size(75, 23);
            this.btnStart.TabIndex = 12;
            this.btnStart.Text = "Start";
            this.btnStart.UseVisualStyleBackColor = true;
            this.btnStart.Click += new System.EventHandler(this.btnStart_Click);
            //
            // btnStop
            //
            this.btnStop.Enabled = false;
            this.btnStop.Location = new System.Drawing.Point(100, 254);
            this.btnStop.Name = "btnStop";
            this.btnStop.Size = new System.Drawing.Size(75, 23);
            this.btnStop.TabIndex = 13;
            this.btnStop.Text = "Stop";
            this.btnStop.UseVisualStyleBackColor = true;
            this.btnStop.Click += new System.EventHandler(this.btnStop_Click);
            //
            // btnPause
            //
            this.btnPause.Enabled = false;
            this.btnPause.Location = new System.Drawing.Point(184, 254);
            this.btnPause.Name = "btnPause";
            this.btnPause.Size = new System.Drawing.Size(75, 23);
            this.btnPause.TabIndex = 14;
            this.btnPause.Text = "Pause";
            this.btnPause.UseVisualStyleBackColor = true;
            this.btnPause.Click += new System.EventHandler(this.btnPause_Click);
            //
            // btnResume
            //
            this.btnResume.Enabled = false;
            this.btnResume.Location = new System.Drawing.Point(273, 253);
            this.btnResume.Name = "btnResume";
            this.btnResume.Size = new System.Drawing.Size(75, 23);
            this.btnResume.TabIndex = 15;
            this.btnResume.Text = "Resume";
            this.btnResume.UseVisualStyleBackColor = true;
            this.btnResume.Click += new System.EventHandler(this.btnResume_Click);
            //
            // lblMessage
            //
            this.lblMessage.Location = new System.Drawing.Point(13, 294);
            this.lblMessage.Name = "lblMessage";
            this.lblMessage.Size = new System.Drawing.Size(347, 34);
            this.lblMessage.TabIndex = 16;
            this.lblMessage.Text = "You can specify the start parameters that apply when you start the service from h" +
                "ere.";
            this.lblMessage.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            //
            // lblStartParameters
            //
            this.lblStartParameters.Location = new System.Drawing.Point(14, 330);
            this.lblStartParameters.Name = "lblStartParameters";
            this.lblStartParameters.Size = new System.Drawing.Size(99, 23);
            this.lblStartParameters.TabIndex = 17;
            this.lblStartParameters.Text = "Start Parameters";
            this.lblStartParameters.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            //
            // txtStartParameters
            //
            this.txtStartParameters.Location = new System.Drawing.Point(119, 332);
            this.txtStartParameters.Name = "txtStartParameters";
            this.txtStartParameters.ReadOnly = true;
            this.txtStartParameters.Size = new System.Drawing.Size(274, 20);
            this.txtStartParameters.TabIndex = 18;
            this.txtStartParameters.TextChanged += new System.EventHandler(this.txtStartParameters_TextChanged);
            //
            // GeneralPropertyPage
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.Name = "GeneralPropertyPage";
            this.Size = new System.Drawing.Size(406, 371);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label lblServiceName;
        private System.Windows.Forms.Label lblServiceNameValue;
        private System.Windows.Forms.TextBox txtDisplayName;
        private System.Windows.Forms.Label lblDisplayName;
        private System.Windows.Forms.TextBox txtDescription;
        private System.Windows.Forms.Label lblDescription;
        private System.Windows.Forms.Label lblPath;
        private System.Windows.Forms.TextBox txtPathToExecute;
        private System.Windows.Forms.Label lblStartupType;
        private System.Windows.Forms.ComboBox cmbStartupType;
        private System.Windows.Forms.Label lblServiceStatusValue;
        private System.Windows.Forms.Label lblServiceStatus;
        private System.Windows.Forms.Button btnResume;
        private System.Windows.Forms.Button btnPause;
        private System.Windows.Forms.Button btnStop;
        private System.Windows.Forms.Button btnStart;
        private System.Windows.Forms.Label lblMessage;
        private System.Windows.Forms.TextBox txtStartParameters;
        private System.Windows.Forms.Label lblStartParameters;
    }
}
