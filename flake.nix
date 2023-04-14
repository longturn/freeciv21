{
  description = "Freeciv21 is an empire-building strategy game inspired by the history of human civilization. ";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachSystem [ "x86_64-linux" "x86_64-darwin" ] (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          allowUnsupportedSystem = true;
        };
        # KArchive dep is not bundled in pkgs for x86_64-darwin
        karchive = (if system == "x86_64-darwin" then
          pkgs.stdenv.mkDerivation
            rec {
              pname = "karchive";
              version = "5.105";
              patch = "0";
              src = pkgs.fetchurl {
                url = "mirror://kde/stable/frameworks/${version}/karchive-${version}.${patch}.tar.xz";
                sha256 = "0jcpva3w3zpxg4a1wk8wbip74pm3cisq3pf7c51ffpsj9k7rbvvp";
                name = "karchive-${version}.${patch}.tar.xz";
              };
              nativeBuildInputs = [ pkgs.extra-cmake-modules pkgs.qt5.qttools ];
              buildInputs = [ pkgs.bzip2 pkgs.xz pkgs.zlib pkgs.zstd ];
              propagatedBuildInputs = [ pkgs.qt5.qtbase ];
              outputs = [ "out" "dev" ];
            }
        else pkgs.libsForQt5.karchive);
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
              # Use the libertinus in nixpkgs opposed to downloading
              libertinus

              qt5.qtbase
              libsForQt5.qt5.qtsvg
              lua5_3_compat
              sqlite.dev
              SDL2_mixer.dev
              karchive
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
        packages = {
          freeciv21 = freeciv21;
          default = freeciv21;
        };
      });
}
