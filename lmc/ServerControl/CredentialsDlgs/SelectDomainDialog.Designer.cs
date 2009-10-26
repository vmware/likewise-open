namespace Likewise.LMC.ServerControl
{
    partial class SelectDomainDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SelectDomainDialog));
            this.label1 = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.tbDomain = new System.Windows.Forms.TextBox();
            this.rbOtherDomain = new System.Windows.Forms.RadioButton();
            this.rbDefaultDomain = new System.Windows.Forms.RadioButton();
            this.btnOk = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.linkCreds = new System.Windows.Forms.LinkLabel();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(7, 13);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(191, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Select the domain you want to manage";
            //
            // groupBox1
            //
            this.groupBox1.Controls.Add(this.tbDomain);
            this.groupBox1.Controls.Add(this.rbOtherDomain);
            this.groupBox1.Controls.Add(this.rbDefaultDomain);
            this.groupBox1.Location = new System.Drawing.Point(10, 36);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(350, 89);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Domain name";
            //
            // tbDomain
            //
            this.tbDomain.Location = new System.Drawing.Point(100, 51);
            this.tbDomain.Name = "tbDomain";
            this.tbDomain.Size = new System.Drawing.Size(236, 20);
            this.tbDomain.TabIndex = 2;
            this.tbDomain.TextChanged += new System.EventHandler(this.tbDomain_TextChanged);
            //
            // rbOtherDomain
            //
            this.rbOtherDomain.AutoSize = true;
            this.rbOtherDomain.Location = new System.Drawing.Point(7, 53);
            this.rbOtherDomain.Name = "rbOtherDomain";
            this.rbOtherDomain.Size = new System.Drawing.Size(88, 17);
            this.rbOtherDomain.TabIndex = 1;
            this.rbOtherDomain.TabStop = true;
            this.rbOtherDomain.Text = "Other domain";
            this.rbOtherDomain.UseVisualStyleBackColor = true;
            this.rbOtherDomain.CheckedChanged += new System.EventHandler(this.rbOtherDomain_CheckedChanged);
            //
            // rbDefaultDomain
            //
            this.rbDefaultDomain.AutoSize = true;
            this.rbDefaultDomain.Location = new System.Drawing.Point(7, 23);
            this.rbDefaultDomain.Name = "rbDefaultDomain";
            this.rbDefaultDomain.Size = new System.Drawing.Size(276, 17);
            this.rbDefaultDomain.TabIndex = 0;
            this.rbDefaultDomain.TabStop = true;
            this.rbDefaultDomain.Text = "Default domain (the domain this computer is joined to)";
            this.rbDefaultDomain.UseVisualStyleBackColor = true;
            this.rbDefaultDomain.CheckedChanged += new System.EventHandler(this.rbDefaultDomain_CheckedChanged);
            //
            // btnOk
            //
            this.btnOk.Location = new System.Drawing.Point(200, 150);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(75, 23);
            this.btnOk.TabIndex = 2;
            this.btnOk.Text = "&OK";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            //
            // btnCancel
            //
            this.btnCancel.Location = new System.Drawing.Point(285, 150);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 3;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(14, 137);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(26, 13);
            this.label2.TabIndex = 4;
            this.label2.Text = "Use";
            //
            // label3
            //
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(89, 137);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(61, 13);
            this.label3.TabIndex = 5;
            this.label3.Text = "credentials.";
            //
            // linkCreds
            //
            this.linkCreds.AutoSize = true;
            this.linkCreds.Location = new System.Drawing.Point(39, 137);
            this.linkCreds.Name = "linkCreds";
            this.linkCreds.Size = new System.Drawing.Size(48, 13);
            this.linkCreds.TabIndex = 6;
            this.linkCreds.TabStop = true;
            this.linkCreds.Text = "alternate";
            this.linkCreds.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkCreds_LinkClicked);
            //
            // SelectDomainDialog
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(371, 184);
            this.Controls.Add(this.linkCreds);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOk);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.label1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "SelectDomainDialog";
            this.Text = "Select Active Directory Domain";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.RadioButton rbOtherDomain;
        private System.Windows.Forms.RadioButton rbDefaultDomain;
        private System.Windows.Forms.TextBox tbDomain;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.LinkLabel linkCreds;
    }
}