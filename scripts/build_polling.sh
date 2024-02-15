#!/bin/bash

./scripts/pass_build.sh compiler-timing CompilerTiming.cpp && cd src/test/NPB-NAS && \
./genparams.sh W && \
cd ../../.. && \
make clean && make -j && make bitcode && make timing && make final && make uImage

