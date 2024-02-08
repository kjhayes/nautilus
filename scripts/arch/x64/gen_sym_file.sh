#!/bin/sh

readelf -s $1 > $2
awk 'match($2, /^[0-9]+/) && NF == 8{print $2 "\t" $8}' $2 > sym.tmp
scripts/arch/x64/format_syms
rm -f sym.tmp $2
