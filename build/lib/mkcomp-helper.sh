#!/bin/bash

function _get_ncpu
{
    case `uname -s` in
	Linux)
	    cat /proc/cpuinfo | grep '^processor' | wc -l
	    ;;
	FreeBSD)
	    sysctl -n hw.ncpu
	    ;;
	*)
	    echo 1
	    ;;
    esac
}

function _get_mtime
{
    case `uname -s` in
	FreeBSD)
	    stat -f "%Uc" $1
	    ;;
	*)
	    stat -c %Y $1
	    ;;
    esac
}

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

    if [ -n "${ENABLE_DEBUG}" ]; then
	flags="${flags} -ggdb"
    else
	flags="${flags} -O2 -g"
    fi

    flags="${flags} -fmessage-length=0 -D_GNU_SOURCE -D_FORTIFY_SOURCE=2"

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

    flags="-j$((`_get_ncpu` * 2))"

    echo "$flags"
}

function set_compiler_env
{
    GCC="gcc"

    CC="${GCC} -pipe"

    case `uname -s` in
	FreeBSD)
	    MAKE=gmake
	    ;;
	*)
	    MAKE=make
	    ;;
    esac

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

function run_autogen
{
    local _target="$1"
    local _autogen="${_target}/autogen.sh"
    local _configure="${_target}/configure"
    local _autogen_modtime=0
    local _configure_modtime=0
    local _must_run_autogen=0

    if [ -z "${_target}" ]; then
	return 1
    fi

    if [ -f "${_configure}" ];
    then
	if [ -f "${_autogen}" -o -h  "${_autogen}" ] && [ -x "${_autogen}" ]; 
	then
	    _autogen_modtime=`_get_mtime ${_autogen}`
	    _configure_modtime=`_get_mtime ${_configure}`

	    if [ ${_autogen_modtime} -ge ${_configure_modtime} ]; then
		_must_run_autogen=1
	    fi
	fi
    else
	_must_run_autogen=1
    fi

    if [ ${_must_run_autogen} -eq 1 ]; then
	echo "Running autogen.sh..."
	(${_autogen})
	return $?
    fi

    return 0
}
