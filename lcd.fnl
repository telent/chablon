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

;; haven't checked this thoroughly, but these local variables seem
;; to use a lot of ram, so commented out all but the ones we use

;(local NOP 0x00)
(local SWRESET 0x01)
;(local RDDID 0x04)
;(local RDDST 0x09)
;(local SLPIN 0x10)
(local SLPOUT 0x11)
;(local PTLON 0x12)
(local NORON 0x13)
(local INVOFF 0x20)
(local INVON 0x21)
(local DISPOFF 0x28)
(local DISPON 0x29)
(local CASET 0x2A)
(local RASET 0x2B)
(local RAMWR 0x2C)
;(local RAMRD 0x2E)
;(local PTLAR 0x30)
(local MADCTL 0x36)
(local COLMOD 0x3A)
;(local WRMEMC 0x3C)
;(local FRMCTR1 0xB1)
;(local FRMCTR2 0xB2)
;(local FRMCTR3 0xB3)
;(local INVCTR 0xB4)
;(local DISSET5 0xB6)
;(local PWCTR1 0xC0)
;(local PWCTR2 0xC1)
;(local PWCTR3 0xC2)
;(local PWCTR4 0xC3)
;(local PWCTR5 0xC4)
;(local VMCTR1 0xC5)
;(local RDID1 0xDA)
;(local RDID2 0xDB)
;(local RDID3 0xDC)
;(local RDID4 0xDD)
;(local GMCTRP1 0xE0)
;(local GMCTRN1 0xE1)
;(local PWCTR6 0xFC)

(fn spi-command [byte]
  (gpio.write dc-pin 0)
  (spi:transfer [byte] 1))

(fn spi-data [bytes count]
  (gpio.write dc-pin 1)
  (spi:transfer bytes count))

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

(local lcd-buffer (byte_buffer.new 240))

(fn clear []
  (spi-command CASET)
  (spi-data [0 0 0 240] 4)
  (spi-command RASET)
  (spi-data [0 0 0 240] 4)
  (spi-command RAMWR)
  (for [i 0 (* 2 240)]
    (spi-data lcd-buffer 240)))

(fn window [x y w h]
  (spi-command CASET) (spi-data [0 x 0 (+ x w -1)])
  (spi-command RASET) (spi-data [0 y 0 (+ y h -1)])
  (spi-command RAMWR)
  spi-data)

{
 :buffer lcd-buffer
 :init init
 :clear clear
 :window window
 }
