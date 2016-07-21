# ex: set tabstop=4 expandtab shiftwidth=4:

Name: 		@PKG_RPM_NAME@
Summary: 	Likewise Open
Version: 	@PKG_RPM_VERSION@
Release: 	@PKG_RPM_RELEASE@
License: 	GPL 2.0,LGPL 2.1
URL: 		http://www.vmware.com/
Group: 		Development/Libraries

Prereq: grep, coreutils >= 8.22, openldap >= 2.4, openssl >= 1.0.1, krb5 >= 1.12, haveged >= 1.9, sed >= 4.2, procps-ng
##For ESX use this line:
##Prereq: grep, sh-utils
Obsoletes:   likewise-open-libs, likewise-open-lsass, likewise-open-netlogon, likewise-open-lwio, likewise-open-eventlog, likewise-open-rpc, likewise-open-lwsm, likewise-open-lwreg, likewise-open-srvsvc

%if @PKG_RPM_COMPAT@
%package compat
Summary:        Likewise Open (compat libraries)
Group:          Development/Libraries
Requires:       @PKG_RPM_NAME@
%endif

%package devel
Summary:        Likewise Open (development)
Group:          Development/Libraries
Requires:       @PKG_RPM_NAME@

%description
Likewise Open 6.1 LWIS

%if @PKG_RPM_COMPAT@
%description compat
This package provides compatibility with 32-bit applications
%endif

%description devel
This package provides files for developing against the Likewise APIs

%post
case "$1" in
    1)

    /bin/systemctl enable lwsmd.service >/dev/null 2>&1
    if [ $? -ne 0 ]; then
        /bin/ln -s /lib/systemd/system/lwsmd.service /etc/systemd/system/multi-user.target.wants/lwsmd.service
    fi

    try_starting_lwregd_svc=true

    if [ "$(stat -c %d:%i /)" != "$(stat -c %d:%i /proc/1/root/.)" ]; then
        try_starting_lwregd_svc=false
    fi

    /bin/systemctl >/dev/null 2>&1
    if [ $? -ne 0 ]; then
        try_starting_lwregd_svc=false
    fi

    if [ $try_starting_lwregd_svc = true ]; then
        /bin/systemctl daemon-reload

        /bin/systemctl start lwsmd.service

        echo "Waiting for lwreg startup."
        while( test -z "`%{_prefix}/bin/lwsm status lwreg | grep standalone:`" )
        do
            echo -n "."
            sleep 1
        done
        echo "ok"
        for file in %{_prefix}/share/config/*.reg; do
            echo "Installing settings from $file..."
            %{_bindir}/lwregshell import $file
        done
        %{_bindir}/lwsm -q refresh
        sleep 2
        %{_bindir}/lwsm start @PRIMARY_SERVICE@
    else
        started_lwregd=false
        if [ -z "`pidof lwsmd`" ]; then
            %{_sbindir}/lwregd &
            sleep 5
            started_lwregd=true
        fi
        for file in %{_prefix}/share/config/*.reg; do
            echo "Installing settings from $file..."
            %{_bindir}/lwregshell import $file
        done
        if [ $started_lwregd = true ]; then
            kill -TERM `pidof lwregd`
            wait
        fi
    fi
    ;;

    2)
    ## Upgrade

    ## chkconfig behaves differently on various updates of RHEL and SUSE
    ## So, we massage the init script according to the release, for now.

    try_starting_lwregd_svc=true

    if [ "$(stat -c %d:%i /)" != "$(stat -c %d:%i /proc/1/root/.)" ]; then
        try_starting_lwregd_svc=false
    fi

    /bin/systemctl >/dev/null 2>&1
    if [ $? -ne 0 ]; then
        try_starting_lwregd_svc=false
    fi

    if [ $try_starting_lwregd_svc = true ]; then
        [ -z "`pidof lwsmd`" ] && /bin/systemctl start lwsmd.service

        echo "Waiting for lwreg startup."
        while( test -z "`%{_prefix}/bin/lwsm status lwreg | grep standalone:`" )
        do
            echo -n "."
            sleep 1
        done
        echo "ok"

        for file in %{_prefix}/share/config/*.reg; do
            echo "Upgrading settings from $file..."
            %{_bindir}/lwregshell import $file
        done
        %{_bindir}/lwsm -q refresh
        sleep 2
        %{_bindir}/lwsm stop lwreg
        %{_bindir}/lwsm start @PRIMARY_SERVICE@
    else
        started_lwregd=false
        if [ -z "`pidof lwsmd`" ]; then
            %{_sbindir}/lwregd &
            sleep 5
            started_lwregd=true
        fi
        for file in %{_prefix}/share/config/*.reg; do
            echo "Upgrading settings from $file..."
            %{_bindir}/lwregshell import $file
        done
        if [ $started_lwregd = true ]; then
            kill -TERM `pidof lwregd`
            wait
        fi
    fi
    ;;

esac

%preun
if [ "$1" = 0 ]; then
    ## Be paranoid about cleaning up
    if [ "@IS_EMBEDDED@" = "no" ]
    then
        %{_bindir}/domainjoin-cli configure --disable pam
        %{_bindir}/domainjoin-cli configure --disable nsswitch
    fi

    %{_bindir}/lwsm stop lwreg

    /bin/systemctl stop lwsmd.service

    /bin/systemctl disable lwsmd.service

    if [ -f /etc/systemd/system/lwsmd.service ]; then
       /bin/rm -f /etc/systemd/system/lwsmd.service
    fi

fi

%changelog


