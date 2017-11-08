#! /bin/bash +x

umask 0022

export CWD=`pwd`

export LW_FEATURE_LEVEL="auth"
export LSA_RPC_SERVERS="yes"
export LW_DEVICE_PROFILE="photon"

usage()
{
  if [ -n "$1" ]; then
    echo "$1"
  fi
  echo "usage: $0 [--force-debug] [--enable-winjoin]"
  exit 1
}

while [ `echo "$1" | grep -c '^-'` -gt 0 ]; do
  if [ \( " $1" = " --help" \) -o \( " $1" = " -h" \) ]; then
    usage ""
  elif [ " $1" = " --force-debug" ]; then
    force_debug="-g -O0"
    shift
  elif [ " $1" = " --enable-winjoin" ]; then
    enable_winjoin="--lwio-drivers=npfs pvfs srv rdr"
    shift
  else
    usage "ERROR: unknown option $1"
  fi
done


if [ \( -n "$force_debug" \) -o \( -n "$enable_winjoin" \) ]; then
  if [ -f ".build_photon_opts" ]; then
    echo "NOTICE: Overriding saved build options from '.build_photon_opts' file"
    rm -f .build_photon_opts
  fi
  echo "force_debug=\"$force_debug\""       >> .build_photon_opts
  echo "enable_winjoin=\"$enable_winjoin\"" >> .build_photon_opts
elif [ -f ".build_photon_opts" ]; then
  echo "NOTICE: Using Overriding saved build options from '.build_photon_opts' file"
  . "./.build_photon_opts"
  echo force_debug=$force_debug
  echo enable_winjoin=$enable_winjoin
fi

export CFLAGS="$force_debug -Wno-error=unused-but-set-variable -Wno-error=implicit-function-declaration -Wno-error=sizeof-pointer-memaccess -Wno-error=unused-local-typedefs -Wno-error=pointer-sign -Wno-error=address"
../configure $DEBUG \
             --prefix=/opt/likewise \
             --libdir=/opt/likewise/lib64 \
             --datadir=/opt/likewise/share \
             --datarootdir=/opt/likewise/share \
             --build-isas=x86_64 \
             --lw-bundled-libs='libedit' \
             $enable_winjoin \
             --enable-vmdir-provider=yes

make all package

