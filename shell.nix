{ }:

let
  pinnedPkgs =
    import (builtins.fetchTarball "https://github.com/NixOS/nixpkgs/archive/nixos-24.11.tar.gz")
      { };

  bochs-src = pinnedPkgs.fetchurl {
    url = "https://downloads.sourceforge.net/project/bochs/bochs/2.8/bochs-2.8.tar.gz";
    sha256 = "sha256-qFsTr/fYQR96nzVrpsM7X13B+7EH61AYzCOmJjnaAFk=";
  };

  customBochs = pinnedPkgs.bochs.overrideAttrs (oldAttrs: {
    name = "bochs-with-debugger-2.8"; # Give our custom version a new name
    src = bochs-src; # Replace the source with the one we just fetched

    configureFlags = oldAttrs.configureFlags ++ [
      "--enable-debugger"
      "--enable-smp"
      "--enable-cpu-level=6"
      "--enable-all-optimizations"
      "--enable-x86-64"
      "--enable-pci"
      "--enable-vmx"
      "--enable-debugger-gui"
      "--enable-logging"
      "--enable-fpu"
      "--enable-3dnow"
      "--enable-sb16=dummy"
      "--enable-cdrom"
      "--enable-x86-debugger"
      "--enable-iodebug"
      "--disable-plugins"
      "--disable-docbook"
      "--with-x11"
      "--with-term"
    ];
  });

  crossToolchain = pinnedPkgs.pkgsCross.i686-embedded.buildPackages.gcc;

in
pinnedPkgs.mkShell {
  buildInputs = with pinnedPkgs; [
    crossToolchain
    nasm
    gdb
    gnumake
    binutils
    xorriso
    clang-tools
    pkg-config
    mdbook
    grub2
    bear
    tree

    customBochs
  ];

  shellHook = ''
    echo "✅ Development environment with custom-built Bochs is ready!"
  '';
}
