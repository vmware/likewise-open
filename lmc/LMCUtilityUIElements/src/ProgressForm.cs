using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Likewise.LMC.UtilityUIElements
{
    public partial class ProgressForm : Form
    {
        public ProgressForm()
        {
            InitializeComponent();
        }

        public string Message
        {
            set
            {
                lblMessage.Text = value;
            }
        }

        public int Progress
        {
            set
            {
                int n = value;
                if (n < 0)
                    n = 0;
                else if (n > 100)
                    n = 100;
                pb.Value = n;

                // let some events be handled so as to immediately update the display
                Application.DoEvents();
            }
        }
    }
}