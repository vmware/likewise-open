#!/bin/bash

top=`pwd`

for configurein in `find . -name configure.in` ; do
    pushd `dirname $configurein` > /dev/null
    echo "Running autoconf in `dirname ${configurein}`/"
    autoconf -I "$top"
    popd > /dev/null
done
