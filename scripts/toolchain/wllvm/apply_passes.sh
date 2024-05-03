#! /usr/bin/bash

LOAD_OPT=`find $2 -name "*.so" | xargs printf -- '-load=%s '`
while IFS="" read -r PASS_OPTS || [ -n "$PASS_OPTS" ]
do
    opt $LOAD_OPT $PASS_OPTS $1 -S -o $1.tmp
    mv $1.tmp $1
done < $3
while IFS="" read -r PASS_PLUGIN || [ -n "$PASS_PLUGIN" ]
do
    opt -load-pass-plugin=$PASS_PLUGIN $1 -S -o $1.tmp
    mv $1.tmp $1
done < $4
while IFS="" read -r PASS_PLUGIN || [ -n "$PASS_PLUGIN" ]
do
    clang -Xclang -fpass-plugin=$PASS_PLUGIN -c -emit-llvm $1 -o $1.tmp
    mv $1.tmp $1
done < $5
