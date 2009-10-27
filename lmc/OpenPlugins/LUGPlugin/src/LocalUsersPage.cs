using System;
using System.Collections.Generic;
using System.Text;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.LUG.src
{
    public class LocalUserPage : LUGPage
    {
        #region Constructor
        public LocalUserPage()
            : base(LUGAPI.LUGType.User)
        {

        }
        #endregion
    }
}
