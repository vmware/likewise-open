#!/bin/bash

function _get_lib_dir
{
    local dir=

    if [ -n "${BUILD_LIBDIR}" ]; then
	dir="${BUILD_LIBDIR}"
    else
	case `uname -s` in
	    Linux)
		case `uname -m` in
		    i*86)
			dir=lib
			;;
		    x86_64)
			dir=lib64
			;;
		    *)
			dir=lib
			;;
		esac
		;;
	    *)
		dir=lib
		;;
	esac
    fi

    echo "$dir"
}

function _get_base_cppflags
{
    local flags="${BUILD_CPPFLAGS}"

    echo "$flags"
}

function _get_base_cflags
{
    local flags="${BUILD_CFLAGS}"

    flags="$flags -O2 -g -fmessage-length=0 -D_GNU_SOURCE -D_FORTIFY_SOURCE=2"
    flags="${flags}"

    echo "$flags"
}

function _get_base_ldflags
{
    local flags="${BUILD_LDFLAGS}"

    flags="${flags} -Wl,-rpath-link -Wl,${STAGE_INSTALL_DIR}/${PREFIXDIR}/${_lib} -Wl,-rpath -Wl,${PREFIXDIR}/${_lib}"

    echo "$flags"
}

function _get_base_ldshflags
{
    local flags=

    echo "$flags"
}

function _get_base_mflags
{
    local flags=

    flags="-j$((`cat /proc/cpuinfo | grep '^processor' | wc -l` * 2))"

    echo "$flags"
}

function set_compiler_env
{
    GCC="gcc"

    CC="${GCC} -pipe"
    MAKE=make

    # Collapse spaces to stop krb5 configure
    # script from complaining about CC changing
    CC=`echo $CC | sed 's/  */ /g'`

    export CC MAKE

    _lib=`_get_lib_dir`
    _cppflags=`_get_base_cppflags`
    _cflags=`_get_base_cflags`
    _mflags=`_get_base_mflags`
    _ldshflags=`_get_base_ldshflags`
    _ldflags=`_get_base_ldflags`

    _cppflags="${_cppflags} -I${STAGE_INSTALL_DIR}/${PREFIXDIR}/include"
    _cflags="${_cflags} -I${STAGE_INSTALL_DIR}/${PREFIXDIR}/include"
    _ldflags="${_ldflags} -L${STAGE_INSTALL_DIR}/${PREFIXDIR}/${_lib}"

    return 0
}

_LBREWRITE=${BUILD_ROOT}/build/lib/libtool-dependency-rewrite.sh

function libtool_rewrite_staging
{
    for lafile in `find ${STAGE_INSTALL_DIR} -name "*.la"`; do
	${_LBREWRITE} -staging ${lafile} ${PREFIXDIR} ${STAGE_INSTALL_DIR}
    done
}
