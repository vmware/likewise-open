namespace Likewise.LMC.Plugins.FileBrowser
{
    partial class ConnectToShareDialog
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
            this.CancelBtn = new System.Windows.Forms.Button();
            this.OKBtn = new System.Windows.Forms.Button();
            this.tbPath = new System.Windows.Forms.TextBox();
            this.cbUseAlternateCreds = new System.Windows.Forms.CheckBox();
            this.label1 = new System.Windows.Forms.Label();
            this.lblErrorMessage = new System.Windows.Forms.Label();
            this.SuspendLayout();
            //
            // CancelBtn
            //
            this.CancelBtn.Location = new System.Drawing.Point(239, 153);
            this.CancelBtn.Name = "CancelBtn";
            this.CancelBtn.Size = new System.Drawing.Size(75, 23);
            this.CancelBtn.TabIndex = 4;
            this.CancelBtn.Text = "&Cancel";
            this.CancelBtn.UseVisualStyleBackColor = true;
            this.CancelBtn.Click += new System.EventHandler(this.CancelBtn_Click);
            //
            // OKBtn
            //
            this.OKBtn.Location = new System.Drawing.Point(158, 153);
            this.OKBtn.Name = "OKBtn";
            this.OKBtn.Size = new System.Drawing.Size(75, 23);
            this.OKBtn.TabIndex = 3;
            this.OKBtn.Text = "&OK";
            this.OKBtn.UseVisualStyleBackColor = true;
            this.OKBtn.Click += new System.EventHandler(this.OKBtn_Click);
            //
            // tbPath
            //
            this.tbPath.Location = new System.Drawing.Point(28, 51);
            this.tbPath.Name = "tbPath";
            this.tbPath.Size = new System.Drawing.Size(286, 20);
            this.tbPath.TabIndex = 1;
            this.tbPath.TextChanged += new System.EventHandler(this.tbPath_TextChanged);
            //
            // cbUseAlternateCreds
            //
            this.cbUseAlternateCreds.AutoSize = true;
            this.cbUseAlternateCreds.Location = new System.Drawing.Point(28, 104);
            this.cbUseAlternateCreds.Name = "cbUseAlternateCreds";
            this.cbUseAlternateCreds.Size = new System.Drawing.Size(166, 17);
            this.cbUseAlternateCreds.TabIndex = 2;
            this.cbUseAlternateCreds.Text = "Use alternate user credentials";
            this.cbUseAlternateCreds.UseVisualStyleBackColor = true;
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(25, 27);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(159, 13);
            this.label1.TabIndex = 5;
            this.label1.Text = "Enter path to file server location:";
            //
            // lblErrorMessage
            //
            this.lblErrorMessage.AutoSize = true;
            this.lblErrorMessage.ForeColor = System.Drawing.Color.DarkRed;
            this.lblErrorMessage.Location = new System.Drawing.Point(76, 74);
            this.lblErrorMessage.Name = "lblErrorMessage";
            this.lblErrorMessage.Size = new System.Drawing.Size(178, 13);
            this.lblErrorMessage.TabIndex = 6;
            this.lblErrorMessage.Text = "Network resource path not available";
            //
            // ConnectToShareDialog
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(335, 188);
            this.Controls.Add(this.lblErrorMessage);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.tbPath);
            this.Controls.Add(this.cbUseAlternateCreds);
            this.Controls.Add(this.OKBtn);
            this.Controls.Add(this.CancelBtn);
            this.Name = "ConnectToShareDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Connect to remote file share";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button CancelBtn;
        private System.Windows.Forms.Button OKBtn;
        private System.Windows.Forms.TextBox tbPath;
        private System.Windows.Forms.CheckBox cbUseAlternateCreds;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label lblErrorMessage;
    }
}