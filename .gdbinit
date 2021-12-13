set substitute-path /build/chablon .
set history save on

define bmp_connect
  target extended-remote /dev/ttyACM0
  monitor swdp_scan
  attach 1
  monitor rtt
end

define ocd_connect
  target extended-remote localhost:3333
end

define reload
  file /home/dan/src/pinetime/chablon/chablon.elf
  load
end
