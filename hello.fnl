(backlight.init)

(for [i 1 10]
   (for [level 0 3]
      (backlight.set_brightness level)
      (task.delay 1000)))
