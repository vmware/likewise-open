namespace Likewise.LMC.UtilityUIElements
{
    partial class SelectComputerDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SelectComputerDialog));
            this.label1 = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.tbComputer = new System.Windows.Forms.TextBox();
            this.rbRemoteComputer = new System.Windows.Forms.RadioButton();
            this.rbLocalComputer = new System.Windows.Forms.RadioButton();
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
            this.label1.Location = new System.Drawing.Point(3, 12);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(201, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Select the computer you want to manage";
            //
            // groupBox1
            //
            this.groupBox1.Controls.Add(this.tbComputer);
            this.groupBox1.Controls.Add(this.rbRemoteComputer);
            this.groupBox1.Controls.Add(this.rbLocalComputer);
            this.groupBox1.Location = new System.Drawing.Point(6, 36);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(348, 84);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Enter the computer\'s name:";
            //
            // tbComputer
            //
            this.tbComputer.Enabled = false;
            this.tbComputer.Location = new System.Drawing.Point(122, 52);
            this.tbComputer.Name = "tbComputer";
            this.tbComputer.Size = new System.Drawing.Size(214, 20);
            this.tbComputer.TabIndex = 2;
            this.tbComputer.TextChanged += new System.EventHandler(this.tbComputer_TextChanged);
            //
            // rbRemoteComputer
            //
            this.rbRemoteComputer.AutoSize = true;
            this.rbRemoteComputer.Location = new System.Drawing.Point(7, 52);
            this.rbRemoteComputer.Name = "rbRemoteComputer";
            this.rbRemoteComputer.Size = new System.Drawing.Size(109, 17);
            this.rbRemoteComputer.TabIndex = 1;
            this.rbRemoteComputer.Text = "Remote computer";
            this.rbRemoteComputer.UseVisualStyleBackColor = true;
            this.rbRemoteComputer.CheckedChanged += new System.EventHandler(this.rbRemoteComputer_CheckedChanged);
            //
            // rbLocalComputer
            //
            this.rbLocalComputer.AutoSize = true;
            this.rbLocalComputer.Checked = true;
            this.rbLocalComputer.Location = new System.Drawing.Point(7, 25);
            this.rbLocalComputer.Name = "rbLocalComputer";
            this.rbLocalComputer.Size = new System.Drawing.Size(98, 17);
            this.rbLocalComputer.TabIndex = 0;
            this.rbLocalComputer.TabStop = true;
            this.rbLocalComputer.Text = "Local computer";
            this.rbLocalComputer.UseVisualStyleBackColor = true;
            this.rbLocalComputer.CheckedChanged += new System.EventHandler(this.rbLocalComputer_CheckedChanged);
            //
            // btnOk
            //
            this.btnOk.Enabled = true;
            this.btnOk.Location = new System.Drawing.Point(194, 150);
            this.btnOk.Name = "btnOk";
            this.btnOk.Size = new System.Drawing.Size(75, 23);
            this.btnOk.TabIndex = 2;
            this.btnOk.Text = "&OK";
            this.btnOk.UseVisualStyleBackColor = true;
            this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
            //
            // btnCancel
            //
            this.btnCancel.Location = new System.Drawing.Point(278, 150);
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
            this.label2.Location = new System.Drawing.Point(6, 135);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(26, 13);
            this.label2.TabIndex = 4;
            this.label2.Text = "Use";
            //
            // label3
            //
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(81, 135);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(61, 13);
            this.label3.TabIndex = 5;
            this.label3.Text = "credentials.";
            //
            // linkCreds
            //
            this.linkCreds.AutoSize = true;
            this.linkCreds.Location = new System.Drawing.Point(31, 135);
            this.linkCreds.Name = "linkCreds";
            this.linkCreds.Size = new System.Drawing.Size(48, 13);
            this.linkCreds.TabIndex = 6;
            this.linkCreds.TabStop = true;
            this.linkCreds.Text = "alternate";
            this.linkCreds.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkCreds_LinkClicked);
            //
            // SelectComputerDialog
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(365, 183);
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
            this.Name = "SelectComputerDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Select computer";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.RadioButton rbRemoteComputer;
        private System.Windows.Forms.RadioButton rbLocalComputer;
        private System.Windows.Forms.TextBox tbComputer;
        private System.Windows.Forms.Button btnOk;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.LinkLabel linkCreds;
    }
}