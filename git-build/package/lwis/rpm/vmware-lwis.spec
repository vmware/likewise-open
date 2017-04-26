# ex: set tabstop=4 expandtab shiftwidth=4:

Name: 		@PKG_RPM_NAME@
Summary: 	VMware LWIS
Version: 	@PKG_RPM_VERSION@
Release: 	@PKG_RPM_RELEASE@
License: 	GPL 2.0,LGPL 2.1
URL: 		http://www.vmware.com/
Group: 		Development/Libraries

Requires(pre,preun): grep, sh-utils, libopenssl1_0_1
##For ESX use this line:
##Prereq: grep, sh-utils
Obsoletes:   likewise-open-libs, likewise-open-lsass, likewise-open-netlogon, likewise-open-lwio, likewise-open-eventlog, likewise-open-rpc, likewise-open-lwsm, likewise-open-lwreg, likewise-open-srvsvc

%if @PKG_RPM_COMPAT@
%package compat
Summary:        VMware LWIS (compat libraries)
Group:          Development/Libraries
Requires:       @PKG_RPM_NAME@
%endif

%package devel
Summary:        VMWare LWIS (development)
Group:          Development/Libraries
Requires:       @PKG_RPM_NAME@

%description
VMware LWIS is a VMware packaging of the  Likewise Open 6.1 LWIS

%if @PKG_RPM_COMPAT@
%description compat
This package provides compatibility with 32-bit applications
%endif

%description devel
This package provides files for developing against the Likewise APIs

%pre
#
# Save pre-existing mech file for later concatentation to installed mech file
#
if [ -f /etc/gss/mech ]; then
  cp /etc/gss/mech /tmp/gss-mech-tmp
fi

%post
#
# Merge saved off mech file with installed mech file
#
  if [ -f /tmp/gss-mech-tmp ]; then
    cat /etc/gss/mech >> /tmp/gss-mech-tmp
    grep '^[a-zA-Z0-9]' /tmp/gss-mech-tmp | sort -u > /etc/gss/mech
    rm -f /tmp/gss-mech-tmp
  fi

case "$1" in
    1)
    ## New install
    # XXX PR 1334329 want to NOT enable pam and nsswitch
    #if [ "@IS_EMBEDDED@" = "no" ]
    #then
    #    %{_bindir}/domainjoin-cli configure --enable pam
    #    %{_bindir}/domainjoin-cli configure --enable nsswitch
    #fi

    ## chkconfig behaves differently on various updates of RHEL and SUSE
    ## So, we massage the init script according to the release, for now.
    for d in lsassd lwsmd lwiod eventlogd dcerpcd netlogond lwregd; do
	daemon="/etc/init.d/$d"
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

    for d in lwsmd lwregd netlogond lwiod dcerpcd eventlogd lsassd; do
        chkconfig --add ${d}
    done

    /etc/init.d/lwsmd start
    echo -n "Waiting for lwreg startup."
    while( test -z "`/opt/likewise/bin/lwsm status lwreg | grep standalone:`" )
    do
        echo -n "."
        sleep 1
    done
    echo "ok"
    for file in %{_prefix}/share/config/*.reg; do
        echo "Installing settings from $file..."
        %{_bindir}/lwregshell import $file
    done
    %{_bindir}/lwsm refresh
    sleep 2
    %{_bindir}/lwsm start @PRIMARY_SERVICE@
    ;;

    2)
    ## Upgrade

    ## chkconfig behaves differently on various updates of RHEL and SUSE
    ## So, we massage the init script according to the release, for now.
    for d in lsassd lwsmd lwiod eventlogd dcerpcd netlogond lwregd; do
	daemon="/etc/init.d/$d"
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

    for d in lwsmd lwregd netlogond lwiod dcerpcd eventlogd lsassd; do
        chkconfig --add ${d}
    done

    [ -z "`pidof lwsmd`" ] && /etc/init.d/lwsmd start

    echo -n "Waiting for lwreg startup."
    while( test -z "`/opt/likewise/bin/lwsm status lwreg | grep standalone:`" )
    do
        echo -n "."
        sleep 1
    done
    echo "ok"

    for file in %{_prefix}/share/config/*.reg; do
        echo "Upgrading settings from $file..."
        %{_bindir}/lwregshell import $file
    done
    %{_bindir}/lwsm refresh
    sleep 2
    %{_bindir}/lwsm stop lwreg
    %{_bindir}/lwsm start @PRIMARY_SERVICE@
    ;;

esac

%preun
#
# Save off a copy of gss/mech when it contains entries other than ntlm
#
if [ -f /etc/gss/mech ]; then
  if [ `grep -c -e '^[^n][^t][^l][^m]' /etc/gss/mech` -gt 0 ]; then
    cp /etc/gss/mech /tmp/gss-mech-tmp
  fi
fi

if [ "$1" = 0 ]; then
    ## Be paranoid about cleaning up
    if [ "@IS_EMBEDDED@" = "no" ]
    then
        %{_bindir}/domainjoin-cli configure --disable pam
        %{_bindir}/domainjoin-cli configure --disable nsswitch
    fi

    %{_bindir}/lwsm stop lwreg
    /etc/init.d/lwsmd stop
fi

%postun
  #
  # Just remove the ntlm section added by Likewise.
  #
  if [ -f /tmp/gss-mech-tmp ]; then
    mkdir -p /etc/gss
    cat /tmp/gss-mech-tmp | sed '/^ntlm/d' > /etc/gss/mech
    #
    # Remove this file if it is empty; ntlm was the only mech entry.
    #
    if [ ! -s /etc/gss/mech ]; then
      rm -rf /etc/gss
    fi
    rm -f /tmp/gss-mech-tmp
  fi

%changelog


