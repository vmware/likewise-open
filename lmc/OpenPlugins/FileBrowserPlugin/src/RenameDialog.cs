using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Likewise.LMC.FileClient;
using Likewise.LMC.Plugins.FileBrowser.Properties;
using Likewise.LMC.Plugins.FileBrowser;
using Likewise.LMC.Utilities;

namespace Likewise.LMC.Plugins.FileBrowser
{
    public partial class RenameDialog : Form
    {
        private string name = null;

        public enum RENAME_OPERATION
        {
            RENAME_FILE,
            RENAME_DIRECTORY
        }

        public RenameDialog()
        {
            InitializeComponent();
        }

        public RenameDialog(
            string filename,
            RENAME_OPERATION op,
            FileBrowserIPlugIn fbPlugin)
            : this()
        {
            string operation = "Rename file";
            Icon ic = Resources.Library;

            if (op == RENAME_OPERATION.RENAME_DIRECTORY)
                operation = "Rename directory";

            this.tbNewName.Text = filename;
            this.tbNewName.SelectAll();
            this.Text = operation;
        }

        public string GetName()
        {
            return name;
        }

        private void CancelBtn_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void OKBtn_Click(object sender, EventArgs e)
        {
            bool okayToExit = true;

            // Copy the values from the edit field.
            this.name = tbNewName.Text;

            if (string.IsNullOrEmpty(name))
            {
                okayToExit = false;
            }

            if (okayToExit == true)
            {
                this.DialogResult = DialogResult.OK;
                this.Close();
            }
        }
    }
}