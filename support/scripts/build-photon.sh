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
    DEBUG=1 mkdir -p $PROJECT_ROOT/debug && \
    cd $PROJECT_ROOT/debug && \
        ../release/photonbuild64.sh --enable-winjoin --force-debug
else
    cd $PROJECT_ROOT/release && \
        ./photonbuild64.sh --enable-winjoin
fi
