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

namespace Likewise.LMC.Utilities
{
    public class AuthException : Exception
    {
        #region Constructors
        public AuthException(string msg, Exception e)
            : base("Authentication error: " + msg, e)
        {
        }

        public AuthException(Exception e) : base("Authentication error", e)
        {
        }
        #endregion
    }

    public class AuthSessionException : Exception
    {
        #region Constructors
        public AuthSessionException(string msg, Exception e)
            : base("Exception while creating network session: " + msg, e)
        {
        }

        public AuthSessionException(Exception e) : base("Exception while creating network session", e)
        {
        }
        #endregion
    }

    public class AuthInadequateInfoException : Exception
    {
        #region Constructors
        public AuthInadequateInfoException(string msg, Exception e)
            : base("Exception: Cannot connect to Windows machine in workgroup mode using its IP address: " + msg, e)
        {
        }

        public AuthInadequateInfoException(Exception e) : base("Exception: Cannot connect to Windows machine in workgroup mode using its IP address", e)
        {
        }
        #endregion
    }
}
