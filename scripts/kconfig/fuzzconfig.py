#! /usr/bin/python3

from kconfiglib import *
import random

def main():
    kconf = standard_kconfig(__doc__)

    for symbol in random.sample(kconf.unique_defined_syms, k=len(kconf.unique_defined_syms)):
        symbol: Symbol
        
        if symbol.no_fuzz:
            continue

        if len(symbol.assignable) == 0:
            continue

        choice = random.choice(symbol.assignable) 
        symbol.set_value(choice)
#        print(f"Setting Symbol={symbol.name} to {choice}")

    print(kconf.write_config())

if __name__ == "__main__":
    main()

