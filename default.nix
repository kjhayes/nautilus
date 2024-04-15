{ pkgs ? import <nixpkgs> {} }:

with pkgs.lib;




pkgs.stdenv.mkDerivation {
  name = "nautilus";

  src = pkgs.nix-gitignore.gitignoreSource [] ./.;


  buildInputs = with pkgs; [
    python3
    grub2

    # Compiler dependencies
    gcc
    binutils
    xorriso
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
    make V=1 -j $NIX_BUILD_CORES isoimage
  '';

  patchPhase = ''
    # Intentionally nothing...
  '';


  installPhase = ''
    mkdir -p $out
    ls -la
    cp nautilus.* $out/
    # cp nautilus.iso $out/
  '';
}
