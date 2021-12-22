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

    pkgsArm =  import <nixpkgs> { crossSystem = { system = "arm-none-eabi"; } ; };
    lua = pkgsArm.stdenv.mkDerivation {
      pname = "lua";
      version = "5.3";

      src = builtins.fetchTarball {
        url = "https://www.lua.org/ftp/lua-5.3.6.tar.gz";
        name = "lua";
        sha256 = "1dp74vnqznvdniqp0is1l55v96kx5yshkgcmzcwcfqzadkzjs0ds";
      };

      configurePhase = ''
        sed -i src/luaconf.h \
          -e '/define LUA_32BITS/c\#define LUA_32BITS' \
          -e '/define LUA_COMPAT_UNPACK/c\#undef LUA_COMPAT_UNPACK' \
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
        "LUA_PATH=${lua}"
        "LUA_BUILD_PATH=${pkgsBuildBuild.lua5_3}"
        "FENNEL=${fennel}"
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

      nativeBuildInputs =  [ gcc-arm-embedded ];
      buildInputs = [
        nrf5Sdk
        #    lua
      ];
    };
in chablon
