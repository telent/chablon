{ lib, buildPythonPackage, fetchPypi }:

buildPythonPackage rec {
  pname = "arpy";
  version = "2.2.0";

  src = fetchPypi {
    inherit pname version;
    sha256 = "1g76w2clkdr0z0l6bbqgn50pf3inp31fhdsb1q48ngfz9kygwzg0";
  };

  doCheck = false;
}
