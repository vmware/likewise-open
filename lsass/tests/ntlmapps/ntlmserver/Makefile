# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
# ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
# TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.
#
# Copyright (C) 1998-1999 Microsoft Corporation.  All rights reserved.

!IF "$(TARGETOS)" == "WINNT"

!include <win32.mak>

DEFS    = -D_UNICODE -DUNICODE -DWIN32_LEAN_AND_MEAN

.c.obj:
        $(cc) $(cdebug) $(cflags) $(cvars) $(DEFS) $<


all: sspi-server.exe

sspi-server.exe: sspi-server.obj globals.obj
        $(link) $(ldebug) $(conlflags) -out:$@ sspi-server.obj globals.obj \
                  $(conlibs) secur32.lib

clean:
        del *.obj
        del *.pdb
        del *.bak

cleaner: clean
        del sspi-server.exe

!ELSE
!MESSAGE Sorry this sample only builds for the Windows NT Platform
!ENDIF
