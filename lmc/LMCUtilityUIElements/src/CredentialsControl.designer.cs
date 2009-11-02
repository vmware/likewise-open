namespace Likewise.LMC.UtilityUIElements
{
    partial class CredentialsControl
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
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.groupBox = new System.Windows.Forms.GroupBox();
            this.tbPassword = new System.Windows.Forms.TextBox();
            this.tbUsername = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.rbUseTheseCreds = new System.Windows.Forms.RadioButton();
            this.rbUseCurrentUserCreds = new System.Windows.Forms.RadioButton();
            this.groupBox1.SuspendLayout();
            this.groupBox.SuspendLayout();
            this.SuspendLayout();
            //
            // groupBox1
            //
            this.groupBox1.Controls.Add(this.groupBox);
            this.groupBox1.Controls.Add(this.rbUseTheseCreds);
            this.groupBox1.Controls.Add(this.rbUseCurrentUserCreds);
            this.groupBox1.Location = new System.Drawing.Point(3, 4);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(326, 184);
            this.groupBox1.TabIndex = 2;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Enter credentials:";
            //
            // groupBox
            //
            this.groupBox.Controls.Add(this.tbPassword);
            this.groupBox.Controls.Add(this.tbUsername);
            this.groupBox.Controls.Add(this.label2);
            this.groupBox.Controls.Add(this.label1);
            this.groupBox.Enabled = false;
            this.groupBox.Location = new System.Drawing.Point(39, 75);
            this.groupBox.Name = "groupBox";
            this.groupBox.Size = new System.Drawing.Size(251, 82);
            this.groupBox.TabIndex = 2;
            this.groupBox.TabStop = false;
            //
            // tbPassword
            //
            this.tbPassword.Location = new System.Drawing.Point(80, 47);
            this.tbPassword.Name = "tbPassword";
            this.tbPassword.Size = new System.Drawing.Size(157, 20);
            this.tbPassword.TabIndex = 3;
            this.tbPassword.TextChanged += new System.EventHandler(this.tbPassword_TextChanged);
            //
            // tbUsername
            //
            this.tbUsername.Location = new System.Drawing.Point(80, 16);
            this.tbUsername.Name = "tbUsername";
            this.tbUsername.Size = new System.Drawing.Size(157, 20);
            this.tbUsername.TabIndex = 2;
            this.tbUsername.TextChanged += new System.EventHandler(this.tbUsername_TextChanged);
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(13, 51);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(53, 13);
            this.label2.TabIndex = 1;
            this.label2.Text = "Password";
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(13, 20);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(55, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Username";
            //
            // rbUseTheseCreds
            //
            this.rbUseTheseCreds.AutoSize = true;
            this.rbUseTheseCreds.Location = new System.Drawing.Point(19, 52);
            this.rbUseTheseCreds.Name = "rbUseTheseCreds";
            this.rbUseTheseCreds.Size = new System.Drawing.Size(147, 17);
            this.rbUseTheseCreds.TabIndex = 1;
            this.rbUseTheseCreds.Text = "Use the credentials below";
            this.rbUseTheseCreds.UseVisualStyleBackColor = true;
            this.rbUseTheseCreds.CheckedChanged += new System.EventHandler(this.rbUseTheseCreds_CheckedChanged);
            //
            // rbUseCurrentUserCreds
            //
            this.rbUseCurrentUserCreds.AutoSize = true;
            this.rbUseCurrentUserCreds.Checked = true;
            this.rbUseCurrentUserCreds.Location = new System.Drawing.Point(19, 27);
            this.rbUseCurrentUserCreds.Name = "rbUseCurrentUserCreds";
            this.rbUseCurrentUserCreds.Size = new System.Drawing.Size(191, 17);
            this.rbUseCurrentUserCreds.TabIndex = 0;
            this.rbUseCurrentUserCreds.TabStop = true;
            this.rbUseCurrentUserCreds.Text = "Use currently logged on credentials";
            this.rbUseCurrentUserCreds.UseVisualStyleBackColor = true;
            this.rbUseCurrentUserCreds.CheckedChanged += new System.EventHandler(this.rbUseCurrentUserCreds_CheckedChanged);
            //
            // CredentialsControl
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.groupBox1);
            this.Name = "CredentialsControl";
            this.Size = new System.Drawing.Size(332, 193);
            this.Load += new System.EventHandler(this.CredentialsControl_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox.ResumeLayout(false);
            this.groupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox;
        private System.Windows.Forms.TextBox tbPassword;
        private System.Windows.Forms.TextBox tbUsername;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.RadioButton rbUseTheseCreds;
        private System.Windows.Forms.RadioButton rbUseCurrentUserCreds;
    }
}
