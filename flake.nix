{
  description = "Freeciv21 is an empire-building strategy game inspired by the history of human civilization. ";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachSystem [ "x86_64-linux" "x86_64-darwin" ] (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
        
        # Common derivation attributes
        commonAttrs = {
            src = self;
            nativeBuildInputs = [ pkgs.kdePackages.wrapQtAppsHook ];
            buildInputs = with pkgs;[
              cmake
              ninja
              gcc
              python3
              gettext
              readline
              qt6.qtbase
              # Use the libertinus in nixpkgs opposed to downloading
              libertinus
              kdePackages.qtsvg
              lua5_3_compat
              sqlite.dev
              SDL2_mixer.dev
              # KArchive dep is automatically satisfied in x86_64-darwin
            ] ++ (if system == "x86_64-darwin" then [ ] else [ kdePackages.karchive ]);

            # Installing simply means copying all files to the output directory
            installPhase = ''# Build source files and copy them over.
                cmake --build build --target install
                cp -R ${pkgs.libertinus}/share/fonts $out/share
                ln -s $out/bin/freeciv21-client $out/bin/freeciv21
            '';
        };

        # Release build
        freeciv21 = pkgs.stdenv.mkDerivation (commonAttrs // {
            name = "freeciv21";
            buildPhase = ''
              mkdir -p $out/
              cmake . -B build -G Ninja \
                -DCMAKE_INSTALL_PREFIX=$out \
                -DCMAKE_BUILD_TYPE=Release \
                -DFREECIV_DOWNLOAD_FONTS=0
            '';

        });

        # Debug build
        freeciv21-debug = pkgs.stdenv.mkDerivation (commonAttrs // {
            name = "freeciv21-debug";
            dontStrip = true;
            dontFixup = true;
            CFLAGS = "-Og -ggdb";
            CXXFLAGS = "-Og -ggdb";
            buildPhase = ''
              mkdir -p $out/
              cmake . -B build -G Ninja \
                -DCMAKE_INSTALL_PREFIX=$out \
                -DCMAKE_BUILD_TYPE=Debug \
                -DFREECIV_DOWNLOAD_FONTS=0
            '';
        });

      in
      {
        packages = {
          freeciv21 = freeciv21;
          freeciv21-debug = freeciv21-debug;
          default = freeciv21;
        };
      });
}
