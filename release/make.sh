#! /bin/bash

umask 0022

if [ -n "$1" ]; then
  target="$1"
  shift
else
  target="all package"
fi
export CWD=`pwd`

export LW_FEATURE_LEVEL="auth"
export LSA_RPC_SERVERS="yes"
export LW_DEVICE_PROFILE="photon"

export CFLAGS="-g -O0 -Wno-error=unused-but-set-variable -Wno-error=implicit-function-declaration -Wno-error=sizeof-pointer-memaccess -Wno-error=unused-local-typedefs -Wno-error=pointer-sign -Wno-error=address"
make $target
