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
using Centeris.DomainJoinLib;
using System.Threading;
using System.Net;

namespace Centeris.DomainJoin
{
	/// <summary>
	/// Summary description for JoinProgressForm.
	/// </summary>
	public class JoinProgressForm : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Label lblProgress;
		private System.ComponentModel.IContainer components;

		private IDomainJoin dj;
		private DomainJoinInfo dji;
		private bool bRenameComputer;
		private bool bJoin;
		private string sUsername;
		private System.Windows.Forms.Button btnClose;
		private System.Windows.Forms.Timer timer;
		private string sPassword;

		// delegate information
		private enum PROGRESS 
		{ 
			Renaming, 
			Renamed, 
			CheckingDNS, 
			CheckedDNS, 
			Joining, 
			Joined,
			Leaving,
			Left,
			Error
		};
		private delegate void ReportProgressDelegate(PROGRESS p);
		private ReportProgressDelegate rpd;
		private System.Windows.Forms.ProgressBar progressBar;
		private System.Windows.Forms.PictureBox pictureBox1;
		private System.Windows.Forms.Button btnDetail;
		private Exception threadError = null;

		public JoinProgressForm(IDomainJoin dj, DomainJoinInfo dji, bool bRenameComputer, bool bJoin, string sUsername, string sPassword)
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			this.dj = dj;
			this.dji = dji;
			this.bRenameComputer = bRenameComputer;
			this.bJoin = bJoin;
			this.sUsername = sUsername;
			this.sPassword = sPassword;

