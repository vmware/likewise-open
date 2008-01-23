%{!?i386compat: %define i386compat 0}

Name:		likewise-open
Version:	__RPM_VER
Release:	__RPM_RELEASE
Summary: 	Likewise Open Active Directory Integration Services
License: 	GPLv3/LGPLv3
Group: 		System Environment/Daemons
URL: 		http://www.likewisesoftware.com/

BuildRequires:  krb5-devel, openldap-devel, e2fsprogs-devel, pam-devel
BuildRequires:  libglade2-devel gtk2-devel
Requires:       krb5-libs, openldap, pam, e2fsprogs

BuildRoot: %{_tmppath}/%{name}-%{version}-root

Source0: %{name}-%{version}.tar.gz
Source999: setup.tar.gz

%package domainjoin
Summary:	Likewise Open Active Directory Integration Services (command line utilities)
Requires:	likewise-open, perl
Group:		Applications/System

%package devel
Summary:        Likewise Open Active Directory Integration Services (Winbind Client library)
Group:          System Environment/Daemons

%package domainjoin-gui
Summary:	Likewise Open Active Directory Integration Services (graphical utilities)
Requires:	likewise-open-domainjoin, libglade2, gtk2
Group:		Applications/System

%description
Likewise Open provides tools and services necessary to integrate
Linux hosts into an Microsoft Active Directory domain.  Installing
Likewise Open will allow you to join your host to an AD domain and
authenticate and authorize Linux services using the AD list of
users and groups.

%description domainjoin
The Likewise Open domain utilities are necessary to join a host to an Active
Directory domain.  This package includes the command line utilities.

%description domainjoin-gui
The Likewise Open domain utilities are necessary to join a host to an Active
Directory domain.  This package includes the graphical utilities for desktop
use.


%description devel
The Likewise Open development files allow you to write programs that
communicate directly with the likewise-winbindd daemon and perform
operations such as authenticating users, translating Windows SIDs to
users and groups in the domain, and listing trusted domains.

%prep
%setup -q -n %{name}-%{version}
%setup -q -T -D -a 999 -n %{name}-%{version}

%build
RPM_OPT_FLAGS="-O2 -g -fmessage-length=0 -D_FORTIFY_SOURCE=2"
%ifarch i386 sparc
    RPM_OPT_FLAGS="$RPM_OPT_FLAGS -D_FILE_OFFSET_BITS=64 -march=i486"
%endif
CFLAGS="$CFLAGS $RPM_OPT_FLAGS -D_GNU_SOURCE"
export CFLAGS

## always run autogen.sh
./autogen.sh

./Configure.sh \
    --prefix=/usr \
    --localstatedir=/var \
    --with-configdir=%{_sysconfdir}/samba \
    --with-libdir=%{_libdir}/%{name} \
    --with-lockdir=/var/lib/%{name} \
    --with-logfilebase=/var/log/%{name} \
    --with-mandir=%{_mandir} \
    --with-piddir=/var/run \
    --with-privatedir=%{_sysconfdir}/samba \
    --enable-require-wrfile-keytab \
    --with-ads \
    --with-fhs \
    --without-readline \
    --with-included-popt \
    --with-included-iniparser \
    --with-pam \
    --with-shared-modules=idmap_lwopen

make SCRIPTDIR="%{_libdir}/%{name}"

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT/%{_libdir}/%{name}/{idmap,nss_info}
mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/logrotate.d
mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/{init.d,samba,security}
mkdir -p $RPM_BUILD_ROOT/var/{lib,log}/%{name}
mkdir -p $RPM_BUILD_ROOT/%{_datadir}/doc/%{name}
mkdir -p $RPM_BUILD_ROOT/%{_lib}/security

## install the binaries
make DESTDIR=${RPM_BUILD_ROOT} PREFIX=/usr install

## Now manually install the PAM/NSS library
pushd winbindd/source
%{__install} -m0755 nsswitch/libnss_lwidentity.so $RPM_BUILD_ROOT/%{_lib}/libnss_lwidentity.so.2
%{__install} -m0755 bin/pam_lwidentity.so $RPM_BUILD_ROOT/%{_lib}/security/pam_lwidentity.so
%{__install} -m0755 bin/libwbclient.so $RPM_BUILD_ROOT/%{_libdir}/libwbclient.so.0.1
ln -s libwbclient.so.0.1 $RPM_BUILD_ROOT/%{_libdir}/libwbclient.so

## cleanup
/bin/rm -f ${RPM_BUILD_ROOT}/%{_libdir}/%{name}/security/pam_lwidentity.so
/bin/rm -f ${RPM_BUILD_ROOT}/%{_libdir}/%{name}/libwbclient.so
popd

## Grab the support scripts
for file in CenterisPam.pm Centeris.pm; do
	%{__install} -m0644 domainjoin/scripts/$file ${RPM_BUILD_ROOT}/%{_libdir}/%{name}; \
done
for file in ConfigureKrb5 ConfigureLogin ConfigurePamForADLogin \
	ConfigureShellPrompt ConfigureSshForGssapi gpcron; \
do
	%{__install} -m0755 domainjoin/scripts/$file ${RPM_BUILD_ROOT}/%{_libdir}/%{name}; \
done

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
%{_bindir}/lwinet
%{_bindir}/lwiinfo
%{_bindir}/lwimsg
%{_sbindir}/likewise-winbindd
%{_libdir}/%{name}/idmap/lwopen.so
%{_libdir}/%{name}/nss_info/lwopen.so
/%{_lib}/security/pam_lwidentity.so
/%{_lib}/libnss_lwidentity.so*
%{_libdir}/libwbclient.so.*

%files devel
%{_libdir}/libwbclient.so
%{_includedir}/wbclient.h

%files domainjoin
## domain join utilities
%{_libdir}/%{name}/*
%{_bindir}/domainjoin-cli

%files domainjoin-gui
%defattr(-,root,root)
%{_bindir}/domainjoin-gui
%{_prefix}/share/domainjoin-gtk.glade

%changelog
* Wed Jan 23 2008 Gerald Carter <gcarter@likewisesoftware.com>
- Updated for 4.0.4 release

* Fri Jan 14 2008 Gerald Carter <gcarter@likewisesoftware.com>
- Package new release which repleace the mono GUI join utility with
  a GTK based variant.

* Fri Dec  7 2007 Günther Deschner <gdeschner@redhat.com>
- initial package
