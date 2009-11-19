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
using System.Windows.Forms;
using Likewise.LMC.Utilities;
using Likewise.LMC.ServerControl;

namespace Likewise.LMC.Plugins.LUG
{
public partial class NewUserDlg : EditDialog
{
    
    #region class data
    
    public string Password;
    public string User;
    public string FullName;
    public string Description;
    public bool CannotChange;
    public bool MustChange;
    public bool NeverExpires;
    public bool IsDisabled;
    
    //This wouldn't be needed, except that this.Parent doesn't work!
    private LUGPage localParent;
    
    #endregion
    
    #region Constructors
    public NewUserDlg(IPlugInContainer container, StandardPage parentPage)
    : base(container, parentPage)
    {
        InitializeComponent();
        
        ButtonCancel.Text = "Cancel";
        ButtonOK.Text = "Create";
        SetAllValueChangedHandlers(this);
        tbFullName.MaxLength = 256;
        
        localParent = (LUGPage) parentPage;
        
        this.tbUserName.Select();
        
    }
    #endregion
    
    #region Initialization Methods
    public void SetData(String userName)
    {
        Text = String.Format(Text, userName);
        tbUserName.Focus();
    }
    #endregion
    
    #region override methods
    
    protected override void ValueChangedHandler(object sender, EventArgs e)
    {
        base.ValueChangedHandler(sender, e);
        bDataWasChanged = true;
    }
    
    #endregion
    
    #region EventHandlers
    protected override bool Apply(EditDialogAction actionCause)
    {
        uint result = (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS;
        string errorMessage = null;
        try
        {
            if (!Hostinfo.ValidatePassword(tbConfirmPassword.Text, tbPassword.Text, out errorMessage))
            {
                if (!(tbConfirmPassword.Text.Trim().Equals(tbPassword.Text)))
                    errorMessage = "The password was not correctly confirmed. Please ensure that the password and confirmation match exactly";
                container.ShowError(errorMessage, MessageBoxButtons.OK);
                return false;
            }

            Password = tbPassword.Text;
            User = tbUserName.Text;
            FullName = tbFullName.Text;
            Description = tbDescription.Text;
            CannotChange = cbCannotChange.Checked;
            MustChange = cbMustChange.Checked;
            NeverExpires = cbNeverExpires.Checked;
            IsDisabled = cbIsDisabled.Checked;
            
            LUGPage lugPg = localParent;
            
            if (lugPg == null)
            {
                Logger.Log("NewUserDlg.Apply  localParent == null", Logger.LogLevel.Error);
                return false;
            }

            result = lugPg.AddLUG(this);

            if (result != (uint)ErrorCodes.WIN32Enum.ERROR_SUCCESS)
            {
                container.ShowError(
                "Likewise Administrative Console encountered an error when trying to add a new user.  " + ErrorCodes.WIN32String((int)result),
                MessageBoxButtons.OK);
                return false;
            }
        }
        catch (Exception e)
        {
            Logger.LogException("NewUserDlg.Apply", e);
        }
        
        return true;
    }
    
    #endregion
    
}
}


