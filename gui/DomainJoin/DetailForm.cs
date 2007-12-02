//
// Copyright (C) Centeris Corporation 2004-2007
// Copyright (C) Likewise Software 2007.  
// All rights reserved.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.IO;
using Centeris.DomainJoinLib;

namespace Centeris.DomainJoin
{
	/// <summary>
	/// Displayed when the user wants to see more error message detail.
	/// (Typically, when an exception is caught.)
	/// </summary>
	public class DetailForm : System.Windows.Forms.Form
	{
        private const string szDetailFile = "/tmp/lwidentity.detail.log";
		private const string szLogFileCopy = "/tmp/lwidentity.detail.log.copy";

		private System.Windows.Forms.TextBox tbDetails;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.Label lblError;
		private System.Windows.Forms.CheckBox cbSendLog;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.Button btnOK;
		private System.Windows.Forms.PictureBox pbIcon;
        private System.Windows.Forms.TextBox tbEmail;
        private System.Windows.Forms.Label lblEmail;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public DetailForm(string sError, string sDetail, string sLogFilePath)
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			if (sError==null)
				sError = "Error!";

			if (sDetail==null)
				sDetail = "Details!";

			lblError.Text = sError;
			tbDetails.Text = sDetail;

			// Set up information to send to supportlogs. Put this in a catch
			// block because this needs to be very unobstrusive
			try
			{
				StreamWriter sw = new StreamWriter(szDetailFile, false);
				sw.WriteLine("Error:   " + sError);
				sw.WriteLine("Details: " + sDetail);

				if (sLogFilePath!=null)
				{
					// copy the log file first since mono seems to have problems
					// with file sharing
					File.Copy(sLogFilePath, szLogFileCopy, true);
					
					StreamReader sr = new StreamReader(szLogFileCopy);
					sw.WriteLine("Log:");
					sw.WriteLine("-----------------------------------------------------");

					string sLog;
					while((sLog=sr.ReadLine())!=null)
						sw.WriteLine(sLog);

					sr.Close();

					// remove the copy
					File.Delete(szLogFileCopy);
				}

				sw.Close();
			}
			catch(Exception ex)
			{
				ShowError("Error: " + ex.Message);
			}

		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(DetailForm));
			this.tbDetails = new System.Windows.Forms.TextBox();
			this.label2 = new System.Windows.Forms.Label();
			this.lblError = new System.Windows.Forms.Label();
			this.cbSendLog = new System.Windows.Forms.CheckBox();
			this.label3 = new System.Windows.Forms.Label();
			this.btnOK = new System.Windows.Forms.Button();
			this.pbIcon = new System.Windows.Forms.PictureBox();
			this.lblEmail = new System.Windows.Forms.Label();
			this.tbEmail = new System.Windows.Forms.TextBox();
			this.SuspendLayout();
			// 
			// tbDetails
			// 
			this.tbDetails.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.tbDetails.AutoSize = false;
			this.tbDetails.BackColor = System.Drawing.SystemColors.Control;
			this.tbDetails.Location = new System.Drawing.Point(16, 85);
			this.tbDetails.Multiline = true;
			this.tbDetails.Name = "tbDetails";
			this.tbDetails.ReadOnly = true;
			this.tbDetails.ScrollBars = System.Windows.Forms.ScrollBars.Both;
			this.tbDetails.Size = new System.Drawing.Size(538, 59);
			this.tbDetails.TabIndex = 2;
			this.tbDetails.Text = "";
			// 
			// label2
			// 
			this.label2.AutoSize = true;
			this.label2.BackColor = System.Drawing.Color.Transparent;
			this.label2.Location = new System.Drawing.Point(16, 64);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(49, 18);
			this.label2.TabIndex = 1;
			this.label2.Text = "&Details:";
			// 
			// lblError
			// 
			this.lblError.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.lblError.BackColor = System.Drawing.SystemColors.Control;
			this.lblError.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.lblError.Location = new System.Drawing.Point(80, 8);
			this.lblError.Name = "lblError";
			this.lblError.Size = new System.Drawing.Size(480, 56);
			this.lblError.TabIndex = 0;
			// 
			// cbSendLog
			// 
			this.cbSendLog.Anchor = System.Windows.Forms.AnchorStyles.Left;
			this.cbSendLog.BackColor = System.Drawing.SystemColors.Control;
			this.cbSendLog.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.cbSendLog.Location = new System.Drawing.Point(16, 152);
			this.cbSendLog.Name = "cbSendLog";
			this.cbSendLog.Size = new System.Drawing.Size(424, 24);
			this.cbSendLog.TabIndex = 3;
			this.cbSendLog.Text = "&Send a log file to Centeris to help diagnose this problem.";
			this.cbSendLog.CheckedChanged += new System.EventHandler(this.cbSendLog_CheckedChanged);
			// 
			// label3
			// 
			this.label3.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.label3.BackColor = System.Drawing.SystemColors.Control;
			this.label3.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.label3.Location = new System.Drawing.Point(32, 176);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(520, 48);
			this.label3.TabIndex = 4;
			this.label3.Text = "Note: Enabling this option requires that you have access to the Internet. The inf" +
				"ormation that will be sent is contained in the file /tmp/lwidentity.detail.log. " +
				"If you enter your  email address you will be contacted by a Centeris support rep" +
				"resentative.";
			// 
			// btnOK
			// 
			this.btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.btnOK.Location = new System.Drawing.Point(472, 264);
			this.btnOK.Name = "btnOK";
			this.btnOK.Size = new System.Drawing.Size(75, 24);
			this.btnOK.TabIndex = 7;
			this.btnOK.Text = "O&K";
			this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
			// 
			// pbIcon
			// 
			this.pbIcon.Image = ((System.Drawing.Image)(resources.GetObject("pbIcon.Image")));
			this.pbIcon.Location = new System.Drawing.Point(16, 10);
			this.pbIcon.Name = "pbIcon";
			this.pbIcon.Size = new System.Drawing.Size(48, 48);
			this.pbIcon.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
			this.pbIcon.TabIndex = 4;
			this.pbIcon.TabStop = false;
			// 
			// lblEmail
			// 
			this.lblEmail.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.lblEmail.AutoSize = true;
			this.lblEmail.BackColor = System.Drawing.SystemColors.Control;
			this.lblEmail.Enabled = false;
			this.lblEmail.Location = new System.Drawing.Point(38, 232);
			this.lblEmail.Name = "lblEmail";
			this.lblEmail.Size = new System.Drawing.Size(157, 18);
			this.lblEmail.TabIndex = 5;
			this.lblEmail.Text = "&Email Address (Optional):";
			// 
			// tbEmail
			// 
			this.tbEmail.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.tbEmail.Location = new System.Drawing.Point(200, 232);
			this.tbEmail.Name = "tbEmail";
			this.tbEmail.Size = new System.Drawing.Size(352, 22);
			this.tbEmail.TabIndex = 6;
			this.tbEmail.Text = "";
			// 
			// DetailForm
			// 
			this.AcceptButton = this.btnOK;
			this.AutoScaleBaseSize = new System.Drawing.Size(6, 15);
			this.BackColor = System.Drawing.SystemColors.Control;
			this.ClientSize = new System.Drawing.Size(570, 296);
			this.Controls.Add(this.tbEmail);
			this.Controls.Add(this.tbDetails);
			this.Controls.Add(this.label2);
			this.Controls.Add(this.lblEmail);
			this.Controls.Add(this.pbIcon);
			this.Controls.Add(this.btnOK);
			this.Controls.Add(this.cbSendLog);
			this.Controls.Add(this.lblError);
			this.Controls.Add(this.label3);
			this.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.MinimumSize = new System.Drawing.Size(440, 320);
			this.Name = "DetailForm";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Likewise Open";
			this.ResumeLayout(false);

		}
		#endregion

		private void btnOK_Click(object sender, System.EventArgs e)
		{
			if (cbSendLog.Checked)
			{
				// Do nothing
			}
		}

        private void cbSendLog_CheckedChanged(object sender, System.EventArgs e)
        {
            lblEmail.Enabled = tbEmail.Enabled = cbSendLog.Checked;
        }

        private void ShowError(string sMessage)
        {
            MessageBox.Show(this, sMessage, "Likewise Open", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
	}
}
