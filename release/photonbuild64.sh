#! /bin/bash

export CWD=`pwd`

export LW_BUILD_PHOTON=1
export LW_FEATURE_LEVEL="auth"
export LSA_RPC_SERVERS="yes"
export LW_DEVICE_PROFILE="photon"

export CFLAGS="-Wno-error=unused-but-set-variable -Wno-error=implicit-function-declaration -Wno-error=sizeof-pointer-memaccess -Wno-error=unused-local-typedefs -Wno-error=pointer-sign -Wno-error=address"
../configure $DEBUG \
             --prefix=/opt/likewise \
             --libdir=/opt/likewise/lib64 \
             --datadir=/opt/likewise/share \
             --datarootdir=/opt/likewise/share \
             --build-isas=x86_64 \
             --lw-bundled-libs='libedit'

make all package

