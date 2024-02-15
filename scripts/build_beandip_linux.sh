#!/bin/bash

# git@github.com:beandip-project/beandip.git

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )/../"

PREFIX=$ROOT/dep/beandip-linux

if [ ! -d "${PREFIX}" ]; then
    git clone git@github.com:beandip-project/beandip.git ${PREFIX}
fi

pushd $PREFIX

make defconfig
make deps
make -j