			rpd = new ReportProgressDelegate(ReportProgress_T1);
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
            this.components = new System.ComponentModel.Container();
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(JoinProgressForm));
            this.lblProgress = new System.Windows.Forms.Label();
            this.btnClose = new System.Windows.Forms.Button();
            this.timer = new System.Windows.Forms.Timer(this.components);
            this.progressBar = new System.Windows.Forms.ProgressBar();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.btnDetail = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // lblProgress
            // 
            this.lblProgress.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.lblProgress.Location = new System.Drawing.Point(96, 32);
            this.lblProgress.Name = "lblProgress";
            this.lblProgress.Size = new System.Drawing.Size(264, 64);
            this.lblProgress.TabIndex = 1;
            // 
            // btnClose
            // 
            this.btnClose.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnClose.Location = new System.Drawing.Point(288, 112);
            this.btnClose.Name = "btnClose";
            this.btnClose.TabIndex = 3;
            this.btnClose.Text = "&Close";
            this.btnClose.Visible = false;
            this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // timer
            // 
            this.timer.Enabled = true;
            this.timer.Tick += new System.EventHandler(this.timer_Tick);
            // 
            // progressBar
            // 
            this.progressBar.Location = new System.Drawing.Point(24, 104);
            this.progressBar.Name = "progressBar";
            this.progressBar.Size = new System.Drawing.Size(328, 23);
            this.progressBar.TabIndex = 4;
            // 
            // pictureBox1
            // 
            this.pictureBox1.BackColor = System.Drawing.Color.Transparent;
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(16, 16);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(48, 48);
            this.pictureBox1.TabIndex = 5;
            this.pictureBox1.TabStop = false;
            // 
            // btnDetail
            // 
            this.btnDetail.Location = new System.Drawing.Point(200, 112);
            this.btnDetail.Name = "btnDetail";
            this.btnDetail.TabIndex = 3;
            this.btnDetail.Text = "&Details...";
            this.btnDetail.Visible = false;
            this.btnDetail.Click += new System.EventHandler(this.btnDetail_Click);
            // 
            // JoinProgressForm
            // 
            this.AcceptButton = this.btnClose;
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.BackColor = System.Drawing.SystemColors.Window;
            this.ClientSize = new System.Drawing.Size(370, 144);
            this.Controls.Add(this.pictureBox1);
            this.Controls.Add(this.progressBar);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.lblProgress);
            this.Controls.Add(this.btnDetail);
            this.Cursor = System.Windows.Forms.Cursors.WaitCursor;
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "JoinProgressForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Open Range";
            this.Load += new System.EventHandler(this.JoinProgressForm_Load);
            this.ResumeLayout(false);

        }
		#endregion

		private void JoinProgressForm_Load(object sender, System.EventArgs e)
		{
		}

		private void timer_Tick(object sender, System.EventArgs e)
		{
			// diable the timer
			timer.Enabled = false;

			// start a thread to do the real work
			this.Cursor = Cursors.WaitCursor;
			Cursor.Current = Cursors.WaitCursor;

			Thread th = new Thread(new ThreadStart(WorkerThread));
			threadError = null;
			th.Start();
			while (th.IsAlive)
				Application.DoEvents();

			this.Cursor = Cursors.Default;
			Cursor.Current = Cursors.Default;
			this.progressBar.Visible = false;
			this.btnClose.Visible = true;
			this.Invalidate(true);

		}

		private void ReportProgress_T1(PROGRESS p)
		{
			switch(p)
			{
				case PROGRESS.CheckingDNS:
					progressBar.Value = 10;
					lblProgress.Text = "Testing DNS configuration...";
					break;

				case PROGRESS.CheckedDNS:
					progressBar.Value = 30;
					lblProgress.Text = "DNS configuration is ok";
					break;

				case PROGRESS.Renaming:
					progressBar.Value = 35;
					lblProgress.Text = "Renaming computer...";
					break;

				case PROGRESS.Renamed:
					progressBar.Value = 50;
					lblProgress.Text = "Renamed computer as: " + dji.Name;
					break;

				case PROGRESS.Joining:
					progressBar.Value = 55;
					lblProgress.Text = "Joining domain...";
					break;

				case PROGRESS.Joined:
					progressBar.Value = 100;
					lblProgress.Text = string.Format("Welcome to the {0} domain", dji.DomainName);
					break;

				case PROGRESS.Leaving:
					progressBar.Value = 10;
					lblProgress.Text = "Leaving domain...";
					break;

				case PROGRESS.Left:
					progressBar.Value = 100;
					lblProgress.Text = string.Format("This computer has left the {0} domain.", dji.DomainName);
					break;

				case PROGRESS.Error:
					lblProgress.Text = string.Format("Error: {0}", threadError.Message);
					if (threadError.InnerException!=null)
						btnDetail.Visible = true;
					break;

			}
		}

		private void ReportProgress(PROGRESS p)
		{
			Invoke(rpd, new object[] { p } );
		}

		private void WorkerThread()
		{
			try
			{
				if (bRenameComputer)
				{
					ReportProgress(PROGRESS.Renaming);
					try
					{
						dj.SetComputerName(dji.Name);
					}
					catch(Exception ex)
					{
						throw new Exception("Can not rename the computer", ex);
					}
					ReportProgress(PROGRESS.Renamed);
				}

				if (bJoin)
				{
					ReportProgress(PROGRESS.CheckingDNS);
					try
					{
						if (!dj.IsDomainNameResolvable(dji.DomainName))
							throw new Exception("Can not resolve the domain name. Is DNS configured properly?");
					}
					catch(Exception ex)
					{
						throw new Exception("Can not resolve the domain name. Is DNS configured properly?", ex);
					}
					ReportProgress(PROGRESS.CheckedDNS);
					ReportProgress(PROGRESS.Joining);
					try
					{
						dj.JoinDomain(dji.DomainName, sUsername, sPassword, dji.OrganizationalUnit, false);
					}
					catch(Exception ex)
					{
						throw new Exception("Can not join the domain", ex);
					}
					ReportProgress(PROGRESS.Joined);
				}
				else
				{
					ReportProgress(PROGRESS.Leaving);
					try
					{
						dj.JoinWorkgroup("WORKGROUP", "dummy", "dummy");
					}
					catch(Exception ex)
					{
						throw new Exception("Can not leave the domain", ex);
					}
					ReportProgress(PROGRESS.Left);
				}
			}
			catch(Exception ex)
			{
				threadError = ex;
				ReportProgress(PROGRESS.Error);
			}
		}

		private void btnClose_Click(object sender, System.EventArgs e)
		{
			// if we got an error, return retry status
			if (threadError!=null)
				DialogResult = DialogResult.Retry;
		}

		private void btnDetail_Click(object sender, System.EventArgs e)
		{
			// MessageBox.Show(this, threadError.InnerException.Message, "Open Range", MessageBoxButtons.OK);
			string sMessage = "";
			if (threadError.InnerException.Message!=null)
				sMessage = threadError.InnerException.Message;
			DetailForm df = new DetailForm(lblProgress.Text, sMessage, dji.LogFilePath);
			this.Close();
			df.ShowDialog(this);
		}
	}
}
