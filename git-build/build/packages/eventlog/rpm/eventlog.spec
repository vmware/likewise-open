# ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:

%{!?Compat32: %define Compat32 0}

Name: 		__PKG_NAME
Summary:	Likewise Audit (Eventlog)
Version: 	__PKG_VERSION
Release: 	1
License: 	LGPLv2.1+/GPLv2+
Group: 		Development/Libraries
URL: 		http://www.likewise.com/
BuildRoot: 	/var/tmp/%{name}-%{version}

Requires: likewise-open-rpc, likewise-open-libs



AutoReq: no


%define INIT_DIR /etc/init.d

%description
Likewise Audit (Eventlog) system

%prep

%build

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rsync -a __PKG_POPULATE_DIR/ ${RPM_BUILD_ROOT}/


%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{INIT_DIR}/*
%config /etc/likewise/eventlogd.reg
%{PrefixDir}/sbin/*
%{PrefixDir}/bin/*

%{PrefixDir}/%{_lib}/*

%define initScriptPathList %{INIT_DIR}/eventlogd
%post
## chkconfig behaves differently on various updates of RHEL and SUSE
## So, we massage the init script according to the release, for now.
for daemon in %{initScriptPathList}; do
    if [ -x $daemon ]; then
        if grep "LWI_STARTUP_TYPE_" $daemon >/dev/null 2>&1; then
            daemon_new=${daemon}.new

            if [ -f /etc/redhat-release ]; then
                /bin/sed \
                    -e 's/^#LWI_STARTUP_TYPE_REDHAT\(.*\)$/\1/' \
                    -e'/^#LWI_STARTUP_TYPE_SUSE.*$/ d' \
                    $daemon > $daemon_new
            else
                /bin/sed \
                    -e 's/^#LWI_STARTUP_TYPE_SUSE\(.*\)$/\1/' \
                    -e '/^#LWI_STARTUP_TYPE_REDHAT.*$/ d' \
                    $daemon > $daemon_new
            fi
            mv $daemon_new $daemon
            chmod 0755 $daemon
        fi
    fi
done


%changelog
