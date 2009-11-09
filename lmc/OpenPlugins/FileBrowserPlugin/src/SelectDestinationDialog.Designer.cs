namespace Likewise.LMC.Plugins.FileBrowser
{
    partial class SelectDestinationDialog
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
            this.label1 = new System.Windows.Forms.Label();
            this.tvDestinationTree = new Likewise.LMC.ServerControl.LWTreeView();
            this.SuspendLayout();
            //
            // CancelBtn
            //
            this.CancelBtn.Location = new System.Drawing.Point(253, 350);
            this.CancelBtn.Name = "CancelBtn";
            this.CancelBtn.Size = new System.Drawing.Size(75, 23);
            this.CancelBtn.TabIndex = 4;
            this.CancelBtn.Text = "&Cancel";
            this.CancelBtn.UseVisualStyleBackColor = true;
            this.CancelBtn.Click += new System.EventHandler(this.CancelBtn_Click);
            //
            // OKBtn
            //
            this.OKBtn.Location = new System.Drawing.Point(172, 350);
            this.OKBtn.Name = "OKBtn";
            this.OKBtn.Size = new System.Drawing.Size(75, 23);
            this.OKBtn.TabIndex = 3;
            this.OKBtn.Text = "&OK";
            this.OKBtn.UseVisualStyleBackColor = true;
            this.OKBtn.Click += new System.EventHandler(this.OKBtn_Click);
            //
            // tbPath
            //
            this.tbPath.Location = new System.Drawing.Point(15, 39);
            this.tbPath.Name = "tbPath";
            this.tbPath.Size = new System.Drawing.Size(313, 20);
            this.tbPath.TabIndex = 1;
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 18);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(113, 13);
            this.label1.TabIndex = 5;
            this.label1.Text = "Enter destination path:";
            //
            // tvDestinationTree
            //
            this.tvDestinationTree.Location = new System.Drawing.Point(12, 82);
            this.tvDestinationTree.Name = "tvDestinationTree";
            this.tvDestinationTree.Size = new System.Drawing.Size(316, 247);
            this.tvDestinationTree.TabIndex = 6;
            this.tvDestinationTree.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.lwDestinationTree_AfterSelect);
            //
            // SelectDestinationDialog
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(340, 385);
            this.Controls.Add(this.tvDestinationTree);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.tbPath);
            this.Controls.Add(this.OKBtn);
            this.Controls.Add(this.CancelBtn);
            this.Name = "SelectDestinationDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Copy file";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button CancelBtn;
        private System.Windows.Forms.Button OKBtn;
        private System.Windows.Forms.TextBox tbPath;
        private System.Windows.Forms.Label label1;
        private Likewise.LMC.ServerControl.LWTreeView tvDestinationTree;
    }
}