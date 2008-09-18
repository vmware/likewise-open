configfile = ${configdir}/smb.conf
AM_CPPFLAGS = -DSMB_PASSWD_FILE=\"${SMB_PASSWD_FILE}\" \
	      -DPRIVATE_DIR=\"${PRIVATE_DIR}\" \
	      -DCONFIGFILE=\"${configfile}\"  \
	      -DSBINDIR=\"${sbindir}\" \
	      -DBINDIR=\"${bindir}\" \
	      -DLMHOSTSFILE=\"${LMHOSTSFILE}\" \
	      -DSWATDIR=\"${SWATDIR}\"  \
	      -DLOCKDIR=\"${LOCKDIR}\" \
	      -DPIDDIR=\"${PIDDIR}\" \
	      -DLIBDIR=\"${libdir}\" \
	      -DLOGFILEBASE=\"${LOGFILEBASE}\" \
	      -DSHLIBEXT=\"so\" \
	      -DCONFIGDIR=\"${configdir}\"

AM_CPPFLAGS += -Iinclude -I.. -I../lib/replace -I../tdb/include -I. -Ilibrpc -Itdb/common

noinst_LTLIBRARIES = librpc.la
librpc_la_SOURCES = ndr/ndr.c ndr/ndr_basic.c ndr/ndr_misc.c ndr/ndr_sec_helper.c ndr/ndr_string.c ndr/sid.c gen_ndr/cli_wkssvc.c gen_ndr/ndr_notify.c gen_ndr/ndr_wkssvc.c
