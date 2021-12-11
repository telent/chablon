set substitute-path /build/chablon .
set history save on

define connect_pinetime
  target extended-remote /dev/ttyACM0
  monitor swdp_scan
  attach 1
  monitor rtt
end

define reload
  file /home/dan/src/pinetime/chablon/chablon.elf
  load
end
