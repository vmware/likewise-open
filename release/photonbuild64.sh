#! /bin/bash


DISTRO=`cat /etc/os-release | grep VERSION_ID | cut -d= -f2`

if [ $DISTRO == "1.0" ]; then
    DIST=""
else
    DIST=".lwph2"
    # hack against glibc-2.26 to avoid getopt declaration mismatch
    sed -i '/stdio.h/a#define _GETOPT_CORE_H 1' ../dcerpc/demos/echo_server/echo_server.c
fi

umask 0022

export CWD=`pwd`

export LW_FEATURE_LEVEL="auth"
export LSA_RPC_SERVERS="yes"
export LW_DEVICE_PROFILE="photon"
export DIST=$DIST

export CFLAGS="-Wno-error=unused-but-set-variable -Wno-error=implicit-function-declaration -Wno-error=sizeof-pointer-memaccess -Wno-error=unused-local-typedefs -Wno-error=pointer-sign -Wno-error=address"
../configure $DEBUG \
             --prefix=/opt/likewise \
             --libdir=/opt/likewise/lib64 \
             --datadir=/opt/likewise/share \
             --datarootdir=/opt/likewise/share \
             --build-isas=x86_64 \
             --lw-bundled-libs='libedit' \
             --enable-vmdir-provider=yes

make all package

