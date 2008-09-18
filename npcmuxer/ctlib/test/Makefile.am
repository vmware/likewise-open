AM_CPPFLAGS=-I../include -D_GNU_SOURCE
AM_CFLAGS=-Wall -Werror -g -fno-strict-aliasing

check_PROGRAMS = testserver testclient
testserver_SOURCES = testserver.c
testserver_LDADD = ../lib/libct.la
testclient_SOURCES = testclient.c
testclient_LDADD = ../lib/libct.la
