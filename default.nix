{ pkgs ? import <nixpkgs> {} }:

with pkgs.lib;




pkgs.stdenv.mkDerivation {
  name = "nautilus";

  src = pkgs.nix-gitignore.gitignoreSource [] ./.;


  buildInputs = with pkgs; [
    python3
    grub2

    # Compiler dependencies
    llvmPackages_16.libllvm
    llvmPackages_16.clang
    llvmPackages_16.stdenv
    binutils
    gllvm

    bashInteractive
  ];


  hardeningDisable = [ "all" ];
  enableParallelBuilding = true;

  # build a default configuration
  preConfigure = ''
    make defconfig
  '';

  buildPhase = ''
    make V=1 -j $NIX_BUILD_CORES
    # TODO: grub-mkrescue isn't working right now...
    # make -j $NIX_BUILD_CORES isoimage
  '';

  patchPhase = ''
    # Intentionally nothing...
  '';


  installPhase = ''
    mkdir -p $out
    ls -la
    cp nautilus.bin $out/
    # cp nautilus.iso $out/
  '';
}
