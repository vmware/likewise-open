namespace Likewise.LMC.ServerControl
{
    partial class CredentialsDialog
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
            this.rbUseCurrentUserCreds = new System.Windows.Forms.RadioButton();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.tbPassword = new System.Windows.Forms.TextBox();
            this.tbUsername = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.rbUseTheseCreds = new System.Windows.Forms.RadioButton();
            this.CancelBtn = new System.Windows.Forms.Button();
            this.OKBtn = new System.Windows.Forms.Button();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            //
            // rbUseCurrentUserCreds
            //
            this.rbUseCurrentUserCreds.AutoSize = true;
            this.rbUseCurrentUserCreds.Location = new System.Drawing.Point(19, 19);
            this.rbUseCurrentUserCreds.Name = "rbUseCurrentUserCreds";
            this.rbUseCurrentUserCreds.Size = new System.Drawing.Size(191, 17);
            this.rbUseCurrentUserCreds.TabIndex = 0;
            this.rbUseCurrentUserCreds.TabStop = true;
            this.rbUseCurrentUserCreds.Text = "Use currently logged on credentials";
            this.rbUseCurrentUserCreds.UseVisualStyleBackColor = true;
            this.rbUseCurrentUserCreds.CheckedChanged += new System.EventHandler(this.rbUseCurrentUserCreds_CheckedChanged);
            //
            // groupBox1
            //
            this.groupBox1.Controls.Add(this.groupBox2);
            this.groupBox1.Controls.Add(this.rbUseTheseCreds);
            this.groupBox1.Controls.Add(this.rbUseCurrentUserCreds);
            this.groupBox1.Location = new System.Drawing.Point(12, 23);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(307, 175);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Enter credentials:";
            //
            // groupBox2
            //
            this.groupBox2.Controls.Add(this.tbPassword);
            this.groupBox2.Controls.Add(this.tbUsername);
            this.groupBox2.Controls.Add(this.label2);
            this.groupBox2.Controls.Add(this.label1);
            this.groupBox2.Location = new System.Drawing.Point(39, 79);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(251, 82);
            this.groupBox2.TabIndex = 2;
            this.groupBox2.TabStop = false;
            //
            // tbPassword
            //
            this.tbPassword.Location = new System.Drawing.Point(74, 47);
            this.tbPassword.Name = "tbPassword";
            this.tbPassword.Size = new System.Drawing.Size(157, 20);
            this.tbPassword.TabIndex = 3;
            //
            // tbUsername
            //
            this.tbUsername.Location = new System.Drawing.Point(74, 16);
            this.tbUsername.Name = "tbUsername";
            this.tbUsername.Size = new System.Drawing.Size(157, 20);
            this.tbUsername.TabIndex = 2;
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(7, 51);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(53, 13);
            this.label2.TabIndex = 1;
            this.label2.Text = "Password";
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(7, 20);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(55, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Username";
            //
            // rbUseTheseCreds
            //
            this.rbUseTheseCreds.AutoSize = true;
            this.rbUseTheseCreds.Location = new System.Drawing.Point(19, 56);
            this.rbUseTheseCreds.Name = "rbUseTheseCreds";
            this.rbUseTheseCreds.Size = new System.Drawing.Size(127, 17);
            this.rbUseTheseCreds.TabIndex = 1;
            this.rbUseTheseCreds.TabStop = true;
            this.rbUseTheseCreds.Text = "Use these credentials";
            this.rbUseTheseCreds.UseVisualStyleBackColor = true;
            this.rbUseTheseCreds.CheckedChanged += new System.EventHandler(this.rbUseTheseCreds_CheckedChanged);
            //
            // CancelBtn
            //
            this.CancelBtn.Location = new System.Drawing.Point(190, 204);
            this.CancelBtn.Name = "CancelBtn";
            this.CancelBtn.Size = new System.Drawing.Size(75, 23);
            this.CancelBtn.TabIndex = 2;
            this.CancelBtn.Text = "&Cancel";
            this.CancelBtn.UseVisualStyleBackColor = true;
            this.CancelBtn.Click += new System.EventHandler(this.CancelBtn_Click);
            //
            // OKBtn
            //
            this.OKBtn.Location = new System.Drawing.Point(51, 204);
            this.OKBtn.Name = "OKBtn";
            this.OKBtn.Size = new System.Drawing.Size(75, 23);
            this.OKBtn.TabIndex = 3;
            this.OKBtn.Text = "&OK";
            this.OKBtn.UseVisualStyleBackColor = true;
            this.OKBtn.Click += new System.EventHandler(this.OKBtn_Click);
            //
            // CredentialsDialog
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(331, 239);
            this.Controls.Add(this.OKBtn);
            this.Controls.Add(this.CancelBtn);
            this.Controls.Add(this.groupBox1);
            this.Name = "CredentialsDialog";
            this.Text = "CredentialsDialog";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.RadioButton rbUseCurrentUserCreds;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.RadioButton rbUseTheseCreds;
        private System.Windows.Forms.TextBox tbPassword;
        private System.Windows.Forms.TextBox tbUsername;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button CancelBtn;
        private System.Windows.Forms.Button OKBtn;
    }
}