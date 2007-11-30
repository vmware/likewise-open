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
using System.Text;
using System.Windows.Forms;
using Centeris.DomainJoinLib;

namespace Centeris.DomainJoin
{
	/// <summary>
	/// This class contains the main entry point for the program. Once joined, it is also
	/// the form that displays the joined domain and allows the user to leave the domain.
	/// </summary>
	public class DomainJoinManagerForm : Form
	{
        private const string szFirstTimeFilePath = "/var/cache/likewise-open/DomainJoin.dat";

        private Label label_change;
        private Label label_domain_header;
        private Label label_fullComputerName_header;
        private Label label_panelDescription;
        private Label label_fullComputerName;
        private Label label_domain;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;
	    private DomainJoinInfo _domainJoinInfo_orig;
	    private DomainJoinInfo _domainJoinInfo_latest;
	    
	    private string _logFilePath = null;
	    private bool   _testMode = false;
		private System.Windows.Forms.Button button_leave;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.Button btnClose;
		private System.Windows.Forms.PictureBox pictureBox1;
	    
	    // Domain Join Service Interface
	    private IDomainJoin _domainJoinLib = null;

		public DomainJoinManagerForm()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

        }
	    
	    public bool TestMode
	    {
	        get
	        {
	            return _testMode;
	        }
	        set
	        {
	            _testMode = value;
	        }
	    }
	    
	    public string LogFilePath
	    {
	        get
	        {
	            return _logFilePath;
	        }
	        set
	        {
	            _logFilePath = value;
	        }
	    }
	    
