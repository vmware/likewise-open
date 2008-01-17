%{!?i386compat: %define i386compat 0}
%define LWPrefix /opt/%{name}

Name: 		likewise-open
Version: 	__RPM_VER
Release: 	__RPM_RELEASE
Summary: 	Likewise Open Active Directory Integration Services
License: 	GPLv3
Group: 		System Environment/Daemons
URL: 		http://www.likewisesoftware.com/

BuildRequires:  krb5-devel, openldap-devel, e2fsprogs-devel, pam-devel, libglade2-devel, gtk2-devel
Requires:       krb5-libs, openldap, pam, perl, e2fsprogs, libglade2, gtk2

BuildRoot: %{_tmppath}/%{name}-%{version}-root

Source0: %{name}-%{version}.tar.gz
Source999: setup.tar.gz

%description
Likewise Open provides tools and services necessary to integrate
Linux hosts into an Microsoft Active Directory domain.  Installing
Likewise Open will allow you to join your host to an AD domain and
authenticate and authroize Linux services using the AD list of
users and groups.

%prep
%setup -q -n %{name}-%{version}
%setup -q -T -D -a 999 -n %{name}-%{version}

%build
RPM_OPT_FLAGS="-O2 -g -fmessage-length=0 -D_FORTIFY_SOURCE=2"
%ifarch i386 sparc
    RPM_OPT_FLAGS="$RPM_OPT_FLAGS -D_FILE_OFFSET_BITS=64 -march=i486"
%endif
CLAGS="$CFLAGS $RPM_OPT_FLAGS -D_GNU_SOURCE"
export CFLAGS

## always run autogen.sh
./autogen.sh

./Configure.sh \
    --prefix=%{LWPrefix} \
    --localstatedir=/var \
    --with-configdir=%{_sysconfdir}/samba \
    --with-libdir=%{LWPrefix}/%{_lib} \
    --with-lockdir=/var/lib/%{name} \
    --with-logfilebase=/var/log/%{name} \
    --with-mandir=%{LWPrefix}/man \
    --with-piddir=/var/run \
    --with-privatedir=%{_sysconfdir}/samba \
    --enable-require-wrfile-keytab \
    --with-ads \
    --with-fhs \
    --without-readline \
    --with-included-popt --with-included-iniparser \
    --with-pam \
    --with-shared-modules=idmap_lwopen

make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT/%{_libdir}
mkdir -p $RPM_BUILD_ROOT/%{LWPrefix}/{bin,sbin,lib,include}
mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/logrotate.d
mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/{init.d,samba,security}
mkdir -p $RPM_BUILD_ROOT/var/{lib,log}/%{name}
mkdir -p $RPM_BUILD_ROOT/%{LWPrefix}/%{_lib}/{idmap,nss_info}
mkdir -p $RPM_BUILD_ROOT/%{_datadir}/doc/%{name}
mkdir -p $RPM_BUILD_ROOT/%{_lib}/security

## install the binaries
make DESTDIR=${RPM_BUILD_ROOT} PREFIX=%{LWPrefix} install

## Now manually install the PAM/NSS library
pushd winbindd/source
%{__install} -m0755 nsswitch/libnss_lwidentity.so $RPM_BUILD_ROOT/%{_lib}/libnss_lwidentity.so.2
%{__install} -m0755 bin/pam_lwidentity.so $RPM_BUILD_ROOT/%{_lib}/security/pam_lwidentity.so
%{__install} -m0755 bin/libwbclient.so $RPM_BUILD_ROOT/%{_libdir}/libwbclient.so.0.1
ln -s libwbclient.so.0.1 $RPM_BUILD_ROOT/%{_libdir}/libwbclient.so

## cleanup
/bin/rm -f ${RPM_BUILD_ROOT}/%{LWPrefix}/lib/security/pam_lwidentity.so
/bin/rm -f ${RPM_BUILD_ROOT}/%{LWPrefix}/lib/libwbclient.so
popd

# Grab the support scripts
for file in gui/DomainJoinLib/scripts/*pm; do \
	%{__install} -m0644 $file ${RPM_BUILD_ROOT}/%{LWPrefix}/bin/; \
done
for file in gui/DomainJoinLib/scripts/*pl; do \
	%{__install} -m0755 $file ${RPM_BUILD_ROOT}/%{LWPrefix}/bin/; \
done
for file in gui/DomainJoinLib/scripts/*sh; do \
	%{__install} -m0755 $file ${RPM_BUILD_ROOT}/%{LWPrefix}/bin/; \
done
# bkoropoff - gtk switchover
#%{__install} -m0755 gui/DomainJoinLib/scripts/domainjoin-gui ${RPM_BUILD_ROOT}/%{LWPrefix}/bin/

## install the vendor files ( init script, etc... )
%{__install} -m0755 setup/likewise-open.init $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/likewise-open
%{__install} -m0755 setup/likewise-open.logrotate $RPM_BUILD_ROOT/%{_sysconfdir}/logrotate.d/likewise-open
%{__install} -m0644 setup/lwiauthd.conf.default $RPM_BUILD_ROOT/%{_sysconfdir}/samba/lwiauthd.conf
%{__install} -m0644 setup/pam_lwidentity.conf.default $RPM_BUILD_ROOT/%{_sysconfdir}/security/pam_lwidentity.conf

## Docs
%{__install} -m0644 setup/README.Likewise $RPM_BUILD_ROOT/%{_datadir}/doc/%{name}/

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%pre

%post
ldconfig

%preun

%postun

%files
%defattr(-,root,root)

## Bits for likewise-winbindd
%dir %attr(755,root,root) /var/lib/%{name}
%dir %attr(755,root,root) /var/log/%{name}
%config %{_sysconfdir}/logrotate.d/likewise-open
%config(noreplace) %{_sysconfdir}/samba/lwiauthd.conf
%config(noreplace) %{_sysconfdir}/security/pam_lwidentity.conf
%{_sysconfdir}/init.d/likewise-open
%docdir %{_datadir}/doc/%{name}
%{_datadir}/doc/%{name}/*
%{LWPrefix}/bin/lwinet
%{LWPrefix}/bin/lwiinfo
%{LWPrefix}/bin/lwimsg
%{LWPrefix}/sbin/likewise-winbindd
%{LWPrefix}/%{_lib}/idmap/lwopen.so
%{LWPrefix}/%{_lib}/nss_info/lwopen.so
/%{_lib}/security/pam_lwidentity.so
/%{_lib}/libnss_lwidentity.so*
%{_libdir}/libwbclient.so*
%{LWPrefix}/include/wbclient.h
%{LWPrefix}/share/domainjoin-gtk.glade

## domain join utilities
%{LWPrefix}/bin/*pm
%{LWPrefix}/bin/*pl
%{LWPrefix}/bin/*sh
# bkoropoff - gtk switchover
#%{LWPrefix}/bin/DomainJoin.exe
#%{LWPrefix}/bin/DomainJoinLib.dll
%{LWPrefix}/bin/domainjoin-cli
%{LWPrefix}/bin/domainjoin-gui

%changelog
