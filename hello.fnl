(backlight.init)
(backlight.set_brightness 3)

(lcd.init)
(collectgarbage "collect")
(lcd.clear)
(collectgarbage "collect")
(lcd.draw_stuff)

"hey"
