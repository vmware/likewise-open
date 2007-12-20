##
## Copyright (C) Centeris Corporation 2004-2007
## Copyright (C) Likewise Software 2007.
## All rights reserved.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http:##www.gnu.org/licenses/>.
##

BUILD_ROOT = ../..

SHELL= /bin/sh


subdirs= @subdirs@
top_srcdir= ../..
srcdir = .
prefix = /opt/likewise-open
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
infodir = ${prefix}/info
libdir = ${prefix}/lib
mandir = ${prefix}/man/man1
sharedir = ${prefix}/share

DESTDIR=@DESTDIR@
CC = gcc
CPPFLAGS =  -D_REENTRANT -fPIC -DSHARE_DIR=\"$(sharedir)\"
CFLAGS = $(CPPFLAGS) -O2 -Wall
LDFLAGS =
LIBS = -lresolv -lnsl
INSTALL = /usr/bin/install -c
LWI_OS_FLAGS = -D__LWI_LINUX__

PUBLIC_INCLUDE_DIR = $(BUILD_ROOT)/include
PROJECT_INCLUDE_DIR = ../include
DOMAINJOIN_DIR = $(BUILD_ROOT)/libdomainjoin
CENTUTILS_DIR = $(BUILD_ROOT)/libcentutils

DOMAINJOIN_LIB = $(DOMAINJOIN_DIR)/src/libdomainjoin.a
CENTUTILS_LIB = $(CENTUTILS_DIR)/libcentutils.a

INCLUDES += -I$(PROJECT_INCLUDE_DIR)    \
            -I$(DOMAINJOIN_DIR)/include \
	    -I$(CENTUTILS_DIR)		\
            -I$(PUBLIC_INCLUDE_DIR)

CFLAGS += $(INCLUDES) \
          $(LWI_OS_FLAGS) \
	  -pthread -I/usr/include/gtk-2.0 -I/usr/lib/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/freetype2 -I/usr/include/libpng12 -I/usr/include/libglade-2.0 -I/usr/include/libxml2

LIBS += $(DOMAINJOIN_LIB) $(CENTUTILS_LIB) -pthread -lglade-2.0 -lgtk-x11-2.0 -lxml2 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangocairo-1.0 -lfontconfig -lXext -lXrender -lXinerama -lXi -lXrandr -lXcursor -lXcomposite -lXdamage -lpango-1.0 -lcairo -lX11 -lXfixes -lgobject-2.0 -lgmodule-2.0 -ldl -lgthread-2.0 -lrt -lglib-2.0

OBJS = common.o joindialog.o joinprogress.o statusdialog.o \
       joinauth.o joinerror.o main.o

PROGRAM = domainjoin-gui

all : $(PROGRAM)

$(PROGRAM) : $(OBJS) $(DOMAINJOIN_LIB)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

$(OBJS) : %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	$(RM) $(OBJS) $(PROGRAM)

distclean:
	$(RM) Makefile

install: all
	@echo $(DESTDIR)
	$(top_srcdir)/mkinstalldirs $(DESTDIR)$(bindir)
	$(top_srcdir)/mkinstalldirs $(DESTDIR)$(libdir)
	$(top_srcdir)/mkinstalldirs $(DESTDIR)$(sharedir)
	$(INSTALL) $(PROGRAM) $(DESTDIR)$(bindir)
	$(INSTALL) domainjoin-gtk.glade $(DESTDIR)$(sharedir)
	$(INSTALL) *.png $(DESTDIR)$(sharedir)

uninstall:
	-/bin/rm -f $(DESTDIR)$(bindir)/$(PROGRAM)