	    /// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
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
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(DomainJoinManagerForm));
            this.label_domain = new System.Windows.Forms.Label();
            this.label_fullComputerName = new System.Windows.Forms.Label();
            this.button_leave = new System.Windows.Forms.Button();
            this.label_change = new System.Windows.Forms.Label();
            this.label_domain_header = new System.Windows.Forms.Label();
            this.label_fullComputerName_header = new System.Windows.Forms.Label();
            this.label_panelDescription = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.btnClose = new System.Windows.Forms.Button();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.SuspendLayout();
            // 
            // label_domain
            // 
            this.label_domain.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.label_domain.Location = new System.Drawing.Point(158, 135);
            this.label_domain.Name = "label_domain";
            this.label_domain.Size = new System.Drawing.Size(232, 57);
            this.label_domain.TabIndex = 4;
            // 
            // label_fullComputerName
            // 
            this.label_fullComputerName.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.label_fullComputerName.Location = new System.Drawing.Point(158, 101);
            this.label_fullComputerName.Name = "label_fullComputerName";
            this.label_fullComputerName.Size = new System.Drawing.Size(232, 21);
            this.label_fullComputerName.TabIndex = 2;
            // 
            // button_leave
            // 
            this.button_leave.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.button_leave.Location = new System.Drawing.Point(328, 203);
            this.button_leave.Name = "button_leave";
            this.button_leave.Size = new System.Drawing.Size(75, 21);
            this.button_leave.TabIndex = 6;
            this.button_leave.Text = "&Leave";
            this.button_leave.Click += new System.EventHandler(this.button_change_Click);
            this.button_leave.KeyDown += new System.Windows.Forms.KeyEventHandler(this.button_change_KeyDown);
            // 
            // label_change
            // 
            this.label_change.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.label_change.Location = new System.Drawing.Point(56, 203);
            this.label_change.Name = "label_change";
            this.label_change.Size = new System.Drawing.Size(256, 21);
            this.label_change.TabIndex = 5;
            this.label_change.Text = "To leave the current domain, click Leave.";
            // 
            // label_domain_header
            // 
            this.label_domain_header.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.label_domain_header.Location = new System.Drawing.Point(30, 135);
            this.label_domain_header.Name = "label_domain_header";
            this.label_domain_header.Size = new System.Drawing.Size(120, 21);
            this.label_domain_header.TabIndex = 3;
            this.label_domain_header.Text = "Domain:";
            // 
            // label_fullComputerName_header
            // 
            this.label_fullComputerName_header.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.label_fullComputerName_header.Location = new System.Drawing.Point(30, 101);
            this.label_fullComputerName_header.Name = "label_fullComputerName_header";
            this.label_fullComputerName_header.Size = new System.Drawing.Size(120, 21);
            this.label_fullComputerName_header.TabIndex = 1;
            this.label_fullComputerName_header.Text = "Computer name:";
            // 
            // label_panelDescription
            // 
            this.label_panelDescription.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.label_panelDescription.Location = new System.Drawing.Point(120, 24);
            this.label_panelDescription.Name = "label_panelDescription";
            this.label_panelDescription.Size = new System.Drawing.Size(288, 48);
            this.label_panelDescription.TabIndex = 0;
            this.label_panelDescription.Text = "Likewise is using  the following information to identify your computer on the net" +
                "work.";
            // 
            // groupBox1
            // 
            this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
                | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox1.BackColor = System.Drawing.SystemColors.WindowText;
            this.groupBox1.Location = new System.Drawing.Point(0, 256);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(456, 1);
            this.groupBox1.TabIndex = 7;
            this.groupBox1.TabStop = false;
            // 
            // btnClose
            // 
            this.btnClose.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnClose.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.btnClose.Location = new System.Drawing.Point(328, 266);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(75, 21);
            this.btnClose.TabIndex = 8;
            this.btnClose.Text = "&Close";
            this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // pictureBox1
            // 
            this.pictureBox1.BackColor = System.Drawing.Color.Transparent;
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(24, 24);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(48, 48);
            this.pictureBox1.TabIndex = 12;
            this.pictureBox1.TabStop = false;
            // 
            // DomainJoinManagerForm
            // 
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.BackColor = System.Drawing.SystemColors.Window;
            this.ClientSize = new System.Drawing.Size(413, 296);
            this.Controls.Add(this.pictureBox1);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.label_fullComputerName_header);
            this.Controls.Add(this.label_fullComputerName);
            this.Controls.Add(this.label_domain);
            this.Controls.Add(this.button_leave);
            this.Controls.Add(this.label_change);
            this.Controls.Add(this.label_domain_header);
            this.Controls.Add(this.label_panelDescription);
            this.Controls.Add(this.btnClose);
            this.DockPadding.All = 10;
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "DomainJoinManagerForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Open Range";
            this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.DomainJoinManagerForm_KeyDown);
            this.Load += new System.EventHandler(this.DomainJoinManagerForm_Load);
            this.ResumeLayout(false);

        }
		#endregion

        private void DomainJoinManagerForm_Load(object sender, EventArgs e)
        {
            DomainJoinLogControl.SetLogFile(LogFilePath);
            
            LoadData();

			bool bQuit = false;

			// check to make sure we're running as root
			if (!DomainJoinServiceInterface.IsRootUser())
			{
				MessageBox.Show("Error: This program can only be run by the root user", "Open Range", MessageBoxButtons.OK);
				bQuit = true;
			}

            // if this is the first time we're running, do a phone home
            if (!System.IO.File.Exists(szFirstTimeFilePath))
            {
                // Create the "first time" file so that we know it's not the first time anymore
                try
                {
                    System.IO.File.Create(szFirstTimeFilePath).Close();
                }
                catch(Exception)
                {
                    // never let phone home code cause problems
                }
            }

			// if we're not joined, show the welcome form here
			if (!bQuit && !IsJoinedToDomain(_domainJoinInfo_latest))
			{
				WelcomeForm we = new WelcomeForm();
				if (we.ShowDialog(this)==DialogResult.Cancel)
					bQuit = true;
				else
				{

					// bring up the join page
					DialogResult dr;
					for(;;)
					{
						ComputerNameChangesDialog computerNameChangeDlg = new ComputerNameChangesDialog(DomainJoinServiceInterface, _domainJoinInfo_latest);
						dr = computerNameChangeDlg.ShowDialog(we);
						LoadData();

						if (dr!=DialogResult.Retry)
							break;

					}

					if (dr==DialogResult.Cancel)
						bQuit = true;
				}
			}

			if (!bQuit)
				RefreshData();
			else
				Application.Exit();
        }
	    
	    private void LoadData()
	    {
	        try
	        {
	            _domainJoinInfo_orig = DomainJoinServiceInterface.QueryInformation();
	            _domainJoinInfo_latest = (DomainJoinInfo)_domainJoinInfo_orig.Clone();
	        }
	        catch(Exception ex)
	        {
	            MessageBox.Show(ex.Message, "Open Range: Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
	        }
	    }
	    
	    private void ShowUnjoined()
	    {
	        label_domain.Visible = false;
	        label_domain_header.Visible = false;
	    }
	    
	    private void ShowDomainInfo()
	    {
            label_domain.Visible = true;
            label_domain_header.Visible = true;
	    }
	    
	    private string GetFullyQualifiedName(DomainJoinInfo joinInfo)
	    {
			if (joinInfo.DnsDomain!=null && joinInfo.DnsDomain != "")
				return String.Format("{0}.{1}", joinInfo.Name, joinInfo.DnsDomain);
			else
				return joinInfo.Name;
	    }
	    
	    private bool IsJoinedToDomain(DomainJoinInfo joinInfo)
	    {
	        return joinInfo.DomainName != null && joinInfo.DomainName.Length > 0;
	    }
	    
	    private void RefreshData()
	    {
	        DomainJoinInfo domainJoinInfo = _domainJoinInfo_latest;
            label_fullComputerName.Text = GetFullyQualifiedName(domainJoinInfo);
	        
	        if (IsJoinedToDomain(_domainJoinInfo_latest))
            {
                label_domain.Text = _domainJoinInfo_latest.DomainName;
                ShowDomainInfo();
            }
	        else 
            {
                ShowUnjoined();
            }
            
	    }

        private void button_change_Click(object sender, EventArgs e)
        {
			string sPrompt = string.Format("Are you sure you want to leave the {0} domain?", _domainJoinInfo_latest.DomainName);
			Prompt p = new Prompt(sPrompt);
			if (p.ShowDialog(this)==DialogResult.OK)
			{
				JoinProgressForm jpf = new JoinProgressForm(DomainJoinServiceInterface, _domainJoinInfo_latest, false, false, "dummy", "dummy");
				jpf.ShowDialog(this);

				// reload data to see our state
				LoadData();
				RefreshData();

				// if we're not joined, show the join form
				if (!IsJoinedToDomain(_domainJoinInfo_latest))
				{
					this.Hide();
					ComputerNameChangesDialog computerNameChangeDlg = new ComputerNameChangesDialog(DomainJoinServiceInterface, _domainJoinInfo_latest);
					if (computerNameChangeDlg.ShowDialog(this)==DialogResult.Cancel)
						Application.Exit();

					// reload data to see our state
					this.Show();
					LoadData();
					RefreshData();
				}
			}

        }

	    private bool ApplicableChangesExist()
	    {
	        return (!_domainJoinInfo_orig.Equals(_domainJoinInfo_latest));
	    }
	    
	    private void SaveComputerInfo()
	    {
	        _domainJoinInfo_orig = (DomainJoinInfo)_domainJoinInfo_latest.Clone();
	    }
	    
        private void DomainJoinManagerForm_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.F5)
            {
                LoadData();
                RefreshData();
            }
        }

        private void textBox_computerDescription_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.F5)
            {
                LoadData();
                RefreshData();
                e.Handled = true;
            }
        }

        private void button_change_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.F5)
            {
                LoadData();
                RefreshData();
                e.Handled = true;
            }
        }

        private void button_ok_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.F5)
            {
                LoadData();
                RefreshData();
                e.Handled = true;
            }
        }

        private void button_cancel_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.F5)
            {
                LoadData();
                RefreshData();
                e.Handled = true;
            }
        }

        private void button_apply_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.F5)
            {
                LoadData();
                RefreshData();
                e.Handled = true;
            }
        }
	    
        private IDomainJoin DomainJoinServiceInterface
        {
            get
            {
                if (_domainJoinLib == null)
                {
                    _domainJoinLib = DomainJoinFactory.Create(TestMode ? DomainJoinClassType.Test : DomainJoinClassType.Real);
                }
                return _domainJoinLib;
            }
        }
	    
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args) 
        {
            DomainJoinManagerForm domainJoinManagerForm = BuildDomainJoinManagerForm(args);
            Application.Run(domainJoinManagerForm);
        }
	    
	    static void ShowUsage(string errorMessage)
	    {
	        StringBuilder sb = new StringBuilder();
	        if (null != errorMessage)
	        {
	            sb.Append(errorMessage + "\r\n\r\n");
	        }
	        string programName = Test.GetProgramName();
	        sb.Append("usage: " + programName + " [--test] [--help] [--log {.|path}]\r\n" +
	                  "\r\n" +
	                  "    Note: use . for the --log option to log to the console\r\n" +
	                  "\r\n" +
	                  "    example:\r\n" +
	                  "\r\n" +
                      "        " + programName + " --test --log /tmp/domainjoin.log\r\n" +
                      "\r\n");
            int p = (int) Environment.OSVersion.Platform;
            if ((p == 4) || (p == 128))
            {
                // Running on Unix i.e. mono
                Console.Write(sb.ToString());
            }
	        else
            {
                MessageBox.Show(sb.ToString(), "Open Range", MessageBoxButtons.OK, MessageBoxIcon.Information);
	        }
	        Environment.Exit(1);
	    }
	    
	    static DomainJoinManagerForm BuildDomainJoinManagerForm(string[] args)
	    {
	        DomainJoinManagerForm result = new DomainJoinManagerForm();
	        Test.CommonOptions options = Test.CommonOptions.Parse(args);

	        if (options.IsHelp)
                {
                    ShowUsage(null);
                }
                else if (options.ArgError != null)
                {
                    ShowUsage(options.ArgError);
                }
                else if (options.IsUnknown)
                {
                    ShowUsage("Unknown option: " + args[options.UnknownIndex]);
                }

	        result.TestMode = options.IsTest;
	        result.LogFilePath = options.LogName;
	        
	        return result;
	    }

		private void btnClose_Click(object sender, System.EventArgs e)
		{
			DialogResult = DialogResult.OK;
			Close();
		}
	}
}
