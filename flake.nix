{
  description = "Freeciv21 is an empire-building strategy game inspired by the history of human civilization. ";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
        freeciv21 =
          pkgs.stdenv.mkDerivation {
            name = "freeciv21";
            src = self;
            nativeBuildInputs = [ pkgs.qt5.wrapQtAppsHook ];
            buildInputs = with pkgs;[
              cmake
              ninja
              gcc
              python3
              gettext
              readline
              qt5.qtbase
              # Use the libertinus in nixpkgs opposed to downloading
              libertinus
              libsForQt5.qt5.qtsvg
              libsForQt5.karchive
              lua5_3_compat
              sqlite.dev
              SDL2_mixer.dev
            ];
            buildPhase = ''
              mkdir -p $out/
              cmake . -B build -G Ninja \
                -DCMAKE_INSTALL_PREFIX=$out \
                -DFREECIV_DOWNLOAD_FONTS=0
            '';
            # Installing simply means copying all files to the output directory
            installPhase = ''# Build source files and copy them over.
            cmake --build build --target install
            cp -R ${pkgs.libertinus}/share/fonts $out/share
            ln -s $out/bin/freeciv21-client $out/bin/freeciv21
        '';
          };
      in
      {
        defaultPackage = freeciv21;
      });
}
