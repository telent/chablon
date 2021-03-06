with import <nixpkgs> { };

let nrf5Sdk = pkgs.fetchzip {
      name = "nRF5_SDK_15.3.0";
      url = "https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v15.x.x/nRF5_SDK_15.3.0_59ac345.zip";
      sha256 = "0kwgafa51idn0cavh78zgakb02xy49vzag7firv9rgqmk1pa3yd5";

    };
    fennel = pkgs.fetchurl {
      name = "fennel.lua";
      url = "https://fennel-lang.org/downloads/fennel-1.0.0";
      hash = "sha256:1nha32yilzagfwrs44hc763jgwxd700kaik1is7x7lsjjvkgapw7";
    };
    arpy = ps: ps.callPackage ./arpy.nix {};
    python = pkgs.python3.withPackages(
        ps: with ps; [
          pyelftools
          (arpy ps)
        ]
    );

    nimble =
      let patch = ./nimble-fix-critical-section-defs.patch;
      in
        pkgs.fetchzip {
          name = "nimble-1.4.0";
          url = "https://dlcdn.apache.org/mynewt/apache-nimble-1.4.0/apache-mynewt-nimble-1.4.0.tgz";
          sha256 = "1z4rfxnqxa6ywivgv3nikzhlv6vahjbn226zni6dz7ym3qrpfan9";
          extraPostFetch = ''
            patch -d $out -p1 < ${patch}
          '';
        };

    pkgsArm =  import <nixpkgs> { crossSystem = { system = "arm-none-eabi"; } ; };
    luaAttributes = {
      pname = "lua";
      version = "5.4.3";

      src = builtins.fetchTarball {
        url = "https://www.lua.org/ftp/lua-5.4.3.tar.gz";
        name = "lua";
        sha256 = "0xwm9czxz71jk4bj4r1p5rm483prp7n8bz3bdxigl2r8p2v1knsz";
      };

      configurePhase = ''
        sed -i src/luaconf.h \
          -e '/define LUA_32BITS/c\#define LUA_32BITS (1)' \
          -e '/define LUA_COMPAT_UNPACK/c\#undef LUA_COMPAT_UNPACK' \
          -e '/define LUA_COMPAT_MATHLIB/c\#undef LUA_COMPAT_MATHLIB' \
          -e '/define LUA_COMPAT_LOADERS/c\#undef LUA_COMPAT_LOADERS' \
          -e '/define LUA_COMPAT_LOG10/c\#undef LUA_COMPAT_LOG10' \
          -e '/define LUA_COMPAT_LOADSTRING/c\#undef LUA_COMPAT_LOADSTRING' \
          -e '/define LUA_COMPAT_MODULE/c\#undef LUA_COMPAT_MODULE' \
          -e '/define LUA_COMPAT_MAXN/c\#undef LUA_COMPAT_MAXN' \
          -e '/define LUAI_MAXSTACK/c\#define LUAI_MAXSTACK 15000'
      '';

      buildPhase =
        let flags="-Os -mthumb -mabi=aapcs -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin --short-enums -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16";
        in ''
          make -C src liblua.a CC=$CC CFLAGS=${builtins.toJSON flags} AR="$AR rcu" RANLIB="$RANLIB"
        '';
      installPhase = ''
        mkdir -p $out/lib $out/include
        cp src/liblua.a $out/lib
        cp src/{lua,luaconf,lualib,lauxlib}.h $out/include
      '';

    };
    lua = pkgsArm.stdenv.mkDerivation luaAttributes;
    luaBuild =  pkgs.stdenv.mkDerivation (luaAttributes // {
      buildPhase =
        let flags="-O3";
        in ''
          make -C src liblua.a lua luac CC=$CC CFLAGS=${builtins.toJSON flags} AR="$AR rcu" RANLIB="$RANLIB"
        '';
      installPhase = ''
        mkdir -p $out/bin
        cp src/{lua,luac} $out/bin
      '';
    });

    json-lua = pkgs.fetchFromGitHub {
      name = "json-lua";
      repo = "json.lua";
      owner = "rxi";
      hash = "sha256:16yzbyp296abirl77xk3fw5jqgcjf3frmwxph22sfxam8npkxcq6";
      rev = "dbf4b2dd2eb7c23be2773c89eb059dadd6436f94";
    };

    chablon = stdenv.mkDerivation {
      src = if lib.inNixShell
            then null
            else builtins.path {
              name = "chablon";
              path = ./.;
              filter = path: type:
                let relpath = lib.removePrefix (builtins.toString ./.) path;
                in !(pkgs.lib.hasPrefix "/build/" relpath);
            };
      pname = "chablon";
      version = "0.0";
      makeFlags = [
        "ARM_NONE_EABI_TOOLCHAIN_PATH=${gcc-arm-embedded}"
        "NRF5_SDK_PATH=${nrf5Sdk}"
        "PATH_TO_LUA=${lua}"
        "LUA_PATH=${json-lua}/?.lua"
        "PATH_TO_BUILD_LUA=${luaBuild}"
        "FENNEL=${fennel}"
        "NIMBLE_PATH=${nimble}"
      ];
      postBuild = ''
        ${ctags}/bin/ctags --recurse -e . ${lua}/ ${nrf5Sdk}
      '';
      installPhase = ''
        mkdir -p $out/lib
        cp *.{elf,map} $out/lib
      '';
      shellHook = ''
        cleanish() { git  clean -f ; git clean -fdX ; }
      '';

      nativeBuildInputs =  [ python gcc-arm-embedded ];
      buildInputs = [
        nrf5Sdk
        #    lua
      ];
    };
in chablon
