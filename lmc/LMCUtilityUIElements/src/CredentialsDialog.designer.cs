namespace Likewise.LMC.UtilityUIElements
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(CredentialsDialog));
            this.CancelBtn = new System.Windows.Forms.Button();
            this.OKBtn = new System.Windows.Forms.Button();
            this.credentialsControl = new Likewise.LMC.UtilityUIElements.CredentialsControl();
            this.SuspendLayout();
            //
            // CancelBtn
            //
            this.CancelBtn.Location = new System.Drawing.Point(253, 202);
            this.CancelBtn.Name = "CancelBtn";
            this.CancelBtn.Size = new System.Drawing.Size(75, 23);
            this.CancelBtn.TabIndex = 2;
            this.CancelBtn.Text = "&Cancel";
            this.CancelBtn.UseVisualStyleBackColor = true;
            this.CancelBtn.Click += new System.EventHandler(this.CancelBtn_Click);
            //
            // OKBtn
            //
            this.OKBtn.Location = new System.Drawing.Point(165, 202);
            this.OKBtn.Name = "OKBtn";
            this.OKBtn.Size = new System.Drawing.Size(75, 23);
            this.OKBtn.TabIndex = 3;
            this.OKBtn.Text = "&OK";
            this.OKBtn.UseVisualStyleBackColor = true;
            this.OKBtn.Click += new System.EventHandler(this.OKBtn_Click);
            //
            // credentialsControl
            //
            this.credentialsControl.Location = new System.Drawing.Point(3, 3);
            this.credentialsControl.Name = "credentialsControl";
            this.credentialsControl.Password = "";
            this.credentialsControl.Size = new System.Drawing.Size(332, 193);
            this.credentialsControl.TabIndex = 4;
            this.credentialsControl.UseDefaultCreds = true;
            this.credentialsControl.Username = "";
            //
            // CredentialsDialog
            //
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.ClientSize = new System.Drawing.Size(339, 234);
            this.Controls.Add(this.credentialsControl);
            this.Controls.Add(this.OKBtn);
            this.Controls.Add(this.CancelBtn);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "CredentialsDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Credentials Dialog";
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button CancelBtn;
        private CredentialsControl credentialsControl;
        public System.Windows.Forms.Button OKBtn;
    }
}