set substitute-path /build/chablon .
set history save on

define bmp_connect
  target extended-remote /dev/ttyBMP0
  monitor swdp_scan
  attach 1
  monitor rtt
end

define ocd_connect
  target extended-remote localhost:3333
  monitor rtt server start 9090 0
end

define ocd_rtt
  monitor rtt setup 0x20000000 65536 "SEGGER RTT"
  monitor rtt start
end

define reload
  file /home/dan/src/pinetime/chablon/chablon.elf
  load
end
