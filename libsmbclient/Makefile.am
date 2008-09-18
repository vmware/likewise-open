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

AM_CPPFLAGS += -Iinclude -Ilib/replace -Itdb/include -I. -Ilibrpc -Itdb/common -Insswitch

SUBDIRS = librpc

lib_LTLIBRARIES = libsmbclient.la

libsmbclient_la_LIBADD = librpc/librpc.la $(LDAP_LDFLAGS) $(KRB5_LDFLAGS)

include_HEADERS = include/libsmbclient.h

libsmbclient_la_SOURCES = libads/ads_status.c libads/dns.c libads/kerberos.c libads/krb5_errs.c registry/reg_objects.c rpc_parse/parse_buffer.c rpc_parse/parse_dfs.c rpc_parse/parse_ds.c rpc_parse/parse_echo.c rpc_parse/parse_eventlog.c rpc_parse/parse_lsa.c rpc_parse/parse_misc.c rpc_parse/parse_net.c rpc_parse/parse_ntsvcs.c rpc_parse/parse_prs.c rpc_parse/parse_reg.c rpc_parse/parse_rpc.c rpc_parse/parse_samr.c rpc_parse/parse_sec.c rpc_parse/parse_shutdown.c rpc_parse/parse_spoolss.c rpc_parse/parse_srv.c rpc_parse/parse_svcctl.c dynconfig.c intl/lang_tdb.c passdb/login_cache.c passdb/lookup_sid.c passdb/machine_sid.c passdb/passdb.c passdb/pdb_compat.c passdb/pdb_get_set.c passdb/pdb_interface.c passdb/pdb_ldap.c passdb/pdb_nds.c passdb/pdb_smbpasswd.c passdb/pdb_tdb.c passdb/secrets.c passdb/util_builtin.c passdb/util_unixsids.c passdb/util_wellknown.c libsmb/asn1.c libsmb/cliconnect.c libsmb/clidfs.c libsmb/clidgram.c libsmb/clientgen.c libsmb/clierror.c libsmb/clifile.c libsmb/clifsinfo.c libsmb/clikrb5.c libsmb/clilist.c libsmb/climessage.c libsmb/clioplock.c libsmb/cliprint.c libsmb/cliquota.c libsmb/clirap2.c libsmb/clirap.c libsmb/clireadwrite.c libsmb/clisecdesc.c libsmb/clispnego.c libsmb/clistr.c libsmb/clitrans.c libsmb/conncache.c libsmb/credentials.c libsmb/dcerpc_err.c libsmb/doserr.c libsmb/errormap.c libsmb/libsmb_cache.c libsmb/libsmbclient.c libsmb/libsmb_compat.c libsmb/namecache.c libsmb/namequery.c libsmb/nmblib.c libsmb/nterr.c libsmb/ntlm_check.c libsmb/ntlmssp.c libsmb/ntlmssp_parse.c libsmb/ntlmssp_sign.c libsmb/pwd_cache.c libsmb/smbdes.c libsmb/smbencrypt.c libsmb/smberr.c libsmb/smb_signing.c libsmb/unexpected.c param/loadparm.c param/params.c rpc_client/cli_dfs.c rpc_client/cli_ds.c rpc_client/cli_echo.c rpc_client/cli_lsarpc.c rpc_client/cli_netlogon.c rpc_client/cli_pipe.c rpc_client/cli_reg.c rpc_client/cli_samr.c rpc_client/cli_shutdown.c rpc_client/cli_spoolss.c rpc_client/cli_spoolss_notify.c rpc_client/cli_srvsvc.c rpc_client/cli_svcctl.c rpc_client/ndr.c lib/replace/replace.c lib/replace/snprintf.c lib/replace/timegm.c lib/talloc/talloc.c lib/access.c lib/account_pol.c lib/adt_tree.c lib/arc4.c lib/audit.c lib/bitmap.c lib/charcnv.c lib/clobber.c lib/crc32.c lib/data_blob.c lib/debug.c lib/dmallocmsg.c lib/dprintf.c lib/dummyroot.c lib/dummysmbd.c lib/events.c lib/fault.c lib/fsusage.c lib/gencache.c lib/genrand.c lib/hmacmd5.c lib/iconv.c lib/interface.c lib/interfaces.c lib/ldap_escape.c lib/md4.c lib/md5.c lib/messages.c lib/module.c lib/ms_fnmatch.c lib/pam_errors.c lib/pidfile.c lib/privileges.c lib/secace.c lib/secacl.c lib/secdesc.c lib/select.c lib/sendfile.c lib/signal.c lib/smbldap.c lib/smbldap_util.c lib/smbrun.c lib/sock_exec.c lib/substitute.c lib/system.c lib/system_smbd.c lib/tallocmsg.c lib/time.c lib/ufc.c lib/username.c lib/util_file.c lib/util_nscd.c lib/util_pw.c lib/util_seaccess.c lib/util_sec.c lib/util_sid.c lib/util_sock.c lib/util_str.c lib/util_tdb.c lib/util_unistr.c lib/util_uuid.c lib/version.c lib/wins_srv.c lib/xfile.c lib/sharesec.c lib/util.c tdb/common/dump.c tdb/common/freelist.c tdb/common/freelistcheck.c tdb/common/io.c tdb/common/lock.c tdb/common/tdbback.c tdb/common/tdb.c tdb/common/transaction.c tdb/common/traverse.c tdb/common/error.c tdb/common/open.c groupdb/mapping.c groupdb/mapping_tdb.c nsswitch/wb_common.c nsswitch/wb_client.c
