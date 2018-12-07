#! /bin/bash

export CWD=`pwd`

export CFLAGS="-Wno-error=unused-but-set-variable -Wno-error=implicit-function-declaration -Wno-error=sizeof-pointer-memaccess -Wno-error=unused-local-typedefs -Wno-error=pointer-sign -Wno-error=address"

../configure \
        $DEBUG \
        --prefix=/opt/likewise \
        --libdir=/opt/likewise/lib \
        --datadir=/opt/likewise/share \
        --datarootdir=/opt/likewise/share \
        --build-isas=x86_64 \
        --lw-feature-level=auth \
        --lw-device-profile=photon \
        --lw-bundled-libs='' \
        --lsa-rpc-servers=yes \
        --package-rpm=yes \
        --package-deb=no \
        --enable-vmdir-provider=yes

make all package
