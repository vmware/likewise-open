#!/bin/bash

top=`pwd`

for configurein in `find . -path ./appl -prune -o -name configure.in -print` ; do
    pushd `dirname $configurein` > /dev/null
    echo "Running autoconf in `dirname ${configurein}`/"
    autoconf -I "$top"
    popd > /dev/null
done
