#!/bin/bash

DEBUG=0

while getopts d: o
do
    case "$o" in
        d)
            DEBUG="$OPTARG"
            ;;
        [?])
            showUsage
            exit 1
    esac
done

PROJECT_ROOT=$(pwd)

if [ "$DEBUG" = "1" ]; then
    if [ -d $PROJECT_ROOT/debug ]; then
        pushd $PROJECT_ROOT/debug
    fi
else
    pushd $PROJECT_ROOT/release
fi

if [ -f Makefile ]; then
    make nuke
fi

popd
