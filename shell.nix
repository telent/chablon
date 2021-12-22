with import <nixpkgs> { };

let chablon = import ./default.nix;
in chablon.overrideAttrs(o: {
  buildInputs = [ pkgs.openocd pkgs.ctags pkgs.stlink ] ++ o.buildInputs;
})
