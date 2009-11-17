/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Windows.Forms;
using Likewise.LMC.Properties;
using Likewise.LMC.ServerControl;
using Likewise.LMC.Utilities;

namespace Likewise.LMC
{
    /// <summary>
    /// Summary description for AboutForm.
    /// </summary>
    public class AboutForm : Form
    {
        #region Class Data
        private Button button1;

        /// <summary>
        /// Required designer variable.
        /// </summary>
        private Container components = null;

        private Label label1;
        private SplashControl splashControl1;
        private LinkLabel linkLabel1;
        private Label labelVersion;
        #endregion

        #region Constructors
        public AboutForm()
        {
            //
            // Required for Windows Form Designer support
            //
            InitializeComponent();

            Text = CommonResources.GetString("Caption_Console");

            string buildversion = string.Empty;

            if (Configurations.currentPlatform == LikewiseTargetPlatform.Windows)
                buildversion = Application.UserAppDataPath.Substring(Application.UserAppDataPath.LastIndexOf(@"\") + 1);
            else
                buildversion = Application.UserAppDataPath.Substring(Application.UserAppDataPath.LastIndexOf(@"/") + 1);

            if (!String.IsNullOrEmpty(buildversion))
            {
                string[] aParts = buildversion.Split('.');

                string version = string.Format("Version {0}.{1} (Build {2}.{3})", 
                                                aParts[0],
                                                aParts[1],
                                                aParts[2],
                                                aParts[3]);

                labelVersion.Text = version;
                splashControl1.label1.Text = string.Concat(splashControl1.label1.Text, " ", string.Format("{0}.{1}", aParts[0], aParts[1]));
            } 
        }
        #endregion

        #region Override Methods
        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (components != null)
                {
                    components.Dispose();
                }
            }
            base.Dispose(disposing);
        }
        #endregion

        #region Events
        private void button1_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void linkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            ProcessStartInfo psi = new ProcessStartInfo();
            psi.UseShellExecute = true;
            psi.FileName = "http://www.likewise.com"; ;
            psi.Verb = "open";
            psi.WindowStyle = ProcessWindowStyle.Normal;
            Process.Start(psi);
        }
        #endregion

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.button1 = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.labelVersion = new System.Windows.Forms.Label();
            this.linkLabel1 = new System.Windows.Forms.LinkLabel();
            this.splashControl1 = new Likewise.LMC.SplashControl();
            this.SuspendLayout();
            // 
            // button1
            // 
            this.button1.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.button1.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.button1.Location = new System.Drawing.Point(408, 334);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(75, 23);
            this.button1.TabIndex = 0;
            this.button1.Text = "OK";
            // 
            // label1
            // 
            this.label1.BackColor = System.Drawing.Color.Transparent;
            this.label1.ForeColor = System.Drawing.SystemColors.ControlText;
            this.label1.Location = new System.Drawing.Point(4, 332);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(144, 32);
            this.label1.TabIndex = 2;
            this.label1.Text = "For technical support, visit:";
            this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // labelVersion
            // 
            this.labelVersion.Location = new System.Drawing.Point(4, 297);
            this.labelVersion.Name = "labelVersion";
            this.labelVersion.Size = new System.Drawing.Size(488, 34);
            this.labelVersion.TabIndex = 4;
            this.labelVersion.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // linkLabel1
            // 
            this.linkLabel1.Location = new System.Drawing.Point(167, 340);
            this.linkLabel1.Name = "linkLabel1";
            this.linkLabel1.Size = new System.Drawing.Size(224, 24);
            this.linkLabel1.TabIndex = 6;
            this.linkLabel1.TabStop = true;
            this.linkLabel1.Text = "www.likewise.com";
            // 
            // splashControl1
            // 
            this.splashControl1.Location = new System.Drawing.Point(5, 4);
            this.splashControl1.Name = "splashControl1";
            this.splashControl1.Size = new System.Drawing.Size(487, 292);
            this.splashControl1.TabIndex = 5;
            // 
            // AboutForm
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.ClientSize = new System.Drawing.Size(292, 266);
            this.Name = "AboutForm";
            this.ResumeLayout(false);

        }

        #endregion
    }
}