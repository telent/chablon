(local reset-pin 26)
(local dc-pin 18)

(local spi
       (spi_controller.new
        0
        {
         :frequency 0x80000000
         :mode 3
         :cs-pin 25
         :cipo-pin 4
         :copi-pin 3
         :sck-pin 2
         :cs-active-high false
         }))

(local panel-width 240)
(local panel-height 320) 	;; really?

(local NOP 0x00)
(local SWRESET 0x01)
(local RDDID 0x04)
(local RDDST 0x09)
(local SLPIN 0x10)
(local SLPOUT 0x11)
(local PTLON 0x12)
(local NORON 0x13)
(local INVOFF 0x20)
(local INVON 0x21)
(local DISPOFF 0x28)
(local DISPON 0x29)
(local CASET 0x2A)
(local RASET 0x2B)
(local RAMWR 0x2C)
(local RAMRD 0x2E)
(local PTLAR 0x30)
(local MADCTL 0x36)
(local COLMOD 0x3A)
(local WRMEMC 0x3C)
(local FRMCTR1 0xB1)
(local FRMCTR2 0xB2)
(local FRMCTR3 0xB3)
(local INVCTR 0xB4)
(local DISSET5 0xB6)
(local PWCTR1 0xC0)
(local PWCTR2 0xC1)
(local PWCTR3 0xC2)
(local PWCTR4 0xC3)
(local PWCTR5 0xC4)
(local VMCTR1 0xC5)
(local RDID1 0xDA)
(local RDID2 0xDB)
(local RDID3 0xDC)
(local RDID4 0xDD)
(local GMCTRP1 0xE0)
(local GMCTRN1 0xE1)
(local PWCTR6 0xFC)

(fn spi-write [payload command? count]
  (gpio.write dc-pin (if command? 0 1))
  (spi:transfer payload count))

(fn spi-command [byte]
  (spi-write [byte] true 1))

(fn spi-data [bytes count]
  (spi-write bytes false count))

(fn hw-reset []
  (gpio.set_direction reset-pin 0)
  (gpio.set_direction dc-pin 0)
  (gpio.write reset-pin 1)
  (gpio.write reset-pin 0)
  (task.delay 200)
  (gpio.write reset-pin 1))

(fn init []
  (hw-reset)
  (spi-command SWRESET)
  (task.delay 150)

  (spi-command SLPOUT)
  (task.delay 150)

  (spi-command COLMOD)
  (spi-data [0x55])
  (task.delay 10)

  (spi-command MADCTL)
  (spi-data [0x00])

  (spi-command CASET)
  (spi-data [0x00 0x00 (// panel-width 256) (% panel-width 256)])

  (spi-command RASET)
  (spi-data [0x00 0x00 (// panel-height 256) (% panel-height 256)])

  (spi-command INVON)
  (task.delay 10)

  (spi-command NORON)
  (task.delay 10)

  (spi-command DISPON))

(local blank (byte_buffer.new 240))

(fn clear []
  (spi-command CASET)
  (spi-data [0 0 0 240] 4)
  (spi-command RASET)
  (spi-data [0 0 0 240] 4)
  (spi-command RAMWR)
  (gpio.write dc-pin (if command? 0 1))

  (for [i 0 (* 2 240)]
    (spi:transfer_raw  blank 240)))


(fn draw-stuff []
  (for [x 30 220 30]
    (spi-command CASET) (spi-data [0 x 0 (+ 8 x)] 4)
    (for [y 30 220 30]
      (spi-command RASET) (spi-data [0 y 0 (+ 8 y)] 4)
      (spi-command RAMWR)
      (for [i 0 15]
        (spi-data [0xf0 0x0f 0xf0 0x0f 0xf0 0x0f 0xf0 0x0f] 8)))))

{
 :init init
 :clear clear
 :draw_stuff draw-stuff
 }