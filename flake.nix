{
  description = "JUCE Dante Audio Backend — Test Application";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
    juce-src = {
      url = "github:oblongular/JUCE/dante-v0.2.2";
      flake = false;
    };
    dante-dep-client-sdk = {
      url = "github:oblongular/Dante-DEP-Client-SDK/v0.2.3";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, juce-src, dante-dep-client-sdk }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f system);

      perSystem = system:
        let
          pkgs = nixpkgs.legacyPackages.${system};

          buildInputs = with pkgs; [
            alsa-lib
            jack2
            curl
            freetype
            fontconfig
            libx11
            libxcomposite
            libxcursor
            libxext
            libxinerama
            libxrandr
            libxrender
          ];
        in {
          package = pkgs.stdenv.mkDerivation {
            pname = "JUCE-Dante-TestApp";
            version = "1.0.0";
            src = ./.;

            nativeBuildInputs = with pkgs; [ cmake pkg-config clang ];
            inherit buildInputs;

            cmakeFlags = [
              "-DJUCE_DIR=${juce-src}"
              "-DDANTE_SDK_DIR=${dante-dep-client-sdk}"
            ];

            installPhase = ''
              runHook preInstall
              mkdir -p $out/bin
              cp JUCE-Dante-TestApp_artefacts/Release/JUCE-Dante-TestApp $out/bin/
              runHook postInstall
            '';
          };

          devShell = pkgs.mkShell {
            nativeBuildInputs = with pkgs; [ cmake ninja pkg-config clang ];

            buildInputs = buildInputs ++ (with pkgs; [
              alsa-utils
              ladspa-sdk
              webkitgtk_4_1
              libGLU
              mesa
            ]);

            shellHook = ''
              export JUCE_DIR="${juce-src}"
              export DANTE_SDK_DIR="${dante-dep-client-sdk}"
            '';
          };
        };
    in {
      packages  = forAllSystems (system: { default = (perSystem system).package; });
      devShells = forAllSystems (system: { default = (perSystem system).devShell; });
    };
}
