using System;
using System.Collections.Generic;
using System.Text;
using Likewise.LMC.NETAPI;

namespace Likewise.LMC.Plugins.LUG.src
{
    public class DummyLUGPage : LUGPage
    {
        #region Constructor
        public DummyLUGPage()
            : base(LUGAPI.LUGType.Dummy)
        {
        }
        #endregion
    }
}
