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
using System.ComponentModel;
using System.Windows.Forms;
using Centeris.DomainJoinLib;

namespace Centeris.DomainJoin
{
	/// <summary>
	/// Summary description for ComputerNameChangesDialog.
	/// </summary>
	public class ComputerNameChangesDialog : Form
	{
	    private const string DOMAIN_JOIN_CREDENTIAL_PROMPT = "Enter the name and password of an account with permission to join the domain.";
	    
        private Label label1;
        private Label label2;
        private TextBox textBox_computerName;
        private Button button_cancel;
        private Button button_ok;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private Container components = null;
	    
	    private IDomainJoin _domainJoinServiceInterface = null;
	    private DomainJoinInfo _domainJoinInfo_orig;
		private System.Windows.Forms.Label label4;
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.PictureBox pictureBox1;
		private System.Windows.Forms.TextBox textBox_domain;
		private System.Windows.Forms.TextBox textBox_ou;
		private System.Windows.Forms.GroupBox groupBox_ou;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.RadioButton radioButton_defaultOu;
		private System.Windows.Forms.RadioButton radioButton_useOu;
	    private DomainJoinInfo _domainJoinInfo_latest;
	    
	    public ComputerNameChangesDialog()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

		}
	    
	    public ComputerNameChangesDialog(IDomainJoin domainJoinServiceInterface, DomainJoinInfo joinInfo)
	    : this()
	    {
	        _domainJoinServiceInterface = domainJoinServiceInterface;
	        _domainJoinInfo_orig = joinInfo;
	        _domainJoinInfo_latest = (DomainJoinInfo)joinInfo.Clone();
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
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(ComputerNameChangesDialog));
			this.label1 = new System.Windows.Forms.Label();
			this.label2 = new System.Windows.Forms.Label();
			this.textBox_computerName = new System.Windows.Forms.TextBox();
			this.button_cancel = new System.Windows.Forms.Button();
			this.button_ok = new System.Windows.Forms.Button();
			this.label4 = new System.Windows.Forms.Label();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.pictureBox1 = new System.Windows.Forms.PictureBox();
			this.textBox_domain = new System.Windows.Forms.TextBox();
			this.textBox_ou = new System.Windows.Forms.TextBox();
			this.groupBox_ou = new System.Windows.Forms.GroupBox();
			this.label5 = new System.Windows.Forms.Label();
			this.label3 = new System.Windows.Forms.Label();
			this.radioButton_useOu = new System.Windows.Forms.RadioButton();
			this.radioButton_defaultOu = new System.Windows.Forms.RadioButton();
			this.groupBox_ou.SuspendLayout();
			this.SuspendLayout();
			// 
			// label1
			// 
			this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.label1.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.label1.Location = new System.Drawing.Point(98, 8);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(310, 64);
			this.label1.TabIndex = 0;
			this.label1.Text = "Active Directory requires that computer names be unique in an organization. If yo" +
				"u wish, you may rename your computer by changing the name below.";
			// 
			// label2
			// 
			this.label2.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.label2.Location = new System.Drawing.Point(16, 77);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(296, 18);
			this.label2.TabIndex = 1;
			this.label2.Text = "&Computer name:";
			// 
			// textBox_computerName
			// 
			this.textBox_computerName.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.textBox_computerName.AutoSize = false;
			this.textBox_computerName.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.textBox_computerName.Location = new System.Drawing.Point(16, 95);
			this.textBox_computerName.MaxLength = 256;
			this.textBox_computerName.Name = "textBox_computerName";
			this.textBox_computerName.Size = new System.Drawing.Size(390, 20);
			this.textBox_computerName.TabIndex = 2;
			this.textBox_computerName.Text = "";
			this.textBox_computerName.WordWrap = false;
			this.textBox_computerName.TextChanged += new System.EventHandler(this.textBox_computerName_TextChanged);
			// 
			// button_cancel
			// 
			this.button_cancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.button_cancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.button_cancel.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.button_cancel.Location = new System.Drawing.Point(333, 300);
			this.button_cancel.Name = "button_cancel";
			this.button_cancel.TabIndex = 13;
			this.button_cancel.Text = "Exit";
			// 
			// button_ok
			// 
			this.button_ok.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.button_ok.Enabled = false;
			this.button_ok.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.button_ok.Location = new System.Drawing.Point(251, 300);
			this.button_ok.Name = "button_ok";
			this.button_ok.TabIndex = 12;
			this.button_ok.Text = "&Next";
			this.button_ok.Click += new System.EventHandler(this.button_ok_Click);
			// 
			// label4
			// 
			this.label4.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.label4.Location = new System.Drawing.Point(16, 116);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(296, 19);
			this.label4.TabIndex = 3;
			this.label4.Text = "&Domain to join:";
			// 
			// groupBox2
			// 
			this.groupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.groupBox2.BackColor = System.Drawing.SystemColors.WindowText;
			this.groupBox2.Location = new System.Drawing.Point(-24, 292);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(520, 1);
			this.groupBox2.TabIndex = 11;
			this.groupBox2.TabStop = false;
			// 
			// pictureBox1
			// 
			this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
			this.pictureBox1.Location = new System.Drawing.Point(26, 10);
			this.pictureBox1.Name = "pictureBox1";
			this.pictureBox1.Size = new System.Drawing.Size(48, 48);
			this.pictureBox1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
			this.pictureBox1.TabIndex = 10;
			this.pictureBox1.TabStop = false;
			// 
			// textBox_domain
			// 
			this.textBox_domain.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.textBox_domain.AutoSize = false;
			this.textBox_domain.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.textBox_domain.Location = new System.Drawing.Point(16, 134);
			this.textBox_domain.MaxLength = 256;
			this.textBox_domain.Name = "textBox_domain";
			this.textBox_domain.Size = new System.Drawing.Size(390, 20);
			this.textBox_domain.TabIndex = 4;
			this.textBox_domain.Text = "";
			this.textBox_domain.TextChanged += new System.EventHandler(this.textBox_domain_TextChanged);
			// 
			// textBox_ou
			// 
			this.textBox_ou.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.textBox_ou.AutoSize = false;
			this.textBox_ou.Enabled = false;
			this.textBox_ou.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.textBox_ou.Location = new System.Drawing.Point(104, 72);
			this.textBox_ou.MaxLength = 256;
			this.textBox_ou.Name = "textBox_ou";
			this.textBox_ou.Size = new System.Drawing.Size(280, 20);
			this.textBox_ou.TabIndex = 9;
			this.textBox_ou.Text = "";
			this.textBox_ou.TextChanged += new System.EventHandler(this.textBox_ou_TextChanged);
			// 
			// groupBox_ou
			// 
			this.groupBox_ou.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.groupBox_ou.Controls.Add(this.label5);
			this.groupBox_ou.Controls.Add(this.label3);
			this.groupBox_ou.Controls.Add(this.radioButton_useOu);
			this.groupBox_ou.Controls.Add(this.radioButton_defaultOu);
			this.groupBox_ou.Controls.Add(this.textBox_ou);
			this.groupBox_ou.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(238)));
			this.groupBox_ou.Location = new System.Drawing.Point(16, 160);
			this.groupBox_ou.Name = "groupBox_ou";
			this.groupBox_ou.Size = new System.Drawing.Size(390, 120);
			this.groupBox_ou.TabIndex = 5;
			this.groupBox_ou.TabStop = false;
			this.groupBox_ou.Text = "Organizational Unit";
			// 
			// label5
			// 
			this.label5.Location = new System.Drawing.Point(104, 96);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(280, 16);
			this.label5.TabIndex = 10;
			this.label5.AutoSize = true;
			this.label5.Text = "Example OU Path: Regional/Sales";
			// 
			// label3
			// 
			this.label3.Location = new System.Drawing.Point(8, 24);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(376, 23);
			this.label3.TabIndex = 6;
			this.label3.Text = "Specify an OU path for the computer account.";
			// 
			// radioButton_useOu
			// 
			this.radioButton_useOu.Location = new System.Drawing.Point(8, 72);
			this.radioButton_useOu.Name = "radioButton_useOu";
			this.radioButton_useOu.Size = new System.Drawing.Size(88, 24);
			this.radioButton_useOu.TabIndex = 8;
			this.radioButton_useOu.TabStop = true;
			this.radioButton_useOu.Text = "&OU Path:";
			this.radioButton_useOu.CheckedChanged += new System.EventHandler(this.radioButton_useOu_CheckedChanged);
			// 
			// radioButton_defaultOu
			// 
			this.radioButton_defaultOu.Checked = true;
			this.radioButton_defaultOu.Location = new System.Drawing.Point(8, 48);
			this.radioButton_defaultOu.Name = "radioButton_defaultOu";
			this.radioButton_defaultOu.Size = new System.Drawing.Size(376, 24);
			this.radioButton_defaultOu.TabIndex = 7;
			this.radioButton_defaultOu.TabStop = true;
			this.radioButton_defaultOu.Text = "De&fault to \"Computers\" container.";
			this.radioButton_defaultOu.CheckedChanged += new System.EventHandler(this.radioButton_defaultOu_CheckedChanged);
			// 
			// ComputerNameChangesDialog
			// 
			this.AcceptButton = this.button_ok;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.BackColor = System.Drawing.SystemColors.Window;
			this.CancelButton = this.button_cancel;
			this.ClientSize = new System.Drawing.Size(418, 328);
			this.Controls.Add(this.groupBox_ou);
			this.Controls.Add(this.textBox_computerName);
			this.Controls.Add(this.textBox_domain);
			this.Controls.Add(this.pictureBox1);
			this.Controls.Add(this.groupBox2);
			this.Controls.Add(this.button_ok);
			this.Controls.Add(this.button_cancel);
			this.Controls.Add(this.label2);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.label4);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "ComputerNameChangesDialog";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Join Active Directory Domain";
			this.Load += new System.EventHandler(this.ComputerNameChangesDialog_Load);
			this.groupBox_ou.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion


	    private bool IsJoinedToDomain(DomainJoinInfo joinInfo)
	    {
	        return joinInfo.DomainName != null && joinInfo.DomainName.Length > 0;
	    }
	    
	    private string FullyQualifiedName(DomainJoinInfo joinInfo)
	    {
	        return string.Format("{0}.{1}", joinInfo.Name, joinInfo.DomainName);
	    }

        private void ComputerNameChangesDialog_Load(object sender, EventArgs e)
        {
            textBox_domain.Text = _domainJoinInfo_latest.DomainName;
            textBox_computerName.Text = _domainJoinInfo_latest.Name;
        }

        private void textBox_computerName_TextChanged(object sender, EventArgs e)
        {
            string currentCompName = textBox_computerName.Text.Trim();
            _domainJoinInfo_latest.Name = currentCompName;
            AdjustOKButton();
        }

        private void textBox_domain_TextChanged(object sender, EventArgs e)
        {
            _domainJoinInfo_latest.DomainName = textBox_domain.Text.Trim();
            AdjustOKButton();
        }		

		private void textBox_ou_TextChanged(object sender, System.EventArgs e)
		{
			AdjustOKButton();
		}

		private void radioButton_defaultOu_CheckedChanged(object sender, System.EventArgs e)
		{			
			textBox_ou.Enabled = !radioButton_defaultOu.Checked;
			AdjustOKButton();
		}

		private void radioButton_useOu_CheckedChanged(object sender, System.EventArgs e)
		{	
			AdjustOKButton();
		}

		private void AdjustOKButton()
	    {
	        button_ok.Enabled = ApplicableChangesExist();			
	    }
	    
	    private bool ApplicableChangesExist()
	    {
	        return
	           (!_domainJoinInfo_latest.Equals(_domainJoinInfo_orig) &&
				(radioButton_defaultOu.Checked || (radioButton_useOu.Checked && textBox_ou.Text.Trim() != "") ) &&
	            (IsValidHostname(_domainJoinInfo_latest.Name)) &&				 
	            ((IsJoinedToDomain(_domainJoinInfo_latest) && IsValidDomainName(_domainJoinInfo_latest.DomainName)) ));
	    }
	    
	    private bool IsValidHostname(string hostName)
	    {
	        return hostName != null && hostName.Length > 0;
	    }
	    
	    private bool IsValidDomainName(string domainName)
	    {
	        return domainName != null && domainName.Length > 0;
	    }

		private bool IsValidUsername(string sUsername)
		{
			return sUsername != null && sUsername.Trim().Length>0;
		}
	    
	    private bool IsValidWorkgroupName(string workgroupName)
	    {
	        return workgroupName != null && workgroupName.Length > 0;
	    }

        private void button_ok_Click(object sender, EventArgs e)
        {
            try
            {
				SetOrganizationalUnit();

				if (HostnameChanged())
				{
					string sNote = "Note that in some Linux distributions changing the name of a computer may\n";
					sNote +=       " cause problems with X Windows until you log out and log in again.";
					MessageBox.Show(this, sNote, "Likewise Open", MessageBoxButtons.OK, MessageBoxIcon.Information);
				}

				CredsDialog creds = new CredsDialog();

				if ( creds.ShowDialog(this) == DialogResult.OK )
				{
					JoinProgressForm jpf = new JoinProgressForm( _domainJoinServiceInterface, 
																 _domainJoinInfo_latest, 
																 HostnameChanged(),
																 DomainInfoChanged(),
																 creds.Username, 
																 creds.Password);

					if (jpf.ShowDialog(this) == DialogResult.OK)
					{
						DialogResult = DialogResult.OK;
						Close();
					}
				}
            }
            catch(Exception ex)
            {
                MessageBox.Show(ex.Message);
                DialogResult = DialogResult.Ignore;
            }
        }
	    
	    private bool HostnameChanged()
	    {
	        return string.Compare(_domainJoinInfo_orig.Name, _domainJoinInfo_latest.Name) != 0;
	    }
	    
	    private bool DomainInfoChanged()
	    {
	        return (0 != string.Compare(_domainJoinInfo_orig.DomainName, _domainJoinInfo_latest.DomainName, true));
	    }			

		private void SetOrganizationalUnit()
		{
			if ( radioButton_useOu.Checked )
				_domainJoinInfo_latest.OrganizationalUnit = textBox_ou.Text.Trim();
			else
				_domainJoinInfo_latest.OrganizationalUnit = null;
		}		
	}
}
