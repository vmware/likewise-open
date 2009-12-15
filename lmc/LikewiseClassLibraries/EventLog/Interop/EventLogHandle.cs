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
using Likewise.LMC.Utilities;

namespace Likewise.LMC.Eventlog
{
public class EventlogHandle : IDisposable
{
    private IntPtr _handle;
    
    private bool _disposed = false;
    
    public EventlogHandle()
    {
        _handle = IntPtr.Zero;
    }
    
    public EventlogHandle(IntPtr handle) : base()
    {
        _handle = handle;
    }
    
    public IntPtr Handle
    {
        get
        {
            if (_disposed)
            {
                throw new ObjectDisposedException("This eventlog handle has been disposed");
            }
            else
            {
                return _handle;
            }
        }
        set
        {
            CloseHandle();
            _handle = value;
        }
    }
    
    private void CloseHandle()
    {
        EventAPI.CloseEventLog(_handle);
        _handle = IntPtr.Zero;
    }
    
    public void Dispose()
    {
        Dispose(true);
        // Take yourself off the Finalization queue
        // to prevent finalization code for this object
        // from executing a second time.
        GC.SuppressFinalize(this);
    }
    
    // Dispose(bool disposing) executes in two distinct scenarios.
    // If disposing equals true, the method has been called directly
    // or indirectly by a user's code. Managed and unmanaged resources
    // can be disposed.
    // If disposing equals false, the method has been called by the
    // runtime from inside the finalizer and you should not reference
    // other objects. Only unmanaged resources can be disposed.
    protected virtual void Dispose(bool disposing)
    {
        // Check to see if Dispose has already been called.
        if    (!_disposed)
        {
            // If disposing equals true, dispose all managed
            // and unmanaged resources.
            if (disposing)
            {
                // Dispose managed resources.
                // Components.Dispose();
            }
            // Release unmanaged resources. If disposing is false,
            // only the following code is executed.
            CloseHandle();
            
            // Note that this is not thread safe.
            // Another thread could start disposing the object
            // after the managed resources are disposed,
            // but before the disposed flag is set to true.
            // If thread safety is necessary, it must be
            // implemented by the client.
        }
        _disposed = true;
    }
    
    // Use C# destructor syntax for finalization code.
    // This destructor will run only if the Dispose method
    // does not get called.
    // It gives your base class the opportunity to finalize.
    // Do not provide destructors in types derived from this class.
    ~EventlogHandle()
    {
        // Do not re-create Dispose clean-up code here.
        // Calling Dispose(false) is optimal in terms of
        // readability and maintainability.
        Dispose(false);
    }
}
}
