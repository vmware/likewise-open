using System;
using System.Collections.Generic;
using System.Text;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.LUG.src
{
    public class LocalGroupPage : LUGPage
    {
        #region Constructor
        public LocalGroupPage()
            : base(LUGAPI.LUGType.Group)
        {

        }
        #endregion
    }
}
