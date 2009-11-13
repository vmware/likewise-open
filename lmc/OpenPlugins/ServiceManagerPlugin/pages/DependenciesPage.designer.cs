namespace Likewise.LMC.Plugins.ServiceManagerPlugin
{
    partial class DependenciesPage
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
            this.lblMessage = new System.Windows.Forms.Label();
            this.lblDisplayName = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.treeViewDependencies = new Likewise.LMC.ServerControl.LWTreeView();
            this.label2 = new System.Windows.Forms.Label();
            this.treeViewDependents = new Likewise.LMC.ServerControl.LWTreeView();
            this.pnlData.SuspendLayout();
            this.SuspendLayout();
            //
            // pnlData
            //
            this.pnlData.Controls.Add(this.treeViewDependencies);
            this.pnlData.Controls.Add(this.label2);
            this.pnlData.Controls.Add(this.treeViewDependents);
            this.pnlData.Controls.Add(this.label1);
            this.pnlData.Controls.Add(this.lblDisplayName);
            this.pnlData.Controls.Add(this.lblMessage);
            this.pnlData.Size = new System.Drawing.Size(406, 371);
            //
            // lblMessage
            //
            this.lblMessage.Location = new System.Drawing.Point(8, 8);
            this.lblMessage.Name = "lblMessage";
            this.lblMessage.Size = new System.Drawing.Size(385, 44);
            this.lblMessage.TabIndex = 0;
            this.lblMessage.Text = "Some services depend on other services, system drivers and load order groups. If " +
                "a system component is stopped or is not running properly, dependent services can" +
                " be affected.";
            //
            // lblDisplayName
            //
            this.lblDisplayName.AutoSize = true;
            this.lblDisplayName.Location = new System.Drawing.Point(8, 56);
            this.lblDisplayName.Name = "lblDisplayName";
            this.lblDisplayName.Size = new System.Drawing.Size(74, 13);
            this.lblDisplayName.TabIndex = 1;
            this.lblDisplayName.Text = "Service Name";
            //
            // label1
            //
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(8, 77);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(281, 13);
            this.label1.TabIndex = 2;
            this.label1.Text = "This service depends on the following system components";
            //
            // treeViewDependencies
            //
            this.treeViewDependencies.Enabled = false;
            this.treeViewDependencies.Location = new System.Drawing.Point(11, 94);
            this.treeViewDependencies.Name = "treeViewDependencies";
            this.treeViewDependencies.Size = new System.Drawing.Size(382, 109);
            this.treeViewDependencies.TabIndex = 3;
            this.treeViewDependencies.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeViewDependencies_AfterSelect);
            this.treeViewDependencies.AfterExpand += new System.Windows.Forms.TreeViewEventHandler(this.treeViewDependencies_AfterSelect);
            //
            // label2
            //
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(8, 218);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(276, 13);
            this.label2.TabIndex = 4;
            this.label2.Text = "The following system components depend on this service";
            //
            // treeViewDependents
            //
            this.treeViewDependents.Enabled = false;
            this.treeViewDependents.Location = new System.Drawing.Point(11, 237);
            this.treeViewDependents.Name = "treeViewDependents";
            this.treeViewDependents.Size = new System.Drawing.Size(382, 121);
            this.treeViewDependents.TabIndex = 5;
            this.treeViewDependents.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeViewDependents_AfterExpand);
            //
            // DependenciesPage
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Name = "DependenciesPage";
            this.Size = new System.Drawing.Size(406, 371);
            this.pnlData.ResumeLayout(false);
            this.pnlData.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Label lblMessage;
        private System.Windows.Forms.Label lblDisplayName;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private Likewise.LMC.ServerControl.LWTreeView treeViewDependents;
        private Likewise.LMC.ServerControl.LWTreeView treeViewDependencies;
    }
}
